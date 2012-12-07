/* Copyright (c) 2012, The Linux Foundation. All Rights Reserved.
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

#include "msm_sensor.h"
#define SENSOR_NAME "ov7695"

DEFINE_MUTEX(ov7695_mut);
static struct msm_sensor_ctrl_t ov7695_s_ctrl;
static int effect_value = CAMERA_EFFECT_OFF;
static unsigned int SAT_U = 0x80; /* DEFAULT SATURATION VALUES*/
static unsigned int SAT_V = 0x80; /* DEFAULT SATURATION VALUES*/

static struct msm_camera_i2c_reg_conf ov7695_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf ov7695_stop_settings[] = {
	{0x0100, 0x00},
};

static struct msm_camera_i2c_reg_conf ov7695_recommend_settings[] = {
	{0x0103, 0x01}, //software reset
	{0x3620, 0x2f},
	{0x3623, 0x12},
	{0x3718, 0x88},
	{0x3631, 0x44},
	{0x3632, 0x05},
	{0x3013, 0xd0},
	{0x3705, 0x1d},
	{0x3713, 0x0e},
	{0x3012, 0x0a},
	{0x3717, 0x19},
	{0x0309, 0x24}, //DAC clk div by 4
	{0x3820, 0x90},
	{0x0101, 0x02}, //mirror off, flip on
	{0x5100, 0x01}, //lenc
	{0x520a, 0xf4}, //red gain from 0x400 to 0xfff
	{0x520b, 0xf4}, //green gain from 0x400 to 0xfff
	{0x520c, 0xf4}, //blue gain from 0x400 to 0xfff
	{0x3a18, 0x01}, //gain ceiling 0x100
	{0x3a19, 0x00}, //gain ceiling
	{0x3503, 0x03}, //AGC manual on, AEC manual on
	{0x3500, 0x00}, //exposure
	{0x3501, 0x21}, //exposure
	{0x3502, 0x00}, //exposure
	{0x350a, 0x00}, //gain
	{0x350b, 0x00}, //gain
	{0x4008, 0x02}, //bl start
	{0x4009, 0x09}, //bl end
	{0x3002, 0x09}, //FSIN output
	{0x3024, 0x00},
	{0x3503, 0x00}, //AGC auto on, AEC auto on
	//OV7695_ISP
	{0x0101, 0x02}, //mirror off, flip on
	{0x5002, 0x40}, //[7:6] Y source select, manual 60Hz
	{0x5910, 0x00}, //Y formula
	{0x3a0f, 0x50}, //AEC in H
	{0x3a10, 0x48}, //38 ;AEC in L
	{0x3a1b, 0x50}, //40 ;AEC out H
	{0x3a1e, 0x48}, //36 ;AEC out L
	{0x3a11, 0x90}, //80 ;control zone H
	{0x3a1f, 0x20}, //18 ;control zone L
	{0x3a18, 0x00}, //gain ceiling
	{0x3a19, 0xf8}, //gain ceiling, max gain 15.5x
	{0x3503, 0x00}, //aec/agc auto on
	{0x5000, 0xff}, //lcd, gma, awb, awbg, bc, wc, lenc, isp
	{0x5001, 0x3f}, //avg, blc, sde, uv_avg, cmx, cip
	//lens
	{0x5100, 0x01},
	{0x5101, 0xbf},
	{0x5102, 0x00},
	{0x5103, 0xaa},
	{0x5104, 0x3f},
	{0x5105, 0x05},
	{0x5106, 0xff},
	{0x5107, 0x0f},
	{0x5108, 0x01},
	{0x5109, 0xff},
	{0x510a, 0x00},
	{0x510b, 0x72},
	{0x510c, 0x45},
	{0x510d, 0x06},
	{0x510e, 0xff},
	{0x510f, 0x0f},
	{0x5110, 0x01},
	{0x5111, 0xfe},
	{0x5112, 0x00},
	{0x5113, 0x70},
	{0x5114, 0x21},
	{0x5115, 0x05},
	{0x5116, 0xff},
	{0x5117, 0x0f},
	//AWB
	{0x520a, 0x74}, //red gain from 0x400 to 0x7ff
	{0x520b, 0x64}, //green gain from 0x400 to 0x7ff
	{0x520c, 0xd4}, //blue gain from 0x400 to 0xdff
	//Gamma
	{0x5301, 0x07},
	{0x5302, 0x0e},
	{0x5303, 0x1d},
	{0x5304, 0x3a},
	{0x5305, 0x45},
	{0x5306, 0x54},
	{0x5307, 0x60},
	{0x5308, 0x6c},
	{0x5309, 0x7a},
	{0x530a, 0x84},
	{0x530b, 0x97},
	{0x530c, 0xa9},
	{0x530d, 0xc8},
	{0x530e, 0xdc},
	{0x530f, 0xef},
	{0x5310, 0x16},
	//sharpen/denoise
	{0x5500, 0x08}, //sharp th1 8x
	{0x5501, 0x48}, //sharp th2 8x
	{0x5502, 0x12}, //sharp mt offset1
	{0x5503, 0x03}, //sharp mt offset2
	{0x5504, 0x08}, //dns th1 8x
	{0x5505, 0x48}, //dns th2 8x
	{0x5506, 0x02}, //dns offset1
	{0x5507, 0x16}, //dns offset2
	{0x5508, 0xad}, // //[6]:sharp_man [4]:dns_man
	{0x5509, 0x08}, //sharpth th1 8x
	{0x550a, 0x48}, //sharpth th2 8x
	{0x550b, 0x06}, //sharpth offset1
	{0x550c, 0x04}, //sharpth offset2
	{0x550d, 0x01}, //recursive_en
	//SDE, for saturation 120% under D65
	{0x5800, 0x06}, //saturation on, contrast on
	{0x5803, 0x2e}, //40 ; sat th2
	{0x5804, 0x20}, //34 ; sat th1
	{0x580b, 0x02}, //Y offset man on
	//CMX QE
	{0x5600, 0x00}, //mtx 1.7, UV CbCr disable
	{0x5601, 0x2c}, //CMX1
	{0x5602, 0x5a}, //CMX2
	{0x5603, 0x06}, //CMX3
	{0x5604, 0x1c}, //CMX4
	{0x5605, 0x65}, //CMX5
	{0x5606, 0x81}, //CMX6
	{0x5607, 0x9f}, //CMX7
	{0x5608, 0x8a}, //CMX8
	{0x5609, 0x15}, //CMX9
	{0x560a, 0x01}, //Sign
	{0x560b, 0x9c}, //Sign
	{0x3811, 0x07}, //Tradeoff position to make YUV/RAW x VGA/QVGA x Mirror/Flip all work
	{0x3813, 0x06},
	{0x3a05, 0xb0}, //banding filter 50hz
	//MIPI
	{0x4800, 0x20},
	{0x4801, 0x0e},
	{0x4802, 0x14},
	{0x4803, 0x0a},
	{0x4804, 0x0a},
	{0x4805, 0x0a},
	{0x4806, 0x30},
	{0x4807, 0x05},
	{0x0100, 0x01}, //streaming
};

static struct msm_camera_i2c_reg_conf ov7695_full_settings[] = {
	{0x034c, 0x02},
	{0x034d, 0x80},
	{0x034e, 0x01},
	{0x034f, 0xe0},
	{0x0340, 0x02},
	{0x0341, 0x18},
	{0x0342, 0x02},
	{0x0343, 0xea},
};


static struct v4l2_subdev_info ov7695_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_YUYV8_2X8,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
	/* more can be supported, to be added later */
};


static struct msm_camera_i2c_conf_array ov7695_init_conf[] = {
	{&ov7695_recommend_settings[0],
	ARRAY_SIZE(ov7695_recommend_settings), 0, MSM_CAMERA_I2C_BYTE_DATA}
};


static struct msm_camera_i2c_conf_array ov7695_confs[] = {
	{&ov7695_full_settings[0],
	ARRAY_SIZE(ov7695_full_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_sensor_output_info_t ov7695_dimensions[] = {
	{
		.x_output = 0x280,
		.y_output = 0x1E0,
		.line_length_pclk = 0x2EA,
		.frame_length_lines = 0x218,
		.vt_pixel_clk = 12000000,
		.op_pixel_clk = 9216000,
		.binning_factor = 0,
	},
};

static struct msm_camera_i2c_reg_conf ov7695_saturation[][6] = {
	{{0x5604, 0x13}, {0x5605, 0x44},
		{0x5606, 0x57}, {0x5607, 0x6c},
		{0x5608, 0x5e}, {0x5609, 0x0e},},/* SATURATION LEVEL0*/
	{{0x5604, 0x15}, {0x5605, 0x4b},
		{0x5606, 0x60}, {0x5607, 0x76},
		{0x5608, 0x67}, {0x5609, 0x0f},},/* SATURATION LEVEL1*/
	{{0x5604, 0x17}, {0x5605, 0x53},
		{0x5606, 0x6a}, {0x5607, 0x83},
		{0x5608, 0x72}, {0x5609, 0x11},},/* SATURATION LEVEL2*/
	{{0x5604, 0x19}, {0x5605, 0x5b},
		{0x5606, 0x74}, {0x5607, 0x90},
		{0x5608, 0x7d}, {0x5609, 0x13},},/* SATURATION LEVEL3*/
	{{0x5604, 0x1c}, {0x5605, 0x65},
		{0x5606, 0x81}, {0x5607, 0x9f},
		{0x5608, 0x8a}, {0x5609, 0x15},},/* SATURATION LEVEL4*/
	{{0x5604, 0x1e}, {0x5605, 0x6f},
		{0x5606, 0x8d}, {0x5607, 0xae},
		{0x5608, 0x97}, {0x5609, 0x17},},/* SATURATION LEVEL5*/
	{{0x5604, 0x21}, {0x5605, 0x7a},
		{0x5606, 0x9b}, {0x5607, 0xbf},
		{0x5608, 0xa6}, {0x5609, 0x19},},/* SATURATION LEVEL6*/
	{{0x5604, 0x25}, {0x5605, 0x86},
		{0x5606, 0xab}, {0x5607, 0xd2},
		{0x5608, 0xb7}, {0x5609, 0x1b},},/* SATURATION LEVEL7*/
	{{0x5604, 0x28}, {0x5605, 0x93},
		{0x5606, 0xbb}, {0x5607, 0xe8},
		{0x5608, 0xca}, {0x5609, 0x1e},},/* SATURATION LEVEL8*/
};

static struct msm_camera_i2c_conf_array ov7695_saturation_confs[][1] = {
	{{ov7695_saturation[0], ARRAY_SIZE(ov7695_saturation[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[1], ARRAY_SIZE(ov7695_saturation[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[2], ARRAY_SIZE(ov7695_saturation[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[3], ARRAY_SIZE(ov7695_saturation[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[4], ARRAY_SIZE(ov7695_saturation[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[5], ARRAY_SIZE(ov7695_saturation[5]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[6], ARRAY_SIZE(ov7695_saturation[6]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[7], ARRAY_SIZE(ov7695_saturation[7]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_saturation[8], ARRAY_SIZE(ov7695_saturation[8]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_saturation_enum_map[] = {
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

static struct msm_camera_i2c_enum_conf_array ov7695_saturation_enum_confs = {
	.conf = &ov7695_saturation_confs[0][0],
	.conf_enum = ov7695_saturation_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_saturation_enum_map),
	.num_index = ARRAY_SIZE(ov7695_saturation_confs),
	.num_conf = ARRAY_SIZE(ov7695_saturation_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov7695_contrast[][4] = {
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x10, 0x00, 0x00, 0x00}, {0x5806, 0x10, 0x00, 0x00, 0x00},},	/* CONTRAST L0*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x14, 0x00, 0x00, 0x00}, {0x5806, 0x14, 0x00, 0x00, 0x00},},	/* CONTRAST L1*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x18, 0x00, 0x00, 0x00}, {0x5806, 0x18, 0x00, 0x00, 0x00},},	/* CONTRAST L2*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x1c, 0x00, 0x00, 0x00}, {0x5806, 0x1c, 0x00, 0x00, 0x00},},	/* CONTRAST L3*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x00, 0x00, 0x00, 0x00}, {0x5806, 0x20, 0x00, 0x00, 0x00},},	/* CONTRAST L4*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x10, 0x00, 0x00, 0x00}, {0x5806, 0x24, 0x00, 0x00, 0x00},},	/* CONTRAST L5*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x18, 0x00, 0x00, 0x00}, {0x5806, 0x28, 0x00, 0x00, 0x00},},	/* CONTRAST L6*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x1c, 0x00, 0x00, 0x00}, {0x5806, 0x2c, 0x00, 0x00, 0x00},},	/* CONTRAST L7*/
	{{0x5001, 0x08, 0x00, 0x00, 0xf7}, {0x5808, 0x00, 0x00, 0x00, 0xfb},
		{0x5805, 0x20, 0x00, 0x00, 0x00}, {0x5806, 0x30, 0x00, 0x00, 0x00},},	/* CONTRAST L8*/
};

static struct msm_camera_i2c_conf_array ov7695_contrast_confs[][1] = {
	{{ov7695_contrast[0], ARRAY_SIZE(ov7695_contrast[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[1], ARRAY_SIZE(ov7695_contrast[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[2], ARRAY_SIZE(ov7695_contrast[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[3], ARRAY_SIZE(ov7695_contrast[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[4], ARRAY_SIZE(ov7695_contrast[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[5], ARRAY_SIZE(ov7695_contrast[5]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[6], ARRAY_SIZE(ov7695_contrast[6]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[7], ARRAY_SIZE(ov7695_contrast[7]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_contrast[8], ARRAY_SIZE(ov7695_contrast[8]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_contrast_enum_map[] = {
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

static struct msm_camera_i2c_enum_conf_array ov7695_contrast_enum_confs = {
	.conf = &ov7695_contrast_confs[0][0],
	.conf_enum = ov7695_contrast_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_contrast_enum_map),
	.num_index = ARRAY_SIZE(ov7695_contrast_confs),
	.num_conf = ARRAY_SIZE(ov7695_contrast_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};
static struct msm_camera_i2c_reg_conf ov7695_sharpness[][2] = {
	{{0x5508, 0x40, 0x00, 0x00, 0xBF}, {0x5502, 0x00, 0x00, 0x00, 0x00},},	/* SHARPNESS LEVEL 0*/
	{{0x5508, 0x40, 0x00, 0x00, 0xBF}, {0x5502, 0x02, 0x00, 0x00, 0x00},},	/* SHARPNESS LEVEL 1*/
	{{0x5508, 0x00, 0x00, 0x00, 0xBF}, {0x5508, 0x00, 0x00, 0x00, 0xbf},},	/* SHARPNESS LEVEL 2*/
	{{0x5508, 0x40, 0x00, 0x00, 0xBF}, {0x5502, 0x04, 0x00, 0x00, 0x00},},	/* SHARPNESS LEVEL 3*/
	{{0x5508, 0x40, 0x00, 0x00, 0xBF}, {0x5502, 0x0c, 0x00, 0x00, 0x00},},	/* SHARPNESS LEVEL 4*/
	{{0x5508, 0x40, 0x00, 0x00, 0xBF}, {0x5502, 0x14, 0x00, 0x00, 0x00},},	/* SHARPNESS LEVEL 5*/
	{{0x5508, 0x40, 0x00, 0x00, 0xBF}, {0x5502, 0x20, 0x00, 0x00, 0x00},},	/* SHARPNESS LEVEL 6*/
};

static struct msm_camera_i2c_conf_array ov7695_sharpness_confs[][1] = {
	{{ov7695_sharpness[0], ARRAY_SIZE(ov7695_sharpness[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_sharpness[1], ARRAY_SIZE(ov7695_sharpness[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_sharpness[2], ARRAY_SIZE(ov7695_sharpness[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_sharpness[3], ARRAY_SIZE(ov7695_sharpness[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_sharpness[4], ARRAY_SIZE(ov7695_sharpness[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_sharpness[5], ARRAY_SIZE(ov7695_sharpness[5]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_sharpness[6], ARRAY_SIZE(ov7695_sharpness[6]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_sharpness_enum_map[] = {
	MSM_V4L2_SHARPNESS_L0,
	MSM_V4L2_SHARPNESS_L1,
	MSM_V4L2_SHARPNESS_L2,
	MSM_V4L2_SHARPNESS_L3,
	MSM_V4L2_SHARPNESS_L4,
	MSM_V4L2_SHARPNESS_L5,
	MSM_V4L2_SHARPNESS_L6,
};

static struct msm_camera_i2c_enum_conf_array ov7695_sharpness_enum_confs = {
	.conf = &ov7695_sharpness_confs[0][0],
	.conf_enum = ov7695_sharpness_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_sharpness_enum_map),
	.num_index = ARRAY_SIZE(ov7695_sharpness_confs),
	.num_conf = ARRAY_SIZE(ov7695_sharpness_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov7695_exposure[][6] = {
	{{0x3a0f, 0x38}, {0x3a10, 0x30}, {0x3a11, 0x61},
		{0x3a1b, 0x38}, {0x3a1e, 0x30}, {0x3a1f, 0x10},},	/*EXPOSURECOMPENSATIONN2*/
	{{0x3a0f, 0x48}, {0x3a10, 0x40}, {0x3a11, 0x80},
		{0x3a1b, 0x48}, {0x3a1e, 0x40}, {0x3a1f, 0x20},},	/*EXPOSURECOMPENSATIONN1*/
	{{0x3a0f, 0x58}, {0x3a10, 0x50}, {0x3a11, 0x91},
		{0x3a1b, 0x58}, {0x3a1e, 0x50}, {0x3a1f, 0x20},},	/*EXPOSURECOMPENSATIOND*/
	{{0x3a0f, 0x70}, {0x3a10, 0x60}, {0x3a11, 0xa0},
		{0x3a1b, 0x70}, {0x3a1e, 0x60}, {0x3a1f, 0x20},},	/*EXPOSURECOMPENSATIONP1*/
	{{0x3a0f, 0x80}, {0x3a10, 0x70}, {0x3a11, 0xa0},
		{0x3a1b, 0x80}, {0x3a1e, 0x70}, {0x3a1f, 0x30},},	/*EXPOSURECOMPENSATIONP2*/
};

static struct msm_camera_i2c_conf_array ov7695_exposure_confs[][1] = {
	{{ov7695_exposure[0], ARRAY_SIZE(ov7695_exposure[0]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_exposure[1], ARRAY_SIZE(ov7695_exposure[1]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_exposure[2], ARRAY_SIZE(ov7695_exposure[2]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_exposure[3], ARRAY_SIZE(ov7695_exposure[3]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_exposure[4], ARRAY_SIZE(ov7695_exposure[4]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_exposure_enum_map[] = {
	MSM_V4L2_EXPOSURE_N2,
	MSM_V4L2_EXPOSURE_N1,
	MSM_V4L2_EXPOSURE_D,
	MSM_V4L2_EXPOSURE_P1,
	MSM_V4L2_EXPOSURE_P2,
};

static struct msm_camera_i2c_enum_conf_array ov7695_exposure_enum_confs = {
	.conf = &ov7695_exposure_confs[0][0],
	.conf_enum = ov7695_exposure_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_exposure_enum_map),
	.num_index = ARRAY_SIZE(ov7695_exposure_confs),
	.num_conf = ARRAY_SIZE(ov7695_exposure_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov7695_iso[][2] = {
	{{0x3a18, 0x00}, {0x3a19, 0xf8},},   /*ISO_AUTO*/
	{{0x3a18, 0x00}, {0x3a19, 0xf8},},   /*ISO_DEBLUR*/
	{{0x3a18, 0x00}, {0x3a19, 0x3c},},   /*ISO_100*/
	{{0x3a18, 0x00}, {0x3a19, 0x7c},},   /*ISO_200*/
	{{0x3a18, 0x00}, {0x3a19, 0xc8},},   /*ISO_400*/
	{{0x3a18, 0x00}, {0x3a19, 0xf8},},   /*ISO_800*/
	{{0x3a18, 0x01}, {0x3a19, 0xf8},},   /*ISO_1600*/
};


static struct msm_camera_i2c_conf_array ov7695_iso_confs[][1] = {
	{{ov7695_iso[0], ARRAY_SIZE(ov7695_iso[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_iso[1], ARRAY_SIZE(ov7695_iso[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_iso[2], ARRAY_SIZE(ov7695_iso[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_iso[3], ARRAY_SIZE(ov7695_iso[3]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_iso[4], ARRAY_SIZE(ov7695_iso[4]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_iso[5], ARRAY_SIZE(ov7695_iso[5]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_iso_enum_map[] = {
	MSM_V4L2_ISO_AUTO ,
	MSM_V4L2_ISO_DEBLUR,
	MSM_V4L2_ISO_100,
	MSM_V4L2_ISO_200,
	MSM_V4L2_ISO_400,
	MSM_V4L2_ISO_800,
	MSM_V4L2_ISO_1600,
};

static struct msm_camera_i2c_enum_conf_array ov7695_iso_enum_confs = {
	.conf = &ov7695_iso_confs[0][0],
	.conf_enum = ov7695_iso_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_iso_enum_map),
	.num_index = ARRAY_SIZE(ov7695_iso_confs),
	.num_conf = ARRAY_SIZE(ov7695_iso_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov7695_special_effect[][4] = {
	{{0x5001, 0x3f}, {0x5800, 0x06},
		{0x5803, 0x00}, {0x5804, 0x18},},	/*for special effect OFF*/
	{{0x5001, 0x04, 0x00, 0x00, 0xfb}, {0x5800, 0x58, 0x00, 0x00, 0xe7},
		{0x5803, 0x80}, {0x5804, 0x80},},	/*for special effect MONO*/
	{{0x5001, 0x04, 0x00, 0x00, 0xfb}, {0x5800, 0x40, 0x00, 0x00, 0xbf},
		{0x5803, 0x00}, {0x5804, 0x18},},	/*for special efefct Negative*/
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/*Solarize is not supported by sensor*/
	{{0x5001, 0x04, 0x00, 0x00, 0xfb}, {0x5800, 0x58, 0x00, 0x00, 0xe7},
		{0x5803, 0x40}, {0x5804, 0xa0},},	/*for sepia*/
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/* Posteraize not supported */
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/* White board not supported*/
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/*Blackboard not supported*/
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/*Aqua not supported*/
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/*Emboss not supported */
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/*sketch not supported*/
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/*Neon not supported*/
	{{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},
		{-1, -1, -1, -1, -1}, {-1, -1, -1, -1, -1},},/*MAX value*/
};

static struct msm_camera_i2c_reg_conf ov7695_no_effect[] = {
	{0x5001, 0x3f}, {0x5800, 0x06},
	{0x5803, 0x00}, {0x5804, 0x18},
};

static struct msm_camera_i2c_conf_array ov7695_no_effect_confs[] = {
	{&ov7695_no_effect[0],
	ARRAY_SIZE(ov7695_no_effect), 0,
	MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},
};

static struct msm_camera_i2c_conf_array ov7695_special_effect_confs[][1] = {
	{{ov7695_special_effect[0],  ARRAY_SIZE(ov7695_special_effect[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[1],  ARRAY_SIZE(ov7695_special_effect[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[2],  ARRAY_SIZE(ov7695_special_effect[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[3],  ARRAY_SIZE(ov7695_special_effect[3]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[4],  ARRAY_SIZE(ov7695_special_effect[4]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[5],  ARRAY_SIZE(ov7695_special_effect[5]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[6],  ARRAY_SIZE(ov7695_special_effect[6]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[7],  ARRAY_SIZE(ov7695_special_effect[7]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[8],  ARRAY_SIZE(ov7695_special_effect[8]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[9],  ARRAY_SIZE(ov7695_special_effect[9]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[10], ARRAY_SIZE(ov7695_special_effect[10]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[11], ARRAY_SIZE(ov7695_special_effect[11]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_special_effect[12], ARRAY_SIZE(ov7695_special_effect[12]), 0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_special_effect_enum_map[] = {
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
		 ov7695_special_effect_enum_confs = {
	.conf = &ov7695_special_effect_confs[0][0],
	.conf_enum = ov7695_special_effect_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_special_effect_enum_map),
	.num_index = ARRAY_SIZE(ov7695_special_effect_confs),
	.num_conf = ARRAY_SIZE(ov7695_special_effect_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov7695_antibanding[][2] = {
	{{0x3a00, 0x00, 0x00, 0x00, 0xdf},
		{0x5002, 0x02, 0x00, 0x00, 0xfd},},   /*ANTIBANDING OFF*/
	{{0x3a00, 0x20, 0x00, 0x00, 0xdf},
		{0x5002, 0x00, 0x00, 0x00, 0xfd},},   /*ANTIBANDING 60HZ*/
	{{0x3a00, 0x20, 0x00, 0x00, 0xdf},
		{0x5002, 0x02, 0x00, 0x00, 0xfd},},   /*ANTIBANDING 50HZ*/
	{{0x3a00, 0x20, 0x00, 0x00, 0xdf},
		{0x5002, 0x02, 0x00, 0x00, 0xfd},},   /* ANTIBANDING AUTO*/
};


static struct msm_camera_i2c_conf_array ov7695_antibanding_confs[][1] = {
	{{ov7695_antibanding[0], ARRAY_SIZE(ov7695_antibanding[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_antibanding[1], ARRAY_SIZE(ov7695_antibanding[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_antibanding[2], ARRAY_SIZE(ov7695_antibanding[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_antibanding[3], ARRAY_SIZE(ov7695_antibanding[3]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_antibanding_enum_map[] = {
	MSM_V4L2_POWER_LINE_OFF,
	MSM_V4L2_POWER_LINE_60HZ,
	MSM_V4L2_POWER_LINE_50HZ,
	MSM_V4L2_POWER_LINE_AUTO,
};


static struct msm_camera_i2c_enum_conf_array ov7695_antibanding_enum_confs = {
	.conf = &ov7695_antibanding_confs[0][0],
	.conf_enum = ov7695_antibanding_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_antibanding_enum_map),
	.num_index = ARRAY_SIZE(ov7695_antibanding_confs),
	.num_conf = ARRAY_SIZE(ov7695_antibanding_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};

static struct msm_camera_i2c_reg_conf ov7695_wb_oem[][7] = {
	{{0x5200, 0x20, 0x00, 0x00, 0xdf}, {0x5204, 0x04}, {0x5205, 0x00},
		{0x5206, 0x04}, {0x5207, 0x00}, {0x5208, 0x04}, {0x5209, 0x00},},	/*WHITEBALNACE OFF*/
	{{0x5200, 0x00, 0x00, 0x00, 0xdf}, {0x5204, 0x00}, {0x5205, 0x00},
		{0x5206, 0x00}, {0x5207, 0x00}, {0x5208, 0x00}, {0x5209, 0x00},},	/*WHITEBALNACE AUTO*/
	{{0x5200, 0x20, 0x00, 0x00, 0xdf}, {0x5204, 0x04}, {0x5205, 0x00},
		{0x5206, 0x04}, {0x5207, 0xdc}, {0x5208, 0x0b}, {0x5209, 0xb4},},	/*WHITEBALNACE CUSTOM*/
	{{0x5200, 0x20, 0x00, 0x00, 0xdf}, {0x5204, 0x05}, {0x5205, 0xa0},
		{0x5206, 0x04}, {0x5207, 0x00}, {0x5208, 0x09}, {0x5209, 0xa0},},	/*INCANDISCENT*/
	{{0x5200, 0x20, 0x00, 0x00, 0xdf}, {0x5204, 0x05}, {0x5205, 0xa0},
		{0x5206, 0x04}, {0x5207, 0x00}, {0x5208, 0x08}, {0x5209, 0x4e},},	/*FLOURESECT*/
	{{0x5200, 0x20, 0x00, 0x00, 0xdf}, {0x5204, 0x05}, {0x5205, 0x7b},
		{0x5206, 0x04}, {0x5207, 0x00}, {0x5208, 0x05}, {0x5209, 0x15},},	/*DAYLIGHT*/
	{{0x5200, 0x20, 0x00, 0x00, 0xdf}, {0x5204, 0x06}, {0x5205, 0x00},
		{0x5206, 0x04}, {0x5207, 0x00}, {0x5208, 0x04}, {0x5209, 0x80},},	/*CLOUDY*/
};

static struct msm_camera_i2c_conf_array ov7695_wb_oem_confs[][1] = {
	{{ov7695_wb_oem[0], ARRAY_SIZE(ov7695_wb_oem[0]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_wb_oem[1], ARRAY_SIZE(ov7695_wb_oem[1]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_wb_oem[2], ARRAY_SIZE(ov7695_wb_oem[2]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_wb_oem[3], ARRAY_SIZE(ov7695_wb_oem[3]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_wb_oem[4], ARRAY_SIZE(ov7695_wb_oem[4]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_wb_oem[5], ARRAY_SIZE(ov7695_wb_oem[5]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
	{{ov7695_wb_oem[6], ARRAY_SIZE(ov7695_wb_oem[6]),  0,
		MSM_CAMERA_I2C_SET_BYTE_WRITE_MASK_DATA},},
};

static int ov7695_wb_oem_enum_map[] = {
	MSM_V4L2_WB_OFF,
	MSM_V4L2_WB_AUTO ,
	MSM_V4L2_WB_CUSTOM,
	MSM_V4L2_WB_INCANDESCENT,
	MSM_V4L2_WB_FLUORESCENT,
	MSM_V4L2_WB_DAYLIGHT,
	MSM_V4L2_WB_CLOUDY_DAYLIGHT,
};

static struct msm_camera_i2c_enum_conf_array ov7695_wb_oem_enum_confs = {
	.conf = &ov7695_wb_oem_confs[0][0],
	.conf_enum = ov7695_wb_oem_enum_map,
	.num_enum = ARRAY_SIZE(ov7695_wb_oem_enum_map),
	.num_index = ARRAY_SIZE(ov7695_wb_oem_confs),
	.num_conf = ARRAY_SIZE(ov7695_wb_oem_confs[0]),
	.data_type = MSM_CAMERA_I2C_BYTE_DATA,
};


int ov7695_saturation_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
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


int ov7695_contrast_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}

int ov7695_sharpness_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
	if (effect_value == CAMERA_EFFECT_OFF) {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}

int ov7695_effect_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
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
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xda, SAT_U,
			MSM_CAMERA_I2C_BYTE_DATA);
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0xdb, SAT_V,
			MSM_CAMERA_I2C_BYTE_DATA);
	} else {
		rc = msm_sensor_write_enum_conf_array(
			s_ctrl->sensor_i2c_client,
			ctrl_info->enum_cfg_settings, value);
	}
	return rc;
}

int ov7695_antibanding_msm_sensor_s_ctrl_by_enum(
		struct msm_sensor_ctrl_t *s_ctrl,
		struct msm_sensor_v4l2_ctrl_info_t *ctrl_info, int value)
{
	int rc = 0;
		return rc;
}

int ov7695_msm_sensor_s_ctrl_by_enum(struct msm_sensor_ctrl_t *s_ctrl,
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

struct msm_sensor_v4l2_ctrl_info_t ov7695_v4l2_ctrl_info[] = {
	{
		.ctrl_id = V4L2_CID_SATURATION,
		.min = MSM_V4L2_SATURATION_L0,
		.max = MSM_V4L2_SATURATION_L8,
		.step = 1,
		.enum_cfg_settings = &ov7695_saturation_enum_confs,
		.s_v4l2_ctrl = ov7695_saturation_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_CONTRAST,
		.min = MSM_V4L2_CONTRAST_L0,
		.max = MSM_V4L2_CONTRAST_L8,
		.step = 1,
		.enum_cfg_settings = &ov7695_contrast_enum_confs,
		.s_v4l2_ctrl = ov7695_contrast_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SHARPNESS,
		.min = MSM_V4L2_SHARPNESS_L0,
		.max = MSM_V4L2_SHARPNESS_L6,
		.step = 1,
		.enum_cfg_settings = &ov7695_sharpness_enum_confs,
		.s_v4l2_ctrl = ov7695_sharpness_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_EXPOSURE,
		.min = MSM_V4L2_EXPOSURE_N2,
		.max = MSM_V4L2_EXPOSURE_P2,
		.step = 1,
		.enum_cfg_settings = &ov7695_exposure_enum_confs,
		.s_v4l2_ctrl = ov7695_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = MSM_V4L2_PID_ISO,
		.min = MSM_V4L2_ISO_AUTO,
		.max = MSM_V4L2_ISO_1600,
		.step = 1,
		.enum_cfg_settings = &ov7695_iso_enum_confs,
		.s_v4l2_ctrl = ov7695_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_SPECIAL_EFFECT,
		.min = MSM_V4L2_EFFECT_OFF,
		.max = MSM_V4L2_EFFECT_SEPIA,
		.step = 1,
		.enum_cfg_settings = &ov7695_special_effect_enum_confs,
		.s_v4l2_ctrl = ov7695_effect_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_POWER_LINE_FREQUENCY,
		.min = MSM_V4L2_POWER_LINE_OFF,
		.max = MSM_V4L2_POWER_LINE_AUTO,
		.step = 1,
		.enum_cfg_settings = &ov7695_antibanding_enum_confs,
		.s_v4l2_ctrl = ov7695_antibanding_msm_sensor_s_ctrl_by_enum,
	},
	{
		.ctrl_id = V4L2_CID_WHITE_BALANCE_TEMPERATURE,
		.min = MSM_V4L2_WB_OFF,
		.max = MSM_V4L2_WB_CLOUDY_DAYLIGHT,
		.step = 1,
		.enum_cfg_settings = &ov7695_wb_oem_enum_confs,
		.s_v4l2_ctrl = ov7695_msm_sensor_s_ctrl_by_enum,
	},

};
static struct msm_camera_csi_params ov7695_csi_params = {
	.data_format = CSI_8BIT,
	.lane_cnt    = 1,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 0x19,
};

static struct msm_camera_csi_params *ov7695_csi_params_array[] = {
	&ov7695_csi_params,
};

static struct msm_sensor_output_reg_addr_t ov7695_reg_addr = {
	.x_output = 0x034C,
	.y_output = 0x034E,
	.line_length_pclk = 0x0342,
	.frame_length_lines = 0x0340,
};

static struct msm_sensor_id_info_t ov7695_id_info = {
	.sensor_id_reg_addr = 0x300A,
	.sensor_id = 0x7695,
};

static const struct i2c_device_id ov7695_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov7695_s_ctrl},
	{ }
};


static struct i2c_driver ov7695_i2c_driver = {
	.id_table = ov7695_i2c_id,
	.probe  = msm_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov7695_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
	int rc = 0;
	CDBG("ov7695\n");

	rc = i2c_add_driver(&ov7695_i2c_driver);

	return rc;
}

static struct v4l2_subdev_core_ops ov7695_subdev_core_ops = {
	.s_ctrl = msm_sensor_v4l2_s_ctrl,
	.queryctrl = msm_sensor_v4l2_query_ctrl,
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov7695_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov7695_subdev_ops = {
	.core = &ov7695_subdev_core_ops,
	.video  = &ov7695_subdev_video_ops,
};

static struct msm_sensor_fn_t ov7695_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_csi_setting = msm_sensor_setting1,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = msm_sensor_power_up,
	.sensor_power_down = msm_sensor_power_down,
	.sensor_get_csi_params = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t ov7695_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov7695_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov7695_start_settings),
	.stop_stream_conf = ov7695_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov7695_stop_settings),
	.init_settings = &ov7695_init_conf[0],
	.init_size = ARRAY_SIZE(ov7695_init_conf),
	.mode_settings = &ov7695_confs[0],
	.no_effect_settings = &ov7695_no_effect_confs[0],
	.output_settings = &ov7695_dimensions[0],
	.num_conf = ARRAY_SIZE(ov7695_confs),
};

static struct msm_sensor_ctrl_t ov7695_s_ctrl = {
	.msm_sensor_reg = &ov7695_regs,
	.msm_sensor_v4l2_ctrl_info = ov7695_v4l2_ctrl_info,
	.num_v4l2_ctrl = ARRAY_SIZE(ov7695_v4l2_ctrl_info),
	.sensor_i2c_client = &ov7695_sensor_i2c_client,
	.sensor_i2c_addr = 0x42,
	.sensor_output_reg_addr = &ov7695_reg_addr,
	.sensor_id_info = &ov7695_id_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &ov7695_csi_params_array[0],
	.msm_sensor_mutex = &ov7695_mut,
	.sensor_i2c_driver = &ov7695_i2c_driver,
	.sensor_v4l2_subdev_info = ov7695_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov7695_subdev_info),
	.sensor_v4l2_subdev_ops = &ov7695_subdev_ops,
	.func_tbl = &ov7695_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision VGA YUV sensor driver");
MODULE_LICENSE("GPL v2");
