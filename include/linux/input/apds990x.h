/*
 * This file is part of the APDS990x sensor driver.
 * Chip is combined proximity and ambient light sensor.
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * Contact: Samu Onkalo <samu.p.onkalo@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#ifndef __LINUX_APDS990X_H
#define __LINUX_APDS990X_H

#include <linux/types.h>

struct apds990x_platform_data {
	int (*power_onoff)(int onoff);
	int irq;  /* proximity/light-sensor- external irq*/
	unsigned int ps_det_thld;
	unsigned int ps_hsyt_thld;
	unsigned int als_hsyt_thld;
};

#endif
