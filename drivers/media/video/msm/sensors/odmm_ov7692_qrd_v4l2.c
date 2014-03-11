/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
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


/*
*	
*	20120402	lixujie modify it for EG808T front camera ov7692
*	20120511	lixujie modify it for EG808T front camera ov7692 effect/wb/iso
*	20120516	lixujie move file position  for EG808T front camera register sequence
*	20120628	lixujie modify it for EG808T Camera diag cmd test .
*	20120713	li.xujie@byd.com modify it for EG808T front Camera  ov7692 Code upgrade
*	20120722	li.xujie@byd.com modify it for EG808T front Camera  ov7692 image effect
*	20120807	li.xujie@byd.com modify it for EG808T front Camera  ov7692 preview/snap effect
*	20120813	li.xujie@byd.com modify it for EG808T front Camera  ov7692 preview/snap Orientation(EG808T_P004990)
*	20121218	li.xujie@byd.com modify it for WG451V front Camera preview/snap effect ( reduce noise )
*	20121220	li.xujie@byd.com modify it for WG451V front Camera match hw circuit
*
*/

//#include "odmm_ov7692_qrd_v4l2.h"
#include "msm_sensor.h"
#include <mach/vreg.h>
#include "msm.h"


#define SENSOR_NAME "ov7692"

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define ov7692_debug(fmt, arg... )		CDBG("[ ov7692-camera ] : " fmt, ## arg)

#define BYD_VCAM_AVDD_PWR_EN_PIN		35

//#define OV7692_VERBOSE_DGB

#ifdef OV7692_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif



//static  bool  FT_camera_test = false ;

DEFINE_MUTEX(ov7692_mut);

static struct msm_sensor_ctrl_t ov7692_s_ctrl;

static int effect_value = CAMERA_EFFECT_OFF;
static unsigned int SAT_U = 0x80; /* DEFAULT SATURATION VALUES*/
static unsigned int SAT_V = 0x80; /* DEFAULT SATURATION VALUES*/

static struct msm_camera_i2c_reg_conf ov7692_start_settings[] = {
	
	{0x0e, 0x00},
		
};

static struct msm_camera_i2c_reg_conf ov7692_stop_settings[] = {
	
	{0x0e, 0x08},
		
};

static struct msm_camera_i2c_reg_conf ov7692_recommend_settings_QRD[] = {

	{0x12, 0x80},
	{0x38, 0x10},	
	{0x0e, 0x08},
	{0x69, 0x52},
	{0x1e, 0xb3},
	{0x48, 0x42},
	{0xff, 0x01},
	{0xae, 0xa0},
	{0xa8, 0x26},
	{0xb4, 0xc0},
	{0xb5, 0x40},
	{0xff, 0x00},
	
	{0x0c, 0xc0},		//{0x0c, 0x00},
	{0x22, 0x20},	
	
	{0x62, 0x10},
	{0x12, 0x00},
	{0x17, 0x65},
	{0x18, 0xa4},
	{0x19, 0x0a},
	{0x1a, 0xf6},
	{0x3e, 0x30},
	{0x64, 0x0a},
	{0xff, 0x01},
	{0xb4, 0xc0},
	{0xff, 0x00},
	{0x67, 0x20},
	{0x81, 0x3f},
	{0xd0, 0x48},
	{0x82, 0x03},
	{0x70, 0x00},
	{0x71, 0x34},
	{0x74, 0x28},
	{0x75, 0x98},
	{0x76, 0x00},
	{0x77, 0x64},
	{0x78, 0x01},
	{0x79, 0xc2},
	{0x7a, 0x4e},
	{0x7b, 0x1f},
	{0x7c, 0x00},
	{0x11, 0x00},
	{0x20, 0x00},
	{0x21, 0x23},
	{0x50, 0x9a},
	{0x51, 0x80},
	{0x4c, 0x7d},
	{0x85, 0x10},
	{0x86, 0x00},
	{0x87, 0x00},
	{0x88, 0x00},
	
	{0x89, 0x35},		//{0x89, 0x2a},
	{0x8a, 0x31},		//{0x8a, 0x26},
	{0x8b, 0x2d},		//{0x8b, 0x22},
	
	{0xbb, 0x61},		//{0xbb, 0x7a},x0.8
	{0xbc, 0x54},		//{0xbc, 0x69},x0.8
	{0xbd, 0x0d},		//{0xbd, 0x11},x0.8
	{0xbe, 0x0f},		//{0xbe, 0x13},x0.8
	{0xbf, 0x67},		//{0xbf, 0x81},x0.8
	{0xc0, 0x78},		//{0xc0, 0x96},x0.8
	
	{0xc1, 0x1e},
	{0xb7, 0x05},
	{0xb8, 0x09},
	{0xb9, 0x00},
	{0xba, 0x18},
	{0x5a, 0x1f},
	{0x5b, 0x9f},
	{0x5c, 0x6a},
	{0x5d, 0x42},
	{0x24, 0x78},
	{0x25, 0x68},
	{0x26, 0xb3},
	{0xa3, 0x0b},
	{0xa4, 0x15},
	{0xa5, 0x2a},
	{0xa6, 0x51},
	{0xa7, 0x63},
	{0xa8, 0x74},
	{0xa9, 0x83},
	{0xaa, 0x91},
	{0xab, 0x9e},
	{0xac, 0xaa},
	{0xad, 0xbe},
	{0xae, 0xce},
	{0xaf, 0xe5},
	{0xb0, 0xf3},
	{0xb1, 0xfb},
	{0xb2, 0x06},
	{0x8c, 0x5c},
	{0x8d, 0x11},
	{0x8e, 0x12},
	{0x8f, 0x19},
	{0x90, 0x50},
	{0x91, 0x20},
	{0x92, 0x96},
	{0x93, 0x80},
	{0x94, 0x13},
	{0x95, 0x1b},
	{0x96, 0xff},
	{0x97, 0x00},
	{0x98, 0x3d},
	{0x99, 0x36},
	{0x9a, 0x51},
	{0x9b, 0x43},
	{0x9c, 0xf0},
	{0x9d, 0xf0},
	{0x9e, 0xf0},
	{0x9f, 0xff},
	{0xa0, 0x68},
	{0xa1, 0x62},
	{0xa2, 0x0e},

	//flowable frame rate 10~30fps	
	{0x14, 0x28},	
	{0x15, 0xa0},
	
};

static struct msm_camera_i2c_reg_conf ov7692_full_settings[] = {
	
	{0xcc, 0x02},
	{0xcd, 0x80},
	{0xce, 0x01},
	{0xcf, 0xe0},
	{0xc8, 0x02},
	{0xc9, 0x80},
	{0xca, 0x01},
	{0xcb, 0xe0},
	
};

static struct v4l2_subdev_info ov7692_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};


static struct msm_camera_i2c_conf_array ov7692_init_conf[] = {
	{&ov7692_recommend_settings_QRD[0],
	ARRAY_SIZE(ov7692_recommend_settings_QRD), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov7692_confs[] = {
	{&ov7692_full_settings[0],
	ARRAY_SIZE(ov7692_full_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};


/*********************************************************************
*
*	saturation config
*
**********************************************************************/

static struct msm_camera_i2c_reg_conf ov7692_saturation[][4] = {
	
	{{0x81, 0x33, 0xCC}, {0xd8, 0x00, 0x00}, {0xd9, 0x00, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL0*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x10, 0x00}, {0xd9, 0x10, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL1*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x20, 0x00}, {0xd9, 0x20, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL2*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x30, 0x00}, {0xd9, 0x30, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL3*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x40, 0x00}, {0xd9, 0x40, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL4*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x50, 0x00}, {0xd9, 0x50, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL5*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x60, 0x00}, {0xd9, 0x60, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL6*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x70, 0x00}, {0xd9, 0x70, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL7*/
		
	{{0x81, 0x33, 0xCC}, {0xd8, 0x80, 0x00}, {0xd9, 0x80, 0x00},
		{0xd2, 0x02, 0x00},},	/* SATURATION LEVEL8*/
		
};


static struct msm_camera_i2c_conf_array ov7692_saturation_confs[][1] = {
	
	{{ov7692_saturation[0], ARRAY_SIZE(ov7692_saturation[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[1], ARRAY_SIZE(ov7692_saturation[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[2], ARRAY_SIZE(ov7692_saturation[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[3], ARRAY_SIZE(ov7692_saturation[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[4], ARRAY_SIZE(ov7692_saturation[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[5], ARRAY_SIZE(ov7692_saturation[5]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[6], ARRAY_SIZE(ov7692_saturation[6]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[7], ARRAY_SIZE(ov7692_saturation[7]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_saturation[8], ARRAY_SIZE(ov7692_saturation[8]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
};

static int ov7692_saturation_enum_map[] = {
	
	MSM_V4L2_SATURATION_L0,
	MSM_V4L2_SATURATION_L1,
	MSM_V4L2_SATURATION_L2,
	MSM_V4L2_SATURATION_L3,
	MSM_V4L2_SATURATION_L4,
	MSM_V4L2_SATURATION_L5,
	MSM_V4L2_SATURATION_L6,
	MSM_V4L2_SATURATION_L7,
	MSM_V4L2_SATURATION_L8,
	
};


static struct msm_sensor_output_info_t ov7692_dimensions[] = {

	{
		.x_output = 0x280,		//640
		.y_output = 0x1E0,		//480
		.line_length_pclk = 0x290,	//656
		.frame_length_lines = 0x1EC,	//492
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},
	
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x290,
		.frame_length_lines = 0x1EC,
		.vt_pixel_clk = 9216000,
		.op_pixel_clk = 9216000,
		.binning_factor = 1,
	},
};


static struct msm_camera_i2c_enum_conf_array ov7692_saturation_enum_confs = {
	
	.conf = &ov7692_saturation_confs[0][0],
	.conf_enum = ov7692_saturation_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_saturation_enum_map),
	.num_index = ARRAY_SIZE(ov7692_saturation_confs),
	.num_conf = ARRAY_SIZE(ov7692_saturation_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	
};


/*********************************************************************
*
*	contrast config
*
**********************************************************************/

static struct msm_camera_i2c_reg_conf ov7692_contrast[][16] = {

	{{0xb2, 0x29}, {0xa3, 0x55}, {0xa4, 0x5b}, {0xa5, 0x67}, {0xa6, 0x7e},
		{0xa7, 0x89}, {0xa8, 0x93}, {0xa9, 0x9c}, {0xaa, 0xa4},
		{0xab, 0xac}, {0xac, 0xb3}, {0xad, 0xbe}, {0xae, 0xc7},
		{0xaf, 0xd5}, {0xb0, 0xdd}, {0xb1, 0xe1},},	/* CONTRAST L0*/
		
	{{0xb2, 0x20}, {0xa3, 0x43}, {0xa4, 0x4a}, {0xa5, 0x58}, {0xa6, 0x73},
		{0xa7, 0x80}, {0xa8, 0x8b}, {0xa9, 0x96}, {0xaa, 0x9f},
		{0xab, 0xa8}, {0xac, 0xb1}, {0xad, 0xbe}, {0xae, 0xc9},
		{0xaf, 0xd8}, {0xb0, 0xe2}, {0xb1, 0xe8},},	/* CONTRAST L1*/
		
	{{0xb2, 0x18}, {0xa3, 0x31}, {0xa4, 0x39}, {0xa5, 0x4a}, {0xa6, 0x68},
		{0xa7, 0x77}, {0xa8, 0x84}, {0xa9, 0x90}, {0xaa, 0x9b},
		{0xab, 0xa5}, {0xac, 0xaf}, {0xad, 0xbe}, {0xae, 0xca},
		{0xaf, 0xdc}, {0xb0, 0xe7}, {0xb1, 0xee},},	/* CONTRAST L2*/
		
	{{0xb2, 0x10}, {0xa3, 0x1f}, {0xa4, 0x28}, {0xa5, 0x3b}, {0xa6, 0x5d},
		{0xa7, 0x6e}, {0xa8, 0x7d}, {0xa9, 0x8a}, {0xaa, 0x96},
		{0xab, 0xa2}, {0xac, 0xad}, {0xad, 0xbe}, {0xae, 0xcc},
		{0xaf, 0xe0}, {0xb0, 0xed}, {0xb1, 0xf4},},	/* CONTRAST L3*/
		
	 {{0xb2, 0x6}, {0xa3, 0xb}, {0xa4, 0x15}, {0xa5, 0x2a}, {0xa6, 0x51},
		{0xa7, 0x63}, {0xa8, 0x74}, {0xa9, 0x83}, {0xaa, 0x91},
		{0xab, 0x9e}, {0xac, 0xaa}, {0xad, 0xbe}, {0xae, 0xce},
		{0xaf, 0xe5}, {0xb0, 0xf3}, {0xb1, 0xfb},},	/* CONTRAST L4*/
		
	{{0xb2, 0xc}, {0xa3, 0x4}, {0xa4, 0xc}, {0xa5, 0x1f}, {0xa6, 0x45},
		{0xa7, 0x58}, {0xa8, 0x6b}, {0xa9, 0x7c}, {0xaa, 0x8d},
		{0xab, 0x9d}, {0xac, 0xac}, {0xad, 0xc3}, {0xae, 0xd2},
		{0xaf, 0xe8}, {0xb0, 0xf2}, {0xb1, 0xf7},},	/* CONTRAST L5*/
		
	{{0xb2, 0x1}, {0xa3, 0x2}, {0xa4, 0x9}, {0xa5, 0x1a}, {0xa6, 0x3e},
		{0xa7, 0x4a}, {0xa8, 0x59}, {0xa9, 0x6a}, {0xaa, 0x79},
		{0xab, 0x8e}, {0xac, 0xa4}, {0xad, 0xc1}, {0xae, 0xdb},
		{0xaf, 0xf4}, {0xb0, 0xff}, {0xb1, 0xff},},	/* CONTRAST L6*/
		
	{{0xb2, 0xc}, {0xa3, 0x4}, {0xa4, 0x8}, {0xa5, 0x17}, {0xa6, 0x27},
		{0xa7, 0x3d}, {0xa8, 0x54}, {0xa9, 0x60}, {0xaa, 0x77},
		{0xab, 0x85}, {0xac, 0xa4}, {0xad, 0xc6}, {0xae, 0xd2},
		{0xaf, 0xe9}, {0xb0, 0xf0}, {0xb1, 0xf7},},	/* CONTRAST L7*/
		
	{{0xb2, 0x1}, {0xa3, 0x4}, {0xa4, 0x4}, {0xa5, 0x7}, {0xa6, 0xb},
		{0xa7, 0x17}, {0xa8, 0x2a}, {0xa9, 0x41}, {0xaa, 0x59},
		{0xab, 0x6b}, {0xac, 0x8b}, {0xad, 0xb1}, {0xae, 0xd2},
		{0xaf, 0xea}, {0xb0, 0xf4}, {0xb1, 0xff},},	/* CONTRAST L8*/
		
};



static struct msm_camera_i2c_conf_array ov7692_contrast_confs[][1] = {
	{{ov7692_contrast[0], ARRAY_SIZE(ov7692_contrast[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[1], ARRAY_SIZE(ov7692_contrast[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[2], ARRAY_SIZE(ov7692_contrast[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[3], ARRAY_SIZE(ov7692_contrast[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[4], ARRAY_SIZE(ov7692_contrast[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[5], ARRAY_SIZE(ov7692_contrast[5]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[6], ARRAY_SIZE(ov7692_contrast[6]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[7], ARRAY_SIZE(ov7692_contrast[7]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_contrast[8], ARRAY_SIZE(ov7692_contrast[8]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
};


static int ov7692_contrast_enum_map[] = {
	
	MSM_V4L2_CONTRAST_L0,
	MSM_V4L2_CONTRAST_L1,
	MSM_V4L2_CONTRAST_L2,
	MSM_V4L2_CONTRAST_L3,
	MSM_V4L2_CONTRAST_L4,
	MSM_V4L2_CONTRAST_L5,
	MSM_V4L2_CONTRAST_L6,
	MSM_V4L2_CONTRAST_L7,
	MSM_V4L2_CONTRAST_L8,
	
};

static struct msm_camera_i2c_enum_conf_array ov7692_contrast_enum_confs = {
	
	.conf = &ov7692_contrast_confs[0][0],
	.conf_enum = ov7692_contrast_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_contrast_enum_map),
	.num_index = ARRAY_SIZE(ov7692_contrast_confs),
	.num_conf = ARRAY_SIZE(ov7692_contrast_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	
};


/*********************************************************************
*
*	sharpness config
*
**********************************************************************/

static struct msm_camera_i2c_reg_conf ov7692_sharpness[][2] = {

	{{0xb4, 0x20, 0xDF}, {0xb6, 0x00, 0xE0},},    /* SHARPNESS LEVEL 0*/
	{{0xb4, 0x20, 0xDF}, {0xb6, 0x01, 0xE0},},    /* SHARPNESS LEVEL 1*/
	{{0xb4, 0x00, 0xDF}, {0xb6, 0x00, 0xE0},},    /* SHARPNESS LEVEL 2*/
	{{0xb4, 0x20, 0xDF}, {0xb6, 0x66, 0xE0},},    /* SHARPNESS LEVEL 3*/
	{{0xb4, 0x20, 0xDF}, {0xb6, 0x99, 0xE0},},    /* SHARPNESS LEVEL 4*/
	{{0xb4, 0x20, 0xDF}, {0xb6, 0xcc, 0xE0},},    /* SHARPNESS LEVEL 5*/
	
};


static struct msm_camera_i2c_conf_array ov7692_sharpness_confs[][1] = {
	
	{{ov7692_sharpness[0], ARRAY_SIZE(ov7692_sharpness[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_sharpness[1], ARRAY_SIZE(ov7692_sharpness[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_sharpness[2], ARRAY_SIZE(ov7692_sharpness[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_sharpness[3], ARRAY_SIZE(ov7692_sharpness[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_sharpness[4], ARRAY_SIZE(ov7692_sharpness[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_sharpness[5], ARRAY_SIZE(ov7692_sharpness[5]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
};

static int ov7692_sharpness_enum_map[] = {
	
	MSM_V4L2_SHARPNESS_L0,
	MSM_V4L2_SHARPNESS_L1,
	MSM_V4L2_SHARPNESS_L2,
	MSM_V4L2_SHARPNESS_L3,
	MSM_V4L2_SHARPNESS_L4,
	MSM_V4L2_SHARPNESS_L5,
	
};

static struct msm_camera_i2c_enum_conf_array ov7692_sharpness_enum_confs = {
	
	.conf = &ov7692_sharpness_confs[0][0],
	.conf_enum = ov7692_sharpness_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_sharpness_enum_map),
	.num_index = ARRAY_SIZE(ov7692_sharpness_confs),
	.num_conf = ARRAY_SIZE(ov7692_sharpness_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	
};



/*********************************************************************
*
*	exposure config
*
**********************************************************************/

static struct msm_camera_i2c_reg_conf ov7692_exposure[][3] = {


	{{0x24, 0x49}, {0x25, 0x41}, {0x26, 0x81},}, /*EXPOSURECOMPENSATIONN2*/
		
	{{0x24, 0x5a}, {0x25, 0x51}, {0x26, 0x92},}, /*EXPOSURECOMPENSATIONN1*/
	
	{{0x24, 0x70}, {0x25, 0x64}, {0x26, 0xc3},},/*EXPOSURECOMPENSATIOND*/
	
	{{0x24, 0x89}, {0x25, 0x7b}, {0x26, 0xd4},}, /*EXPOSURECOMPENSATIONp1*/
	
	{{0x24, 0xa8}, {0x25, 0x9b}, {0x26, 0xd4},}, /*EXPOSURECOMPENSATIONP2*/

};

static struct msm_camera_i2c_conf_array ov7692_exposure_confs[][1] = {
	
	{{ov7692_exposure[0], ARRAY_SIZE(ov7692_exposure[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_exposure[1], ARRAY_SIZE(ov7692_exposure[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_exposure[2], ARRAY_SIZE(ov7692_exposure[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_exposure[3], ARRAY_SIZE(ov7692_exposure[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_exposure[4], ARRAY_SIZE(ov7692_exposure[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
};

static int ov7692_exposure_enum_map[] = {
	
	MSM_V4L2_EXPOSURE_N2,
	MSM_V4L2_EXPOSURE_N1,
	MSM_V4L2_EXPOSURE_D,
	MSM_V4L2_EXPOSURE_P1,
	MSM_V4L2_EXPOSURE_P2,
	
};

static struct msm_camera_i2c_enum_conf_array ov7692_exposure_enum_confs = {
	
	.conf = &ov7692_exposure_confs[0][0],
	.conf_enum = ov7692_exposure_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_exposure_enum_map),
	.num_index = ARRAY_SIZE(ov7692_exposure_confs),
	.num_conf = ARRAY_SIZE(ov7692_exposure_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	
};


/*********************************************************************
*
*	exposure config
*
**********************************************************************/

static struct msm_camera_i2c_reg_conf ov7692_iso[][1] = {

	{{0x14, 0x20, 0x8F},},   /*ISO_AUTO*/
		
	{{0x14, 0x20, 0x8F},},   /*ISO_DEBLUR*/
	
	{{0x14, 0x00, 0x8F},},   /*ISO_100*/
	
	{{0x14, 0x10, 0x8F},},   /*ISO_200*/
	
	{{0x14, 0x20, 0x8F},},   /*ISO_400*/
	
	{{0x14, 0x30, 0x8F},},   /*ISO_800*/
	
	{{0x14, 0x40, 0x8F},},   /*ISO_1600*/
	
};


static struct msm_camera_i2c_conf_array ov7692_iso_confs[][1] = {
	
	{{ov7692_iso[0], ARRAY_SIZE(ov7692_iso[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_iso[1], ARRAY_SIZE(ov7692_iso[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_iso[2], ARRAY_SIZE(ov7692_iso[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_iso[3], ARRAY_SIZE(ov7692_iso[3]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_iso[4], ARRAY_SIZE(ov7692_iso[4]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_iso[5], ARRAY_SIZE(ov7692_iso[5]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
};

static int ov7692_iso_enum_map[] = {
	
	MSM_V4L2_ISO_AUTO ,
	MSM_V4L2_ISO_DEBLUR,
	MSM_V4L2_ISO_100,
	MSM_V4L2_ISO_200,
	MSM_V4L2_ISO_400,
	MSM_V4L2_ISO_800,
	MSM_V4L2_ISO_1600,
	
};


static struct msm_camera_i2c_enum_conf_array ov7692_iso_enum_confs = {
	
	.conf = &ov7692_iso_confs[0][0],
	.conf_enum = ov7692_iso_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_iso_enum_map),
	.num_index = ARRAY_SIZE(ov7692_iso_confs),
	.num_conf = ARRAY_SIZE(ov7692_iso_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
	
};


/*********************************************************************
*
*	effect  config
*
**********************************************************************/

static struct msm_camera_i2c_reg_conf ov7692_no_effect[] = {

	{0xd2, 0x06, }

};

static struct msm_camera_i2c_conf_array ov7692_no_effect_confs[] = {
	
	{&ov7692_no_effect[0],
	ARRAY_SIZE(ov7692_no_effect), 0,
	MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},
	
};


static struct msm_camera_i2c_reg_conf ov7692_special_effect[][5] = {
	{{0xd2, 0x06, }, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},	/*for special effect OFF*/
		
	{{0xd2, 0x1e,}, {0xda, 0x80,}, {0xdb, 0x80,}, {-1, -1, -1},
		{-1, -1, -1},},	/*for special effect MONO*/
		
	{{0xd2, 0x46, }, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},	/*for special efefct Negative*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/*Solarize is not supported by sensor*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},	/*for sepia*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/* Posteraize not supported */
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/* White board not supported*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/*Blackboard not supported*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/*Aqua not supported*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/*Emboss not supported */
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/*sketch not supported*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/*Neon not supported*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},		/*MAX value*/
};


static struct msm_camera_i2c_conf_array ov7692_special_effect_confs[][1] = {
	
	{{ov7692_special_effect[0],  ARRAY_SIZE(ov7692_special_effect[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[1],  ARRAY_SIZE(ov7692_special_effect[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[2],  ARRAY_SIZE(ov7692_special_effect[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[3],  ARRAY_SIZE(ov7692_special_effect[3]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[4],  ARRAY_SIZE(ov7692_special_effect[4]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[5],  ARRAY_SIZE(ov7692_special_effect[5]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[6],  ARRAY_SIZE(ov7692_special_effect[6]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[7],  ARRAY_SIZE(ov7692_special_effect[7]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[8],  ARRAY_SIZE(ov7692_special_effect[8]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[9],  ARRAY_SIZE(ov7692_special_effect[9]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[10], ARRAY_SIZE(ov7692_special_effect[10]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[11], ARRAY_SIZE(ov7692_special_effect[11]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_special_effect[12], ARRAY_SIZE(ov7692_special_effect[12]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
};

static int ov7692_special_effect_enum_map[] = {
	
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

static struct msm_camera_i2c_enum_conf_array
		 ov7692_special_effect_enum_confs = {
	.conf = &ov7692_special_effect_confs[0][0],
	.conf_enum = ov7692_special_effect_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_special_effect_enum_map),
	.num_index = ARRAY_SIZE(ov7692_special_effect_confs),
	.num_conf = ARRAY_SIZE(ov7692_special_effect_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};


static struct msm_camera_i2c_reg_conf ov7692_antibanding[][2] = {
	
	{{0x13, 0x20, 0xDF}, {0x14, 0x16, 0xE8},},   /*ANTIBANDING 60HZ*/
	{{0x13, 0x20, 0xDF}, {0x14, 0x17, 0xE8},},   /*ANTIBANDING 50HZ*/
	{{0x13, 0x20, 0xDF}, {0x14, 0x14, 0xE8},},   /* ANTIBANDING AUTO*/
	
};

static struct msm_camera_i2c_conf_array ov7692_antibanding_confs[][1] = {
	
	{{ov7692_antibanding[0], ARRAY_SIZE(ov7692_antibanding[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_antibanding[1], ARRAY_SIZE(ov7692_antibanding[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_antibanding[2], ARRAY_SIZE(ov7692_antibanding[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
};

static int ov7692_antibanding_enum_map[] = {
	
	MSM_V4L2_POWER_LINE_60HZ,
	MSM_V4L2_POWER_LINE_50HZ,
	MSM_V4L2_POWER_LINE_AUTO,
	
};


static struct msm_camera_i2c_enum_conf_array ov7692_antibanding_enum_confs = {
	.conf = &ov7692_antibanding_confs[0][0],
	.conf_enum = ov7692_antibanding_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_antibanding_enum_map),
	.num_index = ARRAY_SIZE(ov7692_antibanding_confs),
	.num_conf = ARRAY_SIZE(ov7692_antibanding_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov7692_wb_oem[][4] = {
	
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},/*WHITEBALNACE OFF*/
		
	{{0x13, 0xf7}, {0x15, 0x00,0xf0}, {-1, -1, -1},
		{-1, -1, -1},}, /*WHITEBALNACE AUTO*/
		
	{{0x13, 0xf5}, {0x01, 0x56}, {0x02, 0x50},
		{0x15, 0x00,0xf0},},	/*WHITEBALNACE CUSTOM*/
		
	{{0x13, 0xf5}, {0x01, 0x66}, {0x02, 0x40},
		{0x15, 0x00,0xf0},},	/*INCANDISCENT*/
		
	{{-1, -1, -1}, {-1, -1, -1}, {-1, -1, -1},
		{-1, -1, -1},},	/*FLOURESECT NOT SUPPORTED */
		
	{{0x13, 0xf5}, {0x01, 0x43}, {0x02, 0x5d},
		{0x15, 0x00,0xf0},},	/*DAYLIGHT*/
		
	{{0x13, 0xf5}, {0x01, 0x48}, {0x02, 0x63},
		{0x15, 0x00,0xf0},},	/*CLOUDY*/
};

static struct msm_camera_i2c_conf_array ov7692_wb_oem_confs[][1] = {
	
	{{ov7692_wb_oem[0], ARRAY_SIZE(ov7692_wb_oem[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_wb_oem[1], ARRAY_SIZE(ov7692_wb_oem[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_wb_oem[2], ARRAY_SIZE(ov7692_wb_oem[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_wb_oem[3], ARRAY_SIZE(ov7692_wb_oem[3]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_wb_oem[4], ARRAY_SIZE(ov7692_wb_oem[4]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_wb_oem[5], ARRAY_SIZE(ov7692_wb_oem[5]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
		
	{{ov7692_wb_oem[6], ARRAY_SIZE(ov7692_wb_oem[6]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7692_wb_oem_enum_map[] = {
	
	MSM_V4L2_WB_OFF,
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_CUSTOM,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
	
};

static struct msm_camera_i2c_enum_conf_array ov7692_wb_oem_enum_confs = {
	.conf = &ov7692_wb_oem_confs[0][0],
	.conf_enum = ov7692_wb_oem_enum_map,
	.num_enum = ARRAY_SIZE(ov7692_wb_oem_enum_map),
	.num_index = ARRAY_SIZE(ov7692_wb_oem_confs),
	.num_conf = ARRAY_SIZE(ov7692_wb_oem_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};


int ov7692_saturation_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	ov7692_debug(" %s : L(%d ) ...(Start : effect_value =%d ) value = %d : \n", __func__,__LINE__,effect_value,value);
	
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	if (value <= MSM_V4L2_SATURATION_L8)
		SAT_U = SAT_V = value * 0x10;
	
	CDBG("--CAMERA-- %s ...(End)\n", __func__);
	return rc;
}


int ov7692_contrast_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	ov7692_debug(" %s : L(%d ) ...(Start : effect_value =%d ) value = %d : \n", __func__,__LINE__,effect_value,value);
	
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}

	ov7692_debug(" %s : L(%d ) ...(End ) rc=%d  \n", __func__,__LINE__,rc);
	
	return rc;
}

int ov7692_sharpness_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	ov7692_debug(" %s : L(%d ) ...(Start : effect_value =%d ) value = %d : \n", __func__,__LINE__,effect_value,value);
	
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}

	ov7692_debug(" %s : L(%d ) ...(End ) rc=%d  \n", __func__,__LINE__,rc);
	
	return rc;
}

int ov7692_effect_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	ov7692_debug(" %s : L(%d ) ...(Start : effect_value =%d ) value = %d : \n", __func__,__LINE__,effect_value,value);
	
	effect_value = value;
	
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->no_effect_settings, 0);
		if (rc < 0) {
			pr_err("%s : L(%d ) write faield \n",__func__,__LINE__);
			return rc;
		}
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xda, SAT_U,
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xdb, SAT_V,
			MSM_CAMERA_I2C_BYTE_DATA);
	} else {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}

	ov7692_debug(" %s : L(%d ) ...(End ) rc=%d  \n", __func__,__LINE__,rc);
	
	return rc;
	
}

int ov7692_antibanding_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	ov7692_debug(" %s : L(%d ) value = %d : Not supported \n", __func__,__LINE__,value);
	
	return rc;
	
}

int ov7692_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;

	ov7692_debug(" %s : L(%d ) ...(Start) value = %d : \n", __func__,__LINE__,value);
	
	rc = msm_sensor_write_enum_conf_array(
		s_ctrl->sensor_i2c_client,
		ctrl_info->enum_cfg_settings, value);
	if (rc < 0) {
		pr_err("%s : L(%d ) write faield \n",__func__,__LINE__);
		return rc;
	}

	ov7692_debug(" %s : L(%d ) ...(End ) rc=%d  \n", __func__,__LINE__,rc);
	
	return rc;
}


static int ov7692_odmm_set_exposure_compensation(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)

{

	int rc = 0;
	
	ov7692_debug(" %s : L(%d ) ...(Start) value = %d : \n", __func__,__LINE__,value);
	
	rc = ov7692_msm_sensor_s_ctrl_by_enum(s_ctrl,ctrl_info,  value);
	if (rc < 0) {
		pr_err("%s : L(%d ) set exposure compensation faield \n",__func__,__LINE__);
	}
	
	return  rc ;
	
}


static int ov7692_odmm_set_iso(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)

{

	int rc = 0;

	ov7692_debug(" %s : L(%d ) ...(Start) value = %d : \n", __func__,__LINE__,value);
	
	rc = ov7692_msm_sensor_s_ctrl_by_enum(s_ctrl,ctrl_info,  value);
	if (rc < 0) {
		pr_err("%s : L(%d ) set iso faield \n",__func__,__LINE__);		
	}
	
	return  rc ;

}

static int ov7692_odmm_set_wb_oem(struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)

{

	int rc = 0;

	ov7692_debug(" %s : L(%d ) ...(Start) value = %d : \n", __func__,__LINE__,value);
	
	rc = ov7692_msm_sensor_s_ctrl_by_enum(s_ctrl,ctrl_info,  value);
	if (rc < 0) {
		pr_err("%s : L(%d ) set wb oem faield  \n",__func__,__LINE__);
	}
	
	return  rc ;

}


struct msm_sensor_v4l2_ctrl_info_t ov7692_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_SATURATION,
		.min = MSM_V4L2_SATURATION_L0,
		.max = MSM_V4L2_SATURATION_L8,
		.step = 1,
		.enum_cfg_settings = &ov7692_saturation_enum_confs,
		.s_v4l2_ctrl = ov7692_saturation_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_CONTRAST,
		.min = MSM_V4L2_CONTRAST_L0,
		.max = MSM_V4L2_CONTRAST_L8,
		.step = 1,
		.enum_cfg_settings = &ov7692_contrast_enum_confs,
		.s_v4l2_ctrl = ov7692_contrast_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SHARPNESS,
		.min = MSM_V4L2_SHARPNESS_L0,
		.max = MSM_V4L2_SHARPNESS_L5,
		.step = 1,
		.enum_cfg_settings = &ov7692_sharpness_enum_confs,
		.s_v4l2_ctrl = ov7692_sharpness_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_EXPOSURE,
		.min = MSM_V4L2_EXPOSURE_N2,
		.max = MSM_V4L2_EXPOSURE_P2,
		.step = 1,
		.enum_cfg_settings = &ov7692_exposure_enum_confs,
		.s_v4l2_ctrl = ov7692_odmm_set_exposure_compensation,
	},
	{
		.ctrl_id = MSM_V4L2_PID_ISO,
		.min = MSM_V4L2_ISO_AUTO,
		.max = MSM_V4L2_ISO_1600,
		.step = 1,
		.enum_cfg_settings = &ov7692_iso_enum_confs,
		.s_v4l2_ctrl = ov7692_odmm_set_iso,
	},
	{
		.ctrl_id = V4L2_CID_SPECIAL_EFFECT,
		.min = MSM_V4L2_EFFECT_OFF,
		.max = MSM_V4L2_EFFECT_NEGATIVE,
		.step = 1,
		.enum_cfg_settings = &ov7692_special_effect_enum_confs,
		.s_v4l2_ctrl = ov7692_effect_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_POWER_LINE_FREQUENCY,
		.min = MSM_V4L2_POWER_LINE_60HZ,
		.max = MSM_V4L2_POWER_LINE_AUTO,
		.step = 1,
		.enum_cfg_settings = &ov7692_antibanding_enum_confs,
		.s_v4l2_ctrl = ov7692_antibanding_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.min = MSM_V4L2_WB_OFF,
		.max = MSM_V4L2_WB_CLOUDY_DAYLIGHT,
		.step = 1,
		.enum_cfg_settings = &ov7692_wb_oem_enum_confs,
		.s_v4l2_ctrl = ov7692_odmm_set_wb_oem,
	},

};


static struct msm_camera_csi_params ov7692_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x14,
};

static struct msm_camera_csi_params *ov7692_csi_params_array[] = {
	&ov7692_csi_params,
	&ov7692_csi_params,
};

static struct msm_sensor_output_reg_addr_t ov7692_reg_addr = {
	.x_output = 0xCC,
	.y_output = 0xCE,
	.line_length_pclk = 0xC8,
	.frame_length_lines = 0xCA,
};

static struct msm_sensor_id_info_t ov7692_id_info = {
	.sensor_id_reg_addr = 0x0A,
	.sensor_id = 0x7692,
};


static const struct i2c_device_id ov7692_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov7692_s_ctrl},
	{ }
};

static int ov7692_pwdn_gpio;
static int ov7692_reset_gpio;

static int ov7692_probe_init_gpio(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	
	ov7692_debug( "%s , (at %d ) start \n",__func__,__LINE__);

	ov7692_pwdn_gpio = data->sensor_pwd;

	if (data->sensor_reset_enable){
		
		ov7692_reset_gpio = data->sensor_reset ;
		CDBG("%s: reset_gpio:%d\n", __func__, ov7692_reset_gpio);
		
	}
	
	CDBG("%s: pwdn_gpio:%d \n", __func__, ov7692_pwdn_gpio);


	if (data->sensor_reset_enable){
		
		ov7692_debug( "%s , (at %d ) set reset_gpio direction : 1 \n",__func__,__LINE__);
		gpio_direction_output(data->sensor_reset, 0);
		
	}

	gpio_direction_output(data->sensor_pwd, 1);

	return rc;

}

static int32_t ov7692_vcam_vdio_power_up(void)
{
	 struct vreg *vreg_L19_regulator;
	 int32_t rc = 0;

	 CDBG(" start %s\n",__func__);

	 usleep_range(10000, 11000);
	 
	 vreg_L19_regulator = vreg_get(NULL , "ldo19");
	 
	 if (IS_ERR(vreg_L19_regulator)) {
		 pr_err("%s: vreg get failed with : (%ld)\n",
			 __func__, PTR_ERR(vreg_L19_regulator));
		 return -EINVAL;
	 }
	
	 /* Set the voltage level to 1.8V */
	 rc = vreg_set_level(vreg_L19_regulator, 1800);
	 if (rc < 0) {
		 pr_err("%s: set regulator level failed with :(%d)\n",
			 __func__, rc);
		 return -EINVAL;
	 }
	 rc = vreg_enable(vreg_L19_regulator);

	  usleep_range(2000, 2100);
	 
	 return rc;

}

static int32_t ov7692_vcam_vdio_power_down(void)
{
	struct vreg *vreg_L19_regulator;
	int32_t rc = 0;
	
	CDBG(" start %s\n",__func__);
	
	 usleep_range(2000, 2100);
	
	vreg_L19_regulator = vreg_get(NULL , "ldo19");
	if (IS_ERR(vreg_L19_regulator)) {
		  pr_err("%s: vreg get failed with : (%ld)\n",
			  __func__, PTR_ERR(vreg_L19_regulator));
		  return -EINVAL;
	}
	 rc = vreg_disable(vreg_L19_regulator);
	
	return rc;


}


static void ov7692_vcam_avdd_power_up(void)
{
	int32_t rc = 0;
	
	ov7692_debug( "%s , (at %d ) start \n",__func__,__LINE__);

	rc = gpio_request(BYD_VCAM_AVDD_PWR_EN_PIN, "ov7692");
	if (rc < 0){
		pr_err("%s: gpio_request---BYD_VCAM_AVDD_PWR_EN_PIN failed!",__func__);
	}
	
 	rc = gpio_tlmm_config(GPIO_CFG(BYD_VCAM_AVDD_PWR_EN_PIN, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to config BYD_VCAM_AVDD_PWR_EN_PIN !\n",__func__);
		gpio_free(BYD_VCAM_AVDD_PWR_EN_PIN);
	}


 	gpio_direction_output(BYD_VCAM_AVDD_PWR_EN_PIN, 1);
	


}


static void ov7692_vcam_avdd_power_down(void)
{
	int32_t rc = 0;
	
	ov7692_debug( "%s , (at %d ) start \n",__func__,__LINE__);

	 usleep_range(2000, 2100);

	rc = gpio_tlmm_config(GPIO_CFG(BYD_VCAM_AVDD_PWR_EN_PIN, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to config BYD_VCAM_AVDD_PWR_EN_PIN!\n",__func__);
	}

	gpio_direction_output(BYD_VCAM_AVDD_PWR_EN_PIN, 0);
	gpio_free(BYD_VCAM_AVDD_PWR_EN_PIN);

}


int32_t ov7692_sensor_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{

	int32_t rc = 0;

	struct msm_sensor_ctrl_t *s_ctrl;

	ov7692_debug( "%s , L(%d)  start \n",__func__,__LINE__);
	
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

static struct i2c_driver ov7692_i2c_driver = {
	.id_table = ov7692_i2c_id,
	.probe  = ov7692_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov7692_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_BYTE_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int rc = 0;
	CDBG("OV7692\n");

	rc = i2c_add_driver(&ov7692_i2c_driver);

	return rc;
}

static struct v4l2_subdev_core_ops ov7692_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov7692_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov7692_subdev_ops = {
	.core = &ov7692_subdev_core_ops,
	.video  = &ov7692_subdev_video_ops,
};


#if		0

static int32_t ov7692_i2c_txdata(struct i2c_client *ov7692_client,
		unsigned short saddr,
		unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = 2,
			.buf = txdata,
		},
	};
	if (i2c_transfer(ov7692_client->adapter, msg, 1) < 0) {
		pr_err("ov7692_i2c_txdata faild 0x%x\n", ov7692_client->addr);
		return -EIO;
	}

	return 0;
}

static int32_t ov7692_i2c_write_b_sensor(struct i2c_client *ov7692_client,
		uint8_t waddr,
		uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[2];

	memset(buf, 0, sizeof(buf));
	buf[0] = waddr;
	buf[1] = bdata;
	CDBG("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
	rc = ov7692_i2c_txdata(ov7692_client, ov7692_client->addr >> 1, buf, 2);
	if (rc < 0)
		pr_err("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
		waddr, bdata);

	return rc;
}

#endif

int32_t ov7692_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;

	
	printk("[ ov762_debug ] %s, L(%d) \n", __func__, __LINE__);

	info = s_ctrl->sensordata;

	/* camera PowerDown Pin active  */
	rc = ov7692_probe_init_gpio(info);
	if (rc < 0) {
		pr_err("%s: gpio init failed\n", __func__);
		goto power_up_fail;
	}
	usleep_range(10000, 11000);
	
	/* camera DOVDD Power up  */
	ov7692_vcam_vdio_power_up();	

	/* camera AVDD Power up  */
	ov7692_vcam_avdd_power_up();
	
	usleep_range(5000, 5100);
	gpio_direction_output(info->sensor_pwd, 0);

		/* camera CLK Power up  */
	rc = msm_sensor_power_up(s_ctrl);
	if (rc < 0) {
		pr_err("%s: msm_sensor_power_up failed\n", __func__);
		return rc;
	}

	usleep_range(5000, 5100);

	if (info->sensor_reset_enable){
		gpio_direction_output(info->sensor_reset, 1);
	}
	
	usleep_range(25000, 25100);	//msleep(25);
	   
	printk("lxj [ ov762_debug ] %s: %d ,success power up \n", __func__, __LINE__);

	return rc;

power_up_fail:
	
	printk("ov7692_sensor_power_up: OV7692 SENSOR POWER UP FAILS!\n");

	return rc;

}


int32_t ov7692_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{

	int32_t rc = 0;
	struct msm_camera_sensor_info *info = NULL;

	CDBG("%s IN\r\n", __func__);
	info = s_ctrl->sensordata;

	msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
			0x38, 0x50, MSM_CAMERA_I2C_BYTE_DATA);
	
	usleep_range(80000, 80100);		//msleep(80);
	   
	/* camera PowerDown Pin active  */
	gpio_direction_output(info->sensor_pwd, 1);
	
	usleep_range(40000, 40100);		//msleep(40);
	
		/* camera AVDD Power down  */
	ov7692_vcam_avdd_power_down();
		
  		/* camera DOVDD Power down  */
	ov7692_vcam_vdio_power_down();
		
	 rc = msm_sensor_power_down(s_ctrl);

	usleep_range(40000, 40100);		//msleep(40);

	 ov7692_debug( "%s ,L(%d) exit ( %d )\n",__func__,__LINE__,rc);
	 
	 return rc;
}


/*
static void ov7692_odmm_stream_switch(struct msm_sensor_ctrl_t * senosr_ctrl,int bOn)
{
    uint16_t reg_data ;
	
    ov7692_debug("%s : begin\n", __func__);
	
    if (bOn == 0)
    {
    
        ov7692_debug("ov7692_stream_switch: off \n");
        msm_camera_i2c_read(senosr_ctrl->sensor_i2c_client,0x38,&reg_data,MSM_CAMERA_I2C_BYTE_DATA);
	 reg_data|= 0x60;
        msm_camera_i2c_write(senosr_ctrl->sensor_i2c_client,0x38,reg_data,MSM_CAMERA_I2C_BYTE_DATA);
		
    }
    else 
    {
    
        ov7692_debug("ov7692_stream_switch: on \n");
         msm_camera_i2c_read(senosr_ctrl->sensor_i2c_client,0x38,&reg_data,MSM_CAMERA_I2C_BYTE_DATA);
 	 reg_data&= 0x9F;
        msm_camera_i2c_write(senosr_ctrl->sensor_i2c_client,0x38,reg_data,MSM_CAMERA_I2C_BYTE_DATA);
		
    }
	
    ov7692_debug("%s : end\n", __func__);
	
}
*/
/*
static long ov7692_odmm_set_factory_test_params( struct msm_sensor_ctrl_t * senosr_ctrl, int factorytest)
{
	int rc = 0;	

	struct msm_camera_i2c_reg_conf  ov7692_ft_mtf_tbl[] =
		{
			{0x80,0x77},		//	unit gamma, len enable
			
			{0xb4,0x26},		// 	sharpness,smooth disable
			{0xb6,0x00},

			{0x13,0xf7},		//	AE/AG/AWB enable				
		};

		struct msm_camera_i2c_reg_conf  ov7692_ft_Blemish_on_tbl[] =
		{
			{0x80,0x7e},	
				
			{0xb4,0x06},		//	sharpness,enable
			{0xb6,0x02},

			{0x13,0xf2},		//	manual AE/AG

			//{0x0f,0x00},		//  control exposure
			{0x10,0x1f},

			//{0x15,0x00},		//  control gain
			{0x00,0x1f},	
		};

		struct msm_camera_i2c_reg_conf  ov7692_resume_default_tbl[] =
		{
			{0x80,0x7e},	
			{0xb4,0xc0},		 
			{0xb6,0x04},
			{0x13,0xe5},
			{0x10,0x00},
			{0x00,0x00},			
		};

 		struct msm_camera_i2c_reg_conf  ov7692_ftcamera_start[] =
		{
			{0x0c, 0x80},		//{0x0c, 0x00},
			{0x22, 0x00},	

 		};

		 struct msm_camera_i2c_reg_conf  ov7692_ftcamera_stop[] =
		{
			{0x0c, 0xc0},		//{0x0c, 0x00},
			{0x22, 0x20},	

 		};
		
	printk("  %s : L(%d ) ...(Start) factorytest =%d\n", __func__,__LINE__,factorytest);

	ov7692_odmm_stream_switch(senosr_ctrl,0);
	
	usleep_range(20000, 20100);
	
	switch (factorytest) {

		case 0:		//FT CAMERA TEST START
			
			FT_camera_test = true ;
			
			rc = msm_camera_i2c_write_tbl(senosr_ctrl->sensor_i2c_client, ov7692_ftcamera_start,
												ARRAY_SIZE(ov7692_ftcamera_start), MSM_CAMERA_I2C_BYTE_DATA);
			
			break;

		case 1:	     //FT CAMERA TEST STOP
			
			FT_camera_test = false ;
			
			rc = msm_camera_i2c_write_tbl(senosr_ctrl->sensor_i2c_client, ov7692_ftcamera_stop,
												ARRAY_SIZE(ov7692_ftcamera_stop), MSM_CAMERA_I2C_BYTE_DATA);
			
			break;
			
		case 2:	     //FT CAMERA TEST MTF
			
			rc = msm_camera_i2c_write_tbl(senosr_ctrl->sensor_i2c_client, ov7692_ft_mtf_tbl,
												ARRAY_SIZE(ov7692_ft_mtf_tbl), MSM_CAMERA_I2C_BYTE_DATA);
			
			break;
			
 		case 4:	     //FT CAMERA TEST Blemish
			
        		rc = msm_camera_i2c_write_tbl(senosr_ctrl->sensor_i2c_client, ov7692_ft_Blemish_on_tbl,
												ARRAY_SIZE(ov7692_ft_Blemish_on_tbl), MSM_CAMERA_I2C_BYTE_DATA);
				
        		break;
		
    		default:

			 rc = msm_camera_i2c_write_tbl(senosr_ctrl->sensor_i2c_client, ov7692_resume_default_tbl,
												ARRAY_SIZE(ov7692_resume_default_tbl), MSM_CAMERA_I2C_BYTE_DATA);
        		break;
				
    	}
	
	ov7692_odmm_stream_switch(senosr_ctrl,1);
	
	usleep_range(20000, 20100);
	
    return rc;
		
}

*/
int ov7692_qrd_v4l2_sensor_config (struct msm_sensor_ctrl_t * senosr_ctrl,  struct sensor_cfg_data cdata)
{

	int32_t rc = 0;
	
	ov7692_debug( "%s , L(%d)  start : cdata.cfgtype =%d \n",__func__,__LINE__,cdata.cfgtype);

	switch (cdata.cfgtype) {
	/*	
	case CFG_SET_FACTORY_TEST_PARAMS:

		ov7692_debug(" %s : L(%d )  ...CFG_SET_FACTORY_TEST_PARAMS \n", __func__,__LINE__);
		
		rc = ov7692_odmm_set_factory_test_params(senosr_ctrl,cdata.cfg.factorytest);
		
	   	break;
	*/	
	default:
	
		pr_err("--[ ov7692-camera ] :-- %s : L(%d )  ...Default (Not Support cfgtype: %d )\n", __func__,__LINE__,cdata.cfgtype);
		
		break;
	}

	return rc;
	
}



static int32_t ov7692_cis_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;

	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	
	usleep_range(30000, 30100);	//msleep(30);
	
	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);
		msm_sensor_write_conf_array(
			s_ctrl->sensor_i2c_client,
			s_ctrl->msm_sensor_reg->mode_settings, res);
		
		usleep_range(30000, 30100);	//msleep(30);
		
		if (!csi_config) {
			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			
			usleep_range(30000, 30100);	//msleep(30);
			
			csi_config = 1;
		}
		v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
			NOTIFY_PCLK_CHANGE,
			&s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);

		s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
		
		usleep_range(50000, 50100);	//msleep(50);
	}
	
	return rc;
	
}


static struct msm_sensor_fn_t ov7692_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_csi_setting = ov7692_cis_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov7692_sensor_power_up,
	.sensor_power_down = ov7692_sensor_power_down,
	//.odmm_sensor_config= ov7692_qrd_v4l2_sensor_config,
};

static struct msm_sensor_reg_t ov7692_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov7692_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov7692_start_settings),
	.stop_stream_conf = ov7692_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov7692_stop_settings),
	.init_settings = &ov7692_init_conf[0],
	.init_size = ARRAY_SIZE(ov7692_init_conf),
	.mode_settings = &ov7692_confs[0],
	.no_effect_settings = &ov7692_no_effect_confs[0],
	.output_settings = &ov7692_dimensions[0],
	.num_conf = ARRAY_SIZE(ov7692_confs),
};

static struct msm_sensor_ctrl_t ov7692_s_ctrl = {
	.msm_sensor_reg = &ov7692_regs,
	.msm_sensor_v4l2_ctrl_info = ov7692_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(ov7692_v4l2_ctrl_info),
	.sensor_i2c_client = &ov7692_sensor_i2c_client,
	.sensor_i2c_addr = 0x78,
	.sensor_output_reg_addr = &ov7692_reg_addr,
	.sensor_id_info = &ov7692_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &ov7692_csi_params_array[0],
	.msm_sensor_mutex = &ov7692_mut,
	.sensor_i2c_driver = &ov7692_i2c_driver,
	.sensor_v4l2_subdev_info = ov7692_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov7692_subdev_info),
	.sensor_v4l2_subdev_ops = &ov7692_subdev_ops,
	.func_tbl = &ov7692_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision VGA YUV sensor driver");
MODULE_LICENSE("GPL v2");
