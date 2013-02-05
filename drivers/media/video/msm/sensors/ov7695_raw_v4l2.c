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

#include "msm_sensor.h"
#include "msm.h"
#include "pip_ov5648_ov7695.h"

#define SENSOR_NAME "ov7695_raw"

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define ov7695_raw_VERBOSE_DGB

#ifdef ov7695_raw_VERBOSE_DGB
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

static struct msm_sensor_ctrl_t ov7695_raw_s_ctrl;
DEFINE_MUTEX(ov7695_raw_mut);

/* For PIP feature */
static struct msm_camera_i2c_reg_conf ov7695_raw_pip_init_settings_slave[] = {
	//0x0103, 0x01, // software reset
	{0x0101, 0x01},// Mirror off
	{0x3811, 0x06},
	{0x3813, 0x06},
	{0x382c, 0x05}, // manual control ISP window offset 0x3810~0x3813 during subsampled modes
	{0x3620, 0x28},
	{0x3623, 0x12},
	{0x3718, 0x88},
	{0x3632, 0x05},
	{0x3013, 0xd0},
	{0x3717, 0x18},
	{0x034c, 0x01},
	{0x034d, 0x40},
	{0x034e, 0x00},
	{0x034f, 0xf0},
	{0x0383, 0x03}, // x odd inc
	{0x4500, 0x47}, // x sub control
	{0x0387, 0x03}, // y odd inc
	{0x4300, 0xf8}, // output RGB raw
	{0x0301, 0x0a}, // PLL
	{0x0307, 0x50}, // PLL
	{0x030b, 0x04}, // PLL
	{0x3106, 0x91}, // PLL
	{0x301e, 0x60},
	{0x3014, 0x30},
	{0x301a, 0x44},
	{0x4803, 0x08}, // HS prepare in UI
	{0x370a, 0x63},
	{0x520a, 0xf4}, // red gain from 0x400 to 0xfff
	{0x520b, 0xf4}, // green gain from 0x400 to 0xfff
	{0x520c, 0xf4}, // blue gain from 0x400 to 0xfff
	{0x4008, 0x01}, // bl start
	{0x4009, 0x04}, // bl end
	{0x0340, 0x03}, // VTS = 992
	{0x0341, 0xe0}, // VTS
	{0x0342, 0x02}, // HTS = 540
	{0x0343, 0x1c}, // HTS
	{0x0344, 0x00},
	{0x0345, 0x00},

	{0x0348, 0x02}, //x end = 639
	{0x0349, 0x7f}, //x end
	{0x034a, 0x01}, //y end = 479
	{0x034b, 0xdf}, //y end

	{0x3820, 0x94}, // by pass MIPI, v bin off,
	{0x4f05, 0x01}, // SPI 2 lane on
	{0x4f06, 0x02}, // SPI on
	{0x3012, 0x00},
	{0x3005, 0x0f}, // SPI pin on
	{0x3001, 0x1f}, // SPI drive 3.75x, FSIN drive 1.75x
	{0x3823, 0x30},
	{0x4303, 0xee}, // Y max L
	{0x4307, 0xee}, // U max L
	{0x430b, 0xee}, // V max L
	{0x3620, 0x2f},
	{0x3621, 0x77},
	{0x0100, 0x01}, // stream on
	//// PIP Demo
	{0x3503, 0x30}, // delay gain for one frame, and mannual enable.
	{0x5000, 0x8f}, // Gama OFF, AWB OFF
	{0x5002, 0x8a}, // 50Hz/60Hz select,88 for 60Hz, 8a for 50hz
	{0x3a0f, 0x38}, // AEC in H
	{0x3a10, 0x30}, // AEC in L
	{0x3a1b, 0x38}, // AEC out H
	{0x3a1e, 0x30}, // AEC out L
	{0x3a11, 0x70}, // control zone H
	{0x3a1f, 0x18}, // control zone L

	{0x3a18, 0x00}, // gain ceiling = 16x
	{0x3a19, 0xe0}, // gain ceiling

	{0x3a08, 0x00}, // B50
	{0x3a09, 0xde}, // B50
	{0x3a0a, 0x00}, // B60
	{0x3a0b, 0xb9}, // B60
	{0x3a0e, 0x03}, // B50 step
	{0x3a0d, 0x04}, // B60 step
	{0x3a02, 0x02}, // max expo 60
	{0x3a03, 0xe6}, // max expo 60
	{0x3a14, 0x02}, // max expo 50
	{0x3a15, 0xe6}, // max expo 50

	//lens A Light
	{0x5100, 0x01}, // red x0
	{0x5101, 0xc0}, // red x0
	{0x5102, 0x01}, // red y0
	{0x5103, 0x00}, // red y0
	{0x5104, 0x4b}, // red a1
	{0x5105, 0x04}, // red a2
	{0x5106, 0xc2}, // red b1
	{0x5107, 0x08}, // red b2
	{0x5108, 0x01}, // green x0
	{0x5109, 0xd0}, // green x0
	{0x510a, 0x01}, // green y0
	{0x510b, 0x00}, // green y0
	{0x510c, 0x30}, // green a1
	{0x510d, 0x04}, // green a2
	{0x510e, 0xc2}, // green b1
	{0x510f, 0x08}, // green b2
	{0x5110, 0x01}, // blue x0
	{0x5111, 0xd0}, // blue x0
	{0x5112, 0x01}, // blue y0
	{0x5113, 0x00}, // blue y0
	{0x5114, 0x26}, // blue a1
	{0x5115, 0x04}, // blue a2
	{0x5116, 0xc2}, // blue b1
	{0x5117, 0x08}, // blue b2
};

static struct msm_camera_i2c_reg_conf ov7695_raw_pip_preview_settings_slave[] = {
#if 1
	//16:9 settings
	{0x382c, 0x05}, // manual control ISP window offset 0x3810~0x3813 during subsampled modes
	//0x3811, 0x06, // shift one pixel

	{0x0101, 0x03},
	{0x3811, 0x05},
	{0x3813, 0x06},

	{0x034c, 0x01}, // x output size = 320
	{0x034d, 0x40}, // x output size
	{0x034e, 0x00}, // y output size = 240
	{0x034f, 0xf0}, // y output size
	{0x0383, 0x03}, // x odd inc
	{0x4500, 0x47}, // h sub sample on
	{0x0387, 0x03}, // y odd inc
	{0x3820, 0x94}, // v bin off
	{0x3014, 0x30},
	{0x301a, 0x44},
	{0x370a, 0x63},
	{0x4008, 0x01}, // blc start line
	{0x4009, 0x04}, // blc end line
	{0x0340, 0x02}, // VTS = 742
	{0x0341, 0xe6}, // VTS
	{0x0342, 0x02}, // HTS = 540
	{0x0343, 0x1c}, // HTS
	{0x0344, 0x00},
	{0x0345, 0x00},
//	{0x3503, 0x30}, // AGC/AEC on

	{0x0348, 0x02}, //x end = 639
	{0x0349, 0x7f}, //x end
	{0x034a, 0x01}, //y end = 479
	{0x034b, 0xdf}, //y end

	{0x3503, 0x30},// AGC/AEC on

	{0x3a09, 0xde}, // B50
	{0x3a0b, 0xb9}, // B60
	{0x3a0e, 0x03}, // B50 Max
	{0x3a0d, 0x04}, // B60 Max
	{0x3a02, 0x02}, // max expo 60
	{0x3a03, 0xe6}, // max expo 60
	{0x3a14, 0x02}, // max expo 50
	{0x3a15, 0xe6}, // max expo 50
#else
	//4:3 settings
	{0x382c, 0x05}, // manual control ISP window offset 0x3810~0x3813 during subsampled modes
	{0x0101, 0x01}, // mirror off
	{0x3811, 0x06}, //
	{0x3813, 0x06},
	{0x034c, 0x01}, // x output size = 320
	{0x034d, 0x40}, // x output size
	{0x034e, 0x00}, // y output size = 240
	{0x034f, 0xf0}, // y output size
	{0x0383, 0x03}, // x odd inc
	{0x4500, 0x47}, // h sub sample on
	{0x0387, 0x03}, // y odd inc
	{0x3820, 0x94}, // v bin off
	{0x3014, 0x30}, // MIPI phy rst, pd
	{0x301a, 0x44},
	{0x370a, 0x63},
	{0x4008, 0x01}, // blc start line
	{0x4009, 0x04}, // blc end line
	{0x0340, 0x03}, // VTS =992
	{0x0341, 0xe0}, // VTS
	{0x0342, 0x02}, // HTS = 540
	{0x0343, 0x1c}, // HTS
	{0x3503, 0x30}, // AGC/AEC on
	{0x3a09, 0xde}, // B50 L
	{0x3a0b, 0xb9}, // B60 L
	{0x3a0e, 0x03}, // B50 Max
	{0x3a0d, 0x04}, // B60 Max
	{0x3a02, 0x02}, // Max expo 60
	{0x3a03, 0xe6}, // Max expo 60
	{0x3a14, 0x02}, // Max expo 50
	{0x3a15, 0xe6}, // Max expo 50
#endif
};

static struct msm_camera_i2c_reg_conf ov7695_raw_pip_snapshot_settings_slave[] = {
#if 0
	//4:3 settings
	//// OV7695
	{0x382c, 0xc5}, // manual control ISP window offset 0x3810~0x3813 during subsampled modes
	////0x3811, 0x09, //
	{0x034c, 0x02}, // x output size = 640
	{0x034d, 0x80}, // x output size
	{0x034e, 0x01}, // y output size = 480
	{0x034f, 0xe0}, // y output size
	{0x0383, 0x01}, // x odd inc
	{0x4500, 0x25}, // h sub sample off
	{0x0387, 0x01}, // y odd inc
	{0x3820, 0x90}, // v bin off
	{0x3014, 0x0f},
	{0x301a, 0xf0},
	{0x370a, 0x23},
	{0x4008, 0x02}, // blc start line
	{0x4009, 0x09}, // blc end line
	{0x0340, 0x07}, // VTS = 1984
	{0x0341, 0xc0}, // VTS
	{0x0342, 0x03}, // HTS = 806
	{0x0343, 0x26}, // HTS
	{0x3503, 0x33}, // AGC/AEC off
	{0x3a09, 0x95}, // B50 L
	{0x3a0b, 0x7c}, // B60 L
	{0x3a0e, 0x0d}, // B50Max
	{0x3a0d, 0x10}, // B60Max
	{0x3a02, 0x07}, // max expo 60
	{0x3a03, 0xc0}, // max expo 60
	{0x3a14, 0x07}, // max expo 50
	{0x3a15, 0xc0}, // max expo 50
#else
	//16:9 settings
	{0x382c, 0xc5}, //; manual control ISP window offset 0x3810~0x3813 during subsampled modes
	//;pip normal normal
	//;42 0101 00
	//;42 3811 06
	//;pip rotate 180
	{0x0101, 0x03},
	{0x3811, 0x05},

	{0x3813, 0x06},
	{0x034c, 0x02}, //; x output size = 640
	{0x034d, 0x80}, //; x output size
	{0x034e, 0x01}, //; y output size = 480
	{0x034f, 0xe0}, //; y output size
	{0x0383, 0x01}, //; x odd inc
	{0x4500, 0x25}, //; h sub sample off
	{0x0387, 0x01}, //; y odd inc
	{0x3820, 0x94}, //; v bin off
	{0x3014, 0x0f},
	{0x301a, 0xf0},
	{0x370a, 0x23},
	{0x4008, 0x02}, //; blc start line
	{0x4009, 0x09}, //; blc end line
	{0x0340, 0x05}, //; VTS = 1480
	{0x0341, 0xc8}, //; VTS
	{0x0342, 0x03}, //; HTS = 806
	{0x0343, 0x26}, //; HTS
	{0x0344, 0x00},
	{0x0345, 0x00},

	{0x0348, 0x02}, //; x end = 639
	{0x0349, 0x7f}, //; x end
	{0x034a, 0x01}, //; y end = 479
	{0x034b, 0xdf}, //; y end

	{0x3503, 0x07},
	{0x3a09, 0x95}, //; B50 L
	{0x3a0b, 0x7c}, //	; B60 L
	{0x3a0e, 0x0a}, //	; B50Max
	{0x3a0d, 0x0c}, //	; B60Max
	{0x3a02, 0x05}, //; max expo 60
	{0x3a03, 0xc8}, //; max expo 60
	{0x3a14, 0x05}, //; max expo 50
	{0x3a15, 0xc8}, //; max expo 50

#endif
};

static struct msm_camera_i2c_conf_array ov7695_raw_pip_init_conf_slave[] = {
	{&ov7695_raw_pip_init_settings_slave[0],
	ARRAY_SIZE(ov7695_raw_pip_init_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_i2c_conf_array ov7695_raw_pip_confs_slave[] = {
	{&ov7695_raw_pip_snapshot_settings_slave[0],
	ARRAY_SIZE(ov7695_raw_pip_snapshot_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov7695_raw_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov7695_raw_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov7695_raw_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov7695_raw_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov7695_raw_pip_preview_settings_slave[0],
	ARRAY_SIZE(ov7695_raw_pip_preview_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
	{&ov7695_raw_pip_snapshot_settings_slave[0],
	ARRAY_SIZE(ov7695_raw_pip_snapshot_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA},
};


static int ov7695_pwdn_pin = 0;
static uint16_t ov7695_i2c_address = 0;
extern int32_t pip_ov5648_ctrl(int cmd, struct msm_camera_pip_ctl *pip_ctl);

int32_t pip_ov7695_ctrl(int cmd, struct msm_camera_pip_ctl *pip_ctl)
{
	int rc = 0;
	struct msm_camera_i2c_client sensor_i2c_client;
	struct i2c_client client;
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
			client.addr = ov7695_i2c_address;
			sensor_i2c_client.client = &client;
			CDBG("write_ctl:%d\r\n", pip_ctl->write_ctl);
			if(PIP_REG_SETTINGS_INIT == pip_ctl->write_ctl)
			{
				rc = msm_sensor_write_all_conf_array(
					&sensor_i2c_client, &ov7695_raw_pip_init_conf_slave[0],
					ARRAY_SIZE(ov7695_raw_pip_init_conf_slave));
			}
			else
			{
				rc = msm_sensor_write_conf_array(
				&sensor_i2c_client,
				&ov7695_raw_pip_confs_slave[0], pip_ctl->write_ctl);
			}
			break;
		case PIP_CRL_POWERUP:
			if(0 != ov7695_pwdn_pin)
			{
				CDBG("PIP ctrl, set the gpio:%d to 1\r\n", ov7695_pwdn_pin);
				gpio_direction_output(ov7695_pwdn_pin, 1);
			}
			break;
		case PIP_CRL_POWERDOWN:
			if(0 != ov7695_pwdn_pin)
			{
				CDBG("PIP ctrl, set the gpio:%d to 0\r\n", ov7695_pwdn_pin);
				gpio_direction_output(ov7695_pwdn_pin, 0);
			}
			break;
		case PIP_CTL_RESET_HW_PULSE:
			break;
		case PIP_CTL_RESET_SW:
			if(NULL == pip_ctl){
				CDBG("%s parameters check error, exit\r\n", __func__);
				return rc;
			}
			memcpy(&sensor_i2c_client, pip_ctl->sensor_i2c_client, sizeof(struct msm_camera_i2c_client));
			memcpy(&client, pip_ctl->sensor_i2c_client->client, sizeof(struct i2c_client));
			client.addr = ov7695_i2c_address;
			sensor_i2c_client.client = &client;
			msm_camera_i2c_write(&sensor_i2c_client, 0x103, 0x1,
				MSM_CAMERA_I2C_BYTE_DATA);
			break;
		default:
			break;
	}

	return rc;
}
EXPORT_SYMBOL(pip_ov7695_ctrl);
/* End */
#if 1
static struct msm_camera_i2c_reg_conf ov7695_raw_start_settings[] = {
	{0x0100, 0x01},
};

static struct msm_camera_i2c_reg_conf ov7695_raw_stop_settings[] = {
	{0x0100, 0x00},
};
#endif
static struct msm_camera_i2c_reg_conf ov7695_raw_full_settings[] = {
	{0x382c, 0xc5}, //; manual control ISP window offset 0x3810~0x3813 during subsampled modes
//	{0x3811, 0x09}, //;   ##change to 06 or 09 or del

//	;normal
//	;42 0101 00
//	;42 3811 06
//	;42 3813 06
//	; sub mirror
	{0x0101, 0x02},
	{0x3811, 0x00},
	{0x3813, 0x05},

	{0x034c, 0x02},	//; x output size = 656
	{0x034d, 0x90},	//; x output size
	{0x034e, 0x01},	//; y output size = 480
	{0x034f, 0xe0},	//; y output size
	{0x0383, 0x01},	//; x odd inc
	{0x4500, 0x25},	//; h sub sample off
	{0x0387, 0x01},	//; y odd inc
	{0x3820, 0x94},	//; v bin off            ####change to 0x94
	{0x3014, 0x0f}, //
	{0x301a, 0xf0}, //
	{0x370a, 0x23}, //
	{0x4008, 0x02},	//; blc start line
	{0x4009, 0x09},	//; blc end line

	{0x0340, 0x02},	//; OV7695 VTS = 536
	{0x0341, 0x18},	//; OV6595 VTS
	{0x0342, 0x02},	//; OV7695 HTS = 746
	{0x0343, 0xea},	//; OV7695 HTS

//	{0x3503, 0x30}, // AGC/AEC on
	{0x3503, 0x33}, // AGC/AEC off

	{0x3a09, 0xa1},	//; B50
	{0x3a0b, 0x86}, //; B60
	{0x3a0e, 0x03}, //; B50 steps
	{0x3a0d, 0x04}, //; B60 steps
	{0x3a02, 0x02},	//; max expo 60
	{0x3a03, 0x18},	//; max expo 60
	{0x3a14, 0x02},	//; max expo 50
	{0x3a15, 0x18},	//; max expo 50

	{0x3502,0x40},
	{0x3501,0x21},
	{0x3500,0x0},
	{0x350b,0x12},
	{0x350a,0x0},

//	{0x0348, 0x02},	//; x end = 655
//	{0x0349, 0x8f},	//; x end
//	{0x034a, 0x01},	//; y end = 479
//	{0x034b, 0xdf},	//; y end
 };
static struct msm_camera_i2c_conf_array ov7695_raw_confs[] = {
	{&ov7695_raw_full_settings[0],
	ARRAY_SIZE(ov7695_raw_full_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array ov7695_raw_init_conf[] = {
	{&ov7695_raw_pip_init_settings_slave[0],
	ARRAY_SIZE(ov7695_raw_pip_init_settings_slave), 0, MSM_CAMERA_I2C_BYTE_DATA}
};

static struct msm_camera_csi_params ov7695_raw_csi_params = {
	.data_format = CSI_10BIT,
	.lane_cnt    = 2,
	.lane_assign = 0xe4,
	.dpcm_scheme = 0,
	.settle_cnt  = 16,
};

static struct v4l2_subdev_info ov7695_raw_subdev_info[] = {
	{
		.code   = V4L2_MBUS_FMT_SBGGR10_1X10,
		.colorspace = V4L2_COLORSPACE_JPEG,
		.fmt    = 1,
		.order    = 0,
	},
};

static struct msm_sensor_output_info_t ov7695_raw_dimensions[] = {
	{
		.x_output                = 0x290,
		.y_output                = 0x1E0,
		.line_length_pclk        = 0x2ea,
		.frame_length_lines      = 0x218,
		.vt_pixel_clk            = 12000000,
		.op_pixel_clk            = 12000000,
		.binning_factor          = 0,
	},
};

static struct msm_sensor_output_reg_addr_t ov7695_raw_reg_addr = {
	.x_output			=	0x034C,
	.y_output			=	0x034E,
	.line_length_pclk	=	0x0342,
	.frame_length_lines	=	0x0340,
};

static struct msm_camera_csi_params *ov7695_raw_csi_params_array[] = {
	&ov7695_raw_csi_params,
};

static struct msm_sensor_id_info_t ov7695_raw_id_info = {
	.sensor_id_reg_addr	=	0x300A,
	.sensor_id			=	0x7695,
};

static struct msm_sensor_exp_gain_info_t ov7695_raw_exp_gain_info = {
	.coarse_int_time_addr      = 0x0000,
	.global_gain_addr          = 0x0000,
	.vert_offset               = 0,
};

static int32_t ov7695_raw_write_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
	uint16_t gain, uint32_t line)
{
	int rc = 0;
	//uint16_t data, addr;
	CDBG("ov7695_raw_write_prev_exp_gain,gain=0x%x, line=0x%x\n",gain,line);
	/* the line may not larger than VTS */
	if (line > (0x218 - 4) )
	{
		line = 0x218 - 4;
	}

	line &= 0xffff;
	gain &= 0x01ff;

	CDBG("####croped, gainn=0x%x, line=0x%x\n",gain,line);
#if 1
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0104,
		0x1, MSM_CAMERA_I2C_BYTE_DATA);

	//Write exposure
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3502,
		(line & 0x0f) << 4, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3501,
		(line >> 4) & 0xff,MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3500,
		(line >> 12),MSM_CAMERA_I2C_BYTE_DATA);

//	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0104,
//		0x0, MSM_CAMERA_I2C_BYTE_DATA);

//	msleep(50);

//	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0104,
//		0x1, MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x350b,
		(gain & 0xff), MSM_CAMERA_I2C_BYTE_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x350a,
		(gain >> 8), MSM_CAMERA_I2C_BYTE_DATA);


//group off
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x0104,
		0x0, MSM_CAMERA_I2C_BYTE_DATA);
#endif
# if 0
//test codes
	msleep(50);
	addr = 0x3500;
	msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			addr, &data,
			MSM_CAMERA_I2C_BYTE_DATA);

	CDBG("####reg 0x%x=0x%x\n",addr, data);

	addr = 0x3501;
	msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			addr, &data,
			MSM_CAMERA_I2C_BYTE_DATA);

	CDBG("####reg 0x%x=0x%x\n",addr, data);

	addr = 0x3502;
	msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			addr, &data,
			MSM_CAMERA_I2C_BYTE_DATA);

	CDBG("####reg 0x%x=0x%x\n",addr, data);

	addr = 0x3503;
	msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			addr, &data,
			MSM_CAMERA_I2C_BYTE_DATA);

	CDBG("####reg 0x%x=0x%x\n",addr, data);

	addr = 0x350A;
	msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			addr, &data,
			MSM_CAMERA_I2C_BYTE_DATA);

	CDBG("####reg 0x%x=0x%x\n",addr, data);

	addr = 0x350B;
	msm_camera_i2c_read(
			s_ctrl->sensor_i2c_client,
			addr, &data,
			MSM_CAMERA_I2C_BYTE_DATA);

	CDBG("####reg 0x%x=0x%x\n",addr, data);
#endif
	return rc;
}


int32_t ov7695_raw_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
	int32_t rc = 0;
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;
	CDBG("%s IN\r\n", __func__);

	CDBG("%s, sensor_pwd:%d\r\n",__func__, info->sensor_pwd);
	CDBG("%s gpio %d setting to %d\r\n", __func__, info->sensor_pwd, 0);
/* for pip using */
	ov7695_pwdn_pin = info->sensor_pwd;
/*pip end*/
	gpio_direction_output(info->sensor_pwd, 0);
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
	//  lcd_camera_power_onoff(1);
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
	return rc;
}

int32_t ov7695_raw_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
	struct msm_camera_sensor_info *info = s_ctrl->sensordata;;
	CDBG("%s IN\r\n", __func__);

	gpio_direction_output(info->sensor_pwd, 0);
	usleep_range(5000, 5100);
	pip_ov5648_ctrl(PIP_CRL_POWERDOWN, NULL);
	usleep_range(5000, 5100);
	msm_sensor_power_down(s_ctrl);
	msleep(40);
	//if (s_ctrl->sensordata->pmic_gpio_enable){
	//  lcd_camera_power_onoff(0);
	//}
	return 0;
}

int32_t ov7695_raw_sensor_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int32_t rc = 0;
	struct msm_sensor_ctrl_t *s_ctrl;

	CDBG("%s IN\r\n", __func__);
	s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
	s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;
	/* PIP settings */
	ov7695_i2c_address = s_ctrl->sensor_i2c_addr;
	CDBG("%s:PIP front camera I2C addr:0x%x\r\n", __func__, ov7695_i2c_address);

	rc = msm_sensor_i2c_probe(client, id);

	if (client->dev.platform_data == NULL) {
		CDBG_HIGH("%s: NULL sensor data\n", __func__);
		return -EFAULT;
	}

	return rc;
}


static const struct i2c_device_id ov7695_raw_i2c_id[] = {
	{SENSOR_NAME, (kernel_ulong_t)&ov7695_raw_s_ctrl},
	{ }
};

static struct i2c_driver ov7695_raw_i2c_driver = {
	.id_table = ov7695_raw_i2c_id,
	.probe = ov7695_raw_sensor_i2c_probe,
	.driver = {
		.name = SENSOR_NAME,
	},
};

static struct msm_camera_i2c_client ov7695_raw_sensor_i2c_client = {
	.addr_type = MSM_CAMERA_I2C_WORD_ADDR,
};

int32_t ov7695_raw_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
			int update_type, int res)
{
	int32_t rc = 0;
	static int csi_config;
	struct msm_camera_pip_ctl pip_ctl;

//	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
	msleep(30);
	if (update_type == MSM_SENSOR_REG_INIT) {
		CDBG("Register INIT\n");
		pip_ctl.sensor_i2c_client = s_ctrl->sensor_i2c_client;
		/*OV7695 powerup and software reset*/
		pip_ov5648_ctrl(PIP_CRL_POWERUP, &pip_ctl);
		usleep_range(2000, 3000);
		pip_ov5648_ctrl(PIP_CTL_RESET_HW_PULSE, &pip_ctl);
		msleep(10);

		msm_camera_i2c_write(
				s_ctrl->sensor_i2c_client,
				0x103, 0x1,
				MSM_CAMERA_I2C_BYTE_DATA);

		usleep_range(2000, 3000);

		s_ctrl->curr_csi_params = NULL;
		msm_sensor_enable_debugfs(s_ctrl);
		msm_sensor_write_init_settings(s_ctrl);

		usleep_range(2000, 3000);
		pip_ov5648_ctrl(PIP_CTL_RESET_SW, &pip_ctl);

		usleep_range(2000, 3000);
		/*OV5648 init settings*/
		/*writing to OV5648, open lanes*/
		pip_ctl.write_ctl = PIP_REG_SETTINGS_INIT;
		pip_ov5648_ctrl(PIP_CRL_WRITE_SETTINGS, &pip_ctl);
		msleep(60);
		csi_config = 0;
	} else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
		CDBG("PERIODIC : %d\n", res);

		if (!csi_config) {
			pip_ctl.sensor_i2c_client = s_ctrl->sensor_i2c_client;
			pip_ov5648_ctrl(PIP_CTL_STREAM_OFF, &pip_ctl);
			msleep(70);
			pip_ov5648_ctrl(PIP_CTL_STANDBY_SW, &pip_ctl);
			msleep(10);

			msm_sensor_write_all_conf_array(
				s_ctrl->sensor_i2c_client, &ov7695_raw_confs[0],
				ARRAY_SIZE(ov7695_raw_confs));

//			msm_camera_i2c_write(
//					s_ctrl->sensor_i2c_client,
//					0x100, 0x1,
//					MSM_CAMERA_I2C_BYTE_DATA);
			msleep(60);

			pip_ctl.write_ctl = res;
			pip_ov5648_ctrl(PIP_CRL_WRITE_SETTINGS, &pip_ctl);
			msleep(30);

			s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
			CDBG("CSI config in progress\n");
			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_CSIC_CFG,
				s_ctrl->curr_csic_params);
			CDBG("CSI config is done\n");
			mb();
			msleep(30);
			csi_config = 1;

			v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
				NOTIFY_PCLK_CHANGE,
				&s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);

//			s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
			msleep(50);
			pip_ov5648_ctrl(PIP_CTL_STANDBY_EXIT, &pip_ctl);
			msleep(50);
			pip_ov5648_ctrl(PIP_CTL_STREAM_ON, &pip_ctl);
			msleep(50);
		}
	}
	return rc;
}

static int __init msm_sensor_init_module(void)
{
	return i2c_add_driver(&ov7695_raw_i2c_driver);
}

static struct v4l2_subdev_core_ops ov7695_raw_subdev_core_ops = {
	.ioctl = msm_sensor_subdev_ioctl,
	.s_power = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ov7695_raw_subdev_video_ops = {
	.enum_mbus_fmt = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ov7695_raw_subdev_ops = {
	.core = &ov7695_raw_subdev_core_ops,
	.video  = &ov7695_raw_subdev_video_ops,
};

static struct msm_sensor_fn_t ov7695_raw_func_tbl = {
	.sensor_start_stream = msm_sensor_start_stream,
	.sensor_stop_stream = msm_sensor_stop_stream,
	.sensor_set_fps = msm_sensor_set_fps,
	.sensor_write_exp_gain = ov7695_raw_write_exp_gain,
	.sensor_write_snapshot_exp_gain = ov7695_raw_write_exp_gain,
	.sensor_csi_setting = ov7695_raw_sensor_setting,
	.sensor_set_sensor_mode = msm_sensor_set_sensor_mode,
	.sensor_mode_init = msm_sensor_mode_init,
	.sensor_get_output_info = msm_sensor_get_output_info,
	.sensor_config = msm_sensor_config,
	.sensor_power_up = ov7695_raw_sensor_power_up,
	.sensor_power_down = ov7695_raw_sensor_power_down,
	.sensor_match_id   = msm_sensor_match_id,
};

static struct msm_sensor_reg_t ov7695_raw_regs = {
	.default_data_type = MSM_CAMERA_I2C_BYTE_DATA,
	.start_stream_conf = ov7695_raw_start_settings,
	.start_stream_conf_size = ARRAY_SIZE(ov7695_raw_start_settings),
	.stop_stream_conf = ov7695_raw_stop_settings,
	.stop_stream_conf_size = ARRAY_SIZE(ov7695_raw_stop_settings),
	.init_settings = &ov7695_raw_init_conf[0],
	.init_size = ARRAY_SIZE(ov7695_raw_init_conf),
	.mode_settings = &ov7695_raw_confs[0],
	.output_settings = &ov7695_raw_dimensions[0],
	.num_conf = ARRAY_SIZE(ov7695_raw_confs),
};

static struct msm_sensor_ctrl_t ov7695_raw_s_ctrl = {
	.msm_sensor_reg = &ov7695_raw_regs,
	.sensor_i2c_client = &ov7695_raw_sensor_i2c_client,
	.sensor_i2c_addr =  0x42,
	.sensor_output_reg_addr = &ov7695_raw_reg_addr,
	.sensor_id_info = &ov7695_raw_id_info,
	.sensor_exp_gain_info = &ov7695_raw_exp_gain_info,
	.cam_mode = MSM_SENSOR_MODE_INVALID,
	.csic_params = &ov7695_raw_csi_params_array[0],
	.msm_sensor_mutex = &ov7695_raw_mut,
	.sensor_i2c_driver = &ov7695_raw_i2c_driver,
	.sensor_v4l2_subdev_info = ov7695_raw_subdev_info,
	.sensor_v4l2_subdev_info_size = ARRAY_SIZE(ov7695_raw_subdev_info),
	.sensor_v4l2_subdev_ops = &ov7695_raw_subdev_ops,
	.func_tbl = &ov7695_raw_func_tbl,
	.clk_rate = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Omnivision BAYER sensor driver");
MODULE_LICENSE("GPL v2");
