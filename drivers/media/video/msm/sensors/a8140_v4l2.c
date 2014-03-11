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

#include "msm_sensor.h"
#include "msm.h"
#include "msm_ispif.h"
#include "msm_camera_i2c_mux.h"

#define SENSOR_NAME "a8140"
#define PLATFORM_DRIVER_NAME "msm_camera_a8140"
#define a8140_obj a8140_##obj

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define A8140_VERBOSE_DGB

#ifdef A8140_VERBOSE_DGB
#define LOG_TAG "a8140-v4l2"
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

static uint16_t a8140_prev_exp_gain = 0x012A;
static uint16_t a8140_prev_exp_line = 0x07E4;
struct a8140_otp_struct {
  uint16_t Module_info_0;   /* Year                               */
  uint16_t Module_info_1;   /* Date                               */
  uint16_t Module_info_2;   /* Bit[15:12]:Vendor                  */
                            /* Bit[11:8]:Version                  */
                            /* Bit[7:0]:Reserve                   */
  uint16_t Module_info_3;   /* Reserve by customer                */
  uint16_t Lens_type;       /* Lens Type,defined by customer      */
  uint16_t Vcm_type;        /* VCM Type,defined by customer       */
  uint16_t Driver_ic_type;  /* Driver IC Type,defined by customer */
  uint16_t Color_temp_type; /* Color Temperature Type             */
                            /* 00: Day Light                      */
                            /* 01: CWF Light                      */
                            /* 02: A Light                        */
  uint16_t R_Gr;            /* AWB information, R/Gr              */
  uint16_t B_Gr;            /* AWB information, B/Gr              */
  uint16_t Gb_Gr;           /* AWB information, Gb/Gr             */
  uint16_t Golden_R_G;      /* AWB information                    */
  uint16_t Golden_B_G;      /* AWB information                    */
  uint16_t Current_R_G;     /* AWB information                    */
  uint16_t Current_B_G;     /* AWB information                    */
  uint16_t Reserve_0;       /* Reserve for AWB information        */
  uint16_t AF_infinity;     /* AF information                     */
  uint16_t AF_macro;        /* AF information                     */
  uint16_t Reserve_1;       /* Reserve for AF information         */
  uint16_t Reserve_2;       /* Reserve                            */
  uint16_t LSC_data[106];   /* LSC Data                           */
} st_a8140_otp = {
  0x07dc,                   /* Year                               */
  0x0330,                   /* Date                               */
  0x0051,                   /* Bit[15:12]:Vendor                  */
                            /* Bit[11:8]:Version                  */
                            /* Bit[7:0]:Reserve                   */
  0x2100,                   /* Reserve by customer                */
  0x0000,                   /* Lens Type,defined by customer      */
  0x0000,                   /* VCM Type,defined by customer       */
  0x0000,                   /* Driver IC Type,defined by customer */
  0x0000,                   /* Color Temperature Type             */
                            /* 00: Day Light                      */
                            /* 01: CWF Light                      */
                            /* 02: A Light                        */
  0x02a9,                   /* AWB information, R/Gr              */
  0x0290,                   /* AWB information, B/Gr              */
  0x0400,                   /* AWB information, Gb/Gr             */
  0x0000,                   /* AWB information                    */
  0x0000,                   /* AWB information                    */
  0x0000,                   /* AWB information                    */
  0x0000,                   /* AWB information                    */
  0x0000,                   /* Reserve for AWB information        */
  0x0000,                   /* AF information                     */
  0x0000,                   /* AF information                     */
  0x0000,                   /* Reserve for AF information         */
  0x0000,                   /* Reserve                            */
                            /* LSC Data                           */
  {
    0x02b0,0x05ae,0x11d0,0x83ed,0xbbb0,0x0370,0xaced,0x66af,0x50ae,0x9390,
    0x0390,0x3e8e,0x050f,0x978e,0xcfcf,0x03f0,0x8b4e,0x16d0,0x5dce,0xc8b0,
    0xf12d,0xcfcd,0xbe4a,0x6e2d,0x288e,0x982d,0x256e,0x08cc,0xff8e,0x20ed,
    0x50ab,0x3c6e,0x0c8e,0x828f,0xe04e,0x500b,0xc00d,0x3e0d,0x018e,0x9c6e,
    0x7530,0x44ed,0x9093,0xad4e,0x3033,0x0111,0x832d,0xec92,0xba0c,0x03d3,
    0x51b0,0x514d,0xdeb2,0xb3ce,0x0593,0x7450,0x6d2c,0x91b3,0xa1ce,0x3173,
    0x41ec,0x032e,0x75af,0xa2ee,0xa071,0x616d,0x816f,0x9fac,0x0c10,0x91b0,
    0x9c2d,0xb6cf,0xb94e,0x1d50,0x1f2f,0xbc0c,0x538d,0xb82d,0x940e,0x830b,
    0xf271,0xc94f,0x7253,0x34b0,0xf353,0xf071,0x31cf,0x4193,0x8f10,0xad33,
    0xdd91,0xa750,0x4353,0x1171,0xbc73,0xec91,0x0cef,0x6af3,0x8d30,0xe633,
    0x062c,0x04e0,0x6908,0x73ea,0x24eb,0x58a9,
  }
};

static struct msm_sensor_ctrl_t a8140_s_ctrl;
DEFINE_MUTEX(a8140_mut);

static struct msm_camera_i2c_reg_conf a8140_start_stream_conf[] = {

};

static struct msm_camera_i2c_reg_conf a8140_stop_stream_conf[] = {

};

static struct msm_camera_i2c_reg_conf a8140_group_hold_on_conf[] = {
  {0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf a8140_group_hold_off_conf[] = {
  {0x0104, 0x00},
};

static struct msm_camera_i2c_reg_conf a8140_prev_settings[] = {
  //1632 x 1224 @30fps,MCLK=24MHz,2lane mipi,10bit raw
  {0x0344,0x0008},// X_ADDR_START
  {0x0348,0x0CC9},// X_ADDR_END
  {0x0346,0x0008},// Y_ADDR_START
  {0x034A,0x0999},// Y_ADDR_END
  {0x034C,0x0660},// X_OUTPUT_SIZE
  {0x034E,0x04C8},// Y_OUTPUT_SIZE
  {0x3040,0x00C1},// READ_MODE
  {0x3040,0x00C3},// READ_MODE
  {0x306E,0xFCB0},// DATAPATH_SELECT
  {0x3040,0x00C3},// READ_MODE
  {0x3040,0x00C3},// READ_MODE
  {0x3040,0x04C3},// READ_MODE
  {0x3040,0x04C3},// READ_MODE
  {0x3178,0x0000},// ANALOG_CONTROL5
  {0x3ED0,0x1E24},// DAC_LD_4_5
  {0x0400,0x0002},// SCALING_MODE
  {0x0404,0x0010},// SCALE_M
  {0x0342,0x1066},// LINE_LENGTH_PCK
  {0x0340,0x055B},// FRAME_LENGTH_LINES
  {0x0202,0x0557},// COARSE_INTEGRATION_TIME
  {0x3014,0x0846},// FINE_INTEGRATION_TIME_
  {0x3010,0x0130},// FINE_CORRECTION
};

static struct msm_camera_i2c_reg_conf a8140_snap_settings[] = {
  //Snapshot 2-lane mipi 3264 x 2448@14.3fps raw10
  {0x0344,0x0008},// X_ADDR_START
  {0x0348,0x0CC7},// X_ADDR_END
  {0x0346,0x0008},// Y_ADDR_START
  {0x034A,0x0997},// Y_ADDR_END
  {0x034C,0x0CC0},// X_OUTPUT_SIZE
  {0x034E,0x0990},// Y_OUTPUT_SIZE
  {0x3040,0x0041},// READ_MODE
  {0x3040,0x0041},// READ_MODE
  {0x306E,0xFC80},// DATAPATH_SELECT
  {0x3040,0x0041},// READ_MODE
  {0x3040,0x0041},// READ_MODE
  {0x3040,0x0041},// READ_MODE
  {0x3040,0x0041},// READ_MODE
  {0x3178,0x0000},// ANALOG_CONTROL5
  {0x3ED0,0x1E24},// DAC_LD_4_5
  {0x0400,0x0000},// SCALING_MODE
  {0x0404,0x0010},// SCALE_M
  {0x0342,0x1248},// LINE_LENGTH_PCK
  {0x0340,0x0A1F},// FRAME_LENGTH_LINES
  {0x0202,0x0A1F},// COARSE_INTEGRATION_TIME
  {0x3014,0x03F6},// FINE_INTEGRATION_TIME_
  {0x3010,0x0078},// FINE_CORRECTION
};

static struct msm_camera_i2c_reg_conf a8140_init_settings_conf_part1[] = {
  //2-lane MIPI Interface Configuration
  {0x3064,0x7800},
  {0x31AE,0x0202},
  {0x31B8,0x0E3F},
};
static struct msm_camera_i2c_reg_conf a8140_init_settings_conf_part2[] = {
  //Recommended Settings
  {0x3044,0x0590},
  {0x306E,0xFC80},
  {0x30B2,0xC000},
  {0x30D6,0x0800},
  {0x316C,0xB42F},
  {0x316E,0x869A},
  {0x3170,0x210E},
  {0x317A,0x010E},
  {0x31E0,0x1FB9},
  {0x31E6,0x07FC},
  {0x37C0,0x0000},
  {0x37C2,0x0000},
  {0x37C4,0x0000},
  {0x37C6,0x0000},
  {0x3E00,0x0011},
  {0x3E02,0x8801},
  {0x3E04,0x2801},
  {0x3E06,0x8449},
  {0x3E08,0x6841},
  {0x3E0A,0x400C},
  {0x3E0C,0x1001},
  {0x3E0E,0x2603},
  {0x3E10,0x4B41},
  {0x3E12,0x4B24},
  {0x3E14,0xA3CF},
  {0x3E16,0x8802},
  {0x3E18,0x84FF},
  {0x3E1A,0x8601},
  {0x3E1C,0x8401},
  {0x3E1E,0x840A},
  {0x3E20,0xFF00},
  {0x3E22,0x8401},
  {0x3E24,0x00FF},
  {0x3E26,0x0088},
  {0x3E28,0x2E8A},
  {0x3E30,0x0000},
  {0x3E32,0x8801},
  {0x3E34,0x4029},
  {0x3E36,0x00FF},
  {0x3E38,0x8469},
  {0x3E3A,0x00FF},
  {0x3E3C,0x2801},
  {0x3E3E,0x3E2A},
  {0x3E40,0x1C01},
  {0x3E42,0xFF84},
  {0x3E44,0x8401},
  {0x3E46,0x0C01},
  {0x3E48,0x8401},
  {0x3E4A,0x00FF},
  {0x3E4C,0x8402},
  {0x3E4E,0x8984},
  {0x3E50,0x6628},
  {0x3E52,0x8340},
  {0x3E54,0x00FF},
  {0x3E56,0x4A42},
  {0x3E58,0x2703},
  {0x3E5A,0x6752},
  {0x3E5C,0x3F2A},
  {0x3E5E,0x846A},
  {0x3E60,0x4C01},
  {0x3E62,0x8401},
  {0x3E66,0x3901},
  {0x3E90,0x2C01},
  {0x3E98,0x2B02},
  {0x3E92,0x2A04},
  {0x3E94,0x2509},
  {0x3E96,0x0000},
  {0x3E9A,0x2905},
  {0x3E9C,0x00FF},
  {0x3ECC,0x00EB},
  {0x3ED0,0x1E24},
  {0x3ED4,0xAFC4},
  {0x3ED6,0x909B},
  {0x3EE0,0x2424},
  {0x3EE4,0xC100},
  {0x3EE6,0x0540},
  {0x3174,0x8000},
  {0x0112,0x0A0A},// CCP_DATA_FORMAT
  //PLL Configuration
  {0x0300,0x0004},// VT_PIX_CLK_DIV
  {0x0302,0x0001},// VT_SYS_CLK_DIV
  {0x0304,0x0002},// PRE_PLL_CLK_DIV
  {0x0306,0x003A},// PLL_MULTIPLIER
  {0x0308,0x000A},// OP_PIX_CLK_DIV
  {0x030A,0x0001},// OP_SYS_CLK_DIV
};

static struct msm_camera_i2c_conf_array a8140_init_settings[] = {
  {&a8140_init_settings_conf_part1[0],
  ARRAY_SIZE(a8140_init_settings_conf_part1), 5, MSM_CAMERA_I2C_WORD_DATA},
  {&a8140_init_settings_conf_part2[0],
  ARRAY_SIZE(a8140_init_settings_conf_part2), 1, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array a8140_mode_settings[] = {
  {&a8140_snap_settings[0],
  ARRAY_SIZE(a8140_snap_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
  {&a8140_prev_settings[0],
  ARRAY_SIZE(a8140_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_csi_params a8140_csi_params = {
  .data_format               = CSI_10BIT,
  .lane_cnt                  = 2,
  .lane_assign               = 0xe4,
  .dpcm_scheme               = 0,
  .settle_cnt                = 10,
};

static struct v4l2_subdev_info a8140_subdev_info[] = {
  {
    .code                    = V4L2_MBUS_FMT_SGRBG10_1X10,
    .colorspace              = V4L2_COLORSPACE_JPEG,
    .fmt                     = 1,
    .order                   = 0,
  },
};

static struct msm_sensor_output_info_t a8140_output_settings[] = {
  { /* For SNAPSHOT */
    .x_output                = 0x0CC0,   /* 3264 */
    .y_output                = 0x0990,   /* 2448 */
    .line_length_pclk        = 0x1248,   /* 4680 */
    .frame_length_lines      = 0x0A1F,   /* 2591 */
    .vt_pixel_clk            = 174000000,/* =14.3*4680*2591 */
    .op_pixel_clk            = 174000000,
    .binning_factor          = 0x0,
  },
  { /* For PREVIEW */
    .x_output                = 0x0660,   /* 1632 */
    .y_output                = 0x04C8,   /* 1224 */
    .line_length_pclk        = 0x1066,   /* 4198 */
    .frame_length_lines      = 0x055B,   /* 1371 */
    .vt_pixel_clk            = 174000000,/* =30.2*4198*1371 */
    .op_pixel_clk            = 174000000,
    .binning_factor          = 0x0,
  },
};

static struct msm_sensor_output_reg_addr_t a8140_output_reg_addr = {
  .x_output                  = 0x034C,
  .y_output                  = 0x034E,
  .line_length_pclk          = 0x0342,
  .frame_length_lines        = 0x0340,
};

static struct msm_camera_csi_params *a8140_csi_params_array[] = {
  &a8140_csi_params,
  &a8140_csi_params,
};

static struct msm_sensor_id_info_t a8140_id_info = {
  .sensor_id_reg_addr        = 0x0000,
  .sensor_id                 = 0x4b00,
};

static struct msm_sensor_exp_gain_info_t a8140_exp_gain_info = {
  .coarse_int_time_addr      = 0x3012,
  .global_gain_addr          = 0x305E,
  .vert_offset               = 0,
};

static void a8140_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x8250,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x8650,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x8658,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x065c,
    MSM_CAMERA_I2C_WORD_DATA);
}

static void a8140_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x0058,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x0050,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static void a8140_group_hold_on(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static void a8140_group_hold_off(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static int32_t a8140_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
  uint16_t gain, uint32_t line)
{
  uint16_t max_legal_gain = 0x6FF; //16x
  uint16_t min_legal_gain = 0x60;  //1.5x
  int32_t rc = 0;
  if (gain < min_legal_gain) {
    CDBG("gain < min_legal_gain, gain = %d\n", gain);
    gain = min_legal_gain;
  }
  if (gain > max_legal_gain) {
    CDBG("gain > max_legal_gain, gain = %d\n", gain);
    gain = max_legal_gain;
  }
  a8140_prev_exp_gain = gain;
  a8140_prev_exp_line = line;
  CDBG("%s gain = 0x%x (%d)\n", __func__,gain,gain);
  CDBG("%s line = 0x%x (%d)\n", __func__,line,line);
  s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
  rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x305E,(gain|0x1000),
    MSM_CAMERA_I2C_WORD_DATA);
  rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3012,line,
    MSM_CAMERA_I2C_WORD_DATA);
  s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
  return rc;
}

static int32_t a8140_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
  uint16_t gain, uint32_t line)
{
  int32_t rc = 0;
  rc = a8140_write_prev_exp_gain(s_ctrl,gain,line);
  rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,(0x065C|0x2),
    MSM_CAMERA_I2C_WORD_DATA);
  return rc;
}

static const struct i2c_device_id a8140_i2c_id[] = {
  {SENSOR_NAME, (kernel_ulong_t)&a8140_s_ctrl},
  { }
};

int32_t a8140_sensor_i2c_probe(struct i2c_client *client,
  const struct i2c_device_id *id)
{
  int32_t rc = 0;
  struct msm_sensor_ctrl_t *s_ctrl;
  CDBG("%s E\n", __func__);
  s_ctrl = (struct msm_sensor_ctrl_t *)(id->driver_data);
  s_ctrl->sensor_i2c_addr = s_ctrl->sensor_i2c_addr;
  rc = msm_sensor_i2c_probe(client, id);
  if (client->dev.platform_data == NULL) {
    CDBG_HIGH("%s: NULL sensor data\n", __func__);
    return -EFAULT;
  }
  s_ctrl = client->dev.platform_data;
  usleep_range(5000, 5100);
  CDBG("%s X\n", __func__);
  return rc;
}

static struct i2c_driver a8140_i2c_driver = {
  .id_table                  = a8140_i2c_id,
  .probe                     = a8140_sensor_i2c_probe,
  .driver                    = {
    .name                    = SENSOR_NAME,
  },
};

static struct msm_camera_i2c_client a8140_sensor_i2c_client = {
  .addr_type                 = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
  return i2c_add_driver(&a8140_i2c_driver);
}

static struct v4l2_subdev_core_ops a8140_subdev_core_ops = {
  .ioctl                     = msm_sensor_subdev_ioctl,
  .s_power                   = msm_sensor_power,
};

static struct v4l2_subdev_video_ops a8140_subdev_video_ops = {
  .enum_mbus_fmt             = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops a8140_subdev_ops = {
  .core                      = &a8140_subdev_core_ops,
  .video                     = &a8140_subdev_video_ops,
};

int32_t a8140_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
{
  struct msm_camera_sensor_info *info = NULL;
  CDBG("%s: E\n",__func__);
  info = s_ctrl->sensordata;
  s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
  msleep(100);
  gpio_direction_output(info->sensor_pwd, 0);
  gpio_direction_output(info->sensor_reset, 0);
  usleep_range(5000, 5100);
  msm_sensor_power_down(s_ctrl);
  msleep(40);
  a8140_prev_exp_gain = 0x012A;
  a8140_prev_exp_line = 0x07E4;
  CDBG("%s: X\n",__func__);
  return 0;
}

int32_t a8140_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
  int32_t rc = 0;
  struct msm_camera_sensor_info *info = NULL;
  info = s_ctrl->sensordata;
  CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__,
    info->sensor_pwd,info->sensor_reset);
  gpio_direction_output(info->sensor_pwd, 0);
  gpio_direction_output(info->sensor_reset, 0);
  usleep_range(10000, 11000);
  usleep_range(5000, 5100);
  rc = msm_sensor_power_up(s_ctrl);
  if (rc < 0) {
    CDBG("%s: msm_sensor_power_up failed\n", __func__);
    return rc;
  }
  usleep_range(5000, 5100);
  gpio_direction_output(info->sensor_pwd, 1);
  usleep_range(5000, 5100);
  gpio_direction_output(info->sensor_reset, 1);
  usleep_range(5000, 5100);
  return rc;
}

int32_t a8140_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
  int32_t rc = 0;
  uint16_t chipid = 0;
  rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_id_info->sensor_id_reg_addr, &chipid,
    MSM_CAMERA_I2C_WORD_DATA);
  if (rc < 0) {
    CDBG("%s: %s: read id failed\n", __func__,
      s_ctrl->sensordata->sensor_name);
    return rc;
  }
  CDBG("%s: chipid = 0x%x\n", __func__,chipid);
  if (chipid != s_ctrl->sensor_id_info->sensor_id) {
    CDBG("%s: chip id mismatch\n",__func__);
    return -ENODEV;
  }
  return rc;
}
/*******************************************************************************
* otp_type: otp_type should be 0x30,0x31,0x32
* return value:
*     0, OTPM have no valid data
*     1, OTPM have valid data
*******************************************************************************/
int a8140_check_otp_status(struct msm_sensor_ctrl_t *s_ctrl, int otp_type)
{
  uint16_t i = 0;
  uint16_t temp = 0;
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x0610,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3134,0xCD95,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x304C,(otp_type&0xff)<<8,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x304A,0x0200,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x304A,0x0210,
    MSM_CAMERA_I2C_WORD_DATA);

  do{
    msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x304A,&temp,
      MSM_CAMERA_I2C_WORD_DATA);
    if(0x60 == (temp & 0x60)){
      CDBG("%s: read success\n", __func__);
      break;
    }
    usleep_range(5000, 5100);
    i++;
  }while(i < 10);

  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x38FC,&temp,
    MSM_CAMERA_I2C_WORD_DATA);

  if(0xffff==temp){
    //OTPM have valid data
    return 1;
  }else{
    //OTPM have no valid data
    return 0;
  }
}

/*******************************************************************************
* return value:
*     0, OTPM have no valid data
*     1, OTPM have valid data
*******************************************************************************/
int a8140_read_otp(struct msm_sensor_ctrl_t *s_ctrl, struct a8140_otp_struct *p_otp)
{
  uint16_t i = 0;
  int otp_status = 0;//initial OTPM have no valid data status

  if(a8140_check_otp_status(s_ctrl,0x35)){
    //type 0x35 have valid data, now use type 0x35
    otp_status = 1;
  }else{
    if(a8140_check_otp_status(s_ctrl,0x34)){
      //type 0x34 have valid data, now use type 0x34
      otp_status = 1;
    }else{
      if(a8140_check_otp_status(s_ctrl,0x33)){
        //type 0x33 have valid data, now use type 0x33
        otp_status = 1;
      }else{
        if(a8140_check_otp_status(s_ctrl,0x32)){
          //type 0x32 have valid data, now use type 0x32
          otp_status = 1;
        }else{
          if(a8140_check_otp_status(s_ctrl,0x31)){
            //type 0x31 have valid data, now use type 0x31
            otp_status = 1;
          }else{
            if(a8140_check_otp_status(s_ctrl,0x30)){
              //type 0x30 have valid data, now use type 0x30
              otp_status = 1;
            }else{
              //all types 0x35-0x30 have no valid data, no otp data
              ;
            }
          }
        }
      }
    }
  }

  if(otp_status){
    for (i = 0; i < (sizeof(st_a8140_otp)/2); i++) {
      msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (0x3800+i*2),
        ((uint16_t *)p_otp +i), MSM_CAMERA_I2C_WORD_DATA);
    }
  }else{
    CDBG("%s: have no valid otp data\n",__func__);
  }
#if 0
  CDBG("%s: Module_info_0   = 0x%04x (%d)\n",__func__,p_otp->Module_info_0,   p_otp->Module_info_0);
  CDBG("%s: Module_info_1   = 0x%04x (%d)\n",__func__,p_otp->Module_info_1,   p_otp->Module_info_1);
  CDBG("%s: Module_info_2   = 0x%04x (%d)\n",__func__,p_otp->Module_info_2,   p_otp->Module_info_2);
  CDBG("%s: Module_info_3   = 0x%04x (%d)\n",__func__,p_otp->Module_info_3,   p_otp->Module_info_3);
  CDBG("%s: Lens_type       = 0x%04x (%d)\n",__func__,p_otp->Lens_type,       p_otp->Lens_type);
  CDBG("%s: Vcm_type        = 0x%04x (%d)\n",__func__,p_otp->Vcm_type,        p_otp->Vcm_type);
  CDBG("%s: Driver_ic_type  = 0x%04x (%d)\n",__func__,p_otp->Driver_ic_type,  p_otp->Driver_ic_type);
  CDBG("%s: Color_temp_type = 0x%04x (%d)\n",__func__,p_otp->Color_temp_type, p_otp->Color_temp_type);
  CDBG("%s: R_Gr            = 0x%04x (%d)\n",__func__,p_otp->R_Gr,            p_otp->R_Gr);
  CDBG("%s: B_Gr            = 0x%04x (%d)\n",__func__,p_otp->B_Gr,            p_otp->B_Gr);
  CDBG("%s: Gb_Gr           = 0x%04x (%d)\n",__func__,p_otp->Gb_Gr,           p_otp->Gb_Gr);
  CDBG("%s: Golden_R_G      = 0x%04x (%d)\n",__func__,p_otp->Golden_R_G,      p_otp->Golden_R_G);
  CDBG("%s: Golden_B_G      = 0x%04x (%d)\n",__func__,p_otp->Golden_B_G,      p_otp->Golden_B_G);
  CDBG("%s: Current_R_G     = 0x%04x (%d)\n",__func__,p_otp->Current_R_G,     p_otp->Current_R_G);
  CDBG("%s: Current_B_G     = 0x%04x (%d)\n",__func__,p_otp->Current_B_G,     p_otp->Current_B_G);
  CDBG("%s: Reserve_0       = 0x%04x (%d)\n",__func__,p_otp->Reserve_0,       p_otp->Reserve_0);
  CDBG("%s: AF_infinity     = 0x%04x (%d)\n",__func__,p_otp->AF_infinity,     p_otp->AF_infinity);
  CDBG("%s: AF_macro        = 0x%04x (%d)\n",__func__,p_otp->AF_macro,        p_otp->AF_macro);
  CDBG("%s: Reserve_1       = 0x%04x (%d)\n",__func__,p_otp->Reserve_1,       p_otp->Reserve_1);
  CDBG("%s: Reserve_2       = 0x%04x (%d)\n",__func__,p_otp->Reserve_2,       p_otp->Reserve_2);
  CDBG("%s: LSC_data[0]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[0],     p_otp->LSC_data[0]);
  CDBG("%s: LSC_data[1]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[1],     p_otp->LSC_data[1]);
  CDBG("%s: LSC_data[2]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[2],     p_otp->LSC_data[2]);
  CDBG("%s: LSC_data[3]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[3],     p_otp->LSC_data[3]);
  CDBG("%s: LSC_data[4]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[4],     p_otp->LSC_data[4]);
  CDBG("%s: LSC_data[5]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[5],     p_otp->LSC_data[5]);
  CDBG("%s: LSC_data[6]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[6],     p_otp->LSC_data[6]);
  CDBG("%s: LSC_data[7]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[7],     p_otp->LSC_data[7]);
  CDBG("%s: LSC_data[8]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[8],     p_otp->LSC_data[8]);
  CDBG("%s: LSC_data[9]     = 0x%04x (%d)\n",__func__,p_otp->LSC_data[9],     p_otp->LSC_data[9]);
  CDBG("%s: LSC_data[10]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[10],    p_otp->LSC_data[10]);
  CDBG("%s: LSC_data[11]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[11],    p_otp->LSC_data[11]);
  CDBG("%s: LSC_data[12]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[12],    p_otp->LSC_data[12]);
  CDBG("%s: LSC_data[13]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[13],    p_otp->LSC_data[13]);
  CDBG("%s: LSC_data[14]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[14],    p_otp->LSC_data[14]);
  CDBG("%s: LSC_data[15]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[15],    p_otp->LSC_data[15]);
  CDBG("%s: LSC_data[16]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[16],    p_otp->LSC_data[16]);
  CDBG("%s: LSC_data[17]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[17],    p_otp->LSC_data[17]);
  CDBG("%s: LSC_data[18]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[18],    p_otp->LSC_data[18]);
  CDBG("%s: LSC_data[19]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[19],    p_otp->LSC_data[19]);
  CDBG("%s: LSC_data[20]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[20],    p_otp->LSC_data[20]);
  CDBG("%s: LSC_data[21]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[21],    p_otp->LSC_data[21]);
  CDBG("%s: LSC_data[22]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[22],    p_otp->LSC_data[22]);
  CDBG("%s: LSC_data[23]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[23],    p_otp->LSC_data[23]);
  CDBG("%s: LSC_data[24]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[24],    p_otp->LSC_data[24]);
  CDBG("%s: LSC_data[25]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[25],    p_otp->LSC_data[25]);
  CDBG("%s: LSC_data[26]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[26],    p_otp->LSC_data[26]);
  CDBG("%s: LSC_data[27]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[27],    p_otp->LSC_data[27]);
  CDBG("%s: LSC_data[28]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[28],    p_otp->LSC_data[28]);
  CDBG("%s: LSC_data[29]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[29],    p_otp->LSC_data[29]);
  CDBG("%s: LSC_data[30]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[30],    p_otp->LSC_data[30]);
  CDBG("%s: LSC_data[31]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[31],    p_otp->LSC_data[31]);
  CDBG("%s: LSC_data[32]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[32],    p_otp->LSC_data[32]);
  CDBG("%s: LSC_data[33]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[33],    p_otp->LSC_data[33]);
  CDBG("%s: LSC_data[34]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[34],    p_otp->LSC_data[34]);
  CDBG("%s: LSC_data[35]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[35],    p_otp->LSC_data[35]);
  CDBG("%s: LSC_data[36]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[36],    p_otp->LSC_data[36]);
  CDBG("%s: LSC_data[37]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[37],    p_otp->LSC_data[37]);
  CDBG("%s: LSC_data[38]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[38],    p_otp->LSC_data[38]);
  CDBG("%s: LSC_data[39]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[39],    p_otp->LSC_data[39]);
  CDBG("%s: LSC_data[40]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[40],    p_otp->LSC_data[40]);
  CDBG("%s: LSC_data[41]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[41],    p_otp->LSC_data[41]);
  CDBG("%s: LSC_data[42]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[42],    p_otp->LSC_data[42]);
  CDBG("%s: LSC_data[43]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[43],    p_otp->LSC_data[43]);
  CDBG("%s: LSC_data[44]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[44],    p_otp->LSC_data[44]);
  CDBG("%s: LSC_data[45]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[45],    p_otp->LSC_data[45]);
  CDBG("%s: LSC_data[46]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[46],    p_otp->LSC_data[46]);
  CDBG("%s: LSC_data[47]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[47],    p_otp->LSC_data[47]);
  CDBG("%s: LSC_data[48]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[48],    p_otp->LSC_data[48]);
  CDBG("%s: LSC_data[49]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[49],    p_otp->LSC_data[49]);
  CDBG("%s: LSC_data[50]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[50],    p_otp->LSC_data[50]);
  CDBG("%s: LSC_data[51]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[51],    p_otp->LSC_data[51]);
  CDBG("%s: LSC_data[52]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[52],    p_otp->LSC_data[52]);
  CDBG("%s: LSC_data[53]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[53],    p_otp->LSC_data[53]);
  CDBG("%s: LSC_data[54]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[54],    p_otp->LSC_data[54]);
  CDBG("%s: LSC_data[55]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[55],    p_otp->LSC_data[55]);
  CDBG("%s: LSC_data[56]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[56],    p_otp->LSC_data[56]);
  CDBG("%s: LSC_data[57]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[57],    p_otp->LSC_data[57]);
  CDBG("%s: LSC_data[58]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[58],    p_otp->LSC_data[58]);
  CDBG("%s: LSC_data[59]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[59],    p_otp->LSC_data[59]);
  CDBG("%s: LSC_data[60]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[60],    p_otp->LSC_data[60]);
  CDBG("%s: LSC_data[61]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[61],    p_otp->LSC_data[61]);
  CDBG("%s: LSC_data[62]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[62],    p_otp->LSC_data[62]);
  CDBG("%s: LSC_data[63]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[63],    p_otp->LSC_data[63]);
  CDBG("%s: LSC_data[64]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[64],    p_otp->LSC_data[64]);
  CDBG("%s: LSC_data[65]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[65],    p_otp->LSC_data[65]);
  CDBG("%s: LSC_data[66]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[66],    p_otp->LSC_data[66]);
  CDBG("%s: LSC_data[67]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[67],    p_otp->LSC_data[67]);
  CDBG("%s: LSC_data[68]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[68],    p_otp->LSC_data[68]);
  CDBG("%s: LSC_data[69]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[69],    p_otp->LSC_data[69]);
  CDBG("%s: LSC_data[70]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[70],    p_otp->LSC_data[70]);
  CDBG("%s: LSC_data[71]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[71],    p_otp->LSC_data[71]);
  CDBG("%s: LSC_data[72]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[72],    p_otp->LSC_data[72]);
  CDBG("%s: LSC_data[73]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[73],    p_otp->LSC_data[73]);
  CDBG("%s: LSC_data[74]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[74],    p_otp->LSC_data[74]);
  CDBG("%s: LSC_data[75]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[75],    p_otp->LSC_data[75]);
  CDBG("%s: LSC_data[76]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[76],    p_otp->LSC_data[76]);
  CDBG("%s: LSC_data[77]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[77],    p_otp->LSC_data[77]);
  CDBG("%s: LSC_data[78]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[78],    p_otp->LSC_data[78]);
  CDBG("%s: LSC_data[79]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[79],    p_otp->LSC_data[79]);
  CDBG("%s: LSC_data[80]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[80],    p_otp->LSC_data[80]);
  CDBG("%s: LSC_data[81]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[81],    p_otp->LSC_data[81]);
  CDBG("%s: LSC_data[82]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[82],    p_otp->LSC_data[82]);
  CDBG("%s: LSC_data[83]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[83],    p_otp->LSC_data[83]);
  CDBG("%s: LSC_data[84]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[84],    p_otp->LSC_data[84]);
  CDBG("%s: LSC_data[85]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[85],    p_otp->LSC_data[85]);
  CDBG("%s: LSC_data[86]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[86],    p_otp->LSC_data[86]);
  CDBG("%s: LSC_data[87]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[87],    p_otp->LSC_data[87]);
  CDBG("%s: LSC_data[88]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[88],    p_otp->LSC_data[88]);
  CDBG("%s: LSC_data[89]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[89],    p_otp->LSC_data[89]);
  CDBG("%s: LSC_data[90]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[90],    p_otp->LSC_data[90]);
  CDBG("%s: LSC_data[91]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[91],    p_otp->LSC_data[91]);
  CDBG("%s: LSC_data[92]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[92],    p_otp->LSC_data[92]);
  CDBG("%s: LSC_data[93]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[93],    p_otp->LSC_data[93]);
  CDBG("%s: LSC_data[94]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[94],    p_otp->LSC_data[94]);
  CDBG("%s: LSC_data[95]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[95],    p_otp->LSC_data[95]);
  CDBG("%s: LSC_data[96]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[96],    p_otp->LSC_data[96]);
  CDBG("%s: LSC_data[97]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[97],    p_otp->LSC_data[97]);
  CDBG("%s: LSC_data[98]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[98],    p_otp->LSC_data[98]);
  CDBG("%s: LSC_data[99]    = 0x%04x (%d)\n",__func__,p_otp->LSC_data[99],    p_otp->LSC_data[99]);
  CDBG("%s: LSC_data[100]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[100],   p_otp->LSC_data[100]);
  CDBG("%s: LSC_data[101]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[101],   p_otp->LSC_data[101]);
  CDBG("%s: LSC_data[102]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[102],   p_otp->LSC_data[102]);
  CDBG("%s: LSC_data[103]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[103],   p_otp->LSC_data[103]);
  CDBG("%s: LSC_data[104]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[104],   p_otp->LSC_data[104]);
  CDBG("%s: LSC_data[105]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[105],   p_otp->LSC_data[105]);
#endif
  return otp_status;
}

void a8140_write_otp(struct msm_sensor_ctrl_t *s_ctrl, struct a8140_otp_struct *p_otp)
{
  uint16_t i = 0;
  uint16_t temp = 0;
  //0 - 20 words
  for (i = 0; i < 20; i++) {
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, (0x3600+i*2),
      p_otp->LSC_data[i], MSM_CAMERA_I2C_WORD_DATA);
  }
  //20 - 40 words
  for (i = 0; i < 20; i++) {
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, (0x3640+i*2),
      p_otp->LSC_data[20+i], MSM_CAMERA_I2C_WORD_DATA);
  }
  //40 - 60 words
  for (i = 0; i < 20; i++) {
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, (0x3680+i*2),
      p_otp->LSC_data[40+i], MSM_CAMERA_I2C_WORD_DATA);
  }
  //60 - 80 words
  for (i = 0; i < 20; i++) {
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, (0x36c0+i*2),
      p_otp->LSC_data[60+i], MSM_CAMERA_I2C_WORD_DATA);
  }
  //80 - 100 words
  for (i = 0; i < 20; i++) {
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, (0x3700+i*2),
      p_otp->LSC_data[80+i], MSM_CAMERA_I2C_WORD_DATA);
  }

  //last 6 words
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3782,
    p_otp->LSC_data[100], MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x3784,
    p_otp->LSC_data[101], MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C0,
    p_otp->LSC_data[102], MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C2,
    p_otp->LSC_data[103], MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C4,
    p_otp->LSC_data[104], MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x37C6,
    p_otp->LSC_data[105], MSM_CAMERA_I2C_WORD_DATA);

  //enable poly_sc_enable Bit
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x3780,&temp,
    MSM_CAMERA_I2C_WORD_DATA);
  temp = temp | 0x8000;
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3780,temp,
    MSM_CAMERA_I2C_WORD_DATA);
}

void a8140_update_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
  a8140_read_otp(s_ctrl, &st_a8140_otp);
  a8140_write_otp(s_ctrl, &st_a8140_otp);
}

int32_t a8140_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
  int update_type, int res)
{
  int32_t rc = 0;
  static int csi_config;
  static unsigned short af_reg_dac;
  static unsigned short af_reg_damp;
  int af_step_pos;

  if (update_type == MSM_SENSOR_REG_INIT) {
    CDBG("Register INIT\n");
    s_ctrl->curr_csi_params = NULL;
    msm_sensor_enable_debugfs(s_ctrl);
      msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0103,0x01,
      MSM_CAMERA_I2C_BYTE_DATA);
      msleep(100);
      s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
    rc = msm_sensor_write_init_settings(s_ctrl);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x00,
      MSM_CAMERA_I2C_BYTE_DATA);
    csi_config = 0;
    a8140_update_otp(s_ctrl);
  } else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
    CDBG("PERIODIC : %d\n", res);
    msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x30f2, &af_reg_dac,
      MSM_CAMERA_I2C_WORD_DATA);
    msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x30f4, &af_reg_damp,
      MSM_CAMERA_I2C_WORD_DATA);
    //set to zero to avoid lens crash sound
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f4, 0x0,
      MSM_CAMERA_I2C_WORD_DATA);
    for(af_step_pos = af_reg_dac&0xff; af_step_pos > 0; af_step_pos-=8){
      msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f2, af_step_pos&0xff,
        MSM_CAMERA_I2C_WORD_DATA);
      msleep(2);
    }
    s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
    if(res == 0){
      msleep(105);// preview -> capture
    }else{
      msleep(205);// capture -> preview
    }
    s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
    rc = msm_sensor_write_conf_array(s_ctrl->sensor_i2c_client,
      s_ctrl->msm_sensor_reg->mode_settings, res);
   if (!csi_config)
      msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0100,0x01,
        MSM_CAMERA_I2C_BYTE_DATA);
    s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
    CDBG("PERIODIC : register setting is done\n");
    if(res == 0){
      // preview -> capture
      ;
    }else{
      // capture -> preview
      s_ctrl->func_tbl->sensor_write_exp_gain(s_ctrl,
        a8140_prev_exp_gain,a8140_prev_exp_line);
    }
    if (!csi_config) {
      s_ctrl->curr_csic_params = s_ctrl->csic_params[res];
      v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
        NOTIFY_CSIC_CFG,s_ctrl->curr_csic_params);
      mb();
      msleep(30);
      csi_config = 1;
    }
    v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
      NOTIFY_PCLK_CHANGE,
      &s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);
    s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
/*
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f4, af_reg_damp,
      MSM_CAMERA_I2C_WORD_DATA);
*/
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f4, 0x21e,
      MSM_CAMERA_I2C_WORD_DATA);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f2, af_reg_dac,
      MSM_CAMERA_I2C_WORD_DATA);
    msleep(50);
  }
  return rc;
}

static struct msm_sensor_fn_t a8140_func_tbl = {
  .sensor_start_stream            = a8140_start_stream,
  .sensor_stop_stream             = a8140_stop_stream,
  .sensor_group_hold_on           = a8140_group_hold_on,
  .sensor_group_hold_off          = a8140_group_hold_off,
  .sensor_set_fps                 = msm_sensor_set_fps,
  .sensor_write_exp_gain          = a8140_write_prev_exp_gain,
  .sensor_write_snapshot_exp_gain = a8140_write_pict_exp_gain,
  .sensor_csi_setting             = a8140_sensor_setting,
  .sensor_set_sensor_mode         = msm_sensor_set_sensor_mode,
  .sensor_mode_init               = msm_sensor_mode_init,
  .sensor_get_output_info         = msm_sensor_get_output_info,
  .sensor_config                  = msm_sensor_config,
  .sensor_power_up                = a8140_sensor_power_up,
  .sensor_power_down              = a8140_sensor_power_down,
  .sensor_match_id                = a8140_sensor_match_id,
  .sensor_get_csi_params          = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t a8140_regs = {
  .default_data_type              = MSM_CAMERA_I2C_BYTE_DATA,
  .start_stream_conf              = a8140_start_stream_conf,
  .start_stream_conf_size         = ARRAY_SIZE(a8140_start_stream_conf),
  .stop_stream_conf               = a8140_stop_stream_conf,
  .stop_stream_conf_size          = ARRAY_SIZE(a8140_stop_stream_conf),
  .group_hold_on_conf             = a8140_group_hold_on_conf,
  .group_hold_on_conf_size        = ARRAY_SIZE(a8140_group_hold_on_conf),
  .group_hold_off_conf            = a8140_group_hold_off_conf,
  .group_hold_off_conf_size       = ARRAY_SIZE(a8140_group_hold_off_conf),
  .init_settings                  = &a8140_init_settings[0],
  .init_size                      = ARRAY_SIZE(a8140_init_settings),
  .mode_settings                  = &a8140_mode_settings[0],
  .output_settings                = &a8140_output_settings[0],
  .num_conf                       = ARRAY_SIZE(a8140_mode_settings),
};

static struct msm_sensor_ctrl_t a8140_s_ctrl = {
  .msm_sensor_reg                 = &a8140_regs,
  .sensor_i2c_client              = &a8140_sensor_i2c_client,
  .sensor_i2c_addr                = 0x6C,
  .sensor_output_reg_addr         = &a8140_output_reg_addr,
  .sensor_id_info                 = &a8140_id_info,
  .sensor_exp_gain_info           = &a8140_exp_gain_info,
  .cam_mode                       = MSM_SENSOR_MODE_INVALID,
  .csic_params                    = &a8140_csi_params_array[0],
  .msm_sensor_mutex               = &a8140_mut,
  .sensor_i2c_driver              = &a8140_i2c_driver,
  .sensor_v4l2_subdev_info        = a8140_subdev_info,
  .sensor_v4l2_subdev_info_size   = ARRAY_SIZE(a8140_subdev_info),
  .sensor_v4l2_subdev_ops         = &a8140_subdev_ops,
  .func_tbl                       = &a8140_func_tbl,
  .clk_rate                       = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Aptina 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
