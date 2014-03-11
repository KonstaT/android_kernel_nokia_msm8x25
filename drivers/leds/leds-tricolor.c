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
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <linux/spinlock.h>
#include <linux/ctype.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/err.h>
#include <asm/mach-types.h>
#include <mach/pmic.h>
#include <mach/gpio-v1.h>
#include <mach/oem_rapi_client.h>
#include <mach/socinfo.h>

#define DEBUG_TRICOLOR_LED 0

static int qrd5_led_flash_en1 = 13;
static int qrd7_led_flash_en = 96;

enum tri_color_led_color {
	LED_COLOR_RED,
	LED_COLOR_GREEN,
	LED_COLOR_BLUE,
	LED_COLOR_MAX
};

enum tri_led_status{
	ALL_OFF,
	ALL_ON,
	BLUE_ON,
	BLUE_OFF,
	RED_ON,
	RED_OFF,
	GREEN_ON,
	GREEN_OFF,
	BLUE_BLINK,
	RED_BLINK,
	GREEN_BLINK,
	BLUE_BLINK_OFF,
	RED_BLINK_OFF,
	GREEN_BLINK_OFF,
	LED_MAX
};

struct tricolor_led_data {
	struct msm_rpc_client *rpc_client;
	spinlock_t led_lock;
	int led_data[4];
	struct led_classdev leds[4];	/* blue, green, red, flashlight */
};

static void call_oem_rapi_client_streaming_function(struct msm_rpc_client *client,
						    char *input)
{
	struct oem_rapi_client_streaming_func_arg client_arg = {
		OEM_RAPI_CLIENT_EVENT_TRI_COLOR_LED_WORK,
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

	int ret = oem_rapi_client_streaming_function(client, &client_arg, &client_ret);
	if (ret)
		printk(KERN_ERR
			"oem_rapi_client_streaming_function() error=%d\n", ret);
}


static ssize_t led_blink_solid_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	ssize_t ret = 0;
	enum tri_color_led_color color = LED_COLOR_MAX;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct tricolor_led_data *tricolor_led = NULL;
	if (!strcmp(led_cdev->name, "red")) {
		color = LED_COLOR_RED;
	} else if (!strcmp(led_cdev->name, "green")) {
		color = LED_COLOR_GREEN;
	} else {
		color = LED_COLOR_BLUE;
	}
	tricolor_led = container_of(led_cdev, struct tricolor_led_data, leds[color]);
	if(!tricolor_led)
		printk(KERN_ERR "%s tricolor_led is NULL ",__func__);
	ret = sprintf(buf, "%u\n", tricolor_led->led_data[color]);
	return ret;
}

static ssize_t led_blink_solid_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t size)
{
	int blink = 0;
	unsigned long flags = 0;
	enum tri_led_status input = LED_MAX;
	enum tri_color_led_color color = LED_COLOR_MAX;
	struct led_classdev *led_cdev = dev_get_drvdata(dev);
	struct tricolor_led_data *tricolor_led = NULL;
	if (!strcmp(led_cdev->name, "red")) {
		color = LED_COLOR_RED;
	} else if (!strcmp(led_cdev->name, "green")) {
		color = LED_COLOR_GREEN;
	} else {
		color = LED_COLOR_BLUE;
	}
	tricolor_led = container_of(led_cdev, struct tricolor_led_data, leds[color]);
	if(!tricolor_led)
		printk(KERN_ERR "%s tricolor_led is NULL ",__func__);
	sscanf(buf, "%d", &blink);
#if DEBUG_TRICOLOR_LED
	printk("tricolor %s is %d\n",led_cdev->name, blink);
#endif
	spin_lock_irqsave(&tricolor_led->led_lock, flags);
	if(blink){
		switch(color) {
			case LED_COLOR_RED:
				input = RED_BLINK;
				break;
			case LED_COLOR_GREEN:
				input = GREEN_BLINK;
				break;
			case LED_COLOR_BLUE:
				input = BLUE_BLINK;
				break;
			default:
				break;
		}
	} else {
		switch(color) {
			case LED_COLOR_RED:
				input = RED_BLINK_OFF;
				break;
			case LED_COLOR_GREEN:
				input = GREEN_BLINK_OFF;
				break;
			case LED_COLOR_BLUE:
				input = BLUE_BLINK_OFF;
				break;
			default:
				break;
		}
	}
	tricolor_led->led_data[color] = blink;
	spin_unlock_irqrestore(&tricolor_led->led_lock, flags);
	call_oem_rapi_client_streaming_function(tricolor_led->rpc_client, (char*)&input);
	return size;
}

static DEVICE_ATTR(blink, 0644, led_blink_solid_show, led_blink_solid_store);

static void led_brightness_set_tricolor(struct led_classdev *led_cdev,
			       enum led_brightness brightness)
{
	struct tricolor_led_data *tricolor_led = NULL;
	enum tri_color_led_color color = LED_COLOR_MAX;
	enum tri_led_status input = LED_MAX;
	unsigned long flags = 0;

	if (!strcmp(led_cdev->name, "red")) {
		color = LED_COLOR_RED;
	} else if (!strcmp(led_cdev->name, "green")) {
		color = LED_COLOR_GREEN;
	} else {
		color = LED_COLOR_BLUE;
	}
	tricolor_led = container_of(led_cdev, struct tricolor_led_data, leds[color]);
	if(!tricolor_led)
		printk(KERN_ERR "%s tricolor_led is NULL ",__func__);

	spin_lock_irqsave(&tricolor_led->led_lock, flags);
	if(brightness){
		switch(color) {
			case LED_COLOR_RED:
				input = RED_ON;
				break;
			case LED_COLOR_GREEN:
				input = GREEN_ON;
				break;
			case LED_COLOR_BLUE:
				input = BLUE_ON;
				break;
			default:
				break;
		}
	} else {
		switch(color) {
			case LED_COLOR_RED:
				input = RED_OFF;
				break;
			case LED_COLOR_GREEN:
				input = GREEN_OFF;
				break;
			case LED_COLOR_BLUE:
				input = BLUE_OFF;
				break;
			default:
				break;
		}
	}
	spin_unlock_irqrestore(&tricolor_led->led_lock, flags);
	call_oem_rapi_client_streaming_function(tricolor_led->rpc_client, (char*)&input);
}

static void led_brightness_set_flash(struct led_classdev *led_cdev,
				     enum led_brightness brightness)
{
	if(brightness){
		if(machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
			gpio_set_value(qrd5_led_flash_en1, 1);
		else if(machine_is_msm8625_qrd7())
			gpio_set_value(qrd7_led_flash_en, 1);
	} else {
		if(machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
			gpio_set_value(qrd5_led_flash_en1, 0);
		else if(machine_is_msm8625_qrd7())
			gpio_set_value(qrd7_led_flash_en, 0);
	}
}

static int tricolor_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	int i, j;
	struct tricolor_led_data *tricolor_led;
	printk(KERN_ERR "tricolor leds and flashlight: probe init \n");

	tricolor_led = kzalloc(sizeof(struct tricolor_led_data), GFP_KERNEL);
	if (tricolor_led == NULL) {
		printk(KERN_ERR "tricolor_led_probe: no memory for device\n");
		ret = -ENOMEM;
		goto err;
	}
	memset(tricolor_led, 0, sizeof(struct tricolor_led_data));

	spin_lock_init(&tricolor_led->led_lock);

	/* initialize tricolor_led->pc_client */
	tricolor_led->rpc_client = oem_rapi_client_init();
	ret = IS_ERR(tricolor_led->rpc_client);
	if (ret) {
		printk(KERN_ERR "[tricolor-led] cannot initialize rpc_client!\n");
		tricolor_led->rpc_client = NULL;
		goto err_init_rpc_client;
	}

	tricolor_led->leds[0].name = "red";
	tricolor_led->leds[0].brightness_set = led_brightness_set_tricolor;

	tricolor_led->leds[1].name = "green";
	tricolor_led->leds[1].brightness_set = led_brightness_set_tricolor;

	tricolor_led->leds[2].name = "blue";
	tricolor_led->leds[2].brightness_set = led_brightness_set_tricolor;
	
	tricolor_led->leds[3].name = "flashlight";
	tricolor_led->leds[3].brightness_set = led_brightness_set_flash;

	for (i = 0; i < 4; i++) {	/* red, green, blue, flashlight */
		ret = led_classdev_register(&pdev->dev, &tricolor_led->leds[i]);
		if (ret) {
			printk(KERN_ERR
			       "tricolor_led: led_classdev_register failed\n");
			goto err_led_classdev_register_failed;
		}
	}

	for (i = 0; i < 4; i++) {
		ret = device_create_file(tricolor_led->leds[i].dev, &dev_attr_blink);
		if (ret) {
			printk(KERN_ERR
			       "tricolor_led: device_create_file failed\n");
			goto err_out_attr_blink;
		}
	}
	dev_set_drvdata(&pdev->dev, tricolor_led);
	return 0;

err_out_attr_blink:
        for (j = 0; j < i; j++)
                device_remove_file(tricolor_led->leds[j].dev, &dev_attr_blink);
        i = 4;

err_led_classdev_register_failed:
	for (j = 0; j < i; j++)
		led_classdev_unregister(&tricolor_led->leds[j]);

err_init_rpc_client:
	/* If above errors occurred, close pdata->rpc_client */
	if (tricolor_led->rpc_client) {
		oem_rapi_client_close();
		printk(KERN_ERR "tri-color-led: oem_rapi_client_close\n");
	}
	kfree(tricolor_led);
err:
	return ret;
}

static int __devexit tricolor_led_remove(struct platform_device *pdev)
{
	struct tricolor_led_data *tricolor_led;
	int i;
	printk(KERN_ERR "tricolor_led_remove: remove\n");

	tricolor_led = platform_get_drvdata(pdev);

	for (i = 0; i < 4; i++) {
		device_remove_file(tricolor_led->leds[i].dev, &dev_attr_blink);
		led_classdev_unregister(&tricolor_led->leds[i]);
	}
	/* close tricolor_led->rpc_client */
	oem_rapi_client_close();
	tricolor_led->rpc_client = NULL;

	kfree(tricolor_led);
	return 0;
}

static struct platform_driver tricolor_led_driver = {
	.probe = tricolor_led_probe,
	.remove = __devexit_p(tricolor_led_remove),
	.suspend = NULL,
	.resume = NULL,
	.driver = {
		   .name = "tricolor leds and flashlight",
		   .owner = THIS_MODULE,
		   },
};

static int __init tricolor_led_init(void)
{
	printk(KERN_ERR "tricolor_leds_backlight_init: module init\n");
	return platform_driver_register(&tricolor_led_driver);
}

static void __exit tricolor_led_exit(void)
{
	printk(KERN_ERR "tricolor_leds_backlight_exit: module exit\n");
	platform_driver_unregister(&tricolor_led_driver);
}

MODULE_AUTHOR("rockie cheng");
MODULE_DESCRIPTION("tricolor leds and flashlight driver");
MODULE_LICENSE("GPL");

module_init(tricolor_led_init);
module_exit(tricolor_led_exit);
