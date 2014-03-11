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
#ifndef __CACHE_NOSYNC_H
#define __CACHE_NOSYNC_H

#if defined(CONFIG_CPU_V7)
/*
 * dcache_line_size - get the minimum D-cache line size from the CTR register
 * on ARMv7.
 */
#define get_dcache_line_size(size)		\
	do {					\
		unsigned long temp;		\
		__asm__ __volatile__ (		\
		"mrc p15, 0, %0, c0, c0, 1\n\t"	\
		"lsr %0, %0, #16\n\t"		\
		"and %0, %0, #0xf\n\t"		\
		"mov %1, #4\n\t"		\
		"mov %1, %1, lsl %0\n\t"	\
		: "=&r" (temp), "=r" (size)	\
		: : "cc");			\
	} while (0)


#define cache_clean_nosync(start, end, cachesize)	\
	do {						\
		unsigned long temp;			\
		__asm__ __volatile__ (			\
		"sub %0, %3, #1\n\t"			\
		"bic %1, %1, %0\n\t"			\
		"1:\n\t"				\
		"mcr p15, 0, %1, c7, c10, 1\n\t"	\
		"add %1, %1, %3\n\t"			\
		"cmp %1, %2\n\t"			\
		"blo 1b\n\t"				\
		"dsb\n\t"				\
		: "=&r" (temp)				\
		: "r" (start), "r" (end), "r" (cachesize) \
		: "cc", "memory");			\
	} while (0)

/* XXX: Is it safe to remove dsb? since later it will flush L2 cache */
#define cache_clean_nosync_oneline(start, cachesize)	\
	do {						\
		unsigned long temp;			\
		__asm__ __volatile__ (			\
		"sub %0, %2, #1\n\t"			\
		"bic %1, %1, %0\n\t"			\
		"mcr p15, 0, %1, c7, c10, 1\n\t"	\
		"dsb\n\t"				\
		: "=&r" (temp)				\
		: "r" (start), "r" (cachesize) 		\
		: "cc", "memory");			\
	} while (0)


#elif defined(CONFIG_CPU_V6)

#else
#error "not supported the ARCH other than V6, V7"
#endif

#endif
