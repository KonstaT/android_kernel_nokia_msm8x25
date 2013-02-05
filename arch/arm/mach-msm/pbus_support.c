/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
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
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <asm/unwind.h>
#include <asm/cacheflush.h>

#define DRIVER_NAME		"msm_pbus"
#define PBUS_ERROR_STAT         0x14
#define PBUS_ERROR_ADDR         0x18

struct msm_pbus {
	void __iomem* regs;
	int irq;
};

static irqreturn_t pbus_error_handler(int irq, void *data)
{
	struct msm_pbus *pbus_dev = data;

	pr_info("%s:PBUS error detected PBUS_ERROR_STAT =%x\n", __func__,
			readl_relaxed(pbus_dev->regs + PBUS_ERROR_STAT));
	pr_info("%s:PBUS error detected for PBUS_ERROR_ADDR =%x\n", __func__,
			readl_relaxed(pbus_dev->regs + PBUS_ERROR_ADDR));

	unwind_backtrace(NULL, current);
	arch_trigger_all_cpu_backtrace();
	flush_cache_all();
	outer_flush_all();
	writel_relaxed(0x0, pbus_dev->regs + PBUS_ERROR_STAT);
	WARN_ON(1);
	return IRQ_HANDLED;
}

static int __devinit msm_pbus_handler_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct resource *res;
	struct msm_pbus *pbus_dev;

	pbus_dev = devm_kzalloc(&pdev->dev,
			sizeof(struct msm_pbus), GFP_KERNEL);
	if (!pbus_dev) {
		ret = -ENOMEM;
		goto pbus_out;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("%s: failed to get platform resource mem\n", __func__);
		ret = -ENODEV;
		goto pbus_out;
	}

	pbus_dev->regs = devm_ioremap(&pdev->dev,
			res->start, resource_size(res));
	if (!pbus_dev->regs) {
		pr_err("%s: ioremap failed\n", __func__);
		ret = -ENOMEM;
		goto pbus_out;
	}

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!res) {
		pr_err("%s: platform_get_irq failed\n", __func__);
		ret = -ENODEV;
		goto pbus_out;
	}

	pbus_dev->irq = res->start;
	ret = devm_request_irq(&pdev->dev, pbus_dev->irq, pbus_error_handler,
			IRQF_TRIGGER_RISING, "pbus_irq", pbus_dev);
	if (ret) {
		pr_err("%s: request irq failed\n", __func__);
		goto pbus_out;
	}

	dev_set_drvdata(&pdev->dev, pbus_dev);
	pr_info("%s: PBUS handler initialized\n", __func__);

pbus_out:
	return ret;
}

static struct platform_driver msm_pbus_driver = {
	.probe = msm_pbus_handler_probe,
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_pbus_error_handler_init(void)
{
	return platform_driver_register(&msm_pbus_driver);
}

module_init(msm_pbus_error_handler_init);

static void __exit msm_pbus_error_handler_exit(void)
{
	platform_driver_unregister(&msm_pbus_driver);
}

module_exit(msm_pbus_error_handler_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM PBUS HANDLER DRIVER");
MODULE_VERSION("1.00");
