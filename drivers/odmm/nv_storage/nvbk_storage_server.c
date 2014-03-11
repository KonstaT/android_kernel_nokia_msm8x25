/* Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/wakelock.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <asm/pgtable.h>
#include <mach/msm_rpcrouter.h>
#include <mach/msm_smsm.h>
//#include "smd_private.h"

// RPC
#define NVBK_REGISTER_PROC 1

struct nvbk_param {
	uint32_t size;		/* Physical memory size */
};

#define NVBK_STORAGE_IOCTL_MAGIC (0xC2)

#define NVBK_STORAGE_GET_RAM_SIZE_REQ \
	_IOR(NVBK_STORAGE_IOCTL_MAGIC, 0, struct nvbk_param)

#define NVBK_STORAGE_GET_RAM_REQ \
	_IOR(NVBK_STORAGE_IOCTL_MAGIC, 1, struct nvbk_param)

struct nvbk_storage_client_info {
	int open_excl;
    //wait_queue_head_t event_q;
    //atomic_t total_events;
	/* Lock to protect lists */
	spinlock_t lock;
	/* Wakelock to be acquired when processing requests from modem */
	//struct wake_lock wlock;
    void* nv_ram;
	int ram_size;
};
//atomic_inc(&rmc->total_events);
//wake_up(&rmc->event_q);

#define NV_BACKUP_RAM_SIZE (64*1024)

static struct nvbk_storage_client_info* rmc; 

static int nvbk_storage_open(struct inode *ip, struct file *fp)
{
	int ret = 0;

	spin_lock(&rmc->lock);
	if (!rmc->open_excl)
		rmc->open_excl = 1;
	else
		ret = -EBUSY;
	spin_unlock(&rmc->lock);

	return ret;
}

static int nvbk_storage_release(struct inode *ip, struct file *fp)
{
	spin_lock(&rmc->lock);
	rmc->open_excl = 0;
	spin_unlock(&rmc->lock);

	return 0;
}

static long nvbk_storage_ioctl(struct file *fp, unsigned int cmd,
			    unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
	case NVBK_STORAGE_GET_RAM_SIZE_REQ:
		pr_debug("%s: get shared memory size ioctl\n", __func__);
		if (copy_to_user((void __user *)arg,&rmc->ram_size,sizeof(int))){
			pr_err("%s: copy to user failed\n\n", __func__);
			ret = -EFAULT;
		}
		break;
	case NVBK_STORAGE_GET_RAM_REQ:
		pr_debug("%s: get shared memory parameters ioctl\n", __func__);
        if(!rmc->nv_ram){
            ret = -EFAULT;
            break;
           }
		if (copy_to_user((void __user *)arg,rmc->nv_ram,
			NV_BACKUP_RAM_SIZE)) {
			pr_err("%s: copy to user failed\n\n", __func__);
			ret = -EFAULT;
		}
		break;


	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}


const struct file_operations nvbk_storage_fops = {
	.owner = THIS_MODULE,
	.open = nvbk_storage_open,
	.unlocked_ioctl	 = nvbk_storage_ioctl,
	.release = nvbk_storage_release,
};

static struct miscdevice nvbk_storage_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "nvbk_storage",
	.fops = &nvbk_storage_fops,
};





static void rmt_storage_destroy_rmc(void)
{
	//wake_lock_destroy(&rmc->wlock);
}

static void __init nvbk_storage_init_client_info(void)
{
	/* Initialization */
	spin_lock_init(&rmc->lock);
    //init_waitqueue_head(&rmc->event_q);
    //atomic_set(&rmc->total_events, 0);
}



static int __init nvbk_storage_init(void)
{
	int ret = 0;
    unsigned size = 0;
	rmc = kzalloc(sizeof(struct nvbk_storage_client_info), GFP_KERNEL);
	if (!rmc) {
		pr_err("%s: Unable to allocate memory\n", __func__);
		return  -ENOMEM;
	}
	nvbk_storage_init_client_info();

	ret = misc_register(&nvbk_storage_device);
	if (ret) {
		pr_err("%s: Unable to register misc device %d\n", __func__,
				MISC_DYNAMIC_MINOR);
		goto rmc_free;
	}

	rmc->nv_ram = smem_get_entry(SMEM_ID_VENDOR1,&size);
	rmc->ram_size = (int)size;
	pr_err("%s: nvbk_storage_init 0x%x %d\n", __func__,(int)rmc->nv_ram,rmc->ram_size);

	return 0;

//unreg_misc:
//	misc_deregister(&nvbk_storage_device);

rmc_free:
	rmt_storage_destroy_rmc();
	kfree(rmc);
	return ret;
}

module_init(nvbk_storage_init);
MODULE_DESCRIPTION("nv backup Storage RPC Client");
MODULE_LICENSE("GPL v2");
