/*  
 *  Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *  Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 and
 *  only version 2 as published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 */


#ifndef __LINUX_LTR502_H
#define __LINUX_LTR502_H

#include <linux/types.h>

#ifdef __KERNEL__

struct ltr502_platform_data {
	int int_gpio;  /* proximity-sensor- external int-gpio */
};
#endif /* __KERNEL__ */

#endif
