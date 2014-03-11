/*
 * linux/drivers/mmc/host/msm_sdcc_raw.h
 *
 * Driver for Qualcomm MSM 8X55/8960 RAW SDCC Driver
 *
 * Copyright (C) 2012 Motorola Mobility, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 */

#ifndef __MSM_SDCC_MMC_RAW_H
#define __MSM_SDCC_MMC_RAW_H

#ifdef __KERNEL__

#ifdef CONFIG_MMC_MSM_RAW
void __init msm_init_apanic(void);
#else
static inline void __init msm_init_apanic(void) {}
#endif

#endif

#endif
