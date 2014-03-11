/* Copyright (c) 2012-2013, The Linux Foundation. All Rights Reserved.
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


#include "msm_sensor.h"
#include "msm.h"
#include "pip_ov5648_ov7695.h"

#define SENSOR_NAME "ov5648_truly_cm8352"
#define PLATFORM_DRIVER_NAME "msm_camera_ov5648_truly_cm8352"
#define ov5648_truly_cm8352_obj ov5648_truly_cm8352_##obj

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define OV5648_TRULY_CM8352_VERBOSE_DGB

#ifdef OV5648_TRULY_CM8352_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

#define OV5648_TRULY_CM8352_OTP_FEATURE
//#ifdef OV5648_TRULY_CM8352_OTP_FEATURE
//#undef OV5648_TRULY_CM8352_OTP_FEATURE
//#endif

static struct msm_sensor_ctrl_t ov5648_truly_cm8352_s_ctrl;

DEFINE_MUTEX(ov5648_truly_cm8352_mut);

/***********************PIP settings*******************************/

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_pip_init_settings_master[] =
{
	//0x0103, 0x01, // software reset
	{0x3001, 0x00}, // D[7:0] set to input
	{0x3002, 0x80}, // Vsync output, PCLK, FREX, Strobe, CSD, CSK, GPIO input
	{0x3011, 0x02}, // Drive strength 2x
	{0x3018, 0x4c}, // MIPI 2 lane
	{0x301c, 0xf0}, // OTP enable
	{0x3022, 0x00}, // MIP lane enable
	{0x3034, 0x1a}, // 10-bit mode
	{0x3035, 0x21}, // PLL
	{0x3036, 0x69}, // PLL
	{0x3037, 0x03}, // PLL
	{0x3038, 0x00}, // PLL
	{0x3039, 0x00}, // PLL
	{0x303a, 0x00}, // PLLS
	{0x303b, 0x19}, // PLLS
	{0x303c, 0x11}, // PLLS
	{0x303d, 0x30}, // PLLS
	{0x3105, 0x11},
	{0x3106, 0x05}, // PLL
	{0x3304, 0x28},
	{0x3305, 0x41},
	{0x3306, 0x30},
	{0x3308, 0x00},
	{0x3309, 0xc8},
	{0x330a, 0x01},
	{0x330b, 0x90},
	{0x330c, 0x02},
	{0x330d, 0x58},
	{0x330e, 0x03},
	{0x330f, 0x20},
	{0x3300, 0x00},
	{0x3500, 0x00}, // exposure [19:16]
	{0x3501, 0x2d}, // exposure [15:8]
	{0x3502, 0x60}, // exposure [7:0], exposure = 0x2d6 = 726
	{0x3503, 0x07}, // gain has no delay, manual agc/aec
	{0x350a, 0x00}, // gain[9:8]
	{0x350b, 0x40}, // gain[7:0], gain = 4x
	//analog control
	{0x3601, 0x33},
	{0x3602, 0x00},
	{0x3611, 0x0e},
	{0x3612, 0x2b},
	{0x3614, 0x50},
	{0x3620, 0x33},
	{0x3622, 0x00},
	{0x3630, 0xad},
	{0x3631, 0x00},
	{0x3632, 0x94},
	{0x3633, 0x17},
	{0x3634, 0x14},
	{0x3705, 0x2a},
	{0x3708, 0x66},
	{0x3709, 0x52},
	{0x370b, 0x23},
	{0x370c, 0xc3},
	{0x370d, 0x00},
	{0x370e, 0x00},
	{0x371c, 0x07},
	{0x3739, 0xd2},
	{0x373c, 0x00},
	{0x3800, 0x00}, // xstart = 0
	{0x3801, 0x00}, // xstart
	{0x3802, 0x00}, // ystart = 0
	{0x3803, 0x00}, // ystart
	{0x3804, 0x0a}, // xend = 2639
	{0x3805, 0x4f}, // xend
	{0x3806, 0x07}, // yend = 1955
	{0x3807, 0xa3}, // yend
	{0x3808, 0x05}, // x output size = 1296
	{0x3809, 0x10}, // x output size
	{0x380a, 0x03}, // y output size = 972
	{0x380b, 0xcc}, // y output size
	{0x380c, 0x0e}, // hts = 3780
	{0x380d, 0xc4}, // hts
	{0x380e, 0x03}, // vts = 992
	{0x380f, 0xe0}, // vts
	{0x3810, 0x00}, // isp x win = 8
	{0x3811, 0x08}, // isp x win
	{0x3812, 0x00}, // isp y win = 2
	{0x3813, 0x02}, // isp y win
	{0x3814, 0x31}, // x inc
	{0x3815, 0x31}, // y inc
	{0x3817, 0x00}, // hsync start
	{0x3820, 0x08}, // v flip off
	{0x3821, 0x07}, // h mirror on
	{0x3826, 0x03},
	{0x3829, 0x00},
	{0x382b, 0x0b},
	{0x3830, 0x00},
	{0x3836, 0x01},
	{0x3837, 0xd4},
	{0x3838, 0x02},
	{0x3839, 0xe0},
	{0x383a, 0x0a},
	{0x383b, 0x50},
	{0x3b00, 0x00}, // strobe off
	{0x3b02, 0x08}, // shutter delay
	{0x3b03, 0x00}, // shutter delay
	{0x3b04, 0x04}, // frex_exp
	{0x3b05, 0x00}, // frex_exp
	{0x3b06, 0x04},
	{0x3b07, 0x08}, // frex inv
	{0x3b08, 0x00}, // frex exp req
	{0x3b09, 0x02}, // frex end option
	{0x3b0a, 0x04}, // frex rst length
	{0x3b0b, 0x00}, // frex strobe width
	{0x3b0c, 0x3d}, // frex strobe width
	{0x3f01, 0x0d},
	{0x3f0f, 0xf5},
	{0x4000, 0x89}, // blc enable
	{0x4001, 0x02}, // blc start line
	{0x4002, 0x45}, // blc auto, reset frame number = 5
	{0x4004, 0x02}, // black line number
	{0x4005, 0x18}, // blc normal freeze
	{0x4006, 0x08},
	{0x4007, 0x10},
	{0x4008, 0x00},
	{0x4300, 0xf8},
	{0x4303, 0xff},
	{0x4304, 0x00},
	{0x4307, 0xff},
	{0x4520, 0x00},
	{0x4521, 0x00},
	{0x4511, 0x22},
	{0x4800, 0x24},
	{0x481f, 0x3c}, // MIPI clk prepare min
	{0x4826, 0x00}, // MIPI hs prepare min
	{0x4837, 0x18}, // MIPI global timing
	{0x4b00, 0x44},
	{0x4b01, 0x0a},
	{0x4b04, 0x30},
	{0x5000, 0xff}, // bpc on, wpc on
	{0x5001, 0x00}, // AWB ON 00
	{0x5002, 0x40}, // AWB Gain ON 40
	{0x5003, 0x0b}, // buf en, bin auto en
	{0x5004, 0x0c}, // size man on
	{0x5043, 0x00},
	{0x5013, 0x00},
	{0x501f, 0x03}, // ISP output data
	{0x503d, 0x00}, // test pattern off
	{0x5a00, 0x08},
	{0x5b00, 0x01},
	{0x5b01, 0x40},
	{0x5b02, 0x00},
	{0x5b03, 0xf0},
	//ISP AEC/AGC setting from here
	{0x3503, 0x07}, //delay gain for one frame, and mannual enable.
	{0x5000, 0x06}, // DPC On
	{0x5001, 0x00}, // AWB ON
	{0x5002, 0x41}, // AWB Gain ON
	{0x383b, 0x80}, //
	{0x3a0f, 0x38}, // AEC in H
	{0x3a10, 0x30}, // AEC in L
	{0x3a1b, 0x38}, // AEC out H
	{0x3a1e, 0x30}, // AEC out L
	{0x3a11, 0x70}, // Control Zone H
	{0x3a1f, 0x18}, // Conrol Zone L
	//// @@ OV5648 Gain Table Alight
	{0x5180, 0x08}, // Manual AWB
	//// A light Different with OV7695
	{0x5186, 0x05}, // 04
	{0x5187, 0x26}, // 57
	{0x5188, 0x04}, // 04
	{0x5189, 0xbf}, // 00
	{0x518a, 0x04}, // 03
	{0x518b, 0x00}, // 5E
	//0x4202, 0x0f, // MIPI stream off
	//0x0100, 0x01, // wake up from standby
};


static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_pip_snap_settings_master[] = {
#if 0
	//4:3 settings
	//// OV5648 wake up
	{0x3022, 0x00}, // MIP lane enable
	{0x3012, 0x01},
	{0x3013, 0x00},
	{0x3014, 0x0b},
	{0x3005, 0xf0},
	{0x301a, 0xf0},
	{0x301b, 0xf0},
	{0x5000, 0x06}, // turn on ISP
	{0x5001, 0x00}, // turn on ISP
	{0x5002, 0x41}, // turn on ISP
	{0x5003, 0x0b}, // turn on ISP
	{0x3501, 0x7b},
	{0x3502, 0x00},
	{0x3708, 0x63},
	{0x3709, 0x12},
	{0x370c, 0xc0},
	{0x3800, 0x00}, // x start = 0
	{0x3801, 0x00}, // x start
	{0x3802, 0x00}, // y start = 0
	{0x3803, 0x00}, // y start
	{0x3804, 0x0a}, // x end = 2623
	{0x3805, 0x3f}, // x end
	{0x3806, 0x07}, // y end = 1955
	{0x3807, 0xa3}, // y end
	{0x3808, 0x0a}, // x output size = 2592
	{0x3809, 0x20}, // x output size
	{0x380a, 0x07}, // y output size = 1944
	{0x380b, 0x98}, // y output size
	{0x380c, 0x16}, // HTS = 5642
	{0x380d, 0x0a}, // HTS
	{0x380e, 0x07}, // VTS = 1984
	{0x380f, 0xc0}, // VTS
	{0x3810, 0x00}, // x offset = 16
	{0x3811, 0x10}, // x offset
	{0x3812, 0x00}, // y offset = 6
	{0x3813, 0x06}, // y offset
	{0x3814, 0x11}, // x inc
	{0x3815, 0x11}, // y inc
	{0x3817, 0x00}, // h sync start
	{0x3820, 0x40}, // ISP vertical flip on, v bin off
	{0x3821, 0x06}, // mirror on, h bin off
	{0x3830, 0x00},
	{0x3836, 0x0b},
	{0x3837, 0x39},
	{0x3838, 0x06},
	{0x3839, 0x00},
	{0x383a, 0x01},
	{0x383b, 0xb0}, // 80 PIP Location
	{0x4004, 0x04}, // black line number
	{0x5b00, 0x02},
	{0x5b01, 0x80},
	{0x5b02, 0x01},
	{0x5b03, 0xe0},
	{0x4202, 0x0f}, // MIPI stream off
	{0x0100, 0x01}, // wake up from software standby
#else
	//10fps
	{0x3022, 0x00},//	; MIP lane enable
	{0x3012, 0x01},
	{0x3013, 0x00},
	{0x3014, 0x0b},
	{0x3005, 0xf0},
	//{0x301a, 0xf0},
	{0x301b, 0xf0},
	{0x5000, 0x06},//	; turn on ISP
	{0x5001, 0x00},//	; turn on ISP
	{0x5002, 0x41},//	; turn on ISP
	{0x5003, 0x0B},//	; turn on ISP
	{0x3501, 0x7b},
	{0x3502, 0x00},
	{0x3708, 0x63},
	{0x3709, 0x12},
	{0x370c, 0xc0},

	{0x3800, 0x00},//	; x start = 16
	{0x3801, 0x10},//	; x start
	{0x3802, 0x00},//	; y start = 254
	{0x3803, 0xfe},//	; y start
	{0x3804, 0x0a},//	; x end = 2607
	{0x3805, 0x2f},//	; x end
	{0x3806, 0x06},//	; y end = 1701
	{0x3807, 0xa5},//	; y end

	//;6c 3800 00	; x start = 0
	//;6c 3801 00	; x start
	//;6c 3802 00	; y start = 0
	//;6c 3803 00	; y start
	//;6c 3804 0a	; x end = 2623
	//;6c 3805 3f	; x end
	//;6c 3806 07	; y end = 1955
	//;6c 3807 a3	; y end

	{0x3808, 0x0a},//	; x output size = 2560
	{0x3809, 0x00},//	; x output size
	{0x380a, 0x05},//	; y output size = 1440
	{0x380b, 0xa0},//	; y output size

	{0x380c, 0x16},//	; HTS = 5642
	{0x380d, 0x0a},//	; HTS
	{0x380e, 0x05},//	; VTS = 1480
	{0x380f, 0xc8},//	; VTS
	{0x3810, 0x00},//	; x offset = 16
	{0x3811, 0x10},//	; x offset
	{0x3812, 0x00},//	; y offset = 6
	{0x3813, 0x06},//	; y offset
	{0x3814, 0x11},//	; x inc
	{0x3815, 0x11},//	; y inc
	{0x3817, 0x00},//	; h sync start
	{0x3820, 0x40},//	; ISP vertical flip on, v bin off
	{0x3821, 0x06},//	; mirror on, h bin off

	{0x3830, 0x00},
	{0x3838, 0x06},
	{0x3839, 0x00},
	//; pip normal
	//;6c 3836 0b
	//;6c 3837 39
	//;6c 383a 01
	//;6c 383b b8 	; 80 PIP Location
	//; pip rotate 180
	{0x3836, 0x07},
	{0x3837, 0x37},
	{0x383a, 0x01},
	{0x383b, 0xa0},//	; 80 PIP Location

	{0x4004, 0x04},//	; black line number
	{0x4005, 0x1a}, //BLC always update

	{0x5b00, 0x02},
	{0x5b01, 0x80},
	{0x5b02, 0x01},
	{0x5b03, 0xe0},
	//{0x4202, 0x0f}, // MIPI stream off
	//{0x0100, 0x01},//	; stream on
#endif
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_pip_prev_settings_master[] = {
#if 1
	//16:9 settings
	//// OV5648 wake up
	{0x3022, 0x00}, // MIP lane enable
	{0x3012, 0x01},
	{0x3013, 0x00},
	{0x3014, 0x0b},
	{0x3005, 0xf0},
	//{0x301a, 0xf0},
	{0x301b, 0xf0},
	{0x5000, 0x06}, // turn on ISP
	{0x5001, 0x00}, // turn on ISP
	{0x5002, 0x41}, // turn on ISP
	{0x5003, 0x0b}, // turn on ISP
	{0x3501, 0x2d},
	{0x3502, 0x60},
	{0x3708, 0x66},
	{0x3709, 0x52},
	{0x370c, 0xc3},
	{0x3800, 0x00}, // x start = 16
	{0x3801, 0x10}, // x start
	{0x3802, 0x00}, // y start = 254
	{0x3803, 0xfe}, // y start
	{0x3804, 0x0a}, // x end = 2607
	{0x3805, 0x2f}, // x end
	{0x3806, 0x06}, // y end = 1701
	{0x3807, 0xa5}, // y end
	{0x3808, 0x05}, // x output size = 1280
	{0x3809, 0x00}, // x output size
	{0x380a, 0x02}, // y output size = 720
	{0x380b, 0xd0}, // y output size
	{0x380c, 0x0e}, // HTS = 3780
	{0x380d, 0xc4}, // HTS
	{0x380e, 0x02}, // VTS = 742
	{0x380f, 0xe6}, // VTS
	{0x3810, 0x00}, // x offset = 8
	{0x3811, 0x08}, // x offset
	{0x3812, 0x00}, // y offset = 2
	{0x3813, 0x02}, // y offset
	{0x3814, 0x31}, // x inc
	{0x3815, 0x31}, // y inc
	{0x3817, 0x00}, // hsync start
	{0x3820, 0x08}, // v flip off, bin off
	{0x3821, 0x07}, // h mirror on, bin on
	{0x3830, 0x00},

//	{0x3836, 0x01},
//	{0x3837, 0xd4},
//	{0x3838, 0x02},
//	{0x3839, 0xe0},
//	{0x383a, 0x0a},
//	{0x383b, 0x80},

	{0x3838, 0x02},
	{0x3839, 0xe0},
	// pip normal
	//6c 3836 01
	//6c 3837 d4
	//6c 383a 0a
	//6c 383b 80
	// pip rotate 180
	{0x3836, 0x01},
	{0x3837, 0xd7},
	{0x383a, 0x0a},
	{0x383b, 0x80},

	{0x4004, 0x02}, // black line number
//	{0x4005, 0x18},//BLC normal freeze

	{0x5b00, 0x01},
	{0x5b01, 0x40},
	{0x5b02, 0x00},
	{0x5b03, 0xf0},
	//{0x4202, 0x0f}, // MIPI stream off
	//{0x0100, 0x01}, // wake up fromm software standby
#else
	//4:3 settings
	//// OV5648 wake up
	{0x3022, 0x00}, // MIPI lane enable
	{0x3012, 0x01},
	{0x3013, 0x00},
	{0x3014, 0x0b},
	{0x3005, 0xf0},
	{0x301a, 0xf0},
	{0x301b, 0xf0},
	{0x5000, 0x06}, // turn on ISP
	{0x5001, 0x00}, // turn on ISP
	{0x5002, 0x41}, // turn on ISP
	{0x5003, 0x0b}, // turn on ISP
	{0x3501, 0x2d},
	{0x3502, 0x60},
	{0x3708, 0x66},
	{0x3709, 0x52},
	{0x370c, 0xc3},
	{0x3800, 0x00}, // x start = 0
	{0x3801, 0x00}, // x start
	{0x3802, 0x00}, // y start = 0
	{0x3803, 0x00}, // y start
	{0x3804, 0x0a}, // x end = 2639
	{0x3805, 0x4f}, // x end
	{0x3806, 0x07}, // y end = 1955
	{0x3807, 0xa3}, // y end
	{0x3808, 0x05}, // x output size = 1296
	{0x3809, 0x10}, // x output size
	{0x380a, 0x03}, // y output size = 972
	{0x380b, 0xcc}, // y output size
	{0x380c, 0x0e}, // HTS = 3780
	{0x380d, 0xc4}, // HTS
	{0x380e, 0x03}, // VTS = 992
	{0x380f, 0xe0}, // VTS
	{0x3810, 0x00}, // x offset = 8
	{0x3811, 0x08}, // x offset
	{0x3812, 0x00}, // y offset = 2
	{0x3813, 0x02}, // y offset
	{0x3814, 0x31}, // x inc
	{0x3815, 0x31}, // y inc
	{0x3817, 0x00}, // hsync start
	{0x3820, 0x08}, // v flip off, v bin off
	{0x3821, 0x07}, // h mirror on, h bin on
	{0x3830, 0x00},
	{0x3836, 0x02},
	{0x3837, 0xd0},
	{0x3838, 0x02},
	{0x3839, 0xe0},
	{0x383a, 0x0a},
	{0x383b, 0x70}, //0x68,
	{0x4004, 0x02}, // black line number
	{0x5b00, 0x01},
	{0x5b01, 0x40},
	{0x5b02, 0x00},
	{0x5b03, 0xf0},
	{0x4202, 0x0f}, // MIPI stream off
	{0x0100, 0x01}, // wake up from standby
#endif
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_pip_preview_settings_slave[] =
{
	{0x3830, 0x40},
	{0x3808, 0x02}, //Timing X output Size = 656
	{0x3809, 0x90}, //Timing X output Size
	{0x380a, 0x01}, //Timing Y output Size = 480
	{0x380b, 0xe0}, //Timing Y output Size

	{0x380e, 0x02}, //OV5648 VTS = 536
	{0x380f, 0x18}, //OV5648 VTS
//	{0x380c, 0x13}, //OV5648 HTS = 5000
//	{0x380d, 0x88}, //OV5648 HTS
	{0x380c, 0x14}, //OV5648 HTS = 5222 /*30fps*/
	{0x380d, 0x66}, //OV5648 HTS

	//OV5648 Block Power OFF
	{0x3022, 0x03}, //pull down data lane 1/2
	{0x3012, 0xa1}, //debug mode
	{0x3013, 0xf0}, //debug mode
	{0x3014, 0x7b},
	{0x3005, 0xf3}, //debug mode

	//{0x301a, 0x96}, //debug mode
	{0x301b, 0x5a}, //debug mode
	{0x5000, 0x00}, //turn off ISP
	{0x5001, 0x00}, //turn off ISP
	{0x5002, 0x00}, //turn off ISP
	{0x5003, 0x00}, //turn off ISP
};


static struct msm_camera_i2c_conf_array ov5648_truly_cm8352_pip_init_conf_master[] = {
	{&ov5648_truly_cm8352_pip_init_settings_master[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_init_settings_master), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov5648_truly_cm8352_pip_preview_conf_slave[] = {
	{&ov5648_truly_cm8352_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array ov5648_truly_cm8352_pip_confs_master[] = {
	{&ov5648_truly_cm8352_pip_snap_settings_master[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_snap_settings_master), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_prev_settings_master[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_prev_settings_master), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_prev_settings_master[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_prev_settings_master), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_prev_settings_master[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_prev_settings_master), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_pip_snap_settings_master[0],
	ARRAY_SIZE(ov5648_truly_cm8352_pip_snap_settings_master), 0, MSM_CAMERA_I2C_BYTE_DATA}
};


static struct msm_sensor_output_info_t ov5648_truly_cm8352_pip_dimensions[] = {
#if 0
	//4:3 settings
	{ /* For SNAPSHOT */
		.x_output = 0xa20,         /*2592*/
		.y_output = 0x798,         /*1944*/
		.line_length_pclk = 0x160a,
		.frame_length_lines = 0x7c0,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 158000000,
		.binning_factor = 0x0,
	},
#else
	//16:9 settings
	{ /* For SNAPSHOT */
		.x_output = 0xa00,		   /*2560*/
		.y_output = 0x5a0,		   /*1440*/
		.line_length_pclk = 0x160a,
		.frame_length_lines = 0x5c8,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 158000000,
		.binning_factor = 0x0,
	},
#endif


	#if 1
	{ /* For PREVIEW */
		.x_output = 0x500,         /*1280*/
		.y_output = 0x2d0,         /*720*/
		.line_length_pclk = 0xec4,
		.frame_length_lines = 0x2e6,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 110000000,
		.binning_factor = 0x0,
	},
	#else
	{ /* For PREVIEW */
		.x_output = 0x510,         /*1296*/
		.y_output = 0x3cc,         /*972*/
		.line_length_pclk = 0xec4,
		.frame_length_lines = 0x3e0,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 110000000,
		.binning_factor = 0x0,
	},
	#endif
	{ /* For 60fps */
		.x_output = 0x500,		   /*1280*/
		.y_output = 0x2d0,		   /*720*/
		.line_length_pclk = 0xec4,
		.frame_length_lines = 0x2e6,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 110000000,
		.binning_factor = 0x0,
	},
	{ /* For 90fps */
		.x_output = 0x500,		   /*1280*/
		.y_output = 0x2d0,		   /*720*/
		.line_length_pclk = 0xec4,
		.frame_length_lines = 0x2e6,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 110000000,
		.binning_factor = 0x0,
	},
	//16:9 settings
	{ /* For SNAPSHOT */
		.x_output = 0xa00,		   /*2560*/
		.y_output = 0x5a0,		   /*1440*/
		.line_length_pclk = 0x160a,
		.frame_length_lines = 0x5c8,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 158000000,
		.binning_factor = 0x0,
	},


};


/***********************PIP settings end*******************************/
/* For PIP feature */
//static int32_t ov5648_working_mode = CAM_MODE_NORMAL;
static int32_t ov5648_working_mode = CAM_MODE_NORMAL;
//static int32_t ov5648_new_mode = CAMi_MODE_PIP;
static int32_t ov5648_new_mode = CAM_MODE_NORMAL;

extern int32_t pip_ov7695_ctrl(int cmd, struct msm_camera_pip_ctl *pip_ctl);

static int ov5648_pwdn_pin = 0;
static int ov5648_reset_pin = 0;
static uint16_t ov5648_i2c_address = 0;

int32_t pip_ov5648_ctrl(int cmd, struct msm_camera_pip_ctl *pip_ctl)
{
	int rc = 0;
	struct msm_camera_i2c_client sensor_i2c_client;
	struct i2c_client client;
	unsigned short rdata = 0;

	CDBG("%s IN, cmd:%d\r\n", __func__, cmd);
	switch(cmd)
	{
		case PIP_CRL_WRITE_SETTINGS:
			if(NULL == pip_ctl){
				CDBG("%s parameters check error, exit\r\n", __func__);
				return rc;
			}
			memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
			memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
			client.addr = ov5648_i2c_address;
			sensor_i2c_client.client = &client;
			CDBG("write_ctl:%d\r\n", pip_ctl->write_ctl);
			if(PIP_REG_SETTINGS_INIT == pip_ctl->write_ctl)
			{
				rc = msm_sensor_write_all_conf_array(
					&sensor_i2c_client, &ov5648_truly_cm8352_pip_init_conf_master[0],
					ARRAY_SIZE(ov5648_truly_cm8352_pip_init_conf_master));
			}
			else
			{
				rc = msm_sensor_write_conf_array(
				&sensor_i2c_client,
				&ov5648_truly_cm8352_pip_preview_conf_slave[0], pip_ctl->write_ctl);
			}
			break;
		case PIP_CRL_POWERUP:
			if(0 != ov5648_pwdn_pin)
			{
				CDBG("PIP ctrl, set the gpio:%d to 1\r\n", ov5648_pwdn_pin);
				gpio_direction_output(ov5648_pwdn_pin, 1);
			}
			break;
		case PIP_CRL_POWERDOWN:
			if(0 != ov5648_pwdn_pin)
			{
				CDBG("PIP ctrl, set the gpio:%d to 0\r\n", ov5648_pwdn_pin);
				gpio_direction_output(ov5648_pwdn_pin, 0);
			}
			break;

		case PIP_CTL_RESET_HW_PULSE:
			if(0 != ov5648_reset_pin)
			{
				CDBG("PIP ctrl, set the gpio:%d to 0\r\n", ov5648_reset_pin);
				gpio_direction_output(ov5648_reset_pin, 0);
				msleep(10);
				CDBG("PIP ctrl, set the gpio:%d to 1\r\n", ov5648_reset_pin);
				gpio_direction_output(ov5648_reset_pin, 1);
			}
			break;
		case PIP_CTL_RESET_SW:
			if(NULL == pip_ctl){
				CDBG("%s parameters check error, exit\r\n", __func__);
				return rc;
			}
			memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
			memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
			client.addr = ov5648_i2c_address;
			sensor_i2c_client.client = &client;
			msm_camera_i2c_write(&sensor_i2c_client, 0x103, 0x1,
				MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case PIP_CTL_STANDBY_SW:
				if(NULL == pip_ctl){
					CDBG("%s parameters check error, exit\r\n", __func__);
					return rc;
				}
				memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
				memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
				client.addr = ov5648_i2c_address;
				sensor_i2c_client.client = &client;
				msm_camera_i2c_write(&sensor_i2c_client, 0x100, 0x0,
					MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case PIP_CTL_STANDBY_EXIT:
				if(NULL == pip_ctl){
					CDBG("%s parameters check error, exit\r\n", __func__);
					return rc;
				}
				memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
				memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
				client.addr = ov5648_i2c_address;
				sensor_i2c_client.client = &client;
				msm_camera_i2c_write(&sensor_i2c_client, 0x100, 0x1,
					MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case PIP_CTL_STREAM_ON:
				if(NULL == pip_ctl){
					CDBG("%s parameters check error, exit\r\n", __func__);
					return rc;
				}
				memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
				memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
				client.addr = ov5648_i2c_address;
				sensor_i2c_client.client = &client;
				msm_camera_i2c_write(&sensor_i2c_client, 0x301a, 0xf0,
					MSM_CAMERA_I2C_BYTE_DATA);
			break;
		case PIP_CTL_STREAM_OFF:
				if(NULL == pip_ctl){
					CDBG("%s parameters check error, exit\r\n", __func__);
					return rc;
				}
				memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
				memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
				client.addr = ov5648_i2c_address;
				sensor_i2c_client.client = &client;
				msm_camera_i2c_write(&sensor_i2c_client, 0x301a, 0xf1,
					MSM_CAMERA_I2C_BYTE_DATA);
				break;
		case PIP_CTL_RESET_HW_DOWN:
			if(0 != ov5648_reset_pin)
			{
				CDBG("PIP ctrl, set the gpio:%d to 0\r\n", ov5648_reset_pin);
				gpio_direction_output(ov5648_reset_pin, 0);
			}
			break;
		case PIP_CTL_MIPI_DOWN:
			if(NULL == pip_ctl){
				CDBG("%s parameters check error, exit\r\n", __func__);
				return rc;
			}
			memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
			memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
			client.addr = ov5648_i2c_address;
			sensor_i2c_client.client = &client;
			rc = msm_camera_i2c_read(&sensor_i2c_client, 0x3018,
				&rdata, MSM_CAMERA_I2C_BYTE_DATA);
			CDBG("ov5648_truly_cm8352_sensor_power_down: %d\n", rc);
			rdata |= 0x18;
			msm_camera_i2c_write(&sensor_i2c_client,
				0x3018, rdata,
				MSM_CAMERA_I2C_BYTE_DATA);
			break;
		default:
				break;
	}

	return rc;
}
EXPORT_SYMBOL(pip_ov5648_ctrl);
/* End */

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_start_settings[] = {
	{0x301a, 0xf0},  /* streaming on */
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_stop_settings[] = {
	{0x301a, 0xf1},  /* streaming off*/
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_groupon_settings[] = {
	{0x3208, 0x0},
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_groupoff_settings[] = {
	{0x3208, 0x10},
	{0x3208, 0xa0},
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_prev_settings[] = {
	/*1296*972 preview*/
	// 1296x972 30fps 2 lane MIPI 420Mbps/lane
	{0x3035, 0x21}, //PLL
	{0x3501, 0x3d},//exposure
	{0x3502, 0x00},//exposure
	{0x3708, 0x66},
	{0x3709, 0x52},
	{0x370c, 0xc3},

	{0x3500, 0x00}, // exposure [19:16]
	{0x3501, 0x3d}, // exposure [15:8]
	{0x3502, 0xc0}, // exposure [7:0], exposure
	{0x350a, 0x00}, // gain[9:8]
	{0x350b, 0x23}, // gain[7:0],

	{0x3800, 0x00},//xstart = 0
	{0x3801, 0x00},//x start
	{0x3802, 0x00},//y start = 0
	{0x3803, 0x00},//y start
	{0x3804, 0x0a},//xend = 2623
	{0x3805, 0x3f},//xend
	{0x3806, 0x07},//yend = 1955
	{0x3807, 0xa3},//yend
	{0x3808, 0x05},//x output size = 1296
	{0x3809, 0x10},//x output size
	{0x380a, 0x03},//y output size = 972
	{0x380b, 0xcc},//y output size
	{0x380c, 0x0B},//hts = 2816
	{0x380d, 0x00},//hts
	{0x380e, 0x03},//vts = 992
	{0x380f, 0xe0},//vts
	{0x3810, 0x00},//isp x win = 8
	{0x3811, 0x08},//isp x win
	{0x3812, 0x00},//isp y win = 4
	{0x3813, 0x04},//isp y win
	{0x3814, 0x31},//x inc
	{0x3815, 0x31},//y inc
	{0x3817, 0x00},//hsync start
	{0x3820, 0x08},//flip off, v bin off
	{0x3821, 0x07},//mirror on, h bin on

	{0x4004, 0x02},//black line number
	{0x4005, 0x18},//BLC normal freeze
	{0x350b, 0x80},//gain = 8x
	{0x4837, 0x17},//MIPI global timing
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_snap_settings[] = {
	/* 2592*1944 capture */
	//2592x1944 15fps 2 lane MIPI 420Mbps/lane
	{0x3035, 0x21}, //PLL
	{0x3501, 0x7b}, //exposure
	{0x2502, 0x00}, //exposure
	{0x3708, 0x63}, //
	{0x3709, 0x12}, //
	{0x370c, 0xc0}, //

	{0x3800, 0x00}, //xstart = 0
	{0x3801, 0x00}, //xstart
	{0x3802, 0x00}, //ystart = 0
	{0x3803, 0x00}, //ystart
	{0x3804, 0x0a}, //xend = 2623
	{0x3805, 0x3f}, //xend
	{0x3806, 0x07}, //yend = 1955
	{0x3807, 0xa3}, //yend
	{0x3808, 0x0a}, //x output size = 2592
	{0x3809, 0x20}, //x output size
	{0x380a, 0x07}, //y output size = 1944
	{0x380b, 0x98}, //y output size
	{0x380c, 0x0b}, //hts = 2816
	{0x380d, 0x00}, //hts
	{0x380e, 0x07}, //vts = 1984
	{0x380f, 0xc0}, //vts
	{0x3810, 0x00}, //isp x win = 16
	{0x3811, 0x10}, //isp x win
	{0x3812, 0x00}, //isp y win = 6
	{0x3813, 0x06}, //isp y win
	{0x3814, 0x11}, //x inc
	{0x3815, 0x11}, //y inc
	{0x3817, 0x00}, //hsync start
	{0x3820, 0x40}, //flip off, v bin off
	{0x3821, 0x06}, //mirror on, v bin off

	{0x4004, 0x04}, //black line number
	{0x4005, 0x1a}, //BLC always update
	{0x350b, 0x40}, //gain = 4x
	{0x4837, 0x17}, //MIPI global timing
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_video_60fps_settings[] = {
	//640x480 60fps 2 lane MIPI 280Mbps/lane
	{0x3035, 0x31}, //PLL
	{0x2501, 0x1e}, //exposure
	{0x3502, 0xc0}, //exposure
	{0x3708, 0x69},
	{0x3709, 0x92},
	{0x370c, 0xc3},

	{0x3800, 0x00}, //xstart = 0
	{0x3801, 0x00}, //xstart
	{0x3802, 0x00}, //ystart = 2
	{0x3803, 0x02}, //ystart
	{0x3804, 0x0a}, //xend = 2623
	{0x3805, 0x3f}, //xend
	{0x3806, 0x07}, //yend = 1953
	{0x3807, 0xa1}, //yend
	{0x3808, 0x02}, //x output size = 640
	{0x3809, 0x80}, //x output size
	{0x380a, 0x01}, //y output size = 480
	{0x380b, 0xe0}, //y output size
	{0x380c, 0x07}, //hts = 1832
	{0x380d, 0x28}, //hts
	{0x380e, 0x01}, //vts = 508
	{0x380f, 0xfc}, //vts
	{0x3810, 0x00}, //isp x win = 8
	{0x3811, 0x08}, //isp x win
	{0x3812, 0x00}, //isp y win = 4
	{0x3813, 0x04}, //isp y win
	{0x3814, 0x71}, //x inc
	{0x3815, 0x53}, //y inc
	{0x3817, 0x00}, //hsync start
	{0x3820, 0x08}, //flip off, v bin off
	{0x3821, 0x07}, //mirror on, h bin on

	{0x4004, 0x02}, //number of black line
	{0x4005, 0x18}, //BLC normal freeze
	{0x350b, 0xf0}, //gain = 8x
	{0x4837, 0x23}, //MIPI global timing
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_video_90fps_settings[] = {
	// 640x480 90fps 2 lane MIPI 420Mbps/lane
	{0x3035, 0x21}, //PLL
	{0x2501, 0x1e}, //exposure
	{0x3502, 0xc0}, //exposure
	{0x3708, 0x69},
	{0x3709, 0x92},
	{0x370c, 0xc3},

	{0x3800, 0x00}, //xstart = 0
	{0x3801, 0x00}, //xstart
	{0x3802, 0x00}, //ystart = 2
	{0x3803, 0x02}, //ystart
	{0x3804, 0x0a}, //xend = 2623
	{0x3805, 0x3f}, //xend
	{0x3806, 0x07}, //yend = 1953
	{0x3807, 0xa1}, //yend
	{0x3808, 0x02}, //x output size = 640
	{0x3809, 0x80}, //x output size
	{0x380a, 0x01}, //y output size = 480
	{0x380b, 0xe0}, //y output size
	{0x380c, 0x07}, //hts = 1832
	{0x380d, 0x28}, //hts
	{0x380e, 0x01}, //vts = 508
	{0x380f, 0xfc}, //vts
	{0x3810, 0x00}, //isp x win = 8
	{0x3811, 0x08}, //isp x win
	{0x3812, 0x00}, //isp y win = 4
	{0x3813, 0x04}, //isp y win
	{0x3814, 0x71}, //x inc
	{0x3815, 0x53}, //y inc
	{0x3817, 0x00}, //hsync start
	{0x3820, 0x08}, //flip off, v bin off
	{0x3821, 0x07}, //mirror on, h bin on

	{0x4004, 0x02}, //number of black line
	{0x4005, 0x18}, //BLC normal freeze
	{0x350b, 0xf0}, //gain = 8x
	{0x4837, 0x17}, //MIPI global timing
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_zsl_settings[] = {
	{0x3035, 0x21}, //PLL

	{0x3500, 0x00}, // exposure [19:16]
	{0x3501, 0x10}, // exposure [15:8]
	{0x3502, 0x80}, // exposure [7:0], exposure
	{0x3503, 0x03}, // gain has no delay, manual agc/aec
	{0x350a, 0x00}, // gain[9:8]
	{0x350b, 0x7f}, // gain[7:0],

	{0x2502, 0x00}, //exposure
	{0x3708, 0x63}, //
	{0x3709, 0x12}, //
	{0x370c, 0xc0}, //

	{0x3800, 0x00}, //xstart = 0
	{0x3801, 0x00}, //xstart
	{0x3802, 0x00}, //ystart = 0
	{0x3803, 0x00}, //ystart
	{0x3804, 0x0a}, //xend = 2623
	{0x3805, 0x3f}, //xend
	{0x3806, 0x07}, //yend = 1955
	{0x3807, 0xa3}, //yend
	{0x3808, 0x0a}, //x output size = 2592
	{0x3809, 0x20}, //x output size
	{0x380a, 0x07}, //y output size = 1944
	{0x380b, 0x98}, //y output size
	{0x380c, 0x0b}, //hts = 2816
	{0x380d, 0x00}, //hts
	{0x380e, 0x07}, //vts = 1984
	{0x380f, 0xc0}, //vts
	{0x3810, 0x00}, //isp x win = 16
	{0x3811, 0x10}, //isp x win
	{0x3812, 0x00}, //isp y win = 6
	{0x3813, 0x06}, //isp y win
	{0x3814, 0x11}, //x inc
	{0x3815, 0x11}, //y inc
	{0x3817, 0x00}, //hsync start
	{0x3820, 0x40}, //flip off, v bin off
	{0x3821, 0x06}, //mirror on, v bin off

	{0x4004, 0x04}, //black line number
	{0x4005, 0x1a}, //BLC always update
	{0x350b, 0x40}, //gain = 4x
	{0x4837, 0x17}, //MIPI global timing
};

static struct msm_camera_i2c_reg_conf ov5648_truly_cm8352_recommend_settings[] =
{
//	{0x0103, 0x01}, // software reset
	{0x3001, 0x00}, // D[7:0] set to input
	{0x3002, 0x00}, // Vsync, PCLK, FREX, Strobe, CSD, CSK, GPIO input
	{0x3011, 0x02}, // Drive strength 2x
	{0x3018, 0x4c}, // MIPI 2 lane
	{0x3022, 0x00},
	{0x3034, 0x1a}, // 10-bit mode
	{0x3035, 0x21}, // PLL
	{0x3036, 0x69}, // PLL
	{0x3037, 0x03}, // PLL
	{0x3038, 0x00}, // PLL
	{0x3039, 0x00}, // PLL
	{0x303a, 0x00}, // PLLS
	{0x303b, 0x19}, // PLLS
	{0x303c, 0x11}, // PLLS
	{0x303d, 0x30}, //  PLLS
	{0x3105, 0x11},
	{0x3106, 0x05}, // PLL
	{0x3304, 0x28},
	{0x3305, 0x41},
	{0x3306, 0x30},
	{0x3308, 0x00},
	{0x3309, 0xc8},
	{0x330a, 0x01},
	{0x330b, 0x90},
	{0x330c, 0x02},
	{0x330d, 0x58},
	{0x330e, 0x03},
	{0x330f, 0x20},
	{0x3300, 0x00},

//	{0x3500, 0x00}, // exposure [19:16]
//	{0x3501, 0x00}, // exposure [15:8]
//	{0x3502, 0x10}, // exposure [7:0], exposure = 0x3d0 = 976
	{0x3503, 0x07}, // gain has no delay, manual agc/aec
//	{0x350a, 0x00}, // gain[9:8]
//	{0x350b, 0x00}, // gain[7:0],

	{0x3601, 0x33}, // analog control
	{0x3602, 0x00}, // analog control
	{0x3611, 0x0e}, // analog control
	{0x3612, 0x2b}, // analog control
	{0x3614, 0x50}, // analog control
	{0x3620, 0x33}, // analog control
	{0x3622, 0x00}, // analog control
	{0x3630, 0xad}, // analog control
	{0x3631, 0x00}, // analog control
	{0x3632, 0x94}, // analog control
	{0x3633, 0x17}, // analog control
	{0x3634, 0x14}, // analog control
	{0x3705, 0x2a}, // analog control
	{0x3708, 0x66}, // analog control
	{0x3709, 0x52}, // analog control
	{0x370b, 0x23}, // analog control
	{0x370c, 0xc3}, // analog control
	{0x370d, 0x00}, // analog control
	{0x370e, 0x00}, // analog control
	{0x371c, 0x07}, // analog control
	{0x3739, 0xd2}, // analog control
	{0x373c, 0x00},

	{0x3800, 0x00}, // xstart = 0
	{0x3801, 0x00}, // xstart
	{0x3802, 0x00}, // ystart = 0
	{0x3803, 0x00}, // ystart
	{0x3804, 0x0a}, // xend = 2623
	{0x3805, 0x3f}, // yend
	{0x3806, 0x07}, // yend = 1955
	{0x3807, 0xa3}, // yend
	{0x3808, 0x05}, // x output size = 1296
	{0x3809, 0x10}, // x output size
	{0x380a, 0x03}, // y output size = 972
	{0x380b, 0xcc}, // y output size
	{0x380c, 0x05}, // hts = 1408
	{0x380d, 0x80}, // hts
	{0x380e, 0x03}, // vts = 992
	{0x380f, 0xe0}, // vts
	{0x3810, 0x00}, // isp x win = 8
	{0x3811, 0x08}, // isp x win
	{0x3812, 0x00}, // isp y win = 4
	{0x3813, 0x04}, // isp y win
	{0x3814, 0x31}, // x inc
	{0x3815, 0x31}, // y inc
	{0x3817, 0x00}, // hsync start
	{0x3820, 0x08}, // flip off, v bin off
	{0x3821, 0x07}, // mirror on, h bin on
	{0x3826, 0x03},
	{0x3829, 0x00},
	{0x382b, 0x0b},
	{0x3830, 0x00},
	{0x3836, 0x00},
	{0x3837, 0x00},
	{0x3838, 0x00},
	{0x3839, 0x04},
	{0x383a, 0x00},
	{0x383b, 0x01},

	{0x3b00, 0x00}, // strobe off
	{0x3b02, 0x08}, // shutter delay
	{0x3b03, 0x00}, // shutter delay
	{0x3b04, 0x04}, // frex_exp
	{0x3b05, 0x00}, // frex_exp
	{0x3b06, 0x04},
	{0x3b07, 0x08}, // frex inv
	{0x3b08, 0x00}, // frex exp req
	{0x3b09, 0x02}, // frex end option
	{0x3b0a, 0x04}, // frex rst length
	{0x3b0b, 0x00}, // frex strobe width
	{0x3b0c, 0x3d}, // frex strobe width

	{0x3f01, 0x0d},
	{0x3f0f, 0xf5},

	{0x4000, 0x89}, // blc enable
	{0x4001, 0x02}, // blc start line
	{0x4002, 0x45}, // blc auto, reset frame number = 5
	{0x4004, 0x02}, // black line number
	{0x4005, 0x18}, // blc normal freeze
	{0x4006, 0x08},
	{0x4007, 0x10},
	{0x4008, 0x00},

	{0x4300, 0xf8},
	{0x4303, 0xff},
	{0x4304, 0x00},
	{0x4307, 0xff},
	{0x4520, 0x00},
	{0x4521, 0x00},
	{0x4511, 0x22},

	{0x4800, 0x24},
	{0x481f, 0x3c}, // MIPI clk prepare min
	{0x4826, 0x00}, // MIPI hs prepare min
	{0x4837, 0x18}, // MIPI global timing
	{0x4b00, 0x06},
	{0x4b01, 0x0a},
	{0x5000, 0xff}, // bpc on, wpc on
	{0x5001, 0x00}, // awb disable
	{0x5002, 0x41}, // win enable, awb gain enable
	{0x5003, 0x0a}, // buf en, bin auto en
	{0x5004, 0x00}, // size man off
	{0x5043, 0x00},
	{0x5013, 0x00},
	{0x501f, 0x03}, // ISP output data
	{0x503d, 0x00}, // test pattern off
	{0x5180, 0x08}, // manual gain enable
	{0x5a00, 0x08},
	{0x5b00, 0x01},
	{0x5b01, 0x40},
	{0x5b02, 0x00},
	{0x5b03, 0xf0},
	{0x0100, 0x01}, // wake up from software sleep

	{0x350b, 0x80}, // gain = 8x
	{0x4837, 0x17}, // MIPI global timing
};


static struct msm_camera_i2c_conf_array ov5648_truly_cm8352_init_conf[] = {
	{&ov5648_truly_cm8352_recommend_settings[0],
	ARRAY_SIZE(ov5648_truly_cm8352_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov5648_truly_cm8352_confs[] = {
	{&ov5648_truly_cm8352_snap_settings[0],
	ARRAY_SIZE(ov5648_truly_cm8352_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_prev_settings[0],
	ARRAY_SIZE(ov5648_truly_cm8352_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_video_60fps_settings[0],
	ARRAY_SIZE(ov5648_truly_cm8352_video_60fps_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_video_90fps_settings[0],
	ARRAY_SIZE(ov5648_truly_cm8352_video_90fps_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov5648_truly_cm8352_zsl_settings[0],
	ARRAY_SIZE(ov5648_truly_cm8352_zsl_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_csi_params ov5648_truly_cm8352_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 20,
};

static struct v4l2_subdev_info ov5648_truly_cm8352_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_sensor_output_info_t ov5648_truly_cm8352_dimensions[] = {
	{ /* For SNAPSHOT */
		.x_output = 0xa20,         /*2592*/
		.y_output = 0x798,         /*1944*/
		.line_length_pclk = 0xb00,
		.frame_length_lines = 0x7c0,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 158000000,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW */
		.x_output = 0x510,         /*1296*/
		.y_output = 0x3cc,         /*972*/
		.line_length_pclk = 0xb00,
		.frame_length_lines = 0x3e0,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 110000000,
		.binning_factor = 0x0,
	},
	{ /* For 60fps */
		.x_output = 0x280,  /*640*/
		.y_output = 0x1E0,   /*480*/
		.line_length_pclk = 0x728,
		.frame_length_lines = 0x1FC,
		.vt_pixel_clk = 56000000,
		.op_pixel_clk = 159408000,
		.binning_factor = 0x0,
	},
	{ /* For 90fps */
		.x_output = 0x280,  /*640*/
		.y_output = 0x1E0,  /*480*/
		.line_length_pclk = 0x728,
		.frame_length_lines = 0x1FC,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 159408000,
		.binning_factor = 0x0,
	},
	{ /* For ZSL */
		.x_output = 0xa20,         /*2592*/
		.y_output = 0x798,         /*1944*/
		.line_length_pclk = 0xb00,
		.frame_length_lines = 0x7c0,
		.vt_pixel_clk = 84000000,
		.op_pixel_clk = 158000000,
		.binning_factor = 0x0,
	},

};

static struct msm_sensor_output_reg_addr_t ov5648_truly_cm8352_reg_addr = {
	.x_output = 0x3808,
	.y_output = 0x380A,
	.line_length_pclk = 0x380C,
	.frame_length_lines = 0x380E,
};

static struct msm_camera_csi_params *ov5648_truly_cm8352_csi_params_array[] = {
	&ov5648_truly_cm8352_csi_params, /* Snapshot */
	&ov5648_truly_cm8352_csi_params, /* Preview */
	&ov5648_truly_cm8352_csi_params, /* 60fps */
	&ov5648_truly_cm8352_csi_params, /* 90fps */
	&ov5648_truly_cm8352_csi_params, /* ZSL */
};

static struct msm_sensor_id_info_t ov5648_truly_cm8352_id_info = {
	.sensor_id_reg_addr = 0x300a,
	.sensor_id = 0x5648,
};

static struct msm_sensor_exp_gain_info_t ov5648_truly_cm8352_exp_gain_info = {
	.coarse_int_time_addr = 0x3500,
	.global_gain_addr = 0x350A,
	.vert_offset = 4,
};

void ov5648_truly_cm8352_sensor_reset_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
	msm_camera_i2c_write(
		s_ctrl->sensor_i2c_client,
		0x103, 0x1,
		MSM_CAMERA_I2C_BYTE_DATA);
}

#ifdef OV5648_TRULY_CM8352_OTP_FEATURE

int RG_Ratio_Typical = 0x125;
int BG_Ratio_Typical = 0x185;

struct otp_struct {
     int customer_id;
     int module_integrator_id;
     int lens_id;
     int rg_ratio;
     int bg_ratio;
     int user_data[2];
     int light_rg;
     int light_bg;
};

static struct msm_sensor_ctrl_t *otp_s_ctrl = NULL;

int32_t ov5648_truly_cm8352_otp_write(uint16_t addr, uint16_t data)
{
	return msm_camera_i2c_write(otp_s_ctrl->sensor_i2c_client, addr, data,
		MSM_CAMERA_I2C_BYTE_DATA);
}


uint16_t ov5648_truly_cm8352_otp_read(uint16_t addr)
{
    uint16_t temp;
	msm_camera_i2c_read(otp_s_ctrl->sensor_i2c_client, addr, &temp,
		MSM_CAMERA_I2C_BYTE_DATA);
    return temp;
}

// index: index of otp group. (0, 1, 2)
// return:    0, group index is empty
//       1, group index has invalid data
//       2, group index has valid data
int ov5648_truly_cm8352_check_otp(int index)
{
     int temp, i;
     int address;

     if (index<2)
     {
         // read otp --Bank 0
         ov5648_truly_cm8352_otp_write(0x3d84, 0xc0);
         ov5648_truly_cm8352_otp_write(0x3d85, 0x00);
         ov5648_truly_cm8352_otp_write(0x3d86, 0x0f);
         ov5648_truly_cm8352_otp_write(0x3d81, 0x01);

         usleep_range(10000, 10500);
         address = 0x3d05 + index*9;
     }
     else{
         // read otp --Bank 1
         ov5648_truly_cm8352_otp_write(0x3d84, 0xc0);
         ov5648_truly_cm8352_otp_write(0x3d85, 0x10);
         ov5648_truly_cm8352_otp_write(0x3d86, 0x1f);
         ov5648_truly_cm8352_otp_write(0x3d81, 0x01);
         usleep_range(10000, 10500);
         address = 0x3d05 + index*9-16;
     }
     temp = ov5648_truly_cm8352_otp_read(address);

     // disable otp read
     ov5648_truly_cm8352_otp_write(0x3d81, 0x00);

     // clear otp buffer
     for (i=0;i<16;i++) {
         ov5648_truly_cm8352_otp_write(0x3d00 + i, 0x00);
     }

     if (!temp) {
         return 0;
     }
     else if ((!(temp & 0x80)) && (temp&0x7f)) {
         return 2;
     }
     else {
         return 1;
     }
}

// index: index of otp group. (0, 1, 2)
// return:    0,
int ov5648_truly_cm8352_read_otp(int index, struct otp_struct * otp_ptr)
{
     int i;
     int address;

     // read otp into buffer
     if (index<2)
     {
         // read otp --Bank 0
         ov5648_truly_cm8352_otp_write(0x3d84, 0xc0);
         ov5648_truly_cm8352_otp_write(0x3d85, 0x00);
         ov5648_truly_cm8352_otp_write(0x3d86, 0x0f);
         ov5648_truly_cm8352_otp_write(0x3d81, 0x01);
         usleep_range(10000, 10500);
         address = 0x3d05 + index*9;
     }
     else{
         // read otp --Bank 1
         ov5648_truly_cm8352_otp_write(0x3d84, 0xc0);
         ov5648_truly_cm8352_otp_write(0x3d85, 0x10);
         ov5648_truly_cm8352_otp_write(0x3d86, 0x1f);
         ov5648_truly_cm8352_otp_write(0x3d81, 0x01);
         usleep_range(10000, 10500);
         address = 0x3d05 + index*9-16;
     }

     (*otp_ptr).customer_id = (ov5648_truly_cm8352_otp_read(address) & 0x7f);
     (*otp_ptr).module_integrator_id = ov5648_truly_cm8352_otp_read(address);
     (*otp_ptr).lens_id = ov5648_truly_cm8352_otp_read(address + 1);
     (*otp_ptr).rg_ratio = (ov5648_truly_cm8352_otp_read(address + 2)<<2) + (ov5648_truly_cm8352_otp_read(address + 6)>>6) ;
     (*otp_ptr).bg_ratio = (ov5648_truly_cm8352_otp_read(address + 3)<<2) +((ov5648_truly_cm8352_otp_read(address + 6)>>4)&(0x03));
     (*otp_ptr).user_data[0] = ov5648_truly_cm8352_otp_read(address + 4);
     (*otp_ptr).user_data[1] = ov5648_truly_cm8352_otp_read(address + 5);
     (*otp_ptr).light_rg = (int)(ov5648_truly_cm8352_otp_read(address + 7)<<2) + (int)((ov5648_truly_cm8352_otp_read(address + 6)>>2)&(0x03));
     (*otp_ptr).light_bg = (int)(ov5648_truly_cm8352_otp_read(address + 8)<<2) + (int)((ov5648_truly_cm8352_otp_read(address + 6))&(0x03));

     CDBG("==========OV5648 OTP INFO==========\r\n");
     CDBG("%s-customer_id  = 0x%x\r\n", __func__, otp_ptr->customer_id);
     CDBG("%s-module_integrator_id      = 0x%x\r\n", __func__, otp_ptr->module_integrator_id);
     CDBG("%s-lens_id     = 0x%x\r\n", __func__, otp_ptr->lens_id);
     CDBG("%s-rg_ratio     = 0x%x\r\n", __func__, otp_ptr->rg_ratio);
     CDBG("%s-bg_ratio     = 0x%x\r\n", __func__, otp_ptr->bg_ratio);

     CDBG("%s-user_data[0] = 0x%x\r\n", __func__, otp_ptr->user_data[0]);
     CDBG("%s-user_data[1] = 0x%x\r\n", __func__, otp_ptr->user_data[1]);
     CDBG("%s-light_rg = 0x%x\r\n", __func__, otp_ptr->light_rg);
     CDBG("%s-light_bg = 0x%x\r\n", __func__, otp_ptr->light_bg);
     // disable otp read
     ov5648_truly_cm8352_otp_write(0x3d81, 0x00);

     // clear otp buffer
     for (i=0;i<16;i++) {
         ov5648_truly_cm8352_otp_write(0x3d00 + i, 0x00);
     }

     return 0;
}

// R_gain, sensor red gain of AWB, 0x400 =1
// G_gain, sensor green gain of AWB, 0x400 =1
// B_gain, sensor blue gain of AWB, 0x400 =1
// return 0;
int ov5648_truly_cm8352_update_awb_gain(int R_gain, int G_gain, int B_gain)
{
     if (R_gain>0x400) {
         ov5648_truly_cm8352_otp_write(0x5186, R_gain>>8);
         ov5648_truly_cm8352_otp_write(0x5187, R_gain & 0x00ff);
     }

     if (G_gain>0x400) {
         ov5648_truly_cm8352_otp_write(0x5188, G_gain>>8);
         ov5648_truly_cm8352_otp_write(0x5189, G_gain & 0x00ff);
     }

     if (B_gain>0x400) {
         ov5648_truly_cm8352_otp_write(0x518a, B_gain>>8);
         ov5648_truly_cm8352_otp_write(0x518b, B_gain & 0x00ff);
     }
     return 0;
}


// call this function after OV5647 initialization
// return value: 0 update success
//       1, no OTP
int ov5648_truly_cm8352_update_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
     struct otp_struct current_otp;
     int i;
     int otp_index;
     int temp;
     int R_gain, G_gain, B_gain, G_gain_R, G_gain_B;
     int rg,bg;
     otp_s_ctrl = s_ctrl;


     // R/G and B/G of current camera module is read out from sensor OTP
     // check first OTP with valid data
     for(i=0;i<3;i++) {
         temp = ov5648_truly_cm8352_check_otp(i);
         if (temp == 2) {
              otp_index = i;
              break;
         }
     }

     if (i==3) {
         // no valid wb OTP data
         return 1;
     }

     ov5648_truly_cm8352_read_otp(otp_index, &current_otp);

     if(current_otp.light_rg==0) {
         // no light source information in OTP
         rg = current_otp.rg_ratio;
     }
     else {
         // light source information found in OTP
         rg = current_otp.rg_ratio * (current_otp.light_rg +512) / 1024;
     }

     if(current_otp.light_bg==0) {
         // no light source information in OTP
         bg = current_otp.bg_ratio;
     }
     else {
         // light source information found in OTP
         bg = current_otp.bg_ratio * (current_otp.light_bg +512) / 1024;
     }


     //calculate G gain
     //0x400 = 1x gain
     if(bg < BG_Ratio_Typical) {
         if (rg< RG_Ratio_Typical) {
              // current_otp.bg_ratio < BG_Ratio_typical &&
              // current_otp.rg_ratio < RG_Ratio_typical
              G_gain = 0x400;
              B_gain = 0x400 * BG_Ratio_Typical / bg;
              R_gain = 0x400 * RG_Ratio_Typical / rg;
         }
         else {
              // current_otp.bg_ratio < BG_Ratio_typical &&
              // current_otp.rg_ratio >= RG_Ratio_typical
              R_gain = 0x400;
              G_gain = 0x400 * rg / RG_Ratio_Typical;
              B_gain = G_gain * BG_Ratio_Typical /bg;
         }
     }
     else {
         if (rg < RG_Ratio_Typical) {
              // current_otp.bg_ratio >= BG_Ratio_typical &&
              // current_otp.rg_ratio < RG_Ratio_typical
              B_gain = 0x400;
              G_gain = 0x400 * bg / BG_Ratio_Typical;
              R_gain = G_gain * RG_Ratio_Typical / rg;
         }
         else {
              // current_otp.bg_ratio >= BG_Ratio_typical &&
              // current_otp.rg_ratio >= RG_Ratio_typical
              G_gain_B = 0x400 * bg / BG_Ratio_Typical;
              G_gain_R = 0x400 * rg / RG_Ratio_Typical;

              if(G_gain_B > G_gain_R ) {
                  B_gain = 0x400;
                  G_gain = G_gain_B;
                  R_gain = G_gain * RG_Ratio_Typical /rg;
              }
              else {
                   R_gain = 0x400;
                   G_gain = G_gain_R;
                   B_gain = G_gain * BG_Ratio_Typical / bg;
              }
         }
     }

     ov5648_truly_cm8352_update_awb_gain(R_gain, G_gain, B_gain);

     return 0;

}

#endif

static int32_t ov5648_truly_cm8352_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
		uint16_t gain, uint32_t line)
{

	static uint16_t max_line = 1964;
	uint32_t fix_line = s_ctrl->curr_frame_length_lines;
	uint8_t offset = s_ctrl->sensor_exp_gain_info->vert_offset;

	uint8_t gain_lsb, gain_hsb;
	u8 intg_time_hsb, intg_time_msb, intg_time_lsb;
	CDBG(KERN_ERR "curr_frame_length_lines is 0x%x, %d\r\n"
		, fix_line, fix_line);

	gain_lsb = (uint8_t) (gain);
	gain_hsb = (uint8_t)((gain & 0x300)>>8);

	CDBG(KERN_ERR "snapshot exposure seting 0x%x, 0x%x, %d"
		, gain, line, line);

	if(CAM_MODE_NORMAL == ov5648_working_mode)
	{
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		if (line > (fix_line - offset)) {
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				s_ctrl->sensor_output_reg_addr->frame_length_lines,
				(uint8_t)((line + offset) >> 8),
				MSM_CAMERA_I2C_BYTE_DATA);

			msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
				(uint8_t)((line + offset) & 0x00FF),
				MSM_CAMERA_I2C_BYTE_DATA);
			max_line = line + offset;
		} else if (max_line > fix_line) {
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				s_ctrl->sensor_output_reg_addr->frame_length_lines,
				(uint8_t)(fix_line >> 8),
				MSM_CAMERA_I2C_BYTE_DATA);

			 msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
				s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
				(uint8_t)(fix_line & 0x00FF),
				MSM_CAMERA_I2C_BYTE_DATA);
				max_line = fix_line;
		}


		line = line<<4;
		/* ov5648_truly_cm8352 need this operation */
		intg_time_hsb = (u8)(line>>16);
		intg_time_msb = (u8) ((line & 0xFF00) >> 8);
		intg_time_lsb = (u8) (line & 0x00FF);

		/* FIXME for BLC trigger */
		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			intg_time_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			intg_time_msb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
			intg_time_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		/* gain */

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr,
			gain_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
			gain_lsb^0x1,
			MSM_CAMERA_I2C_BYTE_DATA);

		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			intg_time_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			intg_time_msb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
			intg_time_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		/* gain */

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr,
			gain_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
			gain_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);


		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	}
	else if(CAM_MODE_PIP == ov5648_working_mode)
	{
		/* PIP exposure and gain settings, fix VTS, do not write line */
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

		/* FIXME for BLC trigger */

		/* adjust frame rate */

		//.frame_length_lines 0x5c8,
		if (line > (0x5c8 - 4))
		{
			line = (0x5c8 - 4);
		}

		line = line<<4;
		/* ov5648_truly_cm8352 need this operation */
		intg_time_hsb = (u8)(line>>16);
		intg_time_msb = (u8) ((line & 0xFF00) >> 8);
		intg_time_lsb = (u8) (line & 0x00FF);

		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			intg_time_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			intg_time_msb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
			intg_time_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		/* gain */

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr,
			gain_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
			gain_lsb^0x1,
			MSM_CAMERA_I2C_BYTE_DATA);

		/* gain */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr,
			gain_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
			gain_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	}
	return 0;

}


static int esposure_delay_en = 1;

static int32_t ov5648_truly_cm8352_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
						uint16_t gain, uint32_t line)
{
	u8 intg_time_hsb, intg_time_msb, intg_time_lsb;
	uint8_t gain_lsb, gain_hsb;
	uint32_t fl_lines = s_ctrl->curr_frame_length_lines;
	uint8_t offset = s_ctrl->sensor_exp_gain_info->vert_offset;

	CDBG(KERN_ERR "preview exposure setting 0x%x, 0x%x, %d\n",
		gain, line, line);

	gain_lsb = (uint8_t) (gain);
	gain_hsb = (uint8_t)((gain & 0x300)>>8);

	fl_lines = (fl_lines * s_ctrl->fps_divider) / Q10;

	if(CAM_MODE_NORMAL == ov5648_working_mode)
	{
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);

		/* adjust frame rate */
		if ((s_ctrl->curr_res < MSM_SENSOR_RES_2) && (line > (fl_lines - offset)))
			fl_lines = line + offset;

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines,
			(uint8_t)(fl_lines >> 8),
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
			(uint8_t)(fl_lines & 0x00FF),
			MSM_CAMERA_I2C_BYTE_DATA);

		line = line<<4;
		/* ov5648_truly_cm8352 need this operation */
		intg_time_hsb = (u8)(line>>16);
		intg_time_msb = (u8) ((line & 0xFF00) >> 8);
		intg_time_lsb = (u8) (line & 0x00FF);

		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			intg_time_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			intg_time_msb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
			intg_time_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		/* gain */

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr,
			gain_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
			gain_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	}
	else if(CAM_MODE_PIP == ov5648_working_mode)
	{
		s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
		/* adjust frame rate */
		if (line > (fl_lines - offset))
		{
			line = fl_lines - offset;
		}

		line = line<<4;
		/* ov5648_truly_cm8352 need this operation */
		intg_time_hsb = (u8)(line>>16);
		intg_time_msb = (u8) ((line & 0xFF00) >> 8);
		intg_time_lsb = (u8) (line & 0x00FF);

		/* Coarse Integration Time */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
			intg_time_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
			intg_time_msb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 2,
			intg_time_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		/* gain */
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr,
			gain_hsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
			gain_lsb,
			MSM_CAMERA_I2C_BYTE_DATA);

		s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
	}

	if(esposure_delay_en)
	{
		msleep(200);
		esposure_delay_en = 0;
	}
	return 0;
}

static const struct i2c_device_id ov5648_truly_cm8352_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov5648_truly_cm8352_s_ctrl},
	{ }
};
extern void camera_af_software_powerdown(struct i2c_client *client);
int32_t ov5648_truly_cm8352_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	CDBG("%s IN\r\n", __func__);
	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;

	/* PIP settings */
	ov5648_i2c_address = s_ctrl->sensor_i2c_addr;

	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		CDBG_HIGH("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	/* send software powerdown cmd to AF motor, avoid current leak */
	if(0 == rc)
	{
		camera_af_software_powerdown(client);
	}
	usleep_range(5000, 5100);

	return rc;
}

static struct i2c_driver ov5648_truly_cm8352_i2c_driver = {
	.id_table = ov5648_truly_cm8352_i2c_id,
	.probe  = ov5648_truly_cm8352_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov5648_truly_cm8352_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&ov5648_truly_cm8352_i2c_driver);
}

static struct v4l2_subdev_core_ops ov5648_truly_cm8352_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov5648_truly_cm8352_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov5648_truly_cm8352_subdev_ops = {
	.core = &ov5648_truly_cm8352_subdev_core_ops,
	.video  = &ov5648_truly_cm8352_subdev_video_ops,
};

int32_t ov5648_truly_cm8352_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;
	unsigned short rdata;
	int rc;

	CDBG("%s IN\r\n", __func__);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301a, 0xf1, MSM_CAMERA_I2C_BYTE_DATA);
	msleep(40);
	rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x3018,
			&rdata, MSM_CAMERA_I2C_BYTE_DATA);
	CDBG("ov5648_truly_cm8352_sensor_power_down: %d\n", rc);
	rdata |= 0x18;
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x3018, rdata,
		MSM_CAMERA_I2C_BYTE_DATA);
	msleep(40);
	gpio_direction_output(info->sensor_pwd, 0);
	/* PIP powerdown */
	if(CAM_MODE_PIP == ov5648_working_mode)
	{
		pip_ov7695_ctrl(PIP_CRL_POWERDOWN, NULL);
	}
	/* PIP end */

	usleep_range(5000, 5100);
	gpio_direction_output(info->sensor_reset, 0);
	usleep_range(5000, 5100);
	msm_sensor_power_down(s_ctrl);
	msleep(40);
	//if (s_ctrl->sensordata->pmic_gpio_enable){
	//	lcd_camera_power_onoff(0);
	//}
	return 0;
}

int32_t ov5648_truly_cm8352_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;
	CDBG("%s IN\r\n", __func__);

	CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__, info->sensor_pwd, info->sensor_reset);
	/* for PIP purouse */
	ov5648_pwdn_pin = info->sensor_pwd;
	ov5648_reset_pin = info->sensor_reset;
	/* end PIP */
	gpio_direction_output(info->sensor_pwd, 0);
	gpio_direction_output(info->sensor_reset, 0);
	usleep_range(5000, 6000);
#if 0
	for(rc = 0; rc<1000; rc++)
	{
		CDBG("GPIO cycle...\n");
		gpio_direction_output(info->sensor_pwd, 1);
		gpio_direction_output(info->sensor_reset, 0);
		usleep_range(5000, 6000);
		gpio_direction_output(info->sensor_pwd, 0);
		gpio_direction_output(info->sensor_reset, 1);
		usleep_range(5000, 6000);
	}
#endif
	//if (info->pmic_gpio_enable) {
	//	lcd_camera_power_onoff(1);
	//}
	usleep_range(5000, 6000);
	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		CDBG("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}

	/* turn on ldo and vreg */
	usleep_range(1000, 1100);
	gpio_direction_output(info->sensor_pwd, 1);
	msleep(10);
	gpio_direction_output(info->sensor_reset, 1);
	msleep(25);

	return rc;

}

static int32_t vfe_clk = 266667000;
int32_t ov5648_truly_cm8352_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;
	struct msm_camera_pip_ctl pip_ctl;
	/* pip lock */
	CDBG("%s,E\n",__func__);
	CDBG("%s,ov5648_mode: %d\n",__func__,ov5648_working_mode);
	/* Normal process */
	if(CAM_MODE_NORMAL == ov5648_working_mode)
	{
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

		if (update_type == MSM_SENSOR_REG_INIT) {
			CDBG("Normal Register INIT\n");
			s_ctrl->curr_csi_params = NULL;
			msm_camera_i2c_write(
					s_ctrl->sensor_i2c_client,
					0x103, 0x1,
					MSM_CAMERA_I2C_BYTE_DATA);
			msm_sensor_enable_debugfs(s_ctrl);
			msm_sensor_write_init_settings(s_ctrl);
#ifdef OV5648_TRULY_CM8352_OTP_FEATURE
			CDBG("Update OTP\n");
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x100, 0x1,
					MSM_CAMERA_I2C_BYTE_DATA);
			ov5648_truly_cm8352_update_otp(s_ctrl);
			usleep_range(5000, 6000);
			msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x100, 0x0,
			  MSM_CAMERA_I2C_BYTE_DATA);
#endif
			csi_config = 0;
		} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
			CDBG("PERIODIC : %d\n", res);
			msm_sensor_write_conf_array(
				s_ctrl->sensor_i2c_client,
				s_ctrl->msm_sensor_reg->mode_settings, res);
			msleep(30);
			if (!csi_config) {
				s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
				CDBG("CSI config in progress\n");
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_CSIC_CFG,
					s_ctrl->curr_csic_params);
				CDBG("CSI config is done\n");
				mb();
				msleep(30);
				csi_config = 1;
        msm_camera_i2c_write(
          s_ctrl->sensor_i2c_client,
          0x100, 0x1,
          MSM_CAMERA_I2C_BYTE_DATA);
			}

			if(res == MSM_SENSOR_RES_QTR){
				esposure_delay_en = 1;
			}

			if (res == MSM_SENSOR_RES_4)
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_PCLK_CHANGE,
						&vfe_clk);
			s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
			msleep(50);
		}
	}
	/* PIP  process below*/
	/***********************************************************/
	else if(CAM_MODE_PIP == ov5648_working_mode)
	{
		s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);

		if (update_type == MSM_SENSOR_REG_INIT) {
			CDBG("PIP Register INIT\n");
			s_ctrl->curr_csi_params = NULL;
			pip_ctl.sensor_i2c_client = s_ctrl->sensor_i2c_client;
			/*OV7695 powerup and software reset*/
			pip_ov7695_ctrl(PIP_CRL_POWERUP, &pip_ctl);
			usleep_range(2000, 3000);
			pip_ov7695_ctrl(PIP_CTL_RESET_SW, &pip_ctl);
			usleep_range(2000, 3000);
			/*OV7695 init settings*/
			pip_ctl.write_ctl = PIP_REG_SETTINGS_INIT;
			pip_ov7695_ctrl(PIP_CRL_WRITE_SETTINGS, &pip_ctl);
			msm_camera_i2c_write(
					s_ctrl->sensor_i2c_client,
					0x103, 0x1,
					MSM_CAMERA_I2C_BYTE_DATA);
			msleep(30);
			msm_sensor_enable_debugfs(s_ctrl);
			msm_sensor_write_init_settings(s_ctrl);
			csi_config = 0;
		} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
			CDBG("PIP PERIODIC : %d\n", res);
			pip_ctl.sensor_i2c_client = s_ctrl->sensor_i2c_client;
			pip_ctl.write_ctl = res;
			pip_ov7695_ctrl(PIP_CRL_WRITE_SETTINGS, &pip_ctl);
			msleep(60);
			msm_sensor_write_conf_array(
				s_ctrl->sensor_i2c_client,
				s_ctrl->msm_sensor_reg->mode_settings, res);
			msleep(30);
			if (!csi_config) {
				s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
				CDBG("CSI config in progress\n");
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
					NOTIFY_CSIC_CFG,
					s_ctrl->curr_csic_params);
				CDBG("CSI config is done\n");
				mb();
				msleep(30);
				csi_config = 1;
  			msm_camera_i2c_write(
  				s_ctrl->sensor_i2c_client,
  				0x100, 0x1,
  				MSM_CAMERA_I2C_BYTE_DATA);
			}

			if(res == MSM_SENSOR_RES_QTR){
				esposure_delay_en = 1;
			}

			if (res == MSM_SENSOR_RES_4)
				v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
						NOTIFY_PCLK_CHANGE,
						&vfe_clk);
			s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
			msleep(50);
		}
	}
	return rc;
}


int32_t ov5648_turly_cm8352_pip_get_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int32_t * mode);

int32_t ov5648_turly_cm8352_pip_set_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int32_t pip_mode);

int32_t ov5648_turly_cm8352_sensor_get_output_info(struct msm_sensor_ctrl_t *s_ctrl,
		struct sensor_output_info_t *sensor_output_info);

static struct msm_sensor_fn_t ov5648_truly_cm8352_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = ov5648_truly_cm8352_write_prev_exp_gain,
	.sensor_write_snapshot_exp_gain = ov5648_truly_cm8352_write_pict_exp_gain,
	.sensor_csi_setting = ov5648_truly_cm8352_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = ov5648_turly_cm8352_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov5648_truly_cm8352_sensor_power_up,
	.sensor_power_down = ov5648_truly_cm8352_sensor_power_down,
	.sensor_pip_get_mode = ov5648_turly_cm8352_pip_get_mode,
	.sensor_pip_set_mode = ov5648_turly_cm8352_pip_set_mode,
};

static struct msm_sensor_reg_t ov5648_truly_cm8352_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov5648_truly_cm8352_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov5648_truly_cm8352_start_settings),
	.stop_stream_conf = ov5648_truly_cm8352_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov5648_truly_cm8352_stop_settings),
	.group_hold_on_conf = ov5648_truly_cm8352_groupon_settings,
	.group_hold_on_conf_size = ARRAY_SIZE(ov5648_truly_cm8352_groupon_settings),
	.group_hold_off_conf = ov5648_truly_cm8352_groupoff_settings,
	.group_hold_off_conf_size =
		ARRAY_SIZE(ov5648_truly_cm8352_groupoff_settings),
	.init_settings = NULL,//&ov5648_truly_cm8352_init_conf[0],
	.init_size = 0,//ARRAY_SIZE(ov5648_truly_cm8352_init_conf),
	.mode_settings = NULL,//&ov5648_truly_cm8352_confs[0],
	.output_settings = NULL,//&ov5648_truly_cm8352_dimensions[0],
	.num_conf = 0,//ARRAY_SIZE(ov5648_truly_cm8352_confs),
};

static struct msm_sensor_ctrl_t ov5648_truly_cm8352_s_ctrl = {
	.msm_sensor_reg = &ov5648_truly_cm8352_regs,
	.sensor_i2c_client = &ov5648_truly_cm8352_sensor_i2c_client,
	.sensor_i2c_addr =  0x36 << 1 ,
	.sensor_output_reg_addr = &ov5648_truly_cm8352_reg_addr,
	.sensor_id_info = &ov5648_truly_cm8352_id_info,
	.sensor_exp_gain_info = &ov5648_truly_cm8352_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &ov5648_truly_cm8352_csi_params_array[0],
	.msm_sensor_mutex = &ov5648_truly_cm8352_mut,
	.sensor_i2c_driver = &ov5648_truly_cm8352_i2c_driver,
	.sensor_v4l2_subdev_info = ov5648_truly_cm8352_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov5648_truly_cm8352_subdev_info),
	.sensor_v4l2_subdev_ops = &ov5648_truly_cm8352_subdev_ops,
	.func_tbl = &ov5648_truly_cm8352_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

/* PIP switch */
int32_t ov5648_turly_cm8352_sensor_get_output_info(struct msm_sensor_ctrl_t *s_ctrl,
		struct sensor_output_info_t *sensor_output_info)
{
	/* Normal process */
	ov5648_working_mode = ov5648_new_mode;
	if(CAM_MODE_NORMAL == ov5648_working_mode)
	{
		ov5648_truly_cm8352_regs.init_settings = &ov5648_truly_cm8352_init_conf[0];
		ov5648_truly_cm8352_regs.init_size = ARRAY_SIZE(ov5648_truly_cm8352_init_conf);
		ov5648_truly_cm8352_regs.mode_settings = &ov5648_truly_cm8352_confs[0];
		ov5648_truly_cm8352_regs.output_settings = &ov5648_truly_cm8352_dimensions[0];
		ov5648_truly_cm8352_regs.num_conf = ARRAY_SIZE(ov5648_truly_cm8352_confs);
	}
	else if(CAM_MODE_PIP == ov5648_working_mode)
	{
		ov5648_truly_cm8352_regs.init_settings = &ov5648_truly_cm8352_pip_init_conf_master[0];
		ov5648_truly_cm8352_regs.init_size = ARRAY_SIZE(ov5648_truly_cm8352_pip_init_conf_master);
		ov5648_truly_cm8352_regs.mode_settings = &ov5648_truly_cm8352_pip_confs_master[0];
		ov5648_truly_cm8352_regs.output_settings = &ov5648_truly_cm8352_pip_dimensions[0];
		ov5648_truly_cm8352_regs.num_conf = ARRAY_SIZE(ov5648_truly_cm8352_pip_confs_master);
	}
	return msm_sensor_get_output_info(s_ctrl, sensor_output_info);
}

int32_t ov5648_turly_cm8352_pip_set_mode(struct msm_sensor_ctrl_t *s_ctrl,
		int32_t pip_mode)
{
	CDBG("%s, E\n, new pip_mode: %d \n",__func__,pip_mode);
	ov5648_new_mode = pip_mode;
	CDBG("%s, X\n",__func__);
	return 0;
}

int32_t ov5648_turly_cm8352_pip_get_mode(struct msm_sensor_ctrl_t *s_ctrl,
	int32_t * mode)
{
	CDBG("%s IN, mode is %d\r\n", __func__, ov5648_working_mode);
	*mode = ov5648_working_mode;
	CDBG("%s, X\n",__func__);
	return 0;
}

/* end PIP */
module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision WXGA Bayer sensor driver");
MODULE_LICENSE("GPL v2");
