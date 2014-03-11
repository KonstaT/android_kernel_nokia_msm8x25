/*
 * leds-msm-pmic.c - MSM PMIC LEDs driver.
 *
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
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/leds-pmic8029.h>
#include <mach/pmic.h>

static struct pmic8029_leds_platform_data *pdata = NULL;

static void pmic8029_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	int ret;
	
	if (pdata->type == PMIC8029_DRV_TYPE_CUR) {
		ret = pmic_secure_mpp_config_i_sink(pdata->which, pdata->max.cur, value ? PM_MPP__I_SINK__SWITCH_ENA : PM_MPP__I_SINK__SWITCH_DIS);
		
	} else {
		ret = pmic_secure_mpp_control_digital_output(pdata->which, pdata->max.vol, value ? PM_MPP__DLOGIC_OUT__CTRL_HIGH : PM_MPP__DLOGIC_OUT__CTRL_LOW);
	}
	if (ret)
		dev_err(led_cdev->dev, "can't set leds (%d %d %d),ret = %d\n", (int)pdata->which, (int)pdata->type, (int)pdata->max.cur, ret);
}

static struct led_classdev pmic8029_led = {
	.name			= "keyboard-backlight",
	.brightness_set		= pmic8029_led_set,
	.brightness		= LED_OFF,
};

static int pmic8029_led_probe(struct platform_device *pdev)
{
	int rc;

	pdata = pdev->dev.platform_data;
	printk("%s probe\n", __func__);
	if (pdata == NULL) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -EINVAL;
	}

	rc = led_classdev_register(&pdev->dev, &pmic8029_led);
	if (rc) {
		dev_err(&pdev->dev, "unable to register led class driver\n");
		return rc;
	}

	dev_set_drvdata(&pdev->dev, pdata);

	pmic8029_led_set(&pmic8029_led, LED_OFF);

	return rc;
}

static int __devexit pmic8029_led_remove(struct platform_device *pdev)
{
	led_classdev_unregister(&pmic8029_led);

	return 0;
}

#ifdef CONFIG_PM
static int pmic8029_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
	led_classdev_suspend(&pmic8029_led);

	return 0;
}

static int pmic8029_led_resume(struct platform_device *dev)
{
	led_classdev_resume(&pmic8029_led);

	return 0;
}
#else
#define pmic8029_led_suspend NULL
#define pmic8029_led_resume NULL
#endif

static struct platform_driver pmic8029_led_driver = {
	.probe		= pmic8029_led_probe,
	.remove		= __devexit_p(pmic8029_led_remove),
	.suspend	= pmic8029_led_suspend,
	.resume		= pmic8029_led_resume,
	.driver		= {
		.name	= "keyboard-backlight",
		.owner	= THIS_MODULE,
	},
};

static int __init pmic8029_led_init(void)
{
	return platform_driver_register(&pmic8029_led_driver);
}
module_init(pmic8029_led_init);

static void __exit pmic8029_led_exit(void)
{
	platform_driver_unregister(&pmic8029_led_driver);
}
module_exit(pmic8029_led_exit);

MODULE_DESCRIPTION("MSM PMIC LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-leds");
