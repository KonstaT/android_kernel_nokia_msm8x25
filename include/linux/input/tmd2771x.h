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

#ifndef __LINUX_TMD2771X_H
#define __LINUX_TMD2771X_H

#include <linux/types.h>

struct tmd2771x_platform_data {
	int (*power_onoff)(int onoff);
	int irq;  /* proximity/light-sensor- external irq*/
	unsigned int ps_det_thld;
	unsigned int ps_hsyt_thld;
	unsigned int als_hsyt_thld;
};

#endif
