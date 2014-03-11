/* Copyright (c) 2012, The Linux Foundation. All Rights Reserved.
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
#include <linux/slab.h>
#include <linux/module.h>

#include <mach/pmic.h>

#define LED_MPP(x)		((x) & 0xFF)
#define LED_CURR(x)		((x) >> 16)

struct pmic_mpp_led_data {
	struct led_classdev cdev;
	int which;
	int type;
	int max;
};

static void pm_mpp_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{
	struct pmic_mpp_led_data *led;
	int ret;
	bool enable;

	led = container_of(led_cdev, struct pmic_mpp_led_data, cdev);

	if (value < LED_OFF || value > led->cdev.max_brightness) {
		dev_err(led->cdev.dev, "Invalid brightness value");
		return;
	}

	enable = value ? true : false;

	if(value > led->max) {
		value = led->max;
	}

	if(PMIC8029_DRV_TYPE_CUR == led->type) {
		ret = pmic_secure_mpp_config_i_sink(led->which, value,
				enable ? PM_MPP__I_SINK__SWITCH_ENA :
					PM_MPP__I_SINK__SWITCH_DIS);
	} else {
		ret = pmic_secure_mpp_control_digital_output(led->which,
			value,
			enable ? PM_MPP__DLOGIC_OUT__CTRL_HIGH : PM_MPP__DLOGIC_OUT__CTRL_LOW);
	}
	if (ret)
		dev_err(led_cdev->dev, "can't set mpp led\n");
}

static int pmic_mpp_led_probe(struct platform_device *pdev)
{
	const struct pmic8029_leds_platform_data *pdata = pdev->dev.platform_data;
	struct pmic_mpp_led_data *led, *tmp_led;
	int i, rc;

	if (!pdata) {
		dev_err(&pdev->dev, "platform data not supplied\n");
		return -EINVAL;
	}

	led = kcalloc(pdata->num_leds, sizeof(*led), GFP_KERNEL);
	if (!led) {
		dev_err(&pdev->dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, led);

	for (i = 0; i < pdata->num_leds; i++) {
		tmp_led	= &led[i];
		tmp_led->cdev.name = pdata->leds[i].name;
		tmp_led->cdev.brightness_set = pm_mpp_led_set;
		tmp_led->cdev.brightness = LED_OFF;
		tmp_led->cdev.max_brightness = LED_FULL;
		tmp_led->which = pdata->leds[i].which;
		tmp_led->type = pdata->leds[i].type;
		if(PMIC8029_DRV_TYPE_CUR == tmp_led->type) {
			tmp_led->max = pdata->leds[i].max.cur;
		} else {
			tmp_led->max = pdata->leds[i].max.vol;
		}

		if (PMIC8029_DRV_TYPE_CUR == tmp_led->type &&
			(tmp_led->max < PM_MPP__I_SINK__LEVEL_5mA ||
			tmp_led->max > PM_MPP__I_SINK__LEVEL_40mA)) {
			dev_err(&pdev->dev, "invalid current\n");
			goto unreg_led_cdev;
		}

		rc = led_classdev_register(&pdev->dev, &tmp_led->cdev);
		if (rc) {
			dev_err(&pdev->dev, "failed to register led\n");
			goto unreg_led_cdev;
		}
	}

	return 0;

unreg_led_cdev:
	while (i)
		led_classdev_unregister(&led[--i].cdev);

	kfree(led);
	return rc;

}

static int __devexit pmic_mpp_led_remove(struct platform_device *pdev)
{
	const struct pmic8029_leds_platform_data *pdata = pdev->dev.platform_data;
	struct pmic_mpp_led_data *led = platform_get_drvdata(pdev);
	int i;

	for (i = 0; i < pdata->num_leds; i++)
		led_classdev_unregister(&led[i].cdev);

	kfree(led);

	return 0;
}

static struct platform_driver pmic_mpp_led_driver = {
	.probe		= pmic_mpp_led_probe,
	.remove		= __devexit_p(pmic_mpp_led_remove),
	.driver		= {
		.name	= "pmic-mpp-leds",
		.owner	= THIS_MODULE,
	},
};

static int __init pmic_mpp_led_init(void)
{
	return platform_driver_register(&pmic_mpp_led_driver);
}
module_init(pmic_mpp_led_init);

static void __exit pmic_mpp_led_exit(void)
{
	platform_driver_unregister(&pmic_mpp_led_driver);
}
module_exit(pmic_mpp_led_exit);

MODULE_DESCRIPTION("PMIC MPP LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:pmic-mpp-leds");
