/* Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <asm/fiq.h>
#include <asm/unwind.h>
#include <asm/hardware/gic.h>
#include <asm/cacheflush.h>
#include <mach/irqs.h>
#include <mach/socinfo.h>
#include <mach/fiq.h>
#include <mach/msm_iomap.h>

#include "msm_watchdog.h"

#define MODULE_NAME "MSM7K_FIQ"

struct msm_watchdog_dump msm_dump_cpu_ctx[NR_CPUS];
static int fiq_counter;
static int msm_fiq_no;
void *msm7k_fiq_stack[NR_CPUS];
static spinlock_t msm_fiq_lock;

/* Called from the FIQ asm handler */
void msm7k_fiq_handler(void)
{
	struct pt_regs ctx_regs;
	static cpumask_t fiq_cpu_mask;
	int this_cpu;
	unsigned long msm_fiq_flags;

	spin_lock_irqsave(&msm_fiq_lock, msm_fiq_flags);
	this_cpu = smp_processor_id();

	pr_info("%s: Fiq is received on CPU%d\n", __func__, this_cpu);
	fiq_counter += 1;

	ctx_regs.ARM_pc = msm_dump_cpu_ctx[this_cpu].fiq_r14;
	ctx_regs.ARM_lr = msm_dump_cpu_ctx[this_cpu].svc_r14;
	ctx_regs.ARM_sp = msm_dump_cpu_ctx[this_cpu].svc_r13;
	ctx_regs.ARM_fp = msm_dump_cpu_ctx[this_cpu].usr_r11;
	unwind_backtrace(&ctx_regs, current);

	if (fiq_counter == 1 && (cpu_is_msm8625() || cpu_is_msm8625q())) {
		if (cpu_is_msm8625()) {
			/*
			 * Dumping MPA5 status registers
			 */
			pr_info("MPA5_CFG_CTL_REG = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x30));
			pr_info("MPA5_BOOT_REMAP_ADDR = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x34));
			pr_info("MPA5_GDFS_CNT_VAL = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x38));
			pr_info("MPA5_STATUS_REG = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x3c));

			/*
			 * Dumping SPM registers
			 */
			pr_info("SPM0_SAW2_CFG	= 0x%x\n",
					__raw_readl(MSM_SAW0_BASE + 0x8));
			pr_info("SPM0_SAW2_STS_0 = 0x%x\n",
					__raw_readl(MSM_SAW0_BASE + 0xC));
			pr_info("SPM0_SAW2_CTL = 0x%x\n",
					__raw_readl(MSM_SAW0_BASE + 0x20));
			pr_info("SPM1_SAW2_CFG = 0x%x\n",
					__raw_readl(MSM_SAW1_BASE + 0x8));
			pr_info("SPM1_SAW2_STS_0 = 0x%x\n",
					__raw_readl(MSM_SAW1_BASE + 0xC));
			pr_info("SPM1_SAW2_CTL = 0x%x\n",
					__raw_readl(MSM_SAW1_BASE + 0x20));
		} else if (cpu_is_msm8625q()) {
			/*
			 * Dumping MPA5 status registers
			 */
			pr_info("MPA5_CFG_CTL_REG = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x30));
			pr_info("MPA5_BOOT_REMAP_ADDR = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x34));
			pr_info("MPA5_GDFS_CNT_VAL = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x38));
			pr_info("MPA5_STATUS_REG = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x3c));
			pr_info("MPA5_CFG_CTL_REG1 = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x48));
			pr_info("MPA5_BOOT_REMAP_ADDR1 = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x4c));
			pr_info("MPA5_STATUS_REG1 = 0x%x\n",
					__raw_readl(MSM_CFG_CTL_BASE + 0x50));

			/*
			 * Dumping SPM registers
			 */
			pr_info("SPM0_SAW2_CFG = 0x%x\n",
					__raw_readl(MSM_SAW0_BASE + 0x8));
			pr_info("SPM0_SAW2_STS_0 = 0x%x\n",
					__raw_readl(MSM_SAW0_BASE + 0xC));
			pr_info("SPM0_SAW2_CTL = 0x%x\n",
					__raw_readl(MSM_SAW0_BASE + 0x20));
			pr_info("SPM1_SAW2_CFG = 0x%x\n",
					__raw_readl(MSM_SAW1_BASE + 0x8));
			pr_info("SPM1_SAW2_STS_0 = 0x%x\n",
					__raw_readl(MSM_SAW1_BASE + 0xC));
			pr_info("SPM1_SAW2_CTL = 0x%x\n",
					__raw_readl(MSM_SAW1_BASE + 0x20));
			pr_info("SPM2_SAW2_CFG = 0x%x\n",
					__raw_readl(MSM_SAW2_BASE + 0x8));
			pr_info("SPM2_SAW2_STS_0 = 0x%x\n",
					__raw_readl(MSM_SAW2_BASE + 0xC));
			pr_info("SPM2_SAW2_CTL = 0x%x\n",
					__raw_readl(MSM_SAW2_BASE + 0x20));
			pr_info("SPM3_SAW2_CFG = 0x%x\n",
					__raw_readl(MSM_SAW3_BASE + 0x8));
			pr_info("SPM3_SAW2_STS_0 = 0x%x\n",
					__raw_readl(MSM_SAW3_BASE + 0xC));
			pr_info("SPM3_SAW2_CTL = 0x%x\n",
					__raw_readl(MSM_SAW3_BASE + 0x20));
		}
		cpumask_copy(&fiq_cpu_mask, cpu_possible_mask);
		cpu_clear(this_cpu, fiq_cpu_mask);
		gic_raise_secure_softirq(&fiq_cpu_mask, GIC_SECURE_SOFT_IRQ);
	}

	flush_cache_all();
	outer_flush_all();
	spin_unlock_irqrestore(&msm_fiq_lock, msm_fiq_flags);
	return;
}

struct fiq_handler msm7k_fh = {
	.name = MODULE_NAME,
};

static int __init msm_setup_fiq_handler(void)
{
	int i, ret = 0;

	spin_lock_init(&msm_fiq_lock);
	claim_fiq(&msm7k_fh);
	set_fiq_handler(&msm7k_fiq_start, msm7k_fiq_length);

	for_each_possible_cpu(i) {
		msm7k_fiq_stack[i] = (void *)__get_free_pages(GFP_KERNEL,
			THREAD_SIZE_ORDER);
		if (msm7k_fiq_stack[i] == NULL)
			break;
	}

	if (i != nr_cpumask_bits) {
		pr_err("FIQ STACK SETUP IS NOT SUCCESSFUL\n");
		for (i = 0; i < nr_cpumask_bits && msm7k_fiq_stack[i] != NULL;
					i++)
			free_pages((unsigned long)msm7k_fiq_stack[i],
					THREAD_SIZE_ORDER);
		return -ENOMEM;
	}

	fiq_set_type(msm_fiq_no, IRQF_TRIGGER_RISING);
	if (cpu_is_msm8625() || cpu_is_msm8625q())
		gic_set_irq_secure(msm_fiq_no);
	else
		msm_fiq_select(msm_fiq_no);

	enable_irq(msm_fiq_no);
	pr_info("%s : MSM FIQ handler setup--done\n", __func__);
	return ret;
}

static int __init init7k_fiq(void)
{
	if (cpu_is_msm8625() || cpu_is_msm8625q())
		msm_fiq_no = MSM8625_INT_A9_M2A_2;
	else
		msm_fiq_no = INT_A9_M2A_2;

	if (msm_setup_fiq_handler())
		pr_err("MSM FIQ INIT FAILED\n");

	return 0;
}
fs_initcall(init7k_fiq);
