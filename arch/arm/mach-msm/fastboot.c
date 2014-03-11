/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/mutex.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/delay.h>
#include <linux/reboot.h>
#include <asm/mach-types.h>
#include <mach/msm_rpcrouter.h>
#include <mach/pmic.h>
#include <mach/oem_rapi_client.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
/**
 * fastboot_rpc_req_reply() - send request and wait for reply
 * @tbuf:	buffer contains arguments
 * @rbuf:	buffer to be filled with arguments at reply
 * @proc:	command/request id
 *
 * This function send request to modem and wait until reply received
 */
#define FASTBOOT_RPC_PROG		0x30000061
#define FASTBOOT_RPC_PROG_START	0x00000010
#define FASTBOOT_RPC_PROG_STOP	0x00000010

#define FASTBOOT_RPC_VER_1_1	0x00010001
#define FASTBOOT_RPC_VER_2_1	0x00020001
#define FASTBOOT_RPC_VER_3_1	0x00030001
#define FASTBOOT_RPC_VER_5_1	0x00050001
#define FASTBOOT_RPC_VER_6_1	0x00060001

#define FASTBOOT_RPC_TIMEOUT	(5*HZ)
#define	FASTBOOT_BUFF_SIZE		64

#define FB_ERR_FLAG__FEATURE_NOT_SUPPORTED   	(0x001F)

typedef enum
{
  FAST_PWROFF_NONE,
  FAST_PWROFF_ON,
  FAST_PWROFF_LCDOFF,
  FAST_PWROFF_OK,
}FAST_PWROFF_STATUS;

#define OEM_RAPI_CLIENT_EVENT_FAST_POWERON_REG_SET 32
#define OEM_RAPI_CLIENT_EVENT_FAST_POWEROFF_REG_SET 33

struct fastboot_buf {
	char *start;		/* buffer start addr */
	char *end;		/* buffer end addr */
	int size;		/* buffer size */
	char *data;		/* payload begin addr */
	int len;		/* payload len */
};

struct fastboot_ctrl {
	struct fastboot_buf	tbuf;
	struct fastboot_buf	rbuf;
	struct msm_rpc_endpoint *endpoint;
};

struct fastboot_data {
	unsigned long state;
	unsigned long enabled;
	struct fastboot_ctrl fb_ctrl;
	struct device *dev;
	struct mutex lock;
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
} *global_data = NULL;

static DEFINE_MUTEX(fastboot_mtx);

static ssize_t fastboot_start_show(struct device *dev,
				 struct device_attribute *attr,
				 char *buf)
{
	struct fastboot_data *fb_data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%lu\n",
			fb_data->state);
}

static int fastboot_send_rpc(char *input)
{
	int ret;
	struct msm_rpc_client *client = NULL;

	struct oem_rapi_client_streaming_func_arg client_arg = {
		OEM_RAPI_CLIENT_EVENT_FAST_POWEROFF_REG_SET,
		NULL,
		(void *)NULL,
		1,
		input,
		0,
		0,
		0
	};

	struct oem_rapi_client_streaming_func_ret client_ret = {
		(uint32_t *)NULL,
		(char *)NULL
	};

	printk("%s, %d\n", __func__,(int)input[0] );
	client = oem_rapi_client_init();
	if (IS_ERR(client) || (!client)) {
		client = NULL;
		printk(KERN_ERR
			"oem_rapi_client_init() error\n");
		return -ENODEV;
	}

	ret = oem_rapi_client_streaming_function(client, &client_arg, &client_ret);
	if (ret)
		printk(KERN_ERR
			"oem_rapi_client_streaming_function() error=%d\n", ret);

	return ret;
}

int fastboot_enabled(void)
{
	return global_data && global_data->enabled;
}
EXPORT_SYMBOL(fastboot_enabled);

void fastboot_usb_callback(void)
{
	char *envp[2] = {"FASTBOOT_MSG=usb", NULL};

	if (global_data)
		kobject_uevent_env(&global_data->dev->kobj, KOBJ_CHANGE, envp);
}
EXPORT_SYMBOL(fastboot_usb_callback);

extern void msm_otg_turn_on_usb(int on);
static ssize_t fastboot_start_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf,
				  size_t n)
{
	unsigned long tmp;
	char rpc_arg;
	struct fastboot_data *fb_data = dev_get_drvdata(dev);

	if (!strict_strtoul(buf, 10, &tmp)) {
		mutex_lock(&fb_data->lock);
		if (tmp == 1) {
			rpc_arg = FAST_PWROFF_ON;
			fastboot_send_rpc(&rpc_arg);
			fb_data->state = 1;
		}
		else {
			rpc_arg = FAST_PWROFF_NONE;
			fastboot_send_rpc(&rpc_arg);
			fb_data->state = 0;
		}
		mutex_unlock(&fb_data->lock);
		return n;
	} else {
		pr_err("%s: unable to convert: %s to an int\n", __func__,
			buf);
		return -EINVAL;
	}
}

static DEVICE_ATTR(fastboot, 0664, fastboot_start_show, fastboot_start_store);

static struct attribute *fastboot_sysfs_attrs[] = {
	&dev_attr_fastboot.attr,
	NULL
};

static struct attribute_group fastboot_attribute_group = {
	.attrs = fastboot_sysfs_attrs,
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void fastboot_early_suspend(struct early_suspend *h)
{
	struct fastboot_data *fb_data = container_of(h, struct fastboot_data, early_suspend);
	char rpc_arg;

	if (fb_data->state) {
		rpc_arg = FAST_PWROFF_LCDOFF;
		fastboot_send_rpc(&rpc_arg);
		usleep(10000);
		fb_data->enabled = 1;
		rpc_arg = FAST_PWROFF_OK;
		fastboot_send_rpc(&rpc_arg);
	}
}

static void fastboot_late_resume(struct early_suspend *h)
{
	struct fastboot_data *fb_data = container_of(h, struct fastboot_data, early_suspend);
	char *envp[2] = {"FASTBOOT_MSG=resume", NULL};

	if (fb_data->state) {
		kobject_uevent_env(&fb_data->dev->kobj, KOBJ_CHANGE, envp);
		fb_data->enabled = 0;
	}
}
#endif

static int __devinit fastboot_probe(struct platform_device *pdev)
{
	struct fastboot_data *fb_data = NULL;
	int ret;

	global_data = fb_data = kmalloc(sizeof(struct fastboot_data), GFP_KERNEL);
	if (!fb_data) {
		ret = -ENODEV;
		goto fail_mem;
	}
	ret = sysfs_create_group(&pdev->dev.kobj,
				 &fastboot_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		ret = -EBUSY;
		goto fail_sysfs;
	}

	mutex_init(&fb_data->lock);
	//clear this addr
	fb_data->state = 0;
	fb_data->enabled = 0;
	fb_data->dev = &pdev->dev;
	platform_set_drvdata(pdev, fb_data);

#ifdef CONFIG_HAS_EARLYSUSPEND
	fb_data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	fb_data->early_suspend.suspend = fastboot_early_suspend;
	fb_data->early_suspend.resume = fastboot_late_resume;
	register_early_suspend(&fb_data->early_suspend);
#endif
	return 0;

fail_sysfs:
	kfree(fb_data);
fail_mem:
	return ret;
}

static int __devexit fastboot_remove(struct platform_device *pdev)
{
	struct fastboot_data *fb_data = platform_get_drvdata(pdev);

#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&fb_data->early_suspend);
#endif
	sysfs_remove_group(&pdev->dev.kobj, &fastboot_attribute_group);
	kfree(fb_data);

	return 0;
}

#ifdef CONFIG_PM
static int
fastboot_suspend(struct device *dev)
{
	return 0;
}

static int
fastboot_resume(struct device *dev)
{
	return 0;
}

static struct dev_pm_ops fastboot_pm_ops = {
	.suspend	= fastboot_suspend,
	.resume		= fastboot_resume,
};
#endif

static struct platform_driver fastboot_driver = {
	.probe		= fastboot_probe,
	.remove     = fastboot_remove,
	.driver         = {
		.name = "fastboot",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &fastboot_pm_ops,
#endif
	},
};

static int __init fastboot_init(void)
{
	return platform_driver_register(&fastboot_driver);
}

static void __exit fastboot_exit(void)
{
	platform_driver_unregister(&fastboot_driver);
}

module_init(fastboot_init);
module_exit(fastboot_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("msm fastboot driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("fastboot");
