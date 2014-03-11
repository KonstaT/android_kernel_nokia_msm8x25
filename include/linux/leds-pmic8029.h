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
#ifndef __LEDS_PMIC8029_H__
#define __LEDS_PMIC8029_H__

#include <mach/pmic.h>

enum drv_type {
	PMIC8029_DRV_TPYE_VOL,
	PMIC8029_DRV_TYPE_CUR,
};

struct pmic8029_leds_platform_data {
	enum mpp_which which;
	enum drv_type type;
	union {
		enum mpp_dlogic_level vol;
		enum mpp_i_sink_level cur;
	}max;
};

#endif /* __LEDS_PMIC8029_H__ */
