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

#include <asm/hardware/cache-l2x0.h>

/* Add 300 NOPs after 'wfi' for 8x25 target */
.macro DELAY_8x25, rept
#ifdef CONFIG_ARCH_MSM8625
	.rept   \rept
	nop
	.endr
#endif
.endm

/* Switch between smp_to_amp/amp_to_smp configuration */
.macro SET_SMP_COHERENCY, on = 0
	ldr     r0, =target_type
	ldr     r0, [r0]
	mov     r1, #TARGET_IS_8625
	cmp     r0, r1
	bne     skip\@
	mrc	p15, 0, r0, c1, c0, 1	/* read ACTLR register */
	.if     \on
	orr	r0, r0, #(1 << 6)	/* Set the SMP bit in ACTLR */
	.else
	bic	r0, r0, #(1 << 6)	/* Clear the SMP bit */
	.endif
	mcr	p15, 0, r0, c1, c0, 1	/* write ACTLR register */
	isb
skip\@:
.endm

/*
 * Enable the "L2" cache, not require to restore the controller registers
 */
.macro ENABLE_8x25_L2
	ldr     r0, =target_type
	ldr     r0, [r0]
	mov     r1, #TARGET_IS_8625
	cmp     r0, r1
	bne     skip_enable\@
	ldr     r0, =apps_power_collapse
	ldr     r0, [r0]
	cmp     r0, #POWER_COLLAPSED
	bne     skip_enable\@
	ldr     r0, =l2x0_base_addr
	ldr	r0, [r0]
	mov	r1, #0x1
	str	r1, [r0, #L2X0_CTRL]
	dmb
skip_enable\@:
.endm

/*
 * Perform the required operation
 * operation: type of operation on l2 cache (e.g: clean&inv or inv)
 * l2_enable: enable or disable
 */
.macro DO_CACHE_OPERATION, operation, l2_enable
	ldr     r2, =l2x0_base_addr
	ldr	r2, [r2]
	ldr     r0, =0xff
	str     r0, [r2, #\operation]
wait\@:
	ldr	r0, [r2, #\operation]
	ldr	r1, =0xff
	ands    r0, r0, r1
	bne     wait\@
	dsb
	mov	r0, #0x0
	str	r0, [r2, #L2X0_CACHE_SYNC]
	dsb
	mov     r1, #\l2_enable
	str     r1, [r2, #L2X0_CTRL]
.endm

/*
 * Clean and invalidate the L2 cache.
 * 1. Check the target type
 * 2. Check whether we are coming from PC are not
 * 3. Save 'aux', 'data latency', & 'prefetch ctlr' registers
 * 4. Start L2 clean & invalidation operation
 * 5. Disable the L2 cache
 */
.macro SUSPEND_8x25_L2
	ldr     r0, =target_type
	ldr     r0, [r0]
	mov     r1, #TARGET_IS_8625
	cmp     r0, r1
	bne     skip_suspend\@
	ldr	r0, =apps_power_collapse
	ldr	r0, [r0]
	cmp	r0, #POWER_COLLAPSED
	bne	skip_suspend\@
	ldr	r0, =l2x0_saved_ctrl_reg_val
	ldr	r1, =l2x0_base_addr
	ldr	r1, [r1]
	ldr	r2, [r1, #L2X0_AUX_CTRL]
	str	r2, [r0, #0x0] /* store aux_ctlr reg value */
	ldr     r2, [r1, #L2X0_DATA_LATENCY_CTRL]
	str     r2, [r0, #0x4] /* store data latency reg value */
	ldr     r2, [r1, #L2X0_PREFETCH_CTRL]
	str     r2, [r0, #0x8] /* store prefetch_ctlr reg value */
	DO_CACHE_OPERATION L2X0_CLEAN_INV_WAY OFF
	dmb
skip_suspend\@:
.endm

/*
 * Coming back from a successful PC
 * 1. Check the target type
 * 2. Check whether we are going to PC are not
 * 3. Disable the L2 cache
 * 4. Restore 'aux', 'data latency', & 'prefetch ctlr' reg
 * 5. Invalidate the cache
 * 6. Enable the L2 cache
 */
.macro RESUME_8x25_L2
	ldr     r0, =target_type
	ldr     r0, [r0]
	mov     r1, #TARGET_IS_8625
	cmp     r0, r1
	bne     skip_resume\@
	ldr	r0, =apps_power_collapse
	ldr	r0, [r0]
	cmp	r0, #POWER_COLLAPSED
	bne	skip_resume\@
	ldr     r1, =l2x0_base_addr
	ldr	r1, [r1]
	mov     r0, #0x0
	str     r0, [r1, #L2X0_CTRL]
	ldr     r0, =l2x0_saved_ctrl_reg_val
	ldr     r2, [r0, #0x0]
	str	r2, [r1, #L2X0_AUX_CTRL] /* restore aux_ctlr reg value */
	ldr	r2, [r0, #0x4]
	str	r2, [r1, #L2X0_DATA_LATENCY_CTRL]
	ldr	r2, [r0, #0x8]
	str     r2, [r1, #L2X0_PREFETCH_CTRL]
	DO_CACHE_OPERATION L2X0_INV_WAY ON
skip_resume\@:
.endm

.macro WRITE_SMEM, addr, val
	ldr     r0, =apps_power_collapse
	ldr	r0, [r0]
	cmp     r0, #POWER_COLLAPSED
	bne	dont_write_smem
	ldr	r0, =\addr
	ldr	r0, [r0]
	mov	r1, #\val
	/* write to "reserved1" field in "msm_pm_smem_t" structure */
	str	r1, [r0, #0xC]
dont_write_smem:
.endm

.macro MARKER, addr, spc_val, pc_val, on
	.if \on
	stmfd   sp!, {r5, r6}
	.endif
	ldr     r5, =\addr
	.if \on
	ldr     r5, [r5]
	.endif
	mrc     p15, 0, r6, c0, c0, 5
	and	r6, r6, #0x3
	teq     r6, #0x1
	beq     core1\@
	teq     r6, #0x2
	beq     core2\@
	teq     r6, #0x3
	beq     core3\@

	ldr     r6, [r5, #0x34]
	cmp     r6, #0x1
	beq     log_for_pc\@ /* CORE0 is doing PC */
	mov     r6, #\spc_val
	str     r6, [r5, #0x10]
	b       exit\@
core1\@:
	mov     r6, #\spc_val
	str     r6, [r5, #0x14]
	b       exit\@
core2\@:
	mov     r6, #\spc_val
	str     r6, [r5, #0x18]
	b       exit\@
core3\@:
	mov     r6, #\spc_val
	str     r6, [r5, #0x1C]
	b       exit\@
log_for_pc\@:
	mov     r6, #\pc_val
	str     r6, [r5, #0x30]
exit\@:
	.if \on
	ldmfd   sp!, {r5, r6}
	.endif
.endm

.macro	DUMP_SPM_REG, addr
	stmfd	sp!, {r4, r5, r6}
	ldr     r4, =target_type
	ldr     r4, [r4]
	mov     r5, #TARGET_IS_8625
	cmp     r4, r5
	ldr     r5, =\addr
	ldr     r5, [r5]	/* marker base */
	bne     log_apps_pwrdown\@

	mrc     p15, 0, r4, c0, c0, 5
	and     r4, r4, #0x3	/* core number */

	teq     r4, #0x1
	beq     core1\@
	teq     r4, #0x2
	beq     core2\@
	teq     r4, #0x3
	beq     core3\@

	/*
	 * SAW2_CFG (0x8)
	 */
	ldr	r4, =spm0_base_addr
	ldr	r4, [r4]
	ldr	r6, [r4, #0x8]
	str	r6, [r5, #0x50]
	/*
	 * SAW2_SPM_CTL (0x20)
	 */
	ldr     r6, [r4, #0x20]
	str     r6, [r5, #0x54]
	b	log_apps_pwrdown\@
core1\@:
	/*
	 * SAW2_CFG (0x8)
	 */
	ldr	r4, =spm1_base_addr
	ldr	r4, [r4]
	ldr	r6, [r4, #0x8]
	str	r6, [r5, #0x60]
	/*
	 * SAW2_SPM_CTL (0x20)
	 */
	ldr	r6, [r4, #0x20]
	str	r6, [r5, #0x64]
	b	exit\@

core2\@:
	/*
	 * SAW2_CFG (0x8)
	 */
	ldr	r4, =spm2_base_addr
	ldr	r4, [r4]
	ldr	r6, [r4, #0x8]
	str	r6, [r5, #0x70]
	/*
	 * SAW2_SPM_CTL (0x20)
	 */
	ldr     r6, [r4, #0x20]
	str     r6, [r5, #0x74]
	b	exit\@
core3\@:
	/*
	 * SAW2_CFG (0x8)
	 */
	ldr	r4, =spm3_base_addr
	ldr     r4, [r4]
	ldr     r6, [r4, #0x8]
	str	r6, [r5, #0x80]

	/*
	 * SAW2_SPM_CTL (0x20)
	 */
	ldr     r6, [r4, #0x20]
	str     r6, [r5, #0x84]
	b	exit\@
log_apps_pwrdown\@:
	/*
	 * APPS_PWRDOWN
	 */
	ldr     r4, =apps_pwr_dwn
	ldr     r4, [r4]
	ldr     r4, [r4]
	str     r4, [r5, #0x58]
exit\@:
	ldmfd	sp!, {r4, r5, r6}
.endm
