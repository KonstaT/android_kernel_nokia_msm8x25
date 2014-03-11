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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/device.h>
#include <linux/sysfs.h>

#include <mach/oem_rapi_client.h>

#define DEVICE_NAME     "sleepm"

enum sleep_monitor_state {
	SLEEP_MONITOR_DISABLE,
	SLEEP_MONITOR_ENABLE,
	SLEEP_MONITOR_MAX
};

static struct msm_rpc_client *client = NULL;
static int control = SLEEP_MONITOR_DISABLE;

static int call_oem_rapi_client_streaming_function(char *input)
{
	int ret;

	struct oem_rapi_client_streaming_func_arg client_arg = {
		OEM_RAPI_CLIENT_EVENT_DEBUG_SLEEP_MONITOR,
		NULL,
		(void *)NULL,
		sizeof(input),
		input,
		0,
		0,
		0
	};
	struct oem_rapi_client_streaming_func_ret client_ret = {
		(uint32_t *)NULL,
		(char *)NULL
	};

	if (client == NULL) {
		client = oem_rapi_client_init();
		if (IS_ERR(client) || (!client)) {
			client = NULL;
			return -ENODEV;
		}
	}

	ret = oem_rapi_client_streaming_function(client, &client_arg, &client_ret);
	if (ret)
		printk(KERN_ERR
			"oem_rapi_client_streaming_function() error=%d\n", ret);
	return ret;
}

ssize_t sleepm_show_control(struct device *ddev,
		struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", control);
}
ssize_t sleepm_store_control(struct device *ddev,
		 struct device_attribute *attr, const char *buf, size_t count)
{
	int tmp;

	sscanf(buf, "%d", &tmp);
	control = tmp;
	switch((enum sleep_monitor_state)tmp){
		case SLEEP_MONITOR_DISABLE:
			printk(KERN_INFO DEVICE_NAME ": Disable sleep monitor\n");
			break;
		case SLEEP_MONITOR_ENABLE:
			printk(KERN_INFO DEVICE_NAME ": Enable sleep monitor\n");
			break;
		default:
			printk(KERN_INFO DEVICE_NAME ": Extend Command %d\n", tmp);
			break;
	}

	call_oem_rapi_client_streaming_function((char*)&control);

	return count;
}

static DEVICE_ATTR(control, S_IRUGO | S_IWUSR, sleepm_show_control, sleepm_store_control);

static struct class *sleepm_class;
static struct device *sleepm_device;
static dev_t sleepm_devno;

static int __init sleep_monitor_module_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&sleepm_devno, 0, 1, DEVICE_NAME);
	if (ret < 0) {
		printk(KERN_ERR DEVICE_NAME ":  Fail to alloc chardev region (%d)\n", ret);
		goto alloc_chrdev_region_fail;
	}

	//Create sleep monitor class
	sleepm_class = class_create(THIS_MODULE, "sleep_monitor");
	if (IS_ERR(sleepm_class)) {
		ret = PTR_ERR(sleepm_class);
		goto class_create_fail;
	}

	sleepm_device = device_create(sleepm_class, NULL, sleepm_devno, NULL, DEVICE_NAME);
	if (IS_ERR(sleepm_device)) {
		ret = -ENOMEM;
		goto device_create_fail;
	}

	ret = device_create_file(sleepm_device, &dev_attr_control);
	if (ret) {
		pr_err("%s: device_create_file(%s)=%d\n",
				__func__, dev_attr_control.attr.name, ret);
		goto device_create_file_fail;
	}

	return 0;

device_create_file_fail:
	device_destroy(sleepm_class, sleepm_devno);
device_create_fail:
	class_destroy(sleepm_class);
class_create_fail:
	unregister_chrdev_region(sleepm_devno, 1);
alloc_chrdev_region_fail:
	return ret;
}

static void __exit sleep_monitor_module_exit(void)
{
	device_remove_file(sleepm_device, &dev_attr_control);
	device_destroy(sleepm_class, sleepm_devno);
	class_destroy(sleepm_class);
	unregister_chrdev_region(sleepm_devno, 1);
}

module_init(sleep_monitor_module_init);
module_exit(sleep_monitor_module_exit);

MODULE_DESCRIPTION("MSM system event monitor driver");
MODULE_LICENSE("GPL v2");
