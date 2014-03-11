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

#include "hi351_sunny_q3h01b_v4l2.h"
#include "msm_sensor.h"
#include "msm.h"
#include <linux/gpio.h>
#include <mach/vreg.h>
#define SENSOR_NAME "hi351"


#define ODMM_GPIO_PWDN          5 
#define ODMM_GPIO_RESET         6     
#define ODMM_GPIO_VREG_CAM_ANA  35
#define ODMM_GPIO_CAM_IO	109

#define ODMM_OPTIMIZATION_DEBUG 0 

#ifdef ODMM_PROJECT_STAGE_EVB1
#define ODMM_GPIO_VREG_VCM      40
#define ODMM_GPIO_FLASH_PIN      13
#define ODMM_GPIO_STROB_PIN      32
#endif

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define HI351_VERBOSE_DGB

#ifdef HI351_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

DEFINE_MUTEX(hi351_mut);
static struct msm_sensor_ctrl_t hi351_s_ctrl;

static int effect_value = CAMERA_EFFECT_OFF;
static int iso_value = MSM_V4L2_ISO_AUTO;
static int mode = MSM_SENSOR_RES_QTR;


static struct v4l2_subdev_info hi351_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};

static struct msm_camera_i2c_conf_array hi351_init_conf[] = {
	{&hi351_recommend_settings0[0],
	ARRAY_SIZE(hi351_recommend_settings0), 50, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_recommend_settings1[0],
	ARRAY_SIZE(hi351_recommend_settings1), 50, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_recommend_settings2[0],
	ARRAY_SIZE(hi351_recommend_settings2), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_recommend_settings3[0],
	ARRAY_SIZE(hi351_recommend_settings3), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_recommend_settings4[0],
	ARRAY_SIZE(hi351_recommend_settings4), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_recommend_settings5[0],
	ARRAY_SIZE(hi351_recommend_settings5), 200, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array hi351_preview_conf[] = {
	{&hi351_prev_settings0[0],
	ARRAY_SIZE(hi351_prev_settings0), 100, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_prev_settings1[0],
	ARRAY_SIZE(hi351_prev_settings1), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_prev_settings2[0],
	ARRAY_SIZE(hi351_prev_settings2), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_prev_settings3[0],
	ARRAY_SIZE(hi351_prev_settings3), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_prev_settings4[0],
	ARRAY_SIZE(hi351_prev_settings4), 100, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array hi351_snapshot_conf[] = {
	{&hi351_snap_settings0[0],
	ARRAY_SIZE(hi351_snap_settings0), 100, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_snap_settings1[0],
	ARRAY_SIZE(hi351_snap_settings1), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_snap_settings2[0],
	ARRAY_SIZE(hi351_snap_settings2), 100, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array hi351_full_conf[] = {
	{&hi351_full_settings0[0],
	ARRAY_SIZE(hi351_full_settings0), 100, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_full_settings1[0],
	ARRAY_SIZE(hi351_full_settings1), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_full_settings2[0],
	ARRAY_SIZE(hi351_full_settings2), 100, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_full_settings3[0],
	ARRAY_SIZE(hi351_full_settings3), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_full_settings4[0],
	ARRAY_SIZE(hi351_full_settings4), 10, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_full_settings5[0],
	ARRAY_SIZE(hi351_full_settings5), 100, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array hi351_dummy_confs[] = {
	{&hi351_dummy_settings[0],
	ARRAY_SIZE(hi351_dummy_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_dummy_settings[0],
	ARRAY_SIZE(hi351_dummy_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_dummy_settings[0],
	ARRAY_SIZE(hi351_dummy_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t hi351_dimensions[] = {
	{ /* For SNAPSHOT */
		.x_output = 0x800,  /*2048*/  /*for 3Mp*/
		.y_output = 0x600,  /*1536*/
		.line_length_pclk = 0x940,
		.frame_length_lines = 0x621,
		.vt_pixel_clk = 54000000,
		.op_pixel_clk = 54000000,
		.binning_factor = 0x0,
	},
	{ /* For PREVIEW */
		.x_output = 0x0280,  /*640*/  /*for 3Mp*/
		.y_output = 0x01E0,  /*480*/
		.line_length_pclk = 0x3C0,
		.frame_length_lines = 0x201,
		.vt_pixel_clk = 54000000,
		.op_pixel_clk = 54000000,
		.binning_factor = 0x0,
	},
	{
		.x_output = 0x800,  /*2048*/  /*for 3Mp*/
		.y_output = 0x600,  /*1536*/
		.line_length_pclk = 0x940,
		.frame_length_lines = 0x621,
		.vt_pixel_clk = 54000000,
		.op_pixel_clk = 54000000,
		.binning_factor = 0x0,
	},
};

static struct msm_camera_i2c_reg_conf hi351_saturation[][25] = {
	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x60}, /*SATB_00*/
	{0x2c, 0x60}, /*SATB_01*/
	{0x2d, 0x60}, /*SATB_02*/
	{0x2e, 0x60}, /*SATB_03*/
	{0x2f, 0x60}, /*SATB_04*/
	{0x30, 0x94}, /*SATB_05*/
	{0x31, 0x70}, /*SATB_06*/
	{0x32, 0x70}, /*SATB_07*/
	{0x33, 0x70}, /*SATB_08*/
	{0x34, 0x70}, /*SATB_09*/
	{0x35, 0x70}, /*SATB_10*/
	{0x36, 0x70}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x58}, /*SATR_00*/
	{0x38, 0x58}, /*SATR_01*/
	{0x39, 0x58}, /*SATR_02*/
	{0x3a, 0x50}, /*SATR_03*/
	{0x3b, 0x60}, /*SATR_04*/
	{0x3c, 0x8c}, /*SATR_05*/
	{0x3d, 0x64}, /*SATR_06*/
	{0x3e, 0x64}, /*SATR_07*/
	{0x3f, 0x64}, /*SATR_08*/
	{0x40, 0x64}, /*SATR_09*/
	{0x41, 0x64}, /*SATR_10*/
	{0x42, 0x64}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL0*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x68}, /*SATB_00*/
	{0x2c, 0x68}, /*SATB_01*/
	{0x2d, 0x68}, /*SATB_02*/
	{0x2e, 0x68}, /*SATB_03*/
	{0x2f, 0x68}, /*SATB_04*/
	{0x30, 0x9c}, /*SATB_05*/
	{0x31, 0x78}, /*SATB_06*/
	{0x32, 0x78}, /*SATB_07*/
	{0x33, 0x78}, /*SATB_08*/
	{0x34, 0x78}, /*SATB_09*/
	{0x35, 0x78}, /*SATB_10*/
	{0x36, 0x78}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x60}, /*SATR_00*/
	{0x38, 0x60}, /*SATR_01*/
	{0x39, 0x60}, /*SATR_02*/
	{0x3a, 0x58}, /*SATR_03*/
	{0x3b, 0x68}, /*SATR_04*/
	{0x3c, 0x94}, /*SATR_05*/
	{0x3d, 0x6c}, /*SATR_06*/
	{0x3e, 0x6c}, /*SATR_07*/
	{0x3f, 0x6c}, /*SATR_08*/
	{0x40, 0x6c}, /*SATR_09*/
	{0x41, 0x6c}, /*SATR_10*/
	{0x42, 0x6c}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL1*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x70}, /*SATB_00*/
	{0x2c, 0x70}, /*SATB_01*/
	{0x2d, 0x70}, /*SATB_02*/
	{0x2e, 0x70}, /*SATB_03*/
	{0x2f, 0x70}, /*SATB_04*/
	{0x30, 0xa4}, /*SATB_05*/
	{0x31, 0x80}, /*SATB_06*/
	{0x32, 0x80}, /*SATB_07*/
	{0x33, 0x80}, /*SATB_08*/
	{0x34, 0x80}, /*SATB_09*/
	{0x35, 0x80}, /*SATB_10*/
	{0x36, 0x80}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x68}, /*SATR_00*/
	{0x38, 0x68}, /*SATR_01*/
	{0x39, 0x68}, /*SATR_02*/
	{0x3a, 0x60}, /*SATR_03*/
	{0x3b, 0x70}, /*SATR_04*/
	{0x3c, 0x9c}, /*SATR_05*/
	{0x3d, 0x74}, /*SATR_06*/
	{0x3e, 0x74}, /*SATR_07*/
	{0x3f, 0x74}, /*SATR_08*/
	{0x40, 0x74}, /*SATR_09*/
	{0x41, 0x74}, /*SATR_10*/
	{0x42, 0x74}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL2*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x78}, /*SATB_00*/
	{0x2c, 0x78}, /*SATB_01*/
	{0x2d, 0x78}, /*SATB_02*/
	{0x2e, 0x78}, /*SATB_03*/
	{0x2f, 0x78}, /*SATB_04*/
	{0x30, 0xac}, /*SATB_05*/
	{0x31, 0x88}, /*SATB_06*/
	{0x32, 0x88}, /*SATB_07*/
	{0x33, 0x88}, /*SATB_08*/
	{0x34, 0x88}, /*SATB_09*/
	{0x35, 0x88}, /*SATB_10*/
	{0x36, 0x88}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x70}, /*SATR_00*/
	{0x38, 0x70}, /*SATR_01*/
	{0x39, 0x70}, /*SATR_02*/
	{0x3a, 0x68}, /*SATR_03*/
	{0x3b, 0x78}, /*SATR_04*/
	{0x3c, 0xa4}, /*SATR_05*/
	{0x3d, 0x7c}, /*SATR_06*/
	{0x3e, 0x7c}, /*SATR_07*/
	{0x3f, 0x7c}, /*SATR_08*/
	{0x40, 0x7c}, /*SATR_09*/
	{0x41, 0x7c}, /*SATR_10*/
	{0x42, 0x7c}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL3*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x80}, /*SATB_00*/
	{0x2c, 0x80}, /*SATB_01*/
	{0x2d, 0x80}, /*SATB_02*/
	{0x2e, 0x80}, /*SATB_03*/
	{0x2f, 0x80}, /*SATB_04*/
	{0x30, 0xb4}, /*SATB_05*/
	{0x31, 0x90}, /*SATB_06*/
	{0x32, 0x90}, /*SATB_07*/
	{0x33, 0x90}, /*SATB_08*/
	{0x34, 0x90}, /*SATB_09*/
	{0x35, 0x90}, /*SATB_10*/
	{0x36, 0x90}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x78}, /*SATR_00*/
	{0x38, 0x78}, /*SATR_01*/
	{0x39, 0x78}, /*SATR_02*/
	{0x3a, 0x70}, /*SATR_03*/
	{0x3b, 0x80}, /*SATR_04*/
	{0x3c, 0xac}, /*SATR_05*/
	{0x3d, 0x84}, /*SATR_06*/
	{0x3e, 0x84}, /*SATR_07*/
	{0x3f, 0x84}, /*SATR_08*/
	{0x40, 0x84}, /*SATR_09*/
	{0x41, 0x84}, /*SATR_10*/
	{0x42, 0x84}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL4*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x88}, /*SATB_00*/
	{0x2c, 0x88}, /*SATB_01*/
	{0x2d, 0x88}, /*SATB_02*/
	{0x2e, 0x88}, /*SATB_03*/
	{0x2f, 0x88}, /*SATB_04*/
	{0x30, 0xbc}, /*SATB_05*/
	{0x31, 0x98}, /*SATB_06*/
	{0x32, 0x98}, /*SATB_07*/
	{0x33, 0x98}, /*SATB_08*/
	{0x34, 0x98}, /*SATB_09*/
	{0x35, 0x98}, /*SATB_10*/
	{0x36, 0x98}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x80}, /*SATR_00*/
	{0x38, 0x80}, /*SATR_01*/
	{0x39, 0x80}, /*SATR_02*/
	{0x3a, 0x78}, /*SATR_03*/
	{0x3b, 0x88}, /*SATR_04*/
	{0x3c, 0xb4}, /*SATR_05*/
	{0x3d, 0x8c}, /*SATR_06*/
	{0x3e, 0x8c}, /*SATR_07*/
	{0x3f, 0x8c}, /*SATR_08*/
	{0x40, 0x8c}, /*SATR_09*/
	{0x41, 0x8c}, /*SATR_10*/
	{0x42, 0x8c}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL5*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x90}, /*SATB_00*/
	{0x2c, 0x90}, /*SATB_01*/
	{0x2d, 0x90}, /*SATB_02*/
	{0x2e, 0x90}, /*SATB_03*/
	{0x2f, 0x90}, /*SATB_04*/
	{0x30, 0xc4}, /*SATB_05*/
	{0x31, 0xa0}, /*SATB_06*/
	{0x32, 0xa0}, /*SATB_07*/
	{0x33, 0xa0}, /*SATB_08*/
	{0x34, 0xa0}, /*SATB_09*/
	{0x35, 0xa0}, /*SATB_10*/
	{0x36, 0xa0}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x88}, /*SATR_00*/
	{0x38, 0x88}, /*SATR_01*/
	{0x39, 0x88}, /*SATR_02*/
	{0x3a, 0x80}, /*SATR_03*/
	{0x3b, 0x90}, /*SATR_04*/
	{0x3c, 0xbc}, /*SATR_05*/
	{0x3d, 0x94}, /*SATR_06*/
	{0x3e, 0x94}, /*SATR_07*/
	{0x3f, 0x94}, /*SATR_08*/
	{0x40, 0x94}, /*SATR_09*/
	{0x41, 0x94}, /*SATR_10*/
	{0x42, 0x94}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL6*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0x98}, /*SATB_00*/
	{0x2c, 0x98}, /*SATB_01*/
	{0x2d, 0x98}, /*SATB_02*/
	{0x2e, 0x98}, /*SATB_03*/
	{0x2f, 0x98}, /*SATB_04*/
	{0x30, 0xcc}, /*SATB_05*/
	{0x31, 0xa8}, /*SATB_06*/
	{0x32, 0xa8}, /*SATB_07*/
	{0x33, 0xa8}, /*SATB_08*/
	{0x34, 0xa8}, /*SATB_09*/
	{0x35, 0xa8}, /*SATB_10*/
	{0x36, 0xa8}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x90}, /*SATR_00*/
	{0x38, 0x90}, /*SATR_01*/
	{0x39, 0x90}, /*SATR_02*/
	{0x3a, 0x88}, /*SATR_03*/
	{0x3b, 0x98}, /*SATR_04*/
	{0x3c, 0xc4}, /*SATR_05*/
	{0x3d, 0x9c}, /*SATR_06*/
	{0x3e, 0x9c}, /*SATR_07*/
	{0x3f, 0x9c}, /*SATR_08*/
	{0x40, 0x9c}, /*SATR_09*/
	{0x41, 0x9c}, /*SATR_10*/
	{0x42, 0x9c}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL7*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0xa0}, /*SATB_00*/
	{0x2c, 0xa0}, /*SATB_01*/
	{0x2d, 0xa0}, /*SATB_02*/
	{0x2e, 0xa0}, /*SATB_03*/
	{0x2f, 0xa0}, /*SATB_04*/
	{0x30, 0xd4}, /*SATB_05*/
	{0x31, 0xb0}, /*SATB_06*/
	{0x32, 0xb0}, /*SATB_07*/
	{0x33, 0xb0}, /*SATB_08*/
	{0x34, 0xb0}, /*SATB_09*/
	{0x35, 0xb0}, /*SATB_10*/
	{0x36, 0xb0}, /*SATB_11*/
	/*SATR*/
	{0x37, 0x98}, /*SATR_00*/
	{0x38, 0x98}, /*SATR_01*/
	{0x39, 0x98}, /*SATR_02*/
	{0x3a, 0x90}, /*SATR_03*/
	{0x3b, 0x90}, /*SATR_04*/
	{0x3c, 0xcc}, /*SATR_05*/
	{0x3d, 0xa4}, /*SATR_06*/
	{0x3e, 0xa4}, /*SATR_07*/
	{0x3f, 0xa4}, /*SATR_08*/
	{0x40, 0xa4}, /*SATR_09*/
	{0x41, 0xa4}, /*SATR_10*/
	{0x42, 0xa4}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL8*/

	{{0x03, 0xd1}, /*page D1(Adaptive)*/
	/*SATB*/
	{0x2b, 0xa8}, /*SATB_00*/
	{0x2c, 0xa8}, /*SATB_01*/
	{0x2d, 0xa8}, /*SATB_02*/
	{0x2e, 0xa8}, /*SATB_03*/
	{0x2f, 0xa8}, /*SATB_04*/
	{0x30, 0xdc}, /*SATB_05*/
	{0x31, 0xb8}, /*SATB_06*/
	{0x32, 0xb8}, /*SATB_07*/
	{0x33, 0xb8}, /*SATB_08*/
	{0x34, 0xb8}, /*SATB_09*/
	{0x35, 0xb8}, /*SATB_10*/
	{0x36, 0xb8}, /*SATB_11*/
	/*SATR*/
	{0x37, 0xa0}, /*SATR_00*/
	{0x38, 0xa0}, /*SATR_01*/
	{0x39, 0xa0}, /*SATR_02*/
	{0x3a, 0x98}, /*SATR_03*/
	{0x3b, 0x98}, /*SATR_04*/
	{0x3c, 0xd4}, /*SATR_05*/
	{0x3d, 0xac}, /*SATR_06*/
	{0x3e, 0xac}, /*SATR_07*/
	{0x3f, 0xac}, /*SATR_08*/
	{0x40, 0xac}, /*SATR_09*/
	{0x41, 0xac}, /*SATR_10*/
	{0x42, 0xac}, /*SATR_11*/
	//{0xff, 0x00},
	}, /* SATURATION LEVEL9*/
};

static struct msm_camera_i2c_conf_array hi351_saturation_confs[][1] = {
	{{hi351_saturation[0], ARRAY_SIZE(hi351_saturation[0]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[1], ARRAY_SIZE(hi351_saturation[1]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[2], ARRAY_SIZE(hi351_saturation[2]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[3], ARRAY_SIZE(hi351_saturation[3]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[4], ARRAY_SIZE(hi351_saturation[4]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[5], ARRAY_SIZE(hi351_saturation[5]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[6], ARRAY_SIZE(hi351_saturation[6]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[7], ARRAY_SIZE(hi351_saturation[7]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[8], ARRAY_SIZE(hi351_saturation[8]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_saturation[9], ARRAY_SIZE(hi351_saturation[9]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_saturation_enum_map[] = {
	MSM_V4L2_SATURATION_L0,
	MSM_V4L2_SATURATION_L1,
	MSM_V4L2_SATURATION_L2,
	MSM_V4L2_SATURATION_L3,
	MSM_V4L2_SATURATION_L4,
	MSM_V4L2_SATURATION_L5,
	MSM_V4L2_SATURATION_L6,
	MSM_V4L2_SATURATION_L7,
	MSM_V4L2_SATURATION_L8,
	MSM_V4L2_SATURATION_L9,
};

static struct msm_camera_i2c_enum_conf_array hi351_saturation_enum_confs = {
	.conf = &hi351_saturation_confs[0][0],
	.conf_enum = hi351_saturation_enum_map,
	.num_enum = ARRAY_SIZE(hi351_saturation_enum_map),
	.num_index = ARRAY_SIZE(hi351_saturation_confs),
	.num_conf = ARRAY_SIZE(hi351_saturation_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf hi351_contrast[][2] = {
	{{0x03, 0xd9},{0x19, 0x58},/*{0xff, 0x00}*/}, /* CONTRAST L0*/
	{{0x03, 0xd9},{0x19, 0x60},/*{0xff, 0x00}*/}, /* CONTRAST L1*/
	{{0x03, 0xd9},{0x19, 0x68},/*{0xff, 0x00}*/}, /* CONTRAST L2*/
	{{0x03, 0xd9},{0x19, 0x70},/*{0xff, 0x00}*/}, /* CONTRAST L3*/
	{{0x03, 0xd9},{0x19, 0x78},/*{0xff, 0x00}*/}, /* CONTRAST L4*/
	{{0x03, 0xd9},{0x19, 0x80},/*{0xff, 0x00}*/}, /* CONTRAST L5*/
	{{0x03, 0xd9},{0x19, 0x88},/*{0xff, 0x00}*/}, /* CONTRAST L6*/
	{{0x03, 0xd9},{0x19, 0x90},/*{0xff, 0x00}*/}, /* CONTRAST L7*/
	{{0x03, 0xd9},{0x19, 0x98},/*{0xff, 0x00}*/}, /* CONTRAST L8*/
	{{0x03, 0xd9},{0x19, 0xa0},/*{0xff, 0x00}*/}, /* CONTRAST L9*/
};

static struct msm_camera_i2c_conf_array hi351_contrast_confs[][1] = {
	{{hi351_contrast[0], ARRAY_SIZE(hi351_contrast[0]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[1], ARRAY_SIZE(hi351_contrast[1]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[2], ARRAY_SIZE(hi351_contrast[2]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[3], ARRAY_SIZE(hi351_contrast[3]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[4], ARRAY_SIZE(hi351_contrast[4]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[5], ARRAY_SIZE(hi351_contrast[5]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[6], ARRAY_SIZE(hi351_contrast[6]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[7], ARRAY_SIZE(hi351_contrast[7]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[8], ARRAY_SIZE(hi351_contrast[8]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_contrast[9], ARRAY_SIZE(hi351_contrast[9]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_contrast_enum_map[] = {
	MSM_V4L2_CONTRAST_L0,
	MSM_V4L2_CONTRAST_L1,
	MSM_V4L2_CONTRAST_L2,
	MSM_V4L2_CONTRAST_L3,
	MSM_V4L2_CONTRAST_L4,
	MSM_V4L2_CONTRAST_L5,
	MSM_V4L2_CONTRAST_L6,
	MSM_V4L2_CONTRAST_L7,
	MSM_V4L2_CONTRAST_L8,
	MSM_V4L2_CONTRAST_L9,
};

static struct msm_camera_i2c_enum_conf_array hi351_contrast_enum_confs = {
	.conf = &hi351_contrast_confs[0][0],
	.conf_enum = hi351_contrast_enum_map,
	.num_enum = ARRAY_SIZE(hi351_contrast_enum_map),
	.num_index = ARRAY_SIZE(hi351_contrast_confs),
	.num_conf = ARRAY_SIZE(hi351_contrast_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf hi351_sharpness[][32] = {
	{{0x03, 0xdb},
	{0x37, 0x08}, //Out
	{0xbb, 0x10}, //Pos_H
	{0xbd, 0x10}, //Pos_M
	{0xbf, 0x10}, //Pos_L
	{0xc1, 0x34}, //Neg_H
	{0xc3, 0x34}, //Neg_M
	{0xc5, 0x20}, //Neg_L
	{0x03, 0xde},
	{0x37, 0x04}, //Indoor
	{0xbb, 0x10}, //Pos_H
	{0xbd, 0x10}, //Pos_M
	{0xbf, 0x10}, //Pos_L
	{0xc1, 0x38}, //Neg_H
	{0xc3, 0x40}, //Neg_M
	{0xc5, 0x40}, //Neg_L
	{0x03, 0xe1},
	{0x37, 0x04}, //Dark1
	{0xbb, 0x10}, //Pos_H
	{0xbd, 0x10}, //Pos_M
	{0xbf, 0x10}, //Pos_L
	{0xc1, 0x18}, //Neg_H
	{0xc3, 0x20}, //Neg_M
	{0xc5, 0x20}, //Neg_L
	{0x03, 0xe4},
	{0x37, 0x00}, //Dark2
	{0xbb, 0x00}, //Pos_H
	{0xbd, 0x00}, //Pos_M
	{0xbf, 0x00}, //Pos_L
	{0xc1, 0x00}, //Neg_H
	{0xc3, 0x00}, //Neg_M
	{0xc5, 0x00},}, //Neg_L
	//{0xff, 0x00}}, /* SHARPNESS LEVEL 0*/

	{{0x03, 0xdb},
	{0x37, 0x0c}, //Out
	{0xbb, 0x20}, //Pos_H
	{0xbd, 0x20}, //Pos_M
	{0xbf, 0x20}, //Pos_L
	{0xc1, 0x44}, //Neg_H
	{0xc3, 0x44}, //Neg_M
	{0xc5, 0x30}, //Neg_L
	{0x03, 0xde},
	{0x37, 0x08}, //Indoor
	{0xbb, 0x20}, //Pos_H
	{0xbd, 0x20}, //Pos_M
	{0xbf, 0x20}, //Pos_L
	{0xc1, 0x48}, //Neg_H
	{0xc3, 0x50}, //Neg_M
	{0xc5, 0x50}, //Neg_L
	{0x03, 0xe1},
	{0x37, 0x08}, //Dark1
	{0xbb, 0x20}, //Pos_H
	{0xbd, 0x20}, //Pos_M
	{0xbf, 0x20}, //Pos_L
	{0xc1, 0x28}, //Neg_H
	{0xc3, 0x30}, //Neg_M
	{0xc5, 0x30}, //Neg_L
	{0x03, 0xe4},
	{0x37, 0x01}, //Dark2
	{0xbb, 0x00}, //Pos_H
	{0xbd, 0x00}, //Pos_M
	{0xbf, 0x00}, //Pos_L
	{0xc1, 0x08}, //Neg_H
	{0xc3, 0x08}, //Neg_M
	{0xc5, 0x08},}, //Neg_L
	//{0xff, 0x00}}, /* SHARPNESS LEVEL 1*/

	{{0x03, 0xdb},
	{0x37, 0x12}, //Out
	{0xbb, 0x30}, //Pos_H
	{0xbd, 0x30}, //Pos_M
	{0xbf, 0x30}, //Pos_L
	{0xc1, 0x54}, //Neg_H
	{0xc3, 0x54}, //Neg_M
	{0xc5, 0x40}, //Neg_L
	{0x03, 0xde},
	{0x37, 0x0c}, //Indoor
	{0xbb, 0x30}, //Pos_H
	{0xbd, 0x30}, //Pos_M
	{0xbf, 0x30}, //Pos_L
	{0xc1, 0x58}, //Neg_H
	{0xc3, 0x60}, //Neg_M
	{0xc5, 0x60}, //Neg_L
	{0x03, 0xe1},
	{0x37, 0x0c}, //Dark1
	{0xbb, 0x30}, //Pos_H
	{0xbd, 0x30}, //Pos_M
	{0xbf, 0x30}, //Pos_L
	{0xc1, 0x38}, //Neg_H
	{0xc3, 0x40}, //Neg_M
	{0xc5, 0x40}, //Neg_L
	{0x03, 0xe4},
	{0x37, 0x02}, //Dark2
	{0xbb, 0x00}, //Pos_H
	{0xbd, 0x00}, //Pos_M
	{0xbf, 0x00}, //Pos_L
	{0xc1, 0x10}, //Neg_H
	{0xc3, 0x10}, //Neg_M
	{0xc5, 0x10},}, //Neg_L
	//{0xff, 0x00}}, /* SHARPNESS LEVEL 2*/

	{{0x03, 0xdb},
	{0x37, 0x22}, //Out
	{0xbb, 0x40}, //Pos_H
	{0xbd, 0x40}, //Pos_M
	{0xbf, 0x40}, //Pos_L
	{0xc1, 0x64}, //Neg_H
	{0xc3, 0x64}, //Neg_M
	{0xc5, 0x50}, //Neg_L
	{0x03, 0xde},
	{0x37, 0x1c}, //Indoor
	{0xbb, 0x40}, //Pos_H
	{0xbd, 0x40}, //Pos_M
	{0xbf, 0x40}, //Pos_L
	{0xc1, 0x68}, //Neg_H
	{0xc3, 0x70}, //Neg_M
	{0xc5, 0x70}, //Neg_L
	{0x03, 0xe1},
	{0x37, 0x1c}, //Dark1
	{0xbb, 0x40}, //Pos_H
	{0xbd, 0x40}, //Pos_M
	{0xbf, 0x40}, //Pos_L
	{0xc1, 0x48}, //Neg_H
	{0xc3, 0x50}, //Neg_M
	{0xc5, 0x50}, //Neg_L
	{0x03, 0xe4},
	{0x37, 0x12}, //Dark2
	{0xbb, 0x10}, //Pos_H
	{0xbd, 0x10}, //Pos_M
	{0xbf, 0x10}, //Pos_L
	{0xc1, 0x20}, //Neg_H
	{0xc3, 0x20}, //Neg_M
	{0xc5, 0x20},}, //Neg_L
	//{0xff, 0x00}}, /* SHARPNESS LEVEL 3*/

	{{0x03, 0xdb},
	{0x37, 0x32}, //Out
	{0xbb, 0x50}, //Pos_H
	{0xbd, 0x50}, //Pos_M
	{0xbf, 0x50}, //Pos_L
	{0xc1, 0x74}, //Neg_H
	{0xc3, 0x74}, //Neg_M
	{0xc5, 0x60}, //Neg_L
	{0x03, 0xde},
	{0x37, 0x2c}, //Indoor
	{0xbb, 0x50}, //Pos_H
	{0xbd, 0x50}, //Pos_M
	{0xbf, 0x50}, //Pos_L
	{0xc1, 0x78}, //Neg_H
	{0xc3, 0x80}, //Neg_M
	{0xc5, 0x80}, //Neg_L
	{0x03, 0xe1},
	{0x37, 0x2c}, //Dark1
	{0xbb, 0x50}, //Pos_H
	{0xbd, 0x50}, //Pos_M
	{0xbf, 0x50}, //Pos_L
	{0xc1, 0x58}, //Neg_H
	{0xc3, 0x60}, //Neg_M
	{0xc5, 0x60}, //Neg_L
	{0x03, 0xe4},
	{0x37, 0x22}, //Dark2
	{0xbb, 0x20}, //Pos_H
	{0xbd, 0x20}, //Pos_M
	{0xbf, 0x20}, //Pos_L
	{0xc1, 0x30}, //Neg_H
	{0xc3, 0x30}, //Neg_M
	{0xc5, 0x30},}, //Neg_L
	//{0xff, 0x00}}, /* SHARPNESS LEVEL 4*/

	{{0x03, 0xdb},
	{0x37, 0x42}, //Out
	{0xbb, 0x60}, //Pos_H
	{0xbd, 0x60}, //Pos_M
	{0xbf, 0x60}, //Pos_L
	{0xc1, 0x84}, //Neg_H
	{0xc3, 0x84}, //Neg_M
	{0xc5, 0x70}, //Neg_L
	{0x03, 0xde},
	{0x37, 0x3c}, //Indoor
	{0xbb, 0x60}, //Pos_H
	{0xbd, 0x60}, //Pos_M
	{0xbf, 0x60}, //Pos_L
	{0xc1, 0x88}, //Neg_H
	{0xc3, 0x90}, //Neg_M
	{0xc5, 0x90}, //Neg_L
	{0x03, 0xe1},
	{0x37, 0x3c}, //Dark1
	{0xbb, 0x60}, //Pos_H
	{0xbd, 0x60}, //Pos_M
	{0xbf, 0x60}, //Pos_L
	{0xc1, 0x68}, //Neg_H
	{0xc3, 0x70}, //Neg_M
	{0xc5, 0x70}, //Neg_L
	{0x03, 0xe4},
	{0x37, 0x32}, //Dark2
	{0xbb, 0x30}, //Pos_H
	{0xbd, 0x30}, //Pos_M
	{0xbf, 0x30}, //Pos_L
	{0xc1, 0x40}, //Neg_H
	{0xc3, 0x40}, //Neg_M
	{0xc5, 0x40},}, //Neg_L
	//{0xff, 0x00}}, /* SHARPNESS LEVEL 5*/

	{{0x03, 0xdb},
	{0x37, 0x52}, //Out
	{0xbb, 0x70}, //Pos_H
	{0xbd, 0x70}, //Pos_M
	{0xbf, 0x70}, //Pos_L
	{0xc1, 0x94}, //Neg_H
	{0xc3, 0x94}, //Neg_M
	{0xc5, 0x80}, //Neg_L
	{0x03, 0xde},
	{0x37, 0x4c}, //Indoor
	{0xbb, 0x70}, //Pos_H
	{0xbd, 0x70}, //Pos_M
	{0xbf, 0x70}, //Pos_L
	{0xc1, 0x98}, //Neg_H
	{0xc3, 0xa0}, //Neg_M
	{0xc5, 0xa0}, //Neg_L
	{0x03, 0xe1},
	{0x37, 0x4c}, //Dark1
	{0xbb, 0x70}, //Pos_H
	{0xbd, 0x70}, //Pos_M
	{0xbf, 0x70}, //Pos_L
	{0xc1, 0x78}, //Neg_H
	{0xc3, 0x80}, //Neg_M
	{0xc5, 0x80}, //Neg_L
	{0x03, 0xe4},
	{0x37, 0x42}, //Dark2
	{0xbb, 0x40}, //Pos_H
	{0xbd, 0x40}, //Pos_M
	{0xbf, 0x40}, //Pos_L
	{0xc1, 0x50}, //Neg_H
	{0xc3, 0x50}, //Neg_M
	{0xc5, 0x50},}, //Neg_L
	//{0xff, 0x00}}, /* SHARPNESS LEVEL 6*/
};

static struct msm_camera_i2c_conf_array hi351_sharpness_confs[][1] = {
	{{hi351_sharpness[0], ARRAY_SIZE(hi351_sharpness[0]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_sharpness[1], ARRAY_SIZE(hi351_sharpness[1]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_sharpness[2], ARRAY_SIZE(hi351_sharpness[2]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_sharpness[3], ARRAY_SIZE(hi351_sharpness[3]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_sharpness[4], ARRAY_SIZE(hi351_sharpness[4]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_sharpness[5], ARRAY_SIZE(hi351_sharpness[5]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_sharpness[6], ARRAY_SIZE(hi351_sharpness[6]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_sharpness_enum_map[] = {
	MSM_V4L2_SHARPNESS_L0,
	MSM_V4L2_SHARPNESS_L1,
	MSM_V4L2_SHARPNESS_L2,
	MSM_V4L2_SHARPNESS_L3,
	MSM_V4L2_SHARPNESS_L4,
	MSM_V4L2_SHARPNESS_L5,
	MSM_V4L2_SHARPNESS_L6,
};

static struct msm_camera_i2c_enum_conf_array hi351_sharpness_enum_confs = {
	.conf = &hi351_sharpness_confs[0][0],
	.conf_enum = hi351_sharpness_enum_map,
	.num_enum = ARRAY_SIZE(hi351_sharpness_enum_map),
	.num_index = ARRAY_SIZE(hi351_sharpness_confs),
	.num_conf = ARRAY_SIZE(hi351_sharpness_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf hi351_exposure[][14] = {
	{{0x03, 0xcf},
	{0x3f, 0x18}, /*YLVL_00*/
	{0x40, 0x18}, /*YLVL_01*/
	{0x41, 0x18}, /*YLVL_02*/
	{0x42, 0x1e}, /*YLVL_03*/
	{0x43, 0x1e}, /*YLVL_04*/
	{0x44, 0x1e}, /*YLVL_05*/
	{0x45, 0x23}, /*YLVL_06*/
	{0x46, 0x23}, /*YLVL_07*/
	{0x47, 0x23}, /*YLVL_08*/
	{0x48, 0x23}, /*YLVL_09*/
	{0x49, 0x23}, /*YLVL_10*/
	{0x4a, 0x23}, /*YLVL_11*/
	//{0xff, 0x00}
	}, /*EXPOSURECOMPENSATIONN2*/

	{{0x03, 0xcf},
	{0x3f, 0x20}, /*YLVL_00*/
	{0x40, 0x20}, /*YLVL_01*/
	{0x41, 0x20}, /*YLVL_02*/
	{0x42, 0x26}, /*YLVL_03*/
	{0x43, 0x26}, /*YLVL_04*/
	{0x44, 0x26}, /*YLVL_05*/
	{0x45, 0x2b}, /*YLVL_06*/
	{0x46, 0x2b}, /*YLVL_07*/
	{0x47, 0x2b}, /*YLVL_08*/
	{0x48, 0x2b}, /*YLVL_09*/
	{0x49, 0x2b}, /*YLVL_10*/
	{0x4a, 0x2b}, /*YLVL_11*/
	//{0xff, 0x00}
	}, /*EXPOSURECOMPENSATIONN1*/

	{{0x03, 0xcf},
	{0x3f, 0x28}, /*YLVL_00*/
	{0x40, 0x28}, /*YLVL_01*/
	{0x41, 0x28}, /*YLVL_02*/
	{0x42, 0x2e}, /*YLVL_03*/
	{0x43, 0x2e}, /*YLVL_04*/
	{0x44, 0x2e}, /*YLVL_05*/
	{0x45, 0x33}, /*YLVL_06*/
	{0x46, 0x33}, /*YLVL_07*/
	{0x47, 0x33}, /*YLVL_08*/
	{0x48, 0x33}, /*YLVL_09*/
	{0x49, 0x33}, /*YLVL_10*/
	{0x4a, 0x33}, /*YLVL_11*/
	//{0xff, 0x00}
	}, /*EXPOSURECOMPENSATIOND*/

	{{0x03, 0xcf},
	{0x3f, 0x30}, /*YLVL_00*/
	{0x40, 0x30}, /*YLVL_01*/
	{0x41, 0x30}, /*YLVL_02*/
	{0x42, 0x34}, /*YLVL_03*/
	{0x43, 0x34}, /*YLVL_04*/
	{0x44, 0x34}, /*YLVL_05*/
	{0x45, 0x3b}, /*YLVL_06*/
	{0x46, 0x3b}, /*YLVL_07*/
	{0x47, 0x3b}, /*YLVL_08*/
	{0x48, 0x3b}, /*YLVL_09*/
	{0x49, 0x3b}, /*YLVL_10*/
	{0x4a, 0x3b}, /*YLVL_11*/
	//{0xff, 0x00}
	}, /*EXPOSURECOMPENSATIONp1*/

	{{0x03, 0xcf},
	{0x3f, 0x38}, /*YLVL_00*/
	{0x40, 0x38}, /*YLVL_01*/
	{0x41, 0x38}, /*YLVL_02*/
	{0x42, 0x3e}, /*YLVL_03*/
	{0x43, 0x3e}, /*YLVL_04*/
	{0x44, 0x3e}, /*YLVL_05*/
	{0x45, 0x43}, /*YLVL_06*/
	{0x46, 0x43}, /*YLVL_07*/
	{0x47, 0x43}, /*YLVL_08*/
	{0x48, 0x43}, /*YLVL_09*/
	{0x49, 0x43}, /*YLVL_10*/
	{0x4a, 0x43}, /*YLVL_11*/
	//{0xff, 0x00}
	}, /*EXPOSURECOMPENSATIONP2*/
};

static struct msm_camera_i2c_conf_array hi351_exposure_confs[][1] = {
	{{hi351_exposure[0], ARRAY_SIZE(hi351_exposure[0]), 100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_exposure[1], ARRAY_SIZE(hi351_exposure[1]), 100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_exposure[2], ARRAY_SIZE(hi351_exposure[2]), 100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_exposure[3], ARRAY_SIZE(hi351_exposure[3]), 100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_exposure[4], ARRAY_SIZE(hi351_exposure[4]), 100,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_exposure_enum_map[] = {
	MSM_V4L2_EXPOSURE_N2,
	MSM_V4L2_EXPOSURE_N1,
	MSM_V4L2_EXPOSURE_D,
	MSM_V4L2_EXPOSURE_P1,
	MSM_V4L2_EXPOSURE_P2,
};

static struct msm_camera_i2c_enum_conf_array hi351_exposure_enum_confs = {
	.conf = &hi351_exposure_confs[0][0],
	.conf_enum = hi351_exposure_enum_map,
	.num_enum = ARRAY_SIZE(hi351_exposure_enum_map),
	.num_index = ARRAY_SIZE(hi351_exposure_confs),
	.num_conf = ARRAY_SIZE(hi351_exposure_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf hi351_auto_iso_0[] = {
	{0x03,0x00},
	{0x0c, 0xf1}, // STEVE //Sleep on
};

static struct msm_camera_i2c_reg_conf hi351_auto_iso_1[] = {
	//AE OFF
	{0x03, 0xc4},
	{0x10, 0x60},

	{0x03, 0x30}, // STEVE
	{0x36, 0x29}, // STEVE /*Capture Function - Preview*/

	//ISO Auto
	{0x03,0x20},
	{0x12,0x0d}, //Dgain
	{0x51,0xe0}, //Max Gain
	{0x52,0x28}, //Min Gain
	{0x71,0x80}, //DG Max
	{0x12,0x2d}, //Dgain

	{0x03,0xc4},
	{0x19,0x3c}, //Bnd0 Gain
	{0x1a,0x46}, //Bnd1 Gain
	{0x1b,0x5c}, //Bnd2 Gain

	{0x22,0x00}, // band2 min exposure time  1/20s
	{0x23,0x29},
	{0x24,0x32},
	{0x25,0xe0},
	{0x26,0x00},// band3 min exposure time  1/12s
	{0x27,0x41},
	{0x28,0xeb},
	{0x29,0x00},

	{0x03, 0x20}, //STEVE
	{0x24, 0x00}, //STEVE //EXP Max 8.33 fps
	{0x25, 0x62}, //STEVE
	{0x26, 0xe0}, //STEVE
	{0x27, 0x80}, //STEVE

	{0x03, 0xd5}, //STEVE
	{0x41, 0x89}, //STEVE

	{0x03, 0x30}, //STEVE
	{0x36, 0x28}, //STEVE /*Capture Function - Preview*/

	//AE On
	{0x03, 0xc4},
	{0x10, 0xef},
};

static struct msm_camera_i2c_reg_conf hi351_auto_iso_2[] = {
	{0x03, 0x00},
	{0x0c, 0xf0},
	{0x01, 0xf0}, //sleep off
};

static struct msm_camera_i2c_conf_array hi351_auto_iso_conf[] = {
	{&hi351_auto_iso_0[0],
	ARRAY_SIZE(hi351_auto_iso_0), 125, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_auto_iso_1[0],
	ARRAY_SIZE(hi351_auto_iso_1), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_auto_iso_2[0],
	ARRAY_SIZE(hi351_auto_iso_2), 20, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_reg_conf hi351_100_iso_0[] = {
	{0x03,0x00},
	{0x0c, 0xf1}, // STEVE //Sleep on
};

static struct msm_camera_i2c_reg_conf hi351_100_iso_1[] = {
	//AE OFF
	{0x03, 0xc4},
	{0x10, 0x60},

	{0x03, 0x30}, // STEVE
	{0x36, 0x29}, // STEVE /*Capture Function - Preview*/

	//ISO 100
	{0x03,0x20},
	{0x12,0x4d}, //Dgain
	{0x51,0x38}, //Max Gain
	{0x52,0x28}, //Min Gain
	{0x71,0x88}, //DG Max
	{0x12,0x6d}, //Dgain

	{0x03,0xc4},
	{0x19,0x28}, //Bnd0 Gain
	{0x1a,0x30}, //Bnd1 Gain
	{0x1b,0x38}, //Bnd2 Gain

	{0x22,0x00}, // band2 min exposure time  1/20s
	{0x23,0x29},
	{0x24,0x32},
	{0x25,0xe0},
	{0x26,0x00},// band3 min exposure time  1/12s
	{0x27,0x41},
	{0x28,0xeb},
	{0x29,0x00},

	{0x03, 0x20}, //STEVE
	{0x24, 0x00}, //STEVE //EXP Max 8.33 fps
	{0x25, 0x62}, //STEVE
	{0x26, 0xe0}, //STEVE
	{0x27, 0x80}, //STEVE

	{0x03, 0xd5}, //STEVE
	{0x41, 0x89}, //STEVE

	{0x03, 0x30}, //STEVE
	{0x36, 0x28}, //STEVE /*Capture Function - Preview*/

	//AE On
	{0x03, 0xc4},
	{0x10, 0xef},
};

static struct msm_camera_i2c_reg_conf hi351_100_iso_2[] = {
	{0x03, 0x00},
	{0x0c, 0xf0},
	{0x01, 0xf0}, //sleep off
};

static struct msm_camera_i2c_conf_array hi351_100_iso_conf[] = {
	{&hi351_100_iso_0[0],
	ARRAY_SIZE(hi351_100_iso_0), 125, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_100_iso_1[0],
	ARRAY_SIZE(hi351_100_iso_1), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_100_iso_2[0],
	ARRAY_SIZE(hi351_100_iso_2), 20, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_reg_conf hi351_200_iso_0[] = {
	{0x03,0x00},
	{0x0c, 0xf1}, // STEVE //Sleep on
};

static struct msm_camera_i2c_reg_conf hi351_200_iso_1[] = {
	//AE OFF
	{0x03, 0xc4},
	{0x10, 0x60},

	{0x03, 0x30}, // STEVE
	{0x36, 0x29}, // STEVE /*Capture Function - Preview*/

	//ISO 200
	{0x03,0x20},
	{0x12,0x4d}, //Dgain
	{0x51,0x6A}, //Max Gain
	{0x52,0x5A}, //Min Gain
	{0x71,0x88}, //DG Max
	{0x12,0x6d}, //Dgain

	{0x03,0xc4},
	{0x19,0x5A}, //Bnd0 Gain
	{0x1a,0x62}, //Bnd1 Gain
	{0x1b,0x6A}, //Bnd2 Gain

	{0x22,0x00}, // band2 min exposure time  1/20s
	{0x23,0x29},
	{0x24,0x32},
	{0x25,0xe0},
	{0x26,0x00},// band3 min exposure time  1/12s
	{0x27,0x41},
	{0x28,0xeb},
	{0x29,0x00},

	{0x03, 0x20}, //STEVE
	{0x24, 0x00}, //STEVE //EXP Max 8.33 fps
	{0x25, 0x62}, //STEVE
	{0x26, 0xe0}, //STEVE
	{0x27, 0x80}, //STEVE

	{0x03, 0xd5}, //STEVE
	{0x41, 0x89}, //STEVE

	{0x03, 0x30}, //STEVE
	{0x36, 0x28}, //STEVE /*Capture Function - Preview*/

	//AE On
	{0x03, 0xc4},
	{0x10, 0xef},
};

static struct msm_camera_i2c_reg_conf hi351_200_iso_2[] = {
	{0x03, 0x00},
	{0x0c, 0xf0},
	{0x01, 0xf0}, //sleep off
};

static struct msm_camera_i2c_conf_array hi351_200_iso_conf[] = {
	{&hi351_200_iso_0[0],
	ARRAY_SIZE(hi351_200_iso_0), 125, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_200_iso_1[0],
	ARRAY_SIZE(hi351_200_iso_1), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_200_iso_2[0],
	ARRAY_SIZE(hi351_200_iso_2), 20, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_reg_conf hi351_400_iso_0[] = {
	{0x03,0x00},
	{0x0c, 0xf1}, // STEVE //Sleep on
};

static struct msm_camera_i2c_reg_conf hi351_400_iso_1[] = {
	//AE OFF
	{0x03, 0xc4},
	{0x10, 0x60},

	{0x03, 0x30}, // STEVE
	{0x36, 0x29}, // STEVE /*Capture Function - Preview*/

	//ISO 400
	{0x03,0x20},
	{0x12,0x4d}, //Dgain
	{0x51,0x9C}, //Max Gain
	{0x52,0x8c}, //Min Gain
	{0x71,0x88}, //DG Max
	{0x12,0x6d}, //Dgain

	{0x03,0xc4},
	{0x19,0x8C}, //Bnd0 Gain
	{0x1a,0x94}, //Bnd1 Gain
	{0x1b,0x9b}, //Bnd2 Gain

	{0x22,0x00}, // band2 min exposure time  1/20s
	{0x23,0x29},
	{0x24,0x32},
	{0x25,0xe0},
	{0x26,0x00},// band3 min exposure time  1/12s
	{0x27,0x41},
	{0x28,0xeb},
	{0x29,0x00},

	{0x03, 0x20}, //STEVE
	{0x24, 0x00}, //STEVE //EXP Max 8.33 fps
	{0x25, 0x62}, //STEVE
	{0x26, 0xe0}, //STEVE
	{0x27, 0x80}, //STEVE

	{0x03, 0xd5}, //STEVE
	{0x41, 0x89}, //STEVE

	{0x03, 0x30}, //STEVE
	{0x36, 0x28}, //STEVE /*Capture Function - Preview*/

	//AE On
	{0x03, 0xc4},
	{0x10, 0xef},
};

static struct msm_camera_i2c_reg_conf hi351_400_iso_2[] = {
	{0x03, 0x00},
	{0x0c, 0xf0},
	{0x01, 0xf0}, //sleep off
};

static struct msm_camera_i2c_conf_array hi351_400_iso_conf[] = {
	{&hi351_400_iso_0[0],
	ARRAY_SIZE(hi351_400_iso_0), 125, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_400_iso_1[0],
	ARRAY_SIZE(hi351_400_iso_1), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_400_iso_2[0],
	ARRAY_SIZE(hi351_400_iso_2), 20, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_reg_conf hi351_800_iso_0[] = {
	{0x03,0x00},
	{0x0c, 0xf1}, // STEVE //Sleep on
};

static struct msm_camera_i2c_reg_conf hi351_800_iso_1[] = {
	//AE OFF
	{0x03, 0xc4},
	{0x10, 0x60},

	{0x03, 0x30}, // STEVE
	{0x36, 0x29}, // STEVE /*Capture Function - Preview*/

	//ISO 800
	{0x03,0x20},
	{0x12,0x4d}, //Dgain
	{0x51,0xd0}, //Max Gain
	{0x52,0xc0}, //Min Gain
	{0x71,0x88}, //DG Max
	{0x12,0x6d}, //Dgain

	{0x03,0xc4},
	{0x19,0xc0}, //Bnd0 Gain
	{0x1a,0xc8}, //Bnd1 Gain
	{0x1b,0xd0}, //Bnd2 Gain

	{0x22,0x00}, // band2 min exposure time  1/20s
	{0x23,0x29},
	{0x24,0x32},
	{0x25,0xe0},
	{0x26,0x00},// band3 min exposure time  1/12s
	{0x27,0x41},
	{0x28,0xeb},
	{0x29,0x00},

	{0x03, 0x20}, //STEVE
	{0x24, 0x00}, //STEVE //EXP Max 8.33 fps
	{0x25, 0x62}, //STEVE
	{0x26, 0xe0}, //STEVE
	{0x27, 0x80}, //STEVE

	{0x03, 0xd5}, //STEVE
	{0x41, 0x89}, //STEVE

	{0x03, 0x30}, //STEVE
	{0x36, 0x28}, //STEVE /*Capture Function - Preview*/

	//AE On
	{0x03, 0xc4},
	{0x10, 0xef},
};

static struct msm_camera_i2c_reg_conf hi351_800_iso_2[] = {
	{0x03, 0x00},
	{0x0c, 0xf0},
	{0x01, 0xf0}, //sleep off
};

static struct msm_camera_i2c_conf_array hi351_800_iso_conf[] = {
	{&hi351_800_iso_0[0],
	ARRAY_SIZE(hi351_800_iso_0), 125, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_800_iso_1[0],
	ARRAY_SIZE(hi351_800_iso_1), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_800_iso_2[0],
	ARRAY_SIZE(hi351_800_iso_2), 20, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_reg_conf hi351_1600_iso_0[] = {
	{0x03,0x00},
	{0x0c, 0xf1}, // STEVE //Sleep on
};

static struct msm_camera_i2c_reg_conf hi351_1600_iso_1[] = {
	//AE OFF
	{0x03, 0xc4},
	{0x10, 0x60},

	{0x03, 0x30}, // STEVE
	{0x36, 0x29}, // STEVE /*Capture Function - Preview*/

	//ISO 1000
	{0x03,0x20},
	{0x12,0x4d}, //Dgain
	{0x51,0xe0}, //Max Gain
	{0x52,0xd0}, //Min Gain
	{0x71,0x88}, //DG Max
	{0x12,0x6d}, //Dgain

	{0x03,0xc4},
	{0x19,0xd0}, //Bnd0 Gain
	{0x1a,0xd8}, //Bnd1 Gain
	{0x1b,0xe0}, //Bnd2 Gain

	{0x22,0x00}, // band2 min exposure time  1/20s
	{0x23,0x29},
	{0x24,0x32},
	{0x25,0xe0},
	{0x26,0x00},// band3 min exposure time  1/12s
	{0x27,0x41},
	{0x28,0xeb},
	{0x29,0x00},

	{0x03, 0x20}, //STEVE
	{0x24, 0x00}, //STEVE //EXP Max 8.33 fps
	{0x25, 0x62}, //STEVE
	{0x26, 0xe0}, //STEVE
	{0x27, 0x80}, //STEVE

	{0x03, 0xd5}, //STEVE
	{0x41, 0x89}, //STEVE

	{0x03, 0x30}, //STEVE
	{0x36, 0x28}, //STEVE /*Capture Function - Preview*/

	//AE On
	{0x03, 0xc4},
	{0x10, 0xe3},
};

static struct msm_camera_i2c_reg_conf hi351_1600_iso_2[] = {
	{0x03, 0x00},
	{0x0c, 0xf0},
	{0x01, 0xf0}, //sleep off
};

static struct msm_camera_i2c_conf_array hi351_1600_iso_conf[] = {
	{&hi351_1600_iso_0[0],
	ARRAY_SIZE(hi351_1600_iso_0), 125, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_1600_iso_1[0],
	ARRAY_SIZE(hi351_1600_iso_1), 20, MSM_CAMERA_I2C_BYTE_DATA},
	{&hi351_1600_iso_2[0],
	ARRAY_SIZE(hi351_1600_iso_2), 20, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_reg_conf hi351_dummy_iso[][1] = {
	{{-1, -1}},
	{{-1, -1}},
	{{-1, -1}},
	{{-1, -1}},
	{{-1, -1}},
	{{-1, -1}},
	{{-1, -1}},
};

static struct msm_camera_i2c_conf_array hi351_dummy_iso_confs[][1] = {
	{{hi351_dummy_iso[0], ARRAY_SIZE(hi351_dummy_iso[0]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_dummy_iso[1], ARRAY_SIZE(hi351_dummy_iso[1]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_dummy_iso[2], ARRAY_SIZE(hi351_dummy_iso[2]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_dummy_iso[3], ARRAY_SIZE(hi351_dummy_iso[3]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_dummy_iso[4], ARRAY_SIZE(hi351_dummy_iso[4]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_dummy_iso[5], ARRAY_SIZE(hi351_dummy_iso[5]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_iso_enum_map[] = {
	MSM_V4L2_ISO_AUTO ,
	MSM_V4L2_ISO_DEBLUR,
	MSM_V4L2_ISO_100,
	MSM_V4L2_ISO_200,
	MSM_V4L2_ISO_400,
	MSM_V4L2_ISO_800,
	MSM_V4L2_ISO_1600,
};

static struct msm_camera_i2c_enum_conf_array hi351_iso_dummy_enum_confs = {
	.conf = &hi351_dummy_iso_confs[0][0],
	.conf_enum = hi351_iso_enum_map,
	.num_enum = ARRAY_SIZE(hi351_iso_enum_map),
	.num_index = ARRAY_SIZE(hi351_dummy_iso_confs),
	.num_conf = ARRAY_SIZE(hi351_dummy_iso_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf hi351_no_effect[] = {
	{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{0x03, 0x14},{0x80, 0x20},
};

static struct msm_camera_i2c_conf_array hi351_no_effect_confs[] = {
	{&hi351_no_effect[0],
	ARRAY_SIZE(hi351_no_effect), 0,
	MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_reg_conf hi351_special_effect[][9] = {
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{0x03, 0x14},{0x80, 0x20}},  /*support effect OFF*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf3},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{0x03, 0x14},{0x80, 0x20}},  /*support effect MONO*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf8},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{0x03, 0x14},{0x80, 0x20}},  /*support effect negative*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*not support effect solarize*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf3},{0x42, 0x00},{0x43, 0x00},{0x44, 0x60},{0x45, 0xa3},{0x03, 0x14},{0x80, 0x20}},  /*support effect sepia*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*not support effect posteraize*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},              /*not support effect white board*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*not support effect black board*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*not support effect aqua*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*not support effect emboss*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*not support effect sketch*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*not support effect neon*/
	{{0x03, 0x10},{0x11, 0x03},{0x12, 0xf0},{0x42, 0x00},{0x43, 0x00},{0x44, 0x80},{0x45, 0x80},{-1, -1},{-1, -1}},          /*MAX value*/
};

static struct msm_camera_i2c_conf_array hi351_special_effect_confs[][1] = {
	{{hi351_special_effect[0],  ARRAY_SIZE(hi351_special_effect[0]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[1],  ARRAY_SIZE(hi351_special_effect[1]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[2],  ARRAY_SIZE(hi351_special_effect[2]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[3],  ARRAY_SIZE(hi351_special_effect[3]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[4],  ARRAY_SIZE(hi351_special_effect[4]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[5],  ARRAY_SIZE(hi351_special_effect[5]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[6],  ARRAY_SIZE(hi351_special_effect[6]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[7],  ARRAY_SIZE(hi351_special_effect[7]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[8],  ARRAY_SIZE(hi351_special_effect[8]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[9],  ARRAY_SIZE(hi351_special_effect[9]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[10], ARRAY_SIZE(hi351_special_effect[10]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[11], ARRAY_SIZE(hi351_special_effect[11]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_special_effect[12], ARRAY_SIZE(hi351_special_effect[12]), 0,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_special_effect_enum_map[] = {
	MSM_V4L2_EFFECT_OFF,
	MSM_V4L2_EFFECT_MONO,
	MSM_V4L2_EFFECT_NEGATIVE,
	MSM_V4L2_EFFECT_SOLARIZE,
	MSM_V4L2_EFFECT_SEPIA,
	MSM_V4L2_EFFECT_POSTERAIZE,
	MSM_V4L2_EFFECT_WHITEBOARD,
	MSM_V4L2_EFFECT_BLACKBOARD,
	MSM_V4L2_EFFECT_AQUA,
	MSM_V4L2_EFFECT_EMBOSS,
	MSM_V4L2_EFFECT_SKETCH,
	MSM_V4L2_EFFECT_NEON,
	MSM_V4L2_EFFECT_MAX,
};

static struct msm_camera_i2c_enum_conf_array hi351_special_effect_enum_confs = {
	.conf = &hi351_special_effect_confs[0][0],
	.conf_enum = hi351_special_effect_enum_map,
	.num_enum = ARRAY_SIZE(hi351_special_effect_enum_map),
	.num_index = ARRAY_SIZE(hi351_special_effect_confs),
	.num_conf = ARRAY_SIZE(hi351_special_effect_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf hi351_antibanding[][2] = {
	{{-1, -1},{-1, -1}}, /*ANTIBANDING 60HZ*/
	{{-1, -1},{-1, -1}}, /*ANTIBANDING 50HZ*/
	{{-1, -1},{-1, -1}}, /* ANTIBANDING AUTO*/
};

static struct msm_camera_i2c_conf_array hi351_antibanding_confs[][1] = {
	{{hi351_antibanding[0], ARRAY_SIZE(hi351_antibanding[0]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_antibanding[1], ARRAY_SIZE(hi351_antibanding[1]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_antibanding[2], ARRAY_SIZE(hi351_antibanding[2]),  0,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_antibanding_enum_map[] = {
	MSM_V4L2_POWER_LINE_60HZ,
	MSM_V4L2_POWER_LINE_50HZ,
	MSM_V4L2_POWER_LINE_AUTO,
};

static struct msm_camera_i2c_enum_conf_array hi351_antibanding_enum_confs = {
	.conf = &hi351_antibanding_confs[0][0],
	.conf_enum = hi351_antibanding_enum_map,
	.num_enum = ARRAY_SIZE(hi351_antibanding_enum_map),
	.num_index = ARRAY_SIZE(hi351_antibanding_confs),
	.num_conf = ARRAY_SIZE(hi351_antibanding_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf hi351_wb_oem[][10] = {
	/*WB Off*/
	{{0x03, 0xc5}, /*Page c5*/
	{0x11, 0xa4}, /*adaptive on*/
	{0x12, 0x93}, /*adaptive on*/
	{0x03, 0xc6}, /*Page c6*/
	{0x18, 0x46}, /*RgainMin*/
	{0x19, 0x90}, /*RgainMax*/
	{0x1a, 0x40}, /*BgainMin*/
	{0x1b, 0xa6}, /*BgainMax*/
	{0x03, 0xc5},
	{0x10, 0xb1}},

	/*WB Auto*/
	{{0x03, 0xc5}, /*Page c5*/
	{0x11, 0xa4}, /*adaptive on*/
	{0x12, 0x93}, /*adaptive on*/
	{0x03, 0xc6}, /*Page c6*/
	{0x18, 0x46}, /*RgainMin*/
	{0x19, 0x90}, /*RgainMax*/
	{0x1a, 0x40}, /*BgainMin*/
	{0x1b, 0xa6}, /*BgainMax*/
	{0x03, 0xc5},
	{0x10, 0xb1}},

	/*WB CUSTOM*/
	{{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},{-1, -1},},

	/*WB Incandescent*/
	{{0x03, 0xc5}, /*Page c5*/
	{0x11, 0xa0}, /*adaptive off*/
	{0x12, 0x03}, /*adaptive off*/
	{0x03, 0xc6}, /*Page c6*/
	{0x18, 0x47}, /*indoor R gain Min*/
	{0x19, 0x48}, /*indoor R gain Max*/
	{0x1a, 0xa4}, /*indoor B gain Min*/
	{0x1b, 0xa5}, /*indoor B gain Max*/
	{0x03, 0xc5},
	{0x10, 0xb1}},

	/*WB Fluorescent*/
	{{0x03, 0xc5}, /*Page c5*/
	{0x11, 0xa0}, /*adaptive off*/
	{0x12, 0x03}, /*adaptive off*/
	{0x03, 0xc6}, /*Page c6*/
	{0x18, 0x5d}, /*indoor R gain Min*/
	{0x19, 0x5e}, /*indoor R gain Max*/
	{0x1a, 0x94}, /*indoor B gain Min*/
	{0x1b, 0x95}, /*indoor B gain Max*/
	{0x03, 0xc5},
	{0x10, 0xb1}},

	/*WB Daylight*/
	{{0x03, 0xc5}, /*Page c5*/
	{0x11, 0xa0}, /*adaptive off*/
	{0x12, 0x03}, /*adaptive off*/
	{0x03, 0xc6}, /*Page c6*/
	{0x18, 0x75}, /*indoor R gain Min*/
	{0x19, 0x76}, /*indoor R gain Max*/
	{0x1a, 0x6a}, /*indoor B gain Min*/
	{0x1b, 0x6b}, /*indoor B gain Max*/
	{0x03, 0xc5},
	{0x10, 0xb1}},

	/*WB Cloudy*/
	{{0x03, 0xc5}, /*Page c5*/
	{0x11, 0xa0}, /*adaptive off*/
	{0x12, 0x03}, /*adaptive off*/
	{0x03, 0xc6}, /*Page c6*/
	{0x18, 0x90}, /*indoor R gain Min*/
	{0x19, 0x92}, /*indoor R gain Max*/
	{0x1a, 0x5f}, /*indoor B gain Min*/
	{0x1b, 0x61}, /*indoor B gain Max*/
	{0x03, 0xc5},
	{0x10, 0xb1}},
};

static struct msm_camera_i2c_conf_array hi351_wb_oem_confs[][1] = {
	{{hi351_wb_oem[0], ARRAY_SIZE(hi351_wb_oem[0]),  100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_wb_oem[1], ARRAY_SIZE(hi351_wb_oem[1]),  100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_wb_oem[2], ARRAY_SIZE(hi351_wb_oem[2]),  100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_wb_oem[3], ARRAY_SIZE(hi351_wb_oem[3]),  100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_wb_oem[4], ARRAY_SIZE(hi351_wb_oem[4]),  100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_wb_oem[5], ARRAY_SIZE(hi351_wb_oem[5]),  100,
	MSM_CAMERA_I2C_BYTE_DATA},},
	{{hi351_wb_oem[6], ARRAY_SIZE(hi351_wb_oem[6]),  100,
	MSM_CAMERA_I2C_BYTE_DATA},},
};

static int hi351_wb_oem_enum_map[] = {
	MSM_V4L2_WB_OFF,
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_CUSTOM,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
};

static struct msm_camera_i2c_enum_conf_array hi351_wb_oem_enum_confs = {
	.conf = &hi351_wb_oem_confs[0][0],
	.conf_enum = hi351_wb_oem_enum_map,
	.num_enum = ARRAY_SIZE(hi351_wb_oem_enum_map),
	.num_index = ARRAY_SIZE(hi351_wb_oem_confs),
	.num_conf = ARRAY_SIZE(hi351_wb_oem_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

int hi351_effect_msm_sensor_s_ctrl_by_enum(
	struct msm_sensor_ctrl_t *s_ctrl,
	struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	effect_value = value;
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->no_effect_settings, 0);
		if (rc < 0) {
			CDBG("write faield\n");
			return rc;
		}
	} else {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}

	return rc;
}

int hi351_msm_sensor_s_ctrl_by_enum(
	struct msm_sensor_ctrl_t *s_ctrl,
	struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		CDBG("write faield\n");
		return rc;
	}

	return rc;
}

int hi351_msm_sensor_s_ctrl_iso_by_enum(
	struct msm_sensor_ctrl_t *s_ctrl,
	struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	iso_value = value;
	if (mode == MSM_SENSOR_RES_FULL)
	{

	} else if (mode == MSM_SENSOR_RES_QTR)
	{
		switch (value)
		{
		case MSM_V4L2_ISO_AUTO:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_auto_iso_conf[0],
				ARRAY_SIZE(hi351_auto_iso_conf));
			break;

		case MSM_V4L2_ISO_100:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_100_iso_conf[0],
				ARRAY_SIZE(hi351_100_iso_conf));
			break;

		case MSM_V4L2_ISO_200:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_200_iso_conf[0],
				ARRAY_SIZE(hi351_200_iso_conf));
			break;

		case MSM_V4L2_ISO_400:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_400_iso_conf[0],
				ARRAY_SIZE(hi351_400_iso_conf));
			break;

		case MSM_V4L2_ISO_800:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_800_iso_conf[0],
				ARRAY_SIZE(hi351_800_iso_conf));
			break;

		case MSM_V4L2_ISO_1600:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_1600_iso_conf[0],
				ARRAY_SIZE(hi351_1600_iso_conf));
			break;

		case MSM_V4L2_ISO_DEBLUR:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_auto_iso_conf[0],
				ARRAY_SIZE(hi351_auto_iso_conf));
			break;
		}
		msm_sensor_write_all_conf_array(
			s_ctrl->sensor_i2c_client,
			&hi351_preview_conf[0],
			ARRAY_SIZE(hi351_preview_conf));
	}
	else if (mode == MSM_SENSOR_RES_2)
	{
		switch (value)
		{
		case MSM_V4L2_ISO_AUTO:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_auto_iso_conf[0],
				ARRAY_SIZE(hi351_auto_iso_conf));
			break;

		case MSM_V4L2_ISO_100:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_100_iso_conf[0],
				ARRAY_SIZE(hi351_100_iso_conf));
			break;

		case MSM_V4L2_ISO_200:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_200_iso_conf[0],
				ARRAY_SIZE(hi351_200_iso_conf));
			break;

		case MSM_V4L2_ISO_400:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_400_iso_conf[0],
				ARRAY_SIZE(hi351_400_iso_conf));
			break;

		case MSM_V4L2_ISO_800:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_800_iso_conf[0],
				ARRAY_SIZE(hi351_800_iso_conf));
			break;

		case MSM_V4L2_ISO_1600:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_1600_iso_conf[0],
				ARRAY_SIZE(hi351_1600_iso_conf));
			break;

		case MSM_V4L2_ISO_DEBLUR:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_auto_iso_conf[0],
				ARRAY_SIZE(hi351_auto_iso_conf));
			break;
		}
		msm_sensor_write_all_conf_array(
			s_ctrl->sensor_i2c_client,
			&hi351_full_conf[0],
			ARRAY_SIZE(hi351_full_conf));
	}
	return rc;
}

int hi351_msm_sensor_s_ctrl_antibanding_by_enum(
	struct msm_sensor_ctrl_t *s_ctrl,
	struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	unsigned short t1, t2;

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x03, 0x20, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
		0x10, &t1, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x03, 0xc4, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
		0x10, &t2, MSM_CAMERA_I2C_BYTE_DATA);

	switch (value)
	{
	case MSM_V4L2_POWER_LINE_60HZ:
		t1 &= (~(0x10));
		t2 &= (~(0x08));
		break;
	case MSM_V4L2_POWER_LINE_50HZ:
		t1 |= 0x10;
		t2 |= 0x08;
		break;
	case MSM_V4L2_POWER_LINE_AUTO:
		t1 |= 0x40;
		break;
	default:
		return rc;
	}

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x03, 0x20, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x10, t1, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x03, 0xc4, MSM_CAMERA_I2C_BYTE_DATA);

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
		0x10, t2, MSM_CAMERA_I2C_BYTE_DATA);

	return rc;
}

struct msm_sensor_v4l2_ctrl_info_t hi351_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_SATURATION,
		.min = MSM_V4L2_SATURATION_L0,
		.max = MSM_V4L2_SATURATION_L9,
		.step = 1,
		.enum_cfg_settings = &hi351_saturation_enum_confs,
		.s_v4l2_ctrl = hi351_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_CONTRAST,
		.min = MSM_V4L2_CONTRAST_L0,
		.max = MSM_V4L2_CONTRAST_L9,
		.step = 1,
		.enum_cfg_settings = &hi351_contrast_enum_confs,
		.s_v4l2_ctrl = hi351_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SHARPNESS,
		.min = MSM_V4L2_SHARPNESS_L0,
		.max = MSM_V4L2_SHARPNESS_L6,
		.step = 1,
		.enum_cfg_settings = &hi351_sharpness_enum_confs,
		.s_v4l2_ctrl = hi351_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_EXPOSURE,
		.min = MSM_V4L2_EXPOSURE_N2,
		.max = MSM_V4L2_EXPOSURE_P2,
		.step = 1,
		.enum_cfg_settings = &hi351_exposure_enum_confs,
		.s_v4l2_ctrl = hi351_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = MSM_V4L2_PID_ISO,
		.min = MSM_V4L2_ISO_AUTO,
		.max = MSM_V4L2_ISO_1600,
		.step = 1,
		.enum_cfg_settings = &hi351_iso_dummy_enum_confs,
		.s_v4l2_ctrl = hi351_msm_sensor_s_ctrl_iso_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SPECIAL_EFFECT,
		.min = MSM_V4L2_EFFECT_OFF,
		.max = MSM_V4L2_EFFECT_NEGATIVE,
		.step = 1,
		.enum_cfg_settings = &hi351_special_effect_enum_confs,
		.s_v4l2_ctrl = hi351_effect_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_POWER_LINE_FREQUENCY,
		.min = MSM_V4L2_POWER_LINE_60HZ,
		.max = MSM_V4L2_POWER_LINE_AUTO,
		.step = 1,
		.enum_cfg_settings = &hi351_antibanding_enum_confs,
		.s_v4l2_ctrl = hi351_msm_sensor_s_ctrl_antibanding_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.min = MSM_V4L2_WB_OFF,
		.max = MSM_V4L2_WB_CLOUDY_DAYLIGHT,
		.step = 1,
		.enum_cfg_settings = &hi351_wb_oem_enum_confs,
		.s_v4l2_ctrl = hi351_msm_sensor_s_ctrl_by_enum,
	},
};

static struct msm_camera_csi_params hi351_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x16,
};

static struct msm_camera_csi_params *hi351_csi_params_array[] = {
	&hi351_csi_params,
	&hi351_csi_params,
	&hi351_csi_params,
};

static struct msm_sensor_output_reg_addr_t hi351_reg_addr = {
	.x_output = 0x0398,
	.y_output = 0x039A,
	.line_length_pclk = 0x380C,
	.frame_length_lines = 0x380E,
};

static struct msm_sensor_id_info_t hi351_id_info = {
	.sensor_id_reg_addr = 0x04,
	.sensor_id = 0xa403,
};

static const struct i2c_device_id hi351_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&hi351_s_ctrl},
	{}
};

int32_t hi351_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	CDBG("%s IN\r\n", __func__);
	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;

	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		CDBG_HIGH("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	usleep_range(5000, 5100);

	return rc;
}

static struct i2c_driver hi351_i2c_driver = {
	.id_table = hi351_i2c_id,
	.probe  = hi351_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client hi351_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int rc = 0;
	CDBG("HI351\n");

	rc = i2c_add_driver(&hi351_i2c_driver);

	return rc;
}

static struct v4l2_subdev_core_ops hi351_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops hi351_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops hi351_subdev_ops = {
	.core = &hi351_subdev_core_ops,
	.video  = &hi351_subdev_video_ops,
};

int32_t hi351_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
#ifdef ODMM_PROJECT_STAGE_EVB1
//int32_t rc = 0;
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;

	static struct vreg *vreg_L19_regulator;
	static struct vreg *vreg_L16_regulator;
	int32_t rc = 0;
	
	CDBG("%s IN\r\n", __func__);
	CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__, info->sensor_pwd, info->sensor_reset);



	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_VREG_CAM_ANA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_VREG_VCM, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
    	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
   	 gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_PWDN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
    	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_FLASH_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE); //
    	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_STROB_PIN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE);

	
	//reset all of pin (set 0)
	/*========================================================*/
	rc = gpio_request(ODMM_GPIO_PWDN, "hi351");
	rc = gpio_request(ODMM_GPIO_RESET, "hi351");
  	 rc = gpio_request(ODMM_GPIO_FLASH_PIN, "hi351");
    	rc = gpio_request(ODMM_GPIO_STROB_PIN, "hi351");

	gpio_direction_output(ODMM_GPIO_PWDN, 0);   // power down pin active high

	gpio_direction_output(ODMM_GPIO_RESET, 0);   // reset pin active low
	gpio_direction_output(ODMM_GPIO_FLASH_PIN, 0);   // flash pin
	gpio_direction_output(ODMM_GPIO_STROB_PIN, 0);   // strob pin

    #if ODMM_OPTIMIZATION_DEBUG
    //ODMM_OPTIMIZATION_TRACE;
	usleep_range(5000, 5100);
    //ODMM_OPTIMIZATION_TRACE;
    #endif
	
	// step 1 pull up L16 
	/*========================================================*/

		vreg_L19_regulator = vreg_get(NULL , "ldo19");//vddio
	/* Set the voltage level to 1.8V */
	rc = vreg_set_level(vreg_L19_regulator, 1800);
	rc = vreg_enable(vreg_L19_regulator);
	usleep_range(2000, 2100);

	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_VREG_CAM_ANA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	rc = gpio_request(ODMM_GPIO_VREG_CAM_ANA, "hi351");
	gpio_direction_output(ODMM_GPIO_VREG_CAM_ANA, 1);
	
		vreg_L16_regulator = vreg_get(NULL , "ldo6");
	/* Set the voltage level to 1.5V */
	rc = vreg_set_level(vreg_L16_regulator, 1200);
		printk("zy 0v5647 rc=%d\n", rc);
	rc = vreg_enable(vreg_L16_regulator);
	printk("zy 0x5647 rc=%d\n", rc);
	usleep_range(5000, 5100);
	
	

    #if ODMM_OPTIMIZATION_DEBUG
    //ODMM_OPTIMIZATION_TRACE;
	usleep_range(5000, 5100);
    //ODMM_OPTIMIZATION_TRACE;
    #endif

	
	//step 2 pull up Avdd and vcm
	/*========================================================*/


    rc = msm_sensor_power_up(s_ctrl);
	
	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_VREG_VCM, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	rc = gpio_request(ODMM_GPIO_VREG_VCM, "hi351");
	gpio_direction_output(ODMM_GPIO_VREG_VCM, 1);
	usleep_range(5000, 5100);
	
	/*========================================================*/
	gpio_direction_output(ODMM_GPIO_PWDN, 1/*0*/);
	usleep_range(5000, 5100);

	/*========================================================*/
	gpio_direction_output(ODMM_GPIO_RESET, 1);
	usleep_range(10000, 11000);
    

    	usleep_range(5000, 5100);
	gpio_free(ODMM_GPIO_RESET);

	gpio_free(ODMM_GPIO_PWDN);
	gpio_free(ODMM_GPIO_VREG_VCM);
	gpio_free(ODMM_GPIO_VREG_CAM_ANA);
  	gpio_free(ODMM_GPIO_FLASH_PIN);
	gpio_free(ODMM_GPIO_STROB_PIN);
	return 0;
#else

struct msm_camera_sensor_info *info = s_ctrl->sensordata;

	static struct vreg *vreg_L6_regulator;
	int32_t rc = 0;
	
	CDBG("%s IN\r\n", __func__);
	CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__, info->sensor_pwd, info->sensor_reset);



	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_VREG_CAM_ANA, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_RESET, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
 	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_PWDN, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(ODMM_GPIO_CAM_IO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
	
	
	//reset all of pin (set 0)

	rc = gpio_request(ODMM_GPIO_PWDN, "hi351");
	rc = gpio_request(ODMM_GPIO_RESET, "hi351");
	rc = gpio_request(ODMM_GPIO_VREG_CAM_ANA, "hi351");
	rc = gpio_request(ODMM_GPIO_CAM_IO, "hi351");
	gpio_direction_output(ODMM_GPIO_VREG_CAM_ANA, 0);
	gpio_direction_output(ODMM_GPIO_CAM_IO, 0);
	gpio_direction_output(ODMM_GPIO_PWDN, 0);   // power down pin active low
	gpio_direction_output(ODMM_GPIO_RESET, 0);   // reset pin active low
	msleep(20);

	//enable gpio109 control camera I/O voltage
	gpio_direction_output(ODMM_GPIO_CAM_IO, 1);
	usleep_range(1500, 2000);

	//enable VDDA controlled by gpio35
	gpio_direction_output(ODMM_GPIO_VREG_CAM_ANA, 1);
	usleep_range(1500, 2000);
	
	//enable camera core voltage to 1.2V
	vreg_L6_regulator = vreg_get(NULL , "ldo6");  
	rc = vreg_set_level(vreg_L6_regulator, 1200);
	rc = vreg_enable(vreg_L6_regulator);
	usleep_range(2000, 2100);

	//enable MCLK ,msm_sensor_power_up just do mclk on or off
	 rc = msm_sensor_power_up(s_ctrl);
	usleep_range(2000, 2100);

	//enable chip_en contorlled by gpio5
	gpio_direction_output(ODMM_GPIO_PWDN, 1);
	msleep(30);

	//pull up reset, controlled by gpio6
	gpio_direction_output(ODMM_GPIO_RESET, 1);
	/*========================================================*/


	gpio_free(ODMM_GPIO_RESET);
	gpio_free(ODMM_GPIO_PWDN);
	gpio_free(ODMM_GPIO_VREG_CAM_ANA);
	gpio_free(ODMM_GPIO_CAM_IO);
	
	return 0;

#endif
}

int32_t hi351_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{

#ifdef ODMM_PROJECT_STAGE_EVB1

struct msm_camera_sensor_info *info = NULL;

	CDBG("%s IN\r\n", __func__);

	info = s_ctrl->sensordata;

	gpio_direction_output(info->sensor_pwd, 1);
	usleep_range(5000, 5100);
	

	gpio_request(ODMM_GPIO_PWDN, "hi351");
	gpio_direction_output(ODMM_GPIO_PWDN, 1);
	gpio_free(ODMM_GPIO_PWDN);

	gpio_request(ODMM_GPIO_RESET, "hi351");
	gpio_direction_output(ODMM_GPIO_RESET, 0);
	gpio_free(ODMM_GPIO_RESET);
	
	gpio_request(ODMM_GPIO_VREG_VCM, "hi351");
	gpio_direction_output(ODMM_GPIO_VREG_VCM, 0);
	gpio_free(ODMM_GPIO_VREG_VCM);

	gpio_request(ODMM_GPIO_VREG_CAM_ANA, "hi351");
	gpio_direction_output(ODMM_GPIO_VREG_CAM_ANA, 0);
	gpio_free(ODMM_GPIO_VREG_CAM_ANA);
	//add end
	
	msm_sensor_power_down(s_ctrl);
	msleep(40);
	if (s_ctrl->sensordata->pmic_gpio_enable){
		//lcd_camera_power_onoff(0);
	}
	return 0;

#else

struct msm_camera_sensor_info *info = NULL;

	static struct vreg *vreg_L6_regulator;

	CDBG("%s IN\r\n", __func__);

	info = s_ctrl->sensordata;
	//first pull down reset,gpio6
	gpio_request(ODMM_GPIO_RESET, "hi351");
	gpio_direction_output(ODMM_GPIO_RESET, 0);
	gpio_free(ODMM_GPIO_RESET);
	usleep_range(10, 20);

	//disable mclk
	msm_sensor_power_down(s_ctrl);

	// pull down standby ,or gpio5
	gpio_request(ODMM_GPIO_PWDN, "hi351");
	gpio_direction_output(ODMM_GPIO_PWDN, 0);
	gpio_free(ODMM_GPIO_PWDN);

	vreg_L6_regulator = vreg_get(NULL , "ldo6");  //control camera vdd_reg
	vreg_disable(vreg_L6_regulator);
	usleep_range(1500, 2000);
	
	gpio_request(ODMM_GPIO_VREG_CAM_ANA, "hi351");
	gpio_direction_output(ODMM_GPIO_VREG_CAM_ANA, 0);
	gpio_free(ODMM_GPIO_VREG_CAM_ANA);
	usleep_range(1500, 2000);
	
	gpio_request(ODMM_GPIO_CAM_IO, "hi351");
	gpio_direction_output(ODMM_GPIO_CAM_IO, 0);
	gpio_free(ODMM_GPIO_CAM_IO);

	return 0;

#endif
}

int32_t hi351_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
	int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;

	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		if (!csi_config) {
			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(50);
			csi_config = 1;
		}

		switch (iso_value)
		{
		case MSM_V4L2_ISO_AUTO:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_auto_iso_conf[0],
				ARRAY_SIZE(hi351_auto_iso_conf));
			break;

		case MSM_V4L2_ISO_100:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_100_iso_conf[0],
				ARRAY_SIZE(hi351_100_iso_conf));
			break;

		case MSM_V4L2_ISO_200:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_200_iso_conf[0],
				ARRAY_SIZE(hi351_200_iso_conf));
			break;

		case MSM_V4L2_ISO_400:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_400_iso_conf[0],
				ARRAY_SIZE(hi351_400_iso_conf));
			break;

		case MSM_V4L2_ISO_800:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_800_iso_conf[0],
				ARRAY_SIZE(hi351_800_iso_conf));
			break;

		case MSM_V4L2_ISO_1600:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_1600_iso_conf[0],
				ARRAY_SIZE(hi351_1600_iso_conf));
			break;

		case MSM_V4L2_ISO_DEBLUR:
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_auto_iso_conf[0],
				ARRAY_SIZE(hi351_auto_iso_conf));
			break;
		}

		mode = res;
		if (res == MSM_SENSOR_RES_FULL)
		{
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_snapshot_conf[0],
				ARRAY_SIZE(hi351_snapshot_conf));
		} else if (res == MSM_SENSOR_RES_QTR)
		{
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_preview_conf[0],
				ARRAY_SIZE(hi351_preview_conf));
		}
		else if (res == MSM_SENSOR_RES_2)
		{
			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client,
				&hi351_full_conf[0],
				ARRAY_SIZE(hi351_full_conf));
		}

		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE,
			&s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);
	}
	return rc;
}

static struct msm_sensor_fn_t hi351_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_group_hold_on = msm_sensor_group_hold_on,
	.sensor_group_hold_off = msm_sensor_group_hold_off,
	.sensor_csi_setting = hi351_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = hi351_sensor_power_up,
	.sensor_power_down = hi351_sensor_power_down,
};

static struct msm_sensor_reg_t hi351_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = hi351_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(hi351_start_settings),
	.stop_stream_conf = hi351_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(hi351_stop_settings),
	.init_settings = &hi351_init_conf[0],
	.init_size = ARRAY_SIZE(hi351_init_conf),
	.mode_settings = &hi351_dummy_confs[0],
	.no_effect_settings = &hi351_no_effect_confs[0],
	.output_settings = &hi351_dimensions[0],
	.num_conf = ARRAY_SIZE(hi351_dummy_confs),
};

static struct msm_sensor_ctrl_t hi351_s_ctrl = {
	.msm_sensor_reg = &hi351_regs,
	.msm_sensor_v4l2_ctrl_info = hi351_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(hi351_v4l2_ctrl_info),
	.sensor_i2c_client = &hi351_sensor_i2c_client,
	.sensor_i2c_addr = 0x40,
	.sensor_output_reg_addr = &hi351_reg_addr,
	.sensor_id_info = &hi351_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &hi351_csi_params_array[0],
	.msm_sensor_mutex = &hi351_mut,
	.sensor_i2c_driver = &hi351_i2c_driver,
	.sensor_v4l2_subdev_info = hi351_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(hi351_subdev_info),
	.sensor_v4l2_subdev_ops = &hi351_subdev_ops,
	.func_tbl = &hi351_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("SKHynix 3M YUV sensor driver");
MODULE_LICENSE("GPL v2");