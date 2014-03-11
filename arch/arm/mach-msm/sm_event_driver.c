/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/cdev.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/platform_device.h>
#include <linux/debugfs.h>

#include <asm/uaccess.h>
#include <asm/byteorder.h>

#include <mach/peripheral-loader.h>
#include <linux/sm_event_log.h>
#include <linux/sm_event.h>

#include "smd_rpc_sym.h"
#include "smd_rpcrouter.h"

#define SM_EVENT_READ_MAXIMUM 10

#ifdef __SM_DEBUG
static int32_t lock_count = 0;

#define INIT_LOCK(x) \
{ \
	mutex_init(x); \
	lock_count = 0; \
	printk ("mutex_init: lock_count = %d, addr = %p, actual = %d\n", lock_count, x, ((x)->count).counter); \
}

#define LOCK(x) \
{ \
	lock_count++; \
	printk ("mutex_lock: lock_count = %d, addr = %p, actual = %d\n", lock_count, x, ((x)->count).counter); \
	mutex_lock(x); \
}

#define UNLOCK(x) \
{ \
	lock_count--; \
	printk ("mutex_unlock: lock_count = %d, addr = %p, actual = %d\n", lock_count, x, ((x)->count).counter); \
	mutex_unlock(x); \
}
#else
#define INIT_LOCK(x)  mutex_init(x)
#define LOCK(x)  mutex_lock(x)
#define UNLOCK(x)  mutex_unlock(x)
#endif

struct sm_event_info
{
	int event_cur_pos;
	struct mutex lock;
	sm_event_item_t event[SM_EVENT_READ_MAXIMUM];
};

struct sm_event_debug_info
{
	int event_cur_pos;
	struct mutex lock;
	sm_event_item_t event[SM_EVENT_READ_MAXIMUM];
	char debug_buf[PAGE_SIZE];
	uint32_t read_avail;
};

struct class *sm_event_class;
dev_t sm_event_devno;
static struct device *sm_event_device;
static struct cdev sm_event_cdev;

struct dentry *sm_event_dir;
struct dentry *sm_event_log_debugfs;
struct dentry *sm_event_log_mask_debugfs;

static struct sm_event_debug_info sm_debugfs_info;

static long sm_event_ioctl(struct file *filp, unsigned int cmd,
			    unsigned long arg)
{
	return -EINVAL;
}

static int sm_event_open(struct inode *inode, struct file *filp)
{
	struct sm_event_info *log_info;

	log_info = kzalloc(sizeof(struct sm_event_info), GFP_KERNEL);

	log_info->event_cur_pos = 0;

	if (!log_info)
		return -ENOMEM;

	filp->private_data = log_info;
	INIT_LOCK (&(log_info->lock));
	return 0;
}

static int sm_event_release(struct inode *inode, struct file *filp)
{
	kfree (filp->private_data);
	return 0;
}

static ssize_t sm_event_read(struct file *filp, char __user *buf,
			      size_t count, loff_t *ppos)
{
	uint32_t start, r_count;
	struct sm_event_info *log_info;
	sm_event_item_t *ev;
	int32_t rc = -EFAULT;

	log_info = (struct sm_event_info *)filp->private_data;
	LOCK(&(log_info->lock));

	r_count = count/sizeof(sm_event_item_t);

	if (r_count > SM_EVENT_READ_MAXIMUM)
		r_count = SM_EVENT_READ_MAXIMUM;

	start = log_info->event_cur_pos;
	ev = log_info->event;

	if ((rc = sm_get_event_and_data (ev, &start, &r_count, GET_EVENT_ENABLE_REINDEX)) > 0) {
		if (!copy_to_user(buf, ev, (r_count * sizeof(sm_event_item_t)))) {
			log_info->event_cur_pos = start + r_count;
			rc = 0;
		} else {
			rc = -EFAULT;
		}
	}

	UNLOCK(&(log_info->lock));

	if (rc)
		return rc;

	return (r_count * sizeof(sm_event_item_t));
}

/*
 * write 0 to reset the read offset
 */
static ssize_t sm_event_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct sm_event_info *log_info;
	char k_buffer[4];
	size_t len;

	log_info = (struct sm_event_info *)filp->private_data;
	LOCK(&(log_info->lock));

	len = count;
	if (len > sizeof(k_buffer))
		len = sizeof(k_buffer);

	if (copy_from_user(k_buffer, buf, len)) {
		UNLOCK(&(log_info->lock));
		return -EFAULT;
	}

	/*
	 * reset the read position
	 */
	if ((k_buffer[0] == '0') || (k_buffer[0] == 0)) {
		log_info->event_cur_pos = 0;
	} else {
		UNLOCK(&(log_info->lock));
		return -EFAULT;
	}

	UNLOCK(&(log_info->lock));
	return 0;
}

static struct file_operations sm_event_fops = {
	.owner	 = THIS_MODULE,
	.open	 = sm_event_open,
	.release = sm_event_release,
	.read	 = sm_event_read,
	.write   = sm_event_write,
	.unlocked_ioctl = sm_event_ioctl,
};

static int sm_event_debugfs_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sm_event_debugfs_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t sm_event_debugfs_read(
	struct file *file,
	char __user *buff,
	size_t buff_count,
	loff_t *ppos)

{
	uint32_t start, r_count, e_count;
	struct sm_event_debug_info *debug_info;
	sm_event_item_t *ev;
	int32_t rc = 0;

	debug_info = &sm_debugfs_info;

retry:
	LOCK(&(debug_info->lock));
	if (debug_info->read_avail > 0) {
		r_count = buff_count;
		if (buff_count > debug_info->read_avail)
			r_count = debug_info->read_avail;

		if (copy_to_user(buff, debug_info->debug_buf, r_count)) {
			UNLOCK(&(debug_info->lock));
			return -EFAULT;
		}

		debug_info->read_avail -= r_count;
		UNLOCK(&(debug_info->lock));
		return r_count;
	}

	e_count = SM_EVENT_READ_MAXIMUM;
	if (e_count > SM_EVENT_READ_MAXIMUM)
		e_count = SM_EVENT_READ_MAXIMUM;

	start = debug_info->event_cur_pos;
	ev = debug_info->event;

	if ((rc = sm_get_event_and_data (ev,
			&start,
			&e_count,
			GET_EVENT_WAIT_WAKE_ONE | GET_EVENT_ENABLE_REINDEX)) > 0) {
		rc = sm_sprint_info (debug_info->debug_buf, PAGE_SIZE, ev, e_count);
		debug_info->event_cur_pos = start + e_count;
		debug_info->read_avail = rc;
		UNLOCK(&(debug_info->lock));
		goto retry;
	}

	UNLOCK(&(debug_info->lock));
	return rc;
}

static ssize_t sm_event_debugfs_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	struct sm_event_debug_info *debug_info;

	debug_info = &sm_debugfs_info;
	LOCK(&(debug_info->lock));
	debug_info->event_cur_pos = 0;
	debug_info->read_avail = 0;
	UNLOCK(&(debug_info->lock));

	return 0;
}

int32_t sm_debugfs_event_callback (sm_event_item_t *ev)
{
	return 0;
}

static const struct file_operations sm_event_log_debug_fops = {
	.open    = sm_event_debugfs_open,
	.read    = sm_event_debugfs_read,
	.write   = sm_event_debugfs_write,
	.release = sm_event_debugfs_release,
	.owner   = THIS_MODULE,
};

static int sm_event_log_mask_debugfs_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int sm_event_log_mask_debugfs_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t sm_event_log_mask_debugfs_read(
	struct file *file,
	char __user *buff,
	size_t buff_count,
	loff_t *ppos)
{
	char mask_value[12];
	int len;

	len = snprintf(mask_value, 12, "0x%08x\n", sm_get_event_mask());

	return simple_read_from_buffer(buff, buff_count, ppos, mask_value, len);
}

static ssize_t sm_event_log_mask_debugfs_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	char buf[64];
	unsigned long val;
	int ret;

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(&buf, buff, count))
		return -EFAULT;

	buf[count] = 0;
	ret = strict_strtoul(buf, 0, &val);
	if (ret < 0)
		return ret;

	sm_set_event_mask((uint32_t)val);

	return count;
}

static const struct file_operations sm_event_log_mask_debug_fops = {
	.open		= sm_event_log_mask_debugfs_open,
	.read		= sm_event_log_mask_debugfs_read,
	.write		= sm_event_log_mask_debugfs_write,
	.release	= sm_event_log_mask_debugfs_release,
	.owner		= THIS_MODULE,
};

static void debugfs_init (void)
{
	sm_debugfs_info.event_cur_pos = 0;
	sm_debugfs_info.read_avail = 0;
	INIT_LOCK (&(sm_debugfs_info.lock));
}

static void __exit sm_event_exit(void)
{
	cdev_del(&sm_event_cdev);
	debugfs_remove(sm_event_log_debugfs);
	debugfs_remove(sm_event_log_mask_debugfs);
	debugfs_remove(sm_event_dir);
	device_destroy(sm_event_class, sm_event_devno);
	unregister_chrdev_region(sm_event_devno, 1);
	class_destroy(sm_event_class);
}

static int __init sm_event_init(void)
{
	int rc;
	int major;

	/* Create the device nodes */
	sm_event_class = class_create(THIS_MODULE, "sm_event_driver");

	if (IS_ERR(sm_event_class)) {
		rc = -ENOMEM;
		printk(KERN_ERR
		       "sm_event_driver: failed to create event log class\n");
		goto alloc_class_fail;
	}

	rc = alloc_chrdev_region(&sm_event_devno, 0, 1, "sm_event_log");
	if (rc < 0) {
		printk(KERN_ERR
		       "sm_event_driver: Failed to alloc chardev region (%d)\n", rc);
		goto alloc_chr_region_fail;
	}

	major = MAJOR(sm_event_devno);
	sm_event_device = device_create(sm_event_class, NULL, sm_event_devno, NULL, "sm_event_log");
	if (IS_ERR(sm_event_device)) {
		rc = -ENOMEM;
		goto device_create_fail;
	}

	debugfs_init ();
	sm_event_dir = debugfs_create_file("sm_event", S_IFDIR | S_IRWXU | S_IRWXG | S_IRWXO,
						NULL, NULL, NULL);
	if (IS_ERR(sm_event_dir))
		goto debugfs_create_dir_fail;

	sm_event_log_debugfs = debugfs_create_file("sm_event_log", S_IRWXUGO, sm_event_dir, NULL,
				&sm_event_log_debug_fops);
	if (sm_event_log_debugfs == NULL)
		goto sm_event_log_debugfs_fail;

	sm_event_log_mask_debugfs = debugfs_create_file("sm_event_mask", S_IRWXUGO, sm_event_dir, NULL,
				&sm_event_log_mask_debug_fops);
	if (sm_event_log_mask_debugfs == NULL)
		goto sm_event_log_mask_debugfs_fail;

	cdev_init(&sm_event_cdev, &sm_event_fops);
	sm_event_cdev.owner = THIS_MODULE;

	rc = cdev_add(&sm_event_cdev, sm_event_devno, 1);
	if (rc < 0)
		goto add_cdev_fail;

	return 0;

add_cdev_fail:
	debugfs_remove(sm_event_log_mask_debugfs);
sm_event_log_mask_debugfs_fail:
	debugfs_remove(sm_event_log_debugfs);
sm_event_log_debugfs_fail:
	debugfs_remove(sm_event_dir);
debugfs_create_dir_fail:
	device_destroy(sm_event_class, sm_event_devno);
device_create_fail:
	unregister_chrdev_region(sm_event_devno, 1);
alloc_chr_region_fail:
	class_destroy(sm_event_class);
alloc_class_fail:
	return rc;
}

module_init(sm_event_init);
module_exit(sm_event_exit);
MODULE_DESCRIPTION("MSM system event monitor driver");
MODULE_LICENSE("GPL v2");
