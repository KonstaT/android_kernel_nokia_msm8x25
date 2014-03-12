/* Copyright (c) 2012-2013, The Linux Foundation. All Rights Reserved.
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

#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio_event.h>
#include <linux/leds.h>
#include <linux/i2c/atmel_mxt_ts.h>
#include <linux/i2c.h>
#include <linux/input/rmi_platformdata.h>
#include <linux/input/rmi_i2c.h>
#include <linux/delay.h>
#include <linux/atmel_maxtouch.h>
#include <linux/input/ft5x06_ts.h>
#include <linux/leds-msm-tricolor.h>
#include <asm/gpio.h>
#include <asm/mach-types.h>
#include <mach/rpc_server_handset.h>
#include <mach/pmic.h>
#include <mach/socinfo.h>
#include "devices.h"
#include <linux/input/synaptics_dsx.h>
#ifdef CONFIG_TOUCHSCREEN_FT6306
#include <linux/ft6306_touch.h>
#endif
#include "board-msm7627a.h"
#include "devices-msm7x2xa.h"

#define ATMEL_TS_I2C_NAME "maXTouch"
#define ATMEL_X_OFFSET 13
#define ATMEL_Y_OFFSET 0

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_I2C) || \
defined(CONFIG_TOUCHSCREEN_SYNAPTICS_RMI4_I2C_MODULE)

#ifndef CLEARPAD3000_ATTEN_GPIO
#define CLEARPAD3000_ATTEN_GPIO (48)
#define CLEARPAD3000_ATTEN_GPIO_EVBD_PLUS (115)
#endif

#ifndef CLEARPAD3000_RESET_GPIO
#define CLEARPAD3000_RESET_GPIO (26)
#endif

#define KP_INDEX(row, col) ((row)*ARRAY_SIZE(kp_col_gpios) + (col))

/******************** SYNAPTICS *********************************/

/*	Synaptics Thin Driver	*/

#define CLEARPAD3000_ADDR 0x20

static unsigned char synaptic_rmi4_button_codes[] = {KEY_MENU, KEY_HOME,
							KEY_BACK};

static struct synaptics_rmi4_capacitance_button_map synaptic_rmi4_button_map = {
	.nbuttons = ARRAY_SIZE(synaptic_rmi4_button_codes),
	.map = synaptic_rmi4_button_codes,
};

static struct synaptics_rmi4_platform_data rmi4_platformdata = {
	.irq_flags = IRQF_TRIGGER_FALLING,
	.irq_gpio = CLEARPAD3000_ATTEN_GPIO_EVBD_PLUS,
	.capacitance_button_map = &synaptic_rmi4_button_map,
};

static struct i2c_board_info rmi4_i2c_devices[] = {
	{
		I2C_BOARD_INFO("synaptics_rmi4_i2c",
			CLEARPAD3000_ADDR),
		.platform_data = &rmi4_platformdata,
	},
};

/******************** SYNAPTICS *********************************/
#if 0
static unsigned int kp_row_gpios[] = {31, 32, 33, 34, 35};
static unsigned int kp_col_gpios[] = {36, 37, 38, 39, 40};

static const unsigned short keymap[ARRAY_SIZE(kp_col_gpios) *
					  ARRAY_SIZE(kp_row_gpios)] = {
	[KP_INDEX(0, 0)] = KEY_7,
	[KP_INDEX(0, 1)] = KEY_DOWN,
	[KP_INDEX(0, 2)] = KEY_UP,
	[KP_INDEX(0, 3)] = KEY_RIGHT,
	[KP_INDEX(0, 4)] = KEY_ENTER,

	[KP_INDEX(1, 0)] = KEY_LEFT,
	[KP_INDEX(1, 1)] = KEY_SEND,
	[KP_INDEX(1, 2)] = KEY_1,
	[KP_INDEX(1, 3)] = KEY_4,
	[KP_INDEX(1, 4)] = KEY_CLEAR,

	[KP_INDEX(2, 0)] = KEY_6,
	[KP_INDEX(2, 1)] = KEY_5,
	[KP_INDEX(2, 2)] = KEY_8,
	[KP_INDEX(2, 3)] = KEY_3,
	[KP_INDEX(2, 4)] = KEY_NUMERIC_STAR,

	[KP_INDEX(3, 0)] = KEY_9,
	[KP_INDEX(3, 1)] = KEY_NUMERIC_POUND,
	[KP_INDEX(3, 2)] = KEY_0,
	[KP_INDEX(3, 3)] = KEY_2,
	[KP_INDEX(3, 4)] = KEY_SLEEP,

	[KP_INDEX(4, 0)] = KEY_BACK,
	[KP_INDEX(4, 1)] = KEY_HOME,
	[KP_INDEX(4, 2)] = KEY_MENU,
	[KP_INDEX(4, 3)] = KEY_VOLUMEUP,
	[KP_INDEX(4, 4)] = KEY_VOLUMEDOWN,
};

/* SURF keypad platform device information */
static struct gpio_event_matrix_info kp_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keymap,
	.output_gpios	= kp_row_gpios,
	.input_gpios	= kp_col_gpios,
	.noutputs	= ARRAY_SIZE(kp_row_gpios),
	.ninputs	= ARRAY_SIZE(kp_col_gpios),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info[] = {
	&kp_matrix_info.info
};

static struct gpio_event_platform_data kp_pdata = {
	.name		= "7x27a_kp",
	.info		= kp_info,
	.info_count	= ARRAY_SIZE(kp_info)
};

static struct platform_device kp_pdev = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &kp_pdata,
	},
};

#endif
/* 8625 keypad device information */
static unsigned int kp_row_gpios_8625[] = {31};
static unsigned int kp_col_gpios_8625[] = {36, 37};

static const unsigned short keymap_8625[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
};

static const unsigned short keymap_8625_qrd5[] = {
	KEY_VOLUMEDOWN,
	KEY_VOLUMEUP,
};

static struct gpio_event_matrix_info kp_matrix_info_8625 = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_8625,
	.output_gpios   = kp_row_gpios_8625,
	.input_gpios    = kp_col_gpios_8625,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_8625),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_8625),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_8625[] = {
	&kp_matrix_info_8625.info,
};

static struct gpio_event_platform_data kp_pdata_8625 = {
	.name           = "7x27a_kp",
	.info           = kp_info_8625,
	.info_count     = ARRAY_SIZE(kp_info_8625)
};

static struct platform_device kp_pdev_8625 = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_8625,
	},
};

/* skud keypad device information */
static unsigned int kp_row_gpios_skud[] = {31, 32};
static unsigned int kp_col_gpios_skud[] = {37};

static const unsigned short keymap_skud[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
};

static struct gpio_event_matrix_info kp_matrix_info_skud = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_skud,
	.output_gpios   = kp_row_gpios_skud,
	.input_gpios    = kp_col_gpios_skud,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_skud),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_skud),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_ACTIVE_HIGH |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_skud[] = {
	&kp_matrix_info_skud.info,
};

static struct gpio_event_platform_data kp_pdata_skud = {
	.name           = "7x27a_kp",
	.info           = kp_info_skud,
	.info_count     = ARRAY_SIZE(kp_info_skud)
};

static struct platform_device kp_pdev_skud = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_skud,
	},
};
/* end of skud keypad device information */

/* skue keypad device information */
static unsigned int kp_row_gpios_skue[] = {31, 32};
static unsigned int kp_col_gpios_skue[] = {37};

static const unsigned short keymap_skue[] = {
	KEY_VOLUMEUP,
	KEY_VOLUMEDOWN,
};

static struct gpio_event_matrix_info kp_matrix_info_skue = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_skue,
	.output_gpios   = kp_row_gpios_skue,
	.input_gpios    = kp_col_gpios_skue,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_skue),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_skue),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_ACTIVE_HIGH |
			  GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_skue[] = {
	&kp_matrix_info_skue.info,
};

static struct gpio_event_platform_data kp_pdata_skue = {
	.name           = "7x27a_kp",
	.info           = kp_info_skue,
	.info_count     = ARRAY_SIZE(kp_info_skue)
};

static struct platform_device kp_pdev_skue = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_skue,
	},
};
/* end of skue keypad device information */

#define LED_GPIO_PDM 96

#define MXT_TS_IRQ_GPIO         48
#define MXT_TS_RESET_GPIO       26
#define MXT_TS_EVBD_IRQ_GPIO    115
#define MAX_VKEY_LEN		100

static ssize_t mxt_virtual_keys_register(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	char *virtual_keys = __stringify(EV_KEY) ":" __stringify(KEY_MENU) \
		":60:860:110:80" ":" __stringify(EV_KEY) \
		":" __stringify(KEY_HOME)   ":180:860:110:80" \
		":" __stringify(EV_KEY) ":" \
		__stringify(KEY_BACK) ":300:860:110:80" \
		":" __stringify(EV_KEY) ":" \
		__stringify(KEY_SEARCH)   ":420:860:110:80" "\n";

	return snprintf(buf, strnlen(virtual_keys, MAX_VKEY_LEN) + 1 , "%s",
			virtual_keys);
}

static struct kobj_attribute mxt_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.atmel_mxt_ts",
		.mode = S_IRUGO,
	},
	.show = &mxt_virtual_keys_register,
};

static struct attribute *mxt_virtual_key_properties_attrs[] = {
	&mxt_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group mxt_virtual_key_properties_attr_group = {
	.attrs = mxt_virtual_key_properties_attrs,
};

struct kobject *mxt_virtual_key_properties_kobj;

static int mxt_vkey_setup(void)
{
	int retval = 0;

	mxt_virtual_key_properties_kobj =
		kobject_create_and_add("board_properties", NULL);
	if (mxt_virtual_key_properties_kobj)
		retval = sysfs_create_group(mxt_virtual_key_properties_kobj,
				&mxt_virtual_key_properties_attr_group);
	if (!mxt_virtual_key_properties_kobj || retval)
		pr_err("failed to create mxt board_properties\n");

	return retval;
}

static const u8 mxt_config_data[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	16, 1, 0, 0, 0, 0, 0, 0,
	/* T7 Object */
	32, 16, 50,
	/* T8 Object */
	30, 0, 20, 20, 0, 0, 20, 0, 50, 0,
	/* T9 Object */
	3, 0, 0, 18, 11, 0, 32, 75, 3, 3,
	0, 1, 1, 0, 10, 10, 10, 10, 31, 3,
	223, 1, 11, 11, 15, 15, 151, 43, 145, 80,
	100, 15, 0, 0, 0,
	/* T15 Object */
	131, 0, 11, 11, 1, 1, 0, 45, 3, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	0, 0, 0, 0, 0, 0, 0, 0,
	/* T46 Object */
	0, 2, 32, 48, 0, 0, 0, 0, 0,
	/* T47 Object */
	1, 20, 60, 5, 2, 50, 40, 0, 0, 40,
	/* T48 Object */
	1, 12, 80, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};

static const u8 mxt_config_data_qrd5[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	21, 0, 2, 0, 0, 0, 0, 0,
	/* T7 Object */
	24, 12, 10,
	/* T8 Object */
	30, 0, 20, 20, 10, 0, 0, 0, 10, 192,
	/* T9 Object */
	131, 0, 0, 18, 11, 0, 16, 70, 2, 1,
	0, 2, 1, 62, 10, 10, 10, 10, 107, 3,
	223, 1, 2, 2, 20, 20, 172, 40, 139, 110,
	10, 15, 0, 0, 0,
	/* T15 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	3, 20, 45, 40, 128, 0, 0, 0,
	/* T46 Object */
	0, 2, 16, 16, 0, 0, 0, 0, 0,
	/* T47 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T48 Object */
	1, 12, 64, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};

static const u8 mxt_config_data_qrd5a[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	21, 0, 3, 0, 0, 0, 0, 0,
	/* T7 Object */
	24, 12, 10,
	/* T8 Object */
	30, 0, 20, 20, 10, 0, 0, 0, 10, 192,
	/* T9 Object */
	131, 0, 0, 18, 11, 0, 16, 70, 2, 1,
	0, 2, 1, 62, 10, 10, 10, 10, 107, 3,
	223, 1, 2, 2, 20, 20, 172, 40, 139, 110,
	10, 15, 0, 0, 0,
	/* T15 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	3, 20, 45, 40, 128, 0, 0, 0,
	/* T46 Object */
	0, 2, 16, 16, 0, 0, 0, 0, 0,
	/* T47 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T48 Object */
	1, 12, 64, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};


static const u8 mxt_config_data_qrd5_truly[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	21, 0, 2, 0, 0, 0, 0, 85,
	/* T7 Object */
	24, 12, 10,
	/* T8 Object */
	30, 0, 20, 20, 10, 0, 0, 0, 10, 192,
	/* T9 Object */
	131, 0, 0, 18, 11, 0, 16, 70, 2, 3,
	0, 2, 1, 62, 10, 10, 10, 10, 107, 3,
	223, 1, 2, 2, 20, 20, 172, 40, 139, 110,
	10, 15, 0, 0, 0,
	/* T15 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0,
	/* T18 Object */
	0, 0,
	/* T19 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0,
	/* T23 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0,
	/* T40 Object */
	0, 0, 0, 0, 0,
	/* T42 Object */
	3, 20, 45, 40, 128, 0, 0, 0,
	/* T46 Object */
	0, 2, 16, 16, 0, 0, 0, 0, 0,
	/* T47 Object */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* T48 Object */
	1, 12, 64, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
};

static const u8 mxt_config_data_qrd5_new[] = {
	/* T6 Object */
	0, 0, 0, 0, 0, 0,
	/* T38 Object */
	25, 0, 4, 0, 0, 0, 0, 0,
	/* T7 Object */
	24, 12, 10,
	/* T8 Object */
	30, 0, 5, 5, 0, 0, 0, 0, 10, 192,
	/* T9 Object */
	131, 0, 0, 18, 11, 0, 16, 70, 2, 3,
	0, 2, 1, 62, 10, 10, 10, 10, 107, 3,
	223, 1, 2, 2, 20, 20, 172, 40, 139, 110,
	10, 15, 0, 0, 0,
	/* T25 Object */
	0, 0, 0, 0, 0, 0,
	/* T42 Object */
	3, 20, 45, 40, 128, 0, 0, 0,
	/* T46 Object */
	0, 2, 16, 16, 0, 0, 0, 0, 0,
	/* T48 Object */
	1, 12, 64, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 6, 6, 0, 0, 100, 4, 64,
	10, 0, 20, 5, 0, 38, 0, 20, 0, 0,
	0, 0, 0, 0, 16, 65, 3, 1, 1, 0,
	10, 10, 10, 0, 0, 15, 15, 154, 58, 145,
	80, 100, 15, 3,
	/* T64 Object */
	1, 32, 25, 40, 40, 0, 35,
};


static struct mxt_config_info mxt_config_array[] = {
	{
		.config		= mxt_config_data,
		.config_length	= ARRAY_SIZE(mxt_config_data),
		.family_id	= 0x81,
		.variant_id	= 0x01,
		.version	= 0x10,
		.build		= 0xAA,
	},
	{
		.config		= mxt_config_data_qrd5_truly,
		.config_length	= ARRAY_SIZE(mxt_config_data_qrd5_truly),
		.family_id	= 0x81,
		.variant_id	= 0x01,
		.version	= 0x10,
		.build		= 0xAA,
	},
	{
		.config		= mxt_config_data_qrd5_new,
		.config_length	= ARRAY_SIZE(mxt_config_data_qrd5_new),
		.family_id	= 0x81,
		.variant_id	= 0x18,
		.version	= 0x10,
		.build		= 0xAA,
		.fw_name	= "mxt224EC25.enc",
	},
};

static int mxt_key_codes[MXT_KEYARRAY_MAX_KEYS] = {
	[0] = KEY_HOME,
	[1] = KEY_MENU,
	[9] = KEY_BACK,
	[10] = KEY_SEARCH,
};

static struct mxt_platform_data mxt_platform_data = {
	.config_array		= mxt_config_array,
	.config_array_size	= ARRAY_SIZE(mxt_config_array),
	.panel_minx		= 0,
	.panel_maxx		= 479,
	.panel_miny		= 0,
	.panel_maxy		= 799,
	.disp_minx		= 0,
	.disp_maxx		= 479,
	.disp_miny		= 0,
	.disp_maxy		= 799,
	.irqflags		= IRQF_TRIGGER_LOW,
	.i2c_pull_up		= true,
	.reset_gpio		= MXT_TS_RESET_GPIO,
	.irq_gpio		= MXT_TS_IRQ_GPIO,
	.key_codes		= mxt_key_codes,
};

static struct i2c_board_info mxt_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("atmel_mxt_ts", 0x4a),
		.platform_data = &mxt_platform_data,
		.irq = MSM_GPIO_TO_INT(MXT_TS_IRQ_GPIO),
	},
};

static int synaptics_touchpad_setup(void);

static struct msm_gpio clearpad3000_cfg_data[] = {
	{GPIO_CFG(CLEARPAD3000_ATTEN_GPIO, 0, GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL, GPIO_CFG_6MA), "rmi4_attn"},
	{GPIO_CFG(CLEARPAD3000_RESET_GPIO, 0, GPIO_CFG_OUTPUT,
			GPIO_CFG_PULL_DOWN, GPIO_CFG_8MA), "rmi4_reset"},
};

static struct rmi_XY_pair rmi_offset = {.x = 0, .y = 0};
static struct rmi_range rmi_clipx = {.min = 48, .max = 980};
static struct rmi_range rmi_clipy = {.min = 7, .max = 1647};
static struct rmi_f11_functiondata synaptics_f11_data = {
	.swap_axes = false,
	.flipX = false,
	.flipY = false,
	.offset = &rmi_offset,
	.button_height = 113,
	.clipX = &rmi_clipx,
	.clipY = &rmi_clipy,
};

#define MAX_LEN		100

static ssize_t clearpad3000_virtual_keys_register(struct kobject *kobj,
		     struct kobj_attribute *attr, char *buf)
{
	char *virtual_keys = __stringify(EV_KEY) ":" __stringify(KEY_MENU) \
			     ":60:830:120:60" ":" __stringify(EV_KEY) \
			     ":" __stringify(KEY_HOME)   ":180:830:120:60" \
				":" __stringify(EV_KEY) ":" \
				__stringify(KEY_SEARCH) ":300:830:120:60" \
				":" __stringify(EV_KEY) ":" \
			__stringify(KEY_BACK)   ":420:830:120:60" "\n";

	return snprintf(buf, strnlen(virtual_keys, MAX_LEN) + 1 , "%s",
			virtual_keys);
}

static struct kobj_attribute clearpad3000_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.sensor00fn11",
		.mode = S_IRUGO,
	},
	.show = &clearpad3000_virtual_keys_register,
};

static struct attribute *virtual_key_properties_attrs[] = {
	&clearpad3000_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group virtual_key_properties_attr_group = {
	.attrs = virtual_key_properties_attrs,
};

struct kobject *virtual_key_properties_kobj;

static struct rmi_functiondata synaptics_functiondata[] = {
	{
		.function_index = RMI_F11_INDEX,
		.data = &synaptics_f11_data,
	},
};

static struct rmi_functiondata_list synaptics_perfunctiondata = {
	.count = ARRAY_SIZE(synaptics_functiondata),
	.functiondata = synaptics_functiondata,
};

static struct rmi_sensordata synaptics_sensordata = {
	.perfunctiondata = &synaptics_perfunctiondata,
	.rmi_sensor_setup	= synaptics_touchpad_setup,
};

static struct rmi_i2c_platformdata synaptics_platformdata = {
	.i2c_address = 0x2c,
	.irq_type = IORESOURCE_IRQ_LOWLEVEL,
	.sensordata = &synaptics_sensordata,
};

static struct i2c_board_info synaptic_i2c_clearpad3k[] = {
	{
	I2C_BOARD_INFO("rmi4_ts", 0x2c),
	.platform_data = &synaptics_platformdata,
	},
};

static int synaptics_touchpad_setup(void)
{
	int retval = 0;

	virtual_key_properties_kobj =
		kobject_create_and_add("board_properties", NULL);
	if (virtual_key_properties_kobj)
		retval = sysfs_create_group(virtual_key_properties_kobj,
				&virtual_key_properties_attr_group);
	if (!virtual_key_properties_kobj || retval)
		pr_err("failed to create ft5202 board_properties\n");

	retval = msm_gpios_request_enable(clearpad3000_cfg_data,
		    sizeof(clearpad3000_cfg_data)/sizeof(struct msm_gpio));
	if (retval) {
		pr_err("%s:Failed to obtain touchpad GPIO %d. Code: %d.",
				__func__, CLEARPAD3000_ATTEN_GPIO, retval);
		retval = 0; /* ignore the err */
	}
	synaptics_platformdata.irq = gpio_to_irq(CLEARPAD3000_ATTEN_GPIO);

	gpio_set_value(CLEARPAD3000_RESET_GPIO, 0);
	usleep(10000);
	gpio_set_value(CLEARPAD3000_RESET_GPIO, 1);
	usleep(50000);

	return retval;
}
#endif
#ifdef CONFIG_TOUCHSCREEN_FOCAL
struct virtual_key{
	const char	*menu;
	const char	*home;
	const char	*back;
	const char	*search;
};

struct tp_platform_data{
	struct kobj_attribute byd_virtual_keys_attr;
	const char	*input_dev_name;
	// [sun.yu5@byd.com, modify,WG703T2_C000133,support 2  touchscreen, define  struct virtual_key vk from 1 to 2]
	struct virtual_key	vk[2];          
	// [sun.yu5@byd.com, end]
	int lcd_x;
	int lcd_y;
	int tp_x;
	int tp_y;
	int irq;
	int wakeup;
};

static struct tp_platform_data tp_pdata = {
	.byd_virtual_keys_attr = {
		.attr = {
			.name = "virtualkeys.Focal touch",
			.mode = S_IRUGO,
		},
	},
	.input_dev_name = "Focal touch",
	// [sun.yu5@byd.com, modify,WG703T2_C000133,support 2  touchscreen, define  struct virtual_key vk from 1 to 2]
	.vk[0]={
	       .menu = "60:1009:50:50",
		.home = "180:1009:50:50",
		.back = "300:1009:50:50",
		.search = "420:1009:50:50",
	   	},
	.vk[1]={
		.menu = "70:850:50:50",
		.home = "195:850:50:50",
		.back = "310:850:50:50",
		.search = "435:850:50:50",
	   	},  	
	// [sun.yu5@byd.com, end]   	
	.lcd_x = 540,
	.lcd_y = 960,
	.tp_x = 540,
	.tp_y = 960,
	.irq = 48,
	.wakeup = 26,
};

static struct i2c_board_info focal_ts_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("focal_ts", 0x38),
		.irq = MSM_GPIO_TO_INT(48),
		.platform_data = &tp_pdata,
	},
};
#endif

#ifdef CONFIG_TOUCHSCREEN_FT6306
static struct ft6306_touch_platform_data ft6306_pdata ={

      .maxx          = 480,
      .maxy          = 800,
      .reset          = 26,
      .irq            = 48,
          
	};

void *ft6306_platform_data(void *info)
{
	return &ft6306_pdata;
}

static struct i2c_board_info ft6306_touch_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("ft6306_touch", 0x38),
		.irq = MSM_GPIO_TO_INT(48),
		.platform_data = &ft6306_pdata,
	},
};
#endif

static struct regulator_bulk_data regs_atmel[] = {
	{ .supply = "ldo12", .min_uV = 2700000, .max_uV = 3300000 },
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
};

#define ATMEL_TS_GPIO_IRQ 82

static int atmel_ts_power_on(bool on)
{
	int rc = on ?
		regulator_bulk_enable(ARRAY_SIZE(regs_atmel), regs_atmel) :
		regulator_bulk_disable(ARRAY_SIZE(regs_atmel), regs_atmel);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);
	else
		msleep(50);

	return rc;
}

static int atmel_ts_platform_init(struct i2c_client *client)
{
	int rc;
	struct device *dev = &client->dev;

	rc = regulator_bulk_get(dev, ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not get regulators: %d\n",
				__func__, rc);
		goto out;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_atmel), regs_atmel);
	if (rc) {
		dev_err(dev, "%s: could not set voltages: %d\n",
				__func__, rc);
		goto reg_free;
	}

	rc = gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc) {
		dev_err(dev, "%s: gpio_tlmm_config for %d failed\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto reg_free;
	}

	/* configure touchscreen interrupt gpio */
	rc = gpio_request(ATMEL_TS_GPIO_IRQ, "atmel_maxtouch_gpio");
	if (rc) {
		dev_err(dev, "%s: unable to request gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto ts_gpio_tlmm_unconfig;
	}

	rc = gpio_direction_input(ATMEL_TS_GPIO_IRQ);
	if (rc < 0) {
		dev_err(dev, "%s: unable to set the direction of gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto free_ts_gpio;
	}
	return 0;

free_ts_gpio:
	gpio_free(ATMEL_TS_GPIO_IRQ);
ts_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
reg_free:
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
out:
	return rc;
}

static int atmel_ts_platform_exit(struct i2c_client *client)
{
	gpio_free(ATMEL_TS_GPIO_IRQ);
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	regulator_bulk_free(ARRAY_SIZE(regs_atmel), regs_atmel);
	return 0;
}

static u8 atmel_ts_read_chg(void)
{
	return gpio_get_value(ATMEL_TS_GPIO_IRQ);
}

static u8 atmel_ts_valid_interrupt(void)
{
	return !atmel_ts_read_chg();
}


static struct maxtouch_platform_data atmel_ts_pdata = {
	.numtouch = 4,
	.init_platform_hw = atmel_ts_platform_init,
	.exit_platform_hw = atmel_ts_platform_exit,
	.power_on = atmel_ts_power_on,
	.display_res_x = 480,
	.display_res_y = 864,
	.min_x = ATMEL_X_OFFSET,
	.max_x = (505 - ATMEL_X_OFFSET),
	.min_y = ATMEL_Y_OFFSET,
	.max_y = (863 - ATMEL_Y_OFFSET),
	.valid_interrupt = atmel_ts_valid_interrupt,
	.read_chg = atmel_ts_read_chg,
};

static struct i2c_board_info atmel_ts_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO(ATMEL_TS_I2C_NAME, 0x4a),
		.platform_data = &atmel_ts_pdata,
		.irq = MSM_GPIO_TO_INT(ATMEL_TS_GPIO_IRQ),
	},
};

static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

#define FT5X06_IRQ_GPIO		48
#define FT5X06_RESET_GPIO	26

#define FT5X06_IRQ_GPIO_QPR_SKUD	121
#define FT5X06_RESET_GPIO_QPR_SKUD	26

#define FT5X06_IRQ_GPIO_QPR_SKUD_PRIM	122
#define FT5X06_RESET_GPIO_QPR_SKUD_PRIM	26

#define FT5X16_IRQ_GPIO_EVBD	115

#define FT5X06_IRQ_GPIO_QPR_SKUE	121
#define FT5X06_RESET_GPIO_QPR_SKUE	26

static ssize_t
ft5x06_virtual_keys_register(struct kobject *kobj,
			     struct kobj_attribute *attr,
			     char *buf)
{
	if (machine_is_msm8625q_skud() || machine_is_msm8625q_evbd()) {
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_HOME)  ":67:1000:135:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":202:1000:135:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":337:1000:135:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH)   ":472:1000:135:60"
			"\n");
	} if (machine_is_msm8625q_skue()) {
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":90:1020:170:40"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":270:1020:170:40"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK) ":450:1020:170:40"
			"\n");
	} else {
		return snprintf(buf, 200,
			__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":40:510:80:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":120:510:80:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH) ":200:510:80:60"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":280:510:80:60"
			"\n");
	}
}

static struct kobj_attribute ft5x06_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.ft5x06_ts",
		.mode = S_IRUGO,
	},
	.show = &ft5x06_virtual_keys_register,
};

static struct attribute *ft5x06_virtual_key_properties_attrs[] = {
	&ft5x06_virtual_keys_attr.attr,
	NULL,
};

static struct attribute_group ft5x06_virtual_key_properties_attr_group = {
	.attrs = ft5x06_virtual_key_properties_attrs,
};

struct kobject *ft5x06_virtual_key_properties_kobj;

static struct regulator_bulk_data regs_ft5x06[] = {
	{ .supply = "ldo12", .min_uV = 2700000, .max_uV = 3300000 },
	{ .supply = "smps3", .min_uV = 1800000, .max_uV = 1800000 },
};

static int ft5x06_ts_power_on(bool on)
{
	int rc;

	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_ft5x06), regs_ft5x06);
	if (rc) {
		printk("%s: could not get regulators: %d\n",
				__func__, rc);
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_ft5x06), regs_ft5x06);
	if (rc) {
		printk("%s: could not set voltages: %d\n",
				__func__, rc);
	}

	rc = on ?
		regulator_bulk_enable(ARRAY_SIZE(regs_ft5x06), regs_ft5x06) :
		regulator_bulk_disable(ARRAY_SIZE(regs_ft5x06), regs_ft5x06);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, on ? "en" : "dis", rc);
	else
		msleep(50);

	return rc;
}

static struct ft5x06_ts_platform_data ft5x06_platformdata = {
	.x_max		= 320,
	.y_max		= 480,
	.reset_gpio	= FT5X06_RESET_GPIO,
	.irq_gpio	= FT5X06_IRQ_GPIO,
	.irqflags	= IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
	.power_on	= ft5x06_ts_power_on,
};

static struct i2c_board_info ft5x06_device_info[] __initdata = {
	{
		I2C_BOARD_INFO("ft5x06_ts", 0x38),
		.platform_data = &ft5x06_platformdata,
		.irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO),
	},
};

static void __init ft5x06_touchpad_setup(void)
{
	int rc;

	if (machine_is_msm8625q_skud()) {
		if (cpu_is_msm8625()) {
			ft5x06_platformdata.irq_gpio = FT5X06_IRQ_GPIO_QPR_SKUD_PRIM;
			ft5x06_platformdata.reset_gpio = FT5X06_RESET_GPIO_QPR_SKUD_PRIM;
			ft5x06_platformdata.x_max = 540;
			ft5x06_platformdata.y_max = 960;
			ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO_QPR_SKUD_PRIM);
		} else {
			ft5x06_platformdata.irq_gpio = FT5X06_IRQ_GPIO_QPR_SKUD;
			ft5x06_platformdata.reset_gpio = FT5X06_RESET_GPIO_QPR_SKUD;
			ft5x06_platformdata.x_max = 540;
			ft5x06_platformdata.y_max = 960;
			ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO_QPR_SKUD);
		}
	} else if(machine_is_msm8625q_evbd()) {
		ft5x06_platformdata.irq_gpio = FT5X16_IRQ_GPIO_EVBD;
		ft5x06_platformdata.reset_gpio = FT5X06_RESET_GPIO;
		ft5x06_platformdata.x_max = 540;
		ft5x06_platformdata.y_max = 960;
		ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X16_IRQ_GPIO_EVBD);
	} else if (machine_is_msm8625q_skue()) {
		ft5x06_platformdata.irq_gpio = FT5X06_IRQ_GPIO_QPR_SKUE;
		ft5x06_platformdata.reset_gpio = FT5X06_RESET_GPIO_QPR_SKUE;
		ft5x06_platformdata.x_max = 540;
		ft5x06_platformdata.y_max = 960;
		ft5x06_device_info[0].irq = MSM_GPIO_TO_INT(FT5X06_IRQ_GPIO_QPR_SKUE);
	}

	rc = gpio_tlmm_config(GPIO_CFG(ft5x06_platformdata.irq_gpio, 0,
			GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, ft5x06_platformdata.irq_gpio);

	rc = gpio_tlmm_config(GPIO_CFG(ft5x06_platformdata.reset_gpio, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
			GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc)
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, ft5x06_platformdata.reset_gpio);

	ft5x06_virtual_key_properties_kobj =
			kobject_create_and_add("board_properties", NULL);

	if (ft5x06_virtual_key_properties_kobj)
		rc = sysfs_create_group(ft5x06_virtual_key_properties_kobj,
				&ft5x06_virtual_key_properties_attr_group);

	if (!ft5x06_virtual_key_properties_kobj || rc)
		pr_err("%s: failed to create board_properties\n", __func__);

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				ft5x06_device_info,
				ARRAY_SIZE(ft5x06_device_info));
}

/* skud flash led and touch*/
#define FLASH_LED_SKUD 34
#define FLASH_LED_TORCH_SKUD 48

static struct gpio_led gpio_flash_config_skud[] = {
	{
		.name = "flashlight",
		.gpio = FLASH_LED_SKUD,
	},
	{
		.name = "torch",
		.gpio = FLASH_LED_TORCH_SKUD,
	},
};

static struct gpio_led_platform_data gpio_flash_pdata_skud = {
	.num_leds = ARRAY_SIZE(gpio_flash_config_skud),
	.leds = gpio_flash_config_skud,
};

static struct platform_device gpio_flash_skud = {
	.name          = "leds-gpio",
	.id            = -1,
	.dev           = {
		.platform_data = &gpio_flash_pdata_skud,
	},
};
/* end of skud flash led and touch*/

/* skue flash led*/
#define FLASH_LED_SKUE 34

static struct gpio_led gpio_flash_config_skue[] = {
        {
                .name = "flashlight",
                .gpio = FLASH_LED_SKUE,
        },
};

static struct gpio_led_platform_data gpio_flash_pdata_skue = {
        .num_leds = ARRAY_SIZE(gpio_flash_config_skue),
        .leds = gpio_flash_config_skue,
};

static struct platform_device gpio_flash_skue = {
        .name          = "leds-gpio",
        .id            = -1,
        .dev           = {
                .platform_data = &gpio_flash_pdata_skue,
        },
};
/* end of skue flash led*/


#ifdef CONFIG_LEDS_TRICOLOR_FLAHSLIGHT

#define LED_FLASH_EN1 13
#define QRD7_LED_FLASH_EN 96

static struct msm_gpio tricolor_leds_gpio_cfg_data[] = {
{
	GPIO_CFG(-1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA),
		"flashlight"},
};

static int tricolor_leds_gpio_setup(void) {
	int ret = 0;
	if(machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
	{
		tricolor_leds_gpio_cfg_data[0].gpio_cfg = GPIO_CFG(LED_FLASH_EN1, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
	}
	else if(machine_is_msm8625_qrd7())
	{
		tricolor_leds_gpio_cfg_data[0].gpio_cfg = GPIO_CFG(QRD7_LED_FLASH_EN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA);
	}

	ret = msm_gpios_request_enable(tricolor_leds_gpio_cfg_data,
			sizeof(tricolor_leds_gpio_cfg_data)/sizeof(struct msm_gpio));
	if( ret<0 )
		printk(KERN_ERR "%s: Failed to obtain tricolor_leds GPIO . Code: %d\n",
				__func__, ret);
	return ret;
}


static struct platform_device msm_device_tricolor_leds = {
	.name   = "tricolor leds and flashlight",
	.id = -1,
};
#endif
/* SKU3/SKU7 keypad device information */
#define KP_INDEX_SKU3(row, col) ((row)*ARRAY_SIZE(kp_col_gpios_qrd3) + (col))
static unsigned int kp_row_gpios_qrd3[] = {31, 32};
static unsigned int kp_col_gpios_qrd3[] = {36, 37};

static unsigned int kp_row_gpios_evbdp[] = {42, 37};
static unsigned int kp_col_gpios_evbdp[] = {31};

static const unsigned short keymap_qrd3[] = {
	[KP_INDEX_SKU3(0, 0)] = KEY_VOLUMEUP,
	[KP_INDEX_SKU3(0, 1)] = KEY_VOLUMEDOWN,
	[KP_INDEX_SKU3(1, 1)] = KEY_CAMERA,
};

static struct gpio_event_matrix_info kp_matrix_info_qrd3 = {
	.info.func      = gpio_event_matrix_func,
	.keymap         = keymap_qrd3,
	.output_gpios   = kp_row_gpios_qrd3,
	.input_gpios    = kp_col_gpios_qrd3,
	.noutputs       = ARRAY_SIZE(kp_row_gpios_qrd3),
	.ninputs        = ARRAY_SIZE(kp_col_gpios_qrd3),
	.settle_time.tv64 = 40 * NSEC_PER_USEC,
	.poll_time.tv64 = 20 * NSEC_PER_MSEC,
	.flags          = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
				GPIOKPF_PRINT_UNMAPPED_KEYS,
};

static struct gpio_event_info *kp_info_qrd3[] = {
	&kp_matrix_info_qrd3.info,
};
static struct gpio_event_platform_data kp_pdata_qrd3 = {
	.name           = "7x27a_kp",
	.info           = kp_info_qrd3,
	.info_count     = ARRAY_SIZE(kp_info_qrd3)
};

static struct platform_device kp_pdev_qrd3 = {
	.name   = GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev    = {
		.platform_data  = &kp_pdata_qrd3,
	},
};

static struct pmic8029_led_platform_data leds_data_skud[] = {
	{
		.name = "button-backlight",
		.which = PM_MPP_8,
		.type = PMIC8029_DRV_TYPE_CUR,
		.max.cur = PM_MPP__I_SINK__LEVEL_5mA,
	},
};

static struct pmic8029_leds_platform_data pmic8029_leds_pdata_skud = {
	.leds = leds_data_skud,
	.num_leds = 1,
};

static struct platform_device pmic_mpp_leds_pdev_skud = {
	.name   = "pmic-mpp-leds",
	.id     = -1,
	.dev    = {
		.platform_data	= &pmic8029_leds_pdata_skud,
	},
};

static struct pmic8029_led_platform_data leds_data[] = {
	{
		.name = "button-backlight",
		.which = PM_MPP_7,
		.type = PMIC8029_DRV_TYPE_CUR,
		.max.cur = PM_MPP__I_SINK__LEVEL_40mA,
	},
};

static struct pmic8029_leds_platform_data pmic8029_leds_pdata = {
	.leds = leds_data,
	.num_leds = 1,
};

static struct platform_device pmic_mpp_leds_pdev = {
	.name   = "pmic-mpp-leds",
	.id     = -1,
	.dev    = {
		.platform_data	= &pmic8029_leds_pdata,
	},
};

static struct led_info tricolor_led_info[] = {
	[0] = {
		.name           = "red",
		.flags          = LED_COLOR_RED,
	},
	[1] = {
		.name           = "green",
		.flags          = LED_COLOR_GREEN,
	},
};

static struct led_platform_data tricolor_led_pdata = {
	.leds = tricolor_led_info,
	.num_leds = ARRAY_SIZE(tricolor_led_info),
};

static struct platform_device tricolor_leds_pdev = {
	.name   = "msm-tricolor-leds",
	.id     = -1,
	.dev    = {
		.platform_data  = &tricolor_led_pdata,
	},
};

void __init msm7627a_add_io_devices(void)
{
	/* touchscreen */
	if (machine_is_msm7625a_surf() || machine_is_msm7625a_ffa()) {
		atmel_ts_pdata.min_x = 0;
		atmel_ts_pdata.max_x = 480;
		atmel_ts_pdata.min_y = 0;
		atmel_ts_pdata.max_y = 320;
	}

	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				atmel_ts_i2c_info,
				ARRAY_SIZE(atmel_ts_i2c_info));
	/* keypad */
	platform_device_register(&kp_pdev_8625);
#ifdef CONFIG_TOUCHSCREEN_FOCAL
        i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
                    focal_ts_i2c_info,
                    ARRAY_SIZE(focal_ts_i2c_info));
#endif
#ifdef CONFIG_TOUCHSCREEN_FT6306
        i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
                    ft6306_touch_i2c_info,
                    ARRAY_SIZE(ft6306_touch_i2c_info));
#endif
	/* headset */
	platform_device_register(&hs_pdev);

	/* LED: configure it as a pdm function */
	if (gpio_tlmm_config(GPIO_CFG(LED_GPIO_PDM, 3,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE))
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, LED_GPIO_PDM);
	else
		platform_device_register(&led_pdev);

	/* Vibrator */
#ifdef CONFIG_MSM_RPC_VIBRATOR
		msm_init_pmic_vibrator();
#endif
}

void __init qrd7627a_add_io_devices(void)
{
	int rc;

	/* touchscreen */
	if (machine_is_msm7627a_qrd1()) {
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					synaptic_i2c_clearpad3k,
					ARRAY_SIZE(synaptic_i2c_clearpad3k));
	} else if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
			machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {
		/* Use configuration data for sku5 */
		if (machine_is_msm8625_qrd5()) {
			mxt_config_array[0].config = mxt_config_data_qrd5;
			mxt_config_array[0].config_length =
					ARRAY_SIZE(mxt_config_data_qrd5);
			mxt_platform_data.panel_maxy = 875;
			mxt_platform_data.need_calibration = true;
			mxt_vkey_setup();
		}

		if (machine_is_msm7x27a_qrd5a()) {
			mxt_config_array[0].config = mxt_config_data_qrd5a;
			mxt_config_array[0].config_length =
					ARRAY_SIZE(mxt_config_data_qrd5a);
			mxt_platform_data.panel_maxy = 875;
			mxt_vkey_setup();
		}

		rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_IRQ_GPIO, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config for %d failed\n",
				__func__, MXT_TS_IRQ_GPIO);
		}

		rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_RESET_GPIO, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s: gpio_tlmm_config for %d failed\n",
				__func__, MXT_TS_RESET_GPIO);
		}

		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					mxt_device_info,
					ARRAY_SIZE(mxt_device_info));
	} else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7()
				|| machine_is_msm8625q_skud()
				|| machine_is_msm8625q_evbd()
				|| machine_is_msm8625q_skue()) {
		ft5x06_touchpad_setup();
		/* evbd+ can support synaptic as well */
		if (machine_is_msm8625q_evbd() &&
			(socinfo_get_platform_type() == 0x13)) {
			/* for QPR EVBD+ with synaptic touch panel */
			/* TODO: Add  gpio request to the driver
				to support proper dynamic touch detection */
			gpio_tlmm_config(
				GPIO_CFG(CLEARPAD3000_ATTEN_GPIO_EVBD_PLUS, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);

			gpio_tlmm_config(
				GPIO_CFG(CLEARPAD3000_RESET_GPIO, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);

			gpio_set_value(CLEARPAD3000_RESET_GPIO, 0);
			usleep(10000);
			gpio_set_value(CLEARPAD3000_RESET_GPIO, 1);
			usleep(50000);

			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				rmi4_i2c_devices,
				ARRAY_SIZE(rmi4_i2c_devices));
		}
		else {
			if (machine_is_msm8625q_evbd()) {
				mxt_config_array[0].config = mxt_config_data;
				mxt_config_array[0].config_length =
				ARRAY_SIZE(mxt_config_data);
				mxt_platform_data.panel_maxy = 875;
				mxt_platform_data.need_calibration = true;
				mxt_platform_data.irq_gpio = MXT_TS_EVBD_IRQ_GPIO;
				mxt_vkey_setup();

			rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_EVBD_IRQ_GPIO, 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			if (rc) {
				pr_err("%s: gpio_tlmm_config for %d failed\n",
						__func__, MXT_TS_EVBD_IRQ_GPIO);
			}

			rc = gpio_tlmm_config(GPIO_CFG(MXT_TS_RESET_GPIO, 0,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			if (rc) {
				pr_err("%s: gpio_tlmm_config for %d failed\n",
						__func__, MXT_TS_RESET_GPIO);
			}

			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				mxt_device_info,
				ARRAY_SIZE(mxt_device_info));
			}
		}
	}

	/* headset */
	platform_device_register(&hs_pdev);

	/* vibrator */
#ifdef CONFIG_MSM_RPC_VIBRATOR
	msm_init_pmic_vibrator();
#endif

	/* keypad */
	if (machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
		kp_matrix_info_8625.keymap = keymap_8625_qrd5;
	/* keypad info for EVBD+ */
	if (machine_is_msm8625q_evbd() &&
			(socinfo_get_platform_type() == 13)) {
			gpio_tlmm_config(GPIO_CFG(37, 0,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(42, 0,
						GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(31, 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
						GPIO_CFG_8MA), GPIO_CFG_ENABLE);
			kp_matrix_info_skud.output_gpios = kp_row_gpios_evbdp;
			kp_matrix_info_skud.input_gpios = kp_col_gpios_evbdp;
			kp_matrix_info_skud.noutputs = ARRAY_SIZE(kp_row_gpios_evbdp);
			kp_matrix_info_skud.ninputs = ARRAY_SIZE(kp_col_gpios_evbdp);
	}

	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() ||
			machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
		platform_device_register(&kp_pdev_8625);
	else if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())
		platform_device_register(&kp_pdev_qrd3);
	else if (machine_is_msm8625q_skud()||machine_is_msm8625q_evbd())
		platform_device_register(&kp_pdev_skud);
	else if (machine_is_msm8625q_skue())
		platform_device_register(&kp_pdev_skue);

	/* leds */
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
            || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {

		platform_device_register(&pmic_mpp_leds_pdev);
		platform_device_register(&tricolor_leds_pdev);
	} else if (machine_is_msm8625q_skud() || machine_is_msm8625q_evbd()) {
		platform_device_register(&pmic_mpp_leds_pdev_skud);
		/* enable the skud flash and torch by gpio leds driver */
		platform_device_register(&gpio_flash_skud);
	} else if (machine_is_msm8625q_skue()) {
		 /* enable the skue flashlight by gpio leds driver */
                platform_device_register(&gpio_flash_skue);
	}

#ifdef CONFIG_LEDS_TRICOLOR_FLAHSLIGHT
	    /*tricolor leds init*/
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()
            || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {
		platform_device_register(&msm_device_tricolor_leds);
		tricolor_leds_gpio_setup();
	}
#endif
}
