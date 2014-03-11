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
#include "msm_ispif.h"
#include "msm_camera_i2c_mux.h"

#define SENSOR_NAME "ar0542"
#define PLATFORM_DRIVER_NAME "msm_camera_ar0542"
#define ar0542_obj ar0542_##obj

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define AR0542_VERBOSE_DGB

#ifdef AR0542_VERBOSE_DGB
#define LOG_TAG "ar0542-v4l2"
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

static uint16_t ar0542_prev_exp_gain = 0x012A;
static uint16_t ar0542_prev_exp_line = 0x07E4;
struct ar0542_otp_struct {
  uint16_t Module_info_0;   /* Year                               */
  uint16_t Module_info_1;   /* Date                               */
  uint16_t Module_info_2;   /* MID                                */
  uint16_t Reserve_0;       /* Reserved                           */
  uint16_t R_Gr;            /* AWB information, R/Gr              */
  uint16_t B_Gr;            /* AWB information, B/Gr              */
  uint16_t Gb_Gr;           /* AWB information, Gb/Gr             */
  uint16_t Reserve_1;       /* Reserved                           */
  uint16_t LSC_data[106];   /* LSC Data                           */
  uint16_t Reserve_2;       /* Reserved                           */
  uint16_t AF_infinity;     /* AF information                     */
  uint16_t AF_macro;        /* AF information                     */
  uint16_t AF_data;         /* AF information                     */
  uint16_t OTP_flag_0;      /* OTP_flag information               */
  uint16_t OTP_flag_1;      /* OTP_flag information               */
} st_ar0542_otp = {
  0x07dc,                   /* Year                               */
  0x03f9,                   /* Date                               */
  0x000a,                   /* MID                                */
  0x0000,                   /* Reserved                           */
  0x0271,                   /* AWB information, R/Gr              */
  0x0236,                   /* AWB information, B/Gr              */
  0x0400,                   /* AWB information, Gb/Gr             */
  0x0000,                   /* Reserved                           */
                            /* LSC Data                           */
  {
    0x0170,0x144c,0x7f50,0x9ded,0xe830,0x0130,0x8dea,0x0a51,0x778d,0x87f1,
    0x00d0,0x204b,0x798f,0x838e,0xc9af,0x00f0,0x8e89,0x0991,0x6845,0x8a31,
    0x852b,0x87ae,0xa7af,0x742c,0x17d0,0x14eb,0x59ed,0xe0ce,0xbcae,0x1baf,
    0x17ac,0x4bed,0xab0b,0xac0e,0x9c69,0x50ca,0x972e,0xf78c,0x654c,0x4dec,
    0x01d1,0xa5ab,0x8591,0x53ac,0xfa31,0x2751,0xc14e,0x8271,0x9650,0xbbd2,
    0x4230,0xe50c,0xf9ef,0x74af,0xd131,0x74f0,0xad8e,0x90d1,0xde2d,0xab11,
    0xc26e,0xdc8c,0x6e2f,0x1550,0xabf1,0x1168,0x9cce,0xa28f,0x0e10,0xd20e,
    0xc60e,0xd6ce,0x150f,0x6f6f,0x9451,0x838f,0x67cd,0x07cf,0x6bcf,0x89f1,
    0xde70,0xe50e,0xbfd3,0x2631,0x6b55,0x8031,0xb86e,0x9074,0x5ab2,0x1196,
    0xb990,0x95ef,0xb193,0x7a70,0x4515,0xb910,0x0e8c,0xcdf3,0x0972,0x6635,
    0x0530,0x03c8,0x200b,0x16cc,0x326c,0x2d2b,
  },
  0xffff,                   /* Reserved                           */
  0x0000,                   /* AF information                     */
  0x0000,                   /* AF information                     */
  0x0000,                   /* AF information                     */
  0xffff,                   /* OTP_flag information               */
  0xffff,                   /* OTP_flag information               */
};

static struct msm_sensor_ctrl_t ar0542_s_ctrl;
DEFINE_MUTEX(ar0542_mut);

static struct msm_camera_i2c_reg_conf ar0542_start_stream_conf[] = {

};

static struct msm_camera_i2c_reg_conf ar0542_stop_stream_conf[] = {

};

static struct msm_camera_i2c_reg_conf ar0542_group_hold_on_conf[] = {
  {0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf ar0542_group_hold_off_conf[] = {
  {0x0104, 0x00},
};

static struct msm_camera_i2c_reg_conf ar0542_prev_settings[] = {
  //Preview 1296x972 30fps
  {0x3004, 0x0008},//x_addr_start
  {0x3008, 0x0A25},//x_addr_end
  {0x3002, 0x0008},//y_start_addr
  {0x3006, 0x079D},//y_addr_end
  {0x3040, 0x04C3},//read_mode
  {0x034C, 0x0510},//x_output_size
  {0x034E, 0x03CC},//y_output_size
  {0x300C, 0x0CF0},//line_length_pck
  {0x300A, 0x0457},//frame_length_lines
};

static struct msm_camera_i2c_reg_conf ar0542_snap_settings[] = {
  //Snapshot 2592*1944 14.8fps
  {0x3004, 0x0008},/*x_addr_start*/
  {0x3008, 0x0A27},/*x_addr_end*/
  {0x3002, 0x0008},/*y_start_addr*/
  {0x3006, 0x079f},/*y_addr_end*/
  {0x3040, 0x0041},/*read_mode*/
  {0x034C, 0x0A20},/*x_output_size*/
  {0x034E, 0x0798},/*y_output_size*/
  {0x300C, 0x0E70},/*line_length_pck*/
  {0x300A, 0x07E5},/*frame_length_lines*/
  {0x3012, 0x07E4},/*coarse_integration_time*/
  {0x3014, 0x0C8C},/*fine_integration_time*/
  {0x3010, 0x00A0},/*fine_correction*/
};

static struct msm_camera_i2c_reg_conf ar0542_video_60fps_settings[] = {
  {0x3004, 0x0008},/*x_addr_start*/
  {0x3008, 0x0A01},/*x_addr_end*/
  {0x3002, 0x0008},/*y_start_addr*/
  {0x3006, 0x0781},/*y_addr_end*/
  {0x3040, 0x09C7},/*read_mode*/
  {0x034C, 0x0280},/*x_output_size*/
  {0x034E, 0x01E0},/*y_output_size*/
  {0x300C, 0x0CE6},/*line_length_pck*/
  {0x300A, 0x022D},/*frame_length_lines*/
  {0x3012, 0x022C},/*coarse_integration_time*/
  {0x3014, 0x04EC},/*fine_integration_time*/
  {0x3010, 0x00A0},/*fine_correction*/
};

static struct msm_camera_i2c_reg_conf ar0542_video_90fps_settings[] = {
  {0x3004, 0x0008},/*x_addr_start*/
  {0x3008, 0x0A01},/*x_addr_end*/
  {0x3002, 0x0008},/*y_start_addr*/
  {0x3006, 0x0781},/*y_addr_end*/
  {0x3040, 0x09C7},/*read_mode*/
  {0x034C, 0x0280},/*x_output_size*/
  {0x034E, 0x01E0},/*y_output_size*/
  {0x300C, 0x089A},/*line_length_pck*/
  {0x300A, 0x022D},/*frame_length_lines*/
  {0x3012, 0x022C},/*coarse_integration_time*/
  {0x3014, 0x04EC},/*fine_integration_time*/
  {0x3010, 0x00A0},/*fine_correction*/
};

static struct msm_camera_i2c_reg_conf ar0542_zsl_settings[] = {
  {0x3004, 0x0008},/*x_addr_start*/
  {0x3008, 0x0A27},/*x_addr_end*/
  {0x3002, 0x0008},/*y_start_addr*/
  {0x3006, 0x079f},/*y_addr_end*/
  {0x3040, 0x0041},/*read_mode*/
  {0x034C, 0x0A20},/*x_output_size*/
  {0x034E, 0x0798},/*y_output_size*/
  {0x300C, 0x0E70},/*line_length_pck*/
  {0x300A, 0x07E5},/*frame_length_lines*/
  {0x3012, 0x07E4},/*coarse_integration_time*/
  {0x3014, 0x0C8C},/*fine_integration_time*/
  {0x3010, 0x00A0},/*fine_correction*/
};


static struct msm_camera_i2c_reg_conf ar0542_init_settings_conf[] = {
  {0x301A,0x0018},
  {0x3064,0xB800},
  {0x31AE,0x0202},
  {0x316A,0x8400},
  {0x316C,0x8400},
  {0x316E,0x8400},
  {0x3EFA,0x1A1F},
  {0x3ED2,0xD965},
  {0x3ED8,0x7F1B},
  {0x3EDA,0x2F11},
  {0x3EE2,0x0060},
  {0x3EF2,0xD965},
  {0x3EF8,0x797F},
  {0x3EFC,0x286F},
  {0x3EFE,0x2C01},
  {0x3E00,0x042F},
  {0x3E02,0xFFFF},
  {0x3E04,0xFFFF},
  {0x3E06,0xFFFF},
  {0x3E08,0x8071},
  {0x3E0A,0x7281},
  {0x3E0C,0x4011},
  {0x3E0E,0x8010},
  {0x3E10,0x60A5},
  {0x3E12,0x4080},
  {0x3E14,0x4180},
  {0x3E16,0x0018},
  {0x3E18,0x46B7},
  {0x3E1A,0x4994},
  {0x3E1C,0x4997},
  {0x3E1E,0x4682},
  {0x3E20,0x0018},
  {0x3E22,0x4241},
  {0x3E24,0x8000},
  {0x3E26,0x1880},
  {0x3E28,0x4785},
  {0x3E2A,0x4992},
  {0x3E2C,0x4997},
  {0x3E2E,0x4780},
  {0x3E30,0x4D80},
  {0x3E32,0x100C},
  {0x3E34,0x8000},
  {0x3E36,0x184A},
  {0x3E38,0x8042},
  {0x3E3A,0x001A},
  {0x3E3C,0x9610},
  {0x3E3E,0x0C80},
  {0x3E40,0x4DC6},
  {0x3E42,0x4A80},
  {0x3E44,0x0018},
  {0x3E46,0x8042},
  {0x3E48,0x8041},
  {0x3E4A,0x0018},
  {0x3E4C,0x804B},
  {0x3E4E,0xB74B},
  {0x3E50,0x8010},
  {0x3E52,0x6056},
  {0x3E54,0x001C},
  {0x3E56,0x8211},
  {0x3E58,0x8056},
  {0x3E5A,0x827C},
  {0x3E5C,0x0970},
  {0x3E5E,0x8082},
  {0x3E60,0x7281},
  {0x3E62,0x4C40},
  {0x3E64,0x8E4D},
  {0x3E66,0x8110},
  {0x3E68,0x0CAF},
  {0x3E6A,0x4D80},
  {0x3E6C,0x100C},
  {0x3E6E,0x8440},
  {0x3E70,0x4C81},
  {0x3E72,0x7C5F},
  {0x3E74,0x7000},
  {0x3E76,0x0000},
  {0x3E78,0x0000},
  {0x3E7A,0x0000},
  {0x3E7C,0x0000},
  {0x3E7E,0x0000},
  {0x3E80,0x0000},
  {0x3E82,0x0000},
  {0x3E84,0x0000},
  {0x3E86,0x0000},
  {0x3E88,0x0000},
  {0x3E8A,0x0000},
  {0x3E8C,0x0000},
  {0x3E8E,0x0000},
  {0x3E90,0x0000},
  {0x3E92,0x0000},
  {0x3E94,0x0000},
  {0x3E96,0x0000},
  {0x3E98,0x0000},
  {0x3E9A,0x0000},
  {0x3E9C,0x0000},
  {0x3E9E,0x0000},
  {0x3EA0,0x0000},
  {0x3EA2,0x0000},
  {0x3EA4,0x0000},
  {0x3EA6,0x0000},
  {0x3EA8,0x0000},
  {0x3EAA,0x0000},
  {0x3EAC,0x0000},
  {0x3EAE,0x0000},
  {0x3EB0,0x0000},
  {0x3EB2,0x0000},
  {0x3EB4,0x0000},
  {0x3EB6,0x0000},
  {0x3EB8,0x0000},
  {0x3EBA,0x0000},
  {0x3EBC,0x0000},
  {0x3EBE,0x0000},
  {0x3EC0,0x0000},
  {0x3EC2,0x0000},
  {0x3EC4,0x0000},
  {0x3EC6,0x0000},
  {0x3EC8,0x0000},
  {0x3ECA,0x0000},
  {0x3170,0x2150},
  {0x317A,0x0150},
  {0x3ECC,0x2200},
  {0x3174,0x0000},
  {0x3176,0X0000},
  {0x30BC,0x0384},
  {0x30C0,0x1220},
  {0x30D4,0x9200},
  {0x30B2,0xC000},
  {0x31B0,0x00C4},
  {0x31B2,0x0064},
  {0x31B4,0x0E77},
  {0x31B6,0x0D24},
  {0x31B8,0x020E},
  {0x31BA,0x0710},
  {0x31BC,0x2A0D},
  {0x31BE,0xC007},
  {0x305E,0x112A},
  {0x3ECE,0X000A},
  {0x0400,0x0000},
  {0x0404,0x0010},
  {0x0300,0x0005},
  {0x0302,0x0001},
  {0x0304,0x0002},
  {0x0306,0x002E},
  {0x0308,0x000a},
  {0x030A,0x0001},
  {0x3100,0x0000},
};

static struct msm_camera_i2c_conf_array ar0542_init_settings[] = {
  {&ar0542_init_settings_conf[0],
  ARRAY_SIZE(ar0542_init_settings_conf), 1, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_i2c_conf_array ar0542_mode_settings[] = {
  {&ar0542_snap_settings[0],
  ARRAY_SIZE(ar0542_snap_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
  {&ar0542_prev_settings[0],
  ARRAY_SIZE(ar0542_prev_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
  {&ar0542_video_60fps_settings[0],
  ARRAY_SIZE(ar0542_video_60fps_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
  {&ar0542_video_90fps_settings[0],
  ARRAY_SIZE(ar0542_video_90fps_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
  {&ar0542_zsl_settings[0],
  ARRAY_SIZE(ar0542_zsl_settings), 0, MSM_CAMERA_I2C_WORD_DATA},
};

static struct msm_camera_csi_params ar0542_csi_params = {
  .data_format               = CSI_10BIT,
  .lane_cnt                  = 2,
  .lane_assign               = 0xe4,
  .dpcm_scheme               = 0,
  .settle_cnt                = 10,
};

static struct v4l2_subdev_info ar0542_subdev_info[] = {
  {
    .code                    = V4L2_MBUS_FMT_SGRBG10_1X10,
    .colorspace              = V4L2_COLORSPACE_JPEG,
    .fmt                     = 1,
    .order                   = 0,
  },
};

static struct msm_sensor_output_info_t ar0542_output_settings[] = {
  { /* For SNAPSHOT */
    .x_output                = 0xA20,    /* 2592 */
    .y_output                = 0x798,    /* 1944 */
    .line_length_pclk        = 0x0E70,   /* 3696 */
    .frame_length_lines      = 0x07E5,   /* 2021 */
    .vt_pixel_clk            = 110400000,/* =14.8*3696*2021 */
    .op_pixel_clk            = 55200000,
    .binning_factor          = 0x0,
  },
  { /* For PREVIEW */
    .x_output                = 0x0510,   /* 1296 */
    .y_output                = 0x03CC,   /* 972  */
    .line_length_pclk        = 0x0CF0,   /* 3312 */
    .frame_length_lines      = 0x0457,   /* 1111 */
    .vt_pixel_clk            = 110400000,/* =30.003*3312*1111 */
    .op_pixel_clk            = 55200000,
    .binning_factor          = 0x0,
  },
  { /* For 60fps*/
    .x_output                = 0x280,   /* 640 */
    .y_output                = 0x1E0,   /* 480  */
    .line_length_pclk        = 0x0CE6,   /* 3302 */
    .frame_length_lines      = 0x022D,   /* 557 */
    .vt_pixel_clk            = 110400000,/* =60.026*3302*557 */
    .op_pixel_clk            = 55200000,
    .binning_factor          = 0x0,
  },
  { /* For 90fps*/
    .x_output                = 0x280,   /* 640 */
    .y_output                = 0x1E0,   /* 480  */
    .line_length_pclk        = 0x089A,   /* 2202 */
    .frame_length_lines      = 0x022D,   /* 557 */
    .vt_pixel_clk            = 110400000,/* =90.0112*2202*557*/
    .op_pixel_clk            = 55200000,
    .binning_factor          = 0x0,
  },
  { /* For ZSL */
    .x_output                = 0xA20,    /* 2592 */
    .y_output                = 0x798,    /* 1944 */
    .line_length_pclk        = 0x0E70,   /* 3696 */
    .frame_length_lines      = 0x07E5,   /* 2021 */
    .vt_pixel_clk            = 110400000,/* =14.8*3696*2021 */
    .op_pixel_clk            = 55200000,
    .binning_factor          = 0x0,
  },
};

static struct msm_sensor_output_reg_addr_t ar0542_output_reg_addr = {
  .x_output                  = 0x034C,
  .y_output                  = 0x034E,
  .line_length_pclk          = 0x300C,
  .frame_length_lines        = 0x300A,
};

static struct msm_camera_csi_params *ar0542_csi_params_array[] = {
  &ar0542_csi_params,
  &ar0542_csi_params,
  &ar0542_csi_params,
  &ar0542_csi_params,
  &ar0542_csi_params,
};

static struct msm_sensor_id_info_t ar0542_id_info = {
  .sensor_id_reg_addr        = 0x0000,
  .sensor_id                 = 0x4800,
};

static struct msm_sensor_exp_gain_info_t ar0542_exp_gain_info = {
  .coarse_int_time_addr      = 0x3012,
  .global_gain_addr          = 0x305E,
  .vert_offset               = 0,
};

static void ar0542_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
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

static void ar0542_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x0058,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,0x0050,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static void ar0542_group_hold_on(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static void ar0542_group_hold_off(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static int32_t ar0542_write_prev_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
  uint16_t gain, uint32_t line)
{
  uint16_t max_legal_gain = 0xE7F; //32x
  uint16_t min_legal_gain = 0x127; //1.584375x
  int32_t rc = 0;

  if(s_ctrl->curr_res > MSM_SENSOR_RES_QTR &&
    line > s_ctrl->msm_sensor_reg->output_settings[s_ctrl->curr_res].frame_length_lines)
    line = s_ctrl->msm_sensor_reg->output_settings[s_ctrl->curr_res].frame_length_lines;

  if (gain < min_legal_gain) {
    CDBG("gain < min_legal_gain, gain = %d\n", gain);
    gain = min_legal_gain;
  }
  if (gain > max_legal_gain) {
    CDBG("gain > max_legal_gain, gain = %d\n", gain);
    gain = max_legal_gain;
  }
  ar0542_prev_exp_gain = gain;
  ar0542_prev_exp_line = line;
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

static int32_t ar0542_write_pict_exp_gain(struct msm_sensor_ctrl_t *s_ctrl,
  uint16_t gain, uint32_t line)
{
  int32_t rc = 0;
  rc = ar0542_write_prev_exp_gain(s_ctrl,gain,line);
  rc = msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x301A,(0x065C|0x2),
    MSM_CAMERA_I2C_WORD_DATA);
  return rc;
}

static const struct i2c_device_id ar0542_i2c_id[] = {
  {SENSOR_NAME, (kernel_ulong_t)&ar0542_s_ctrl},
  { }
};

int32_t ar0542_sensor_i2c_probe(struct i2c_client *client,
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

static struct i2c_driver ar0542_i2c_driver = {
  .id_table                  = ar0542_i2c_id,
  .probe                     = ar0542_sensor_i2c_probe,
  .driver                    = {
    .name                    = SENSOR_NAME,
  },
};

static struct msm_camera_i2c_client ar0542_sensor_i2c_client = {
  .addr_type                 = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
  return i2c_add_driver(&ar0542_i2c_driver);
}

static struct v4l2_subdev_core_ops ar0542_subdev_core_ops = {
  .ioctl                     = msm_sensor_subdev_ioctl,
  .s_power                   = msm_sensor_power,
};

static struct v4l2_subdev_video_ops ar0542_subdev_video_ops = {
  .enum_mbus_fmt             = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops ar0542_subdev_ops = {
  .core                      = &ar0542_subdev_core_ops,
  .video                     = &ar0542_subdev_video_ops,
};

int32_t ar0542_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
  ar0542_prev_exp_gain = 0x012A;
  ar0542_prev_exp_line = 0x07E4;
  CDBG("%s: X\n",__func__);
  return 0;
}

int32_t ar0542_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
  int32_t rc = 0;
  struct msm_camera_sensor_info *info = NULL;
  info = s_ctrl->sensordata;
  CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__,
    info->sensor_pwd,info->sensor_reset);
  gpio_direction_output(info->sensor_pwd, 0);
  gpio_direction_output(info->sensor_reset, 0);
  usleep_range(10000, 11000);
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

int32_t ar0542_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
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
int ar0542_check_otp_status(struct msm_sensor_ctrl_t *s_ctrl, int otp_type)
{
  uint16_t i = 0;
  uint16_t temp = 0;
  uint16_t temp_otp_flag0 = 0;
  uint16_t temp_otp_flag1 = 0;
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

  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x38EC,&temp_otp_flag0,
    MSM_CAMERA_I2C_WORD_DATA);
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x38EE,&temp_otp_flag1,
    MSM_CAMERA_I2C_WORD_DATA);

  if((0xFFFF==temp_otp_flag0)&&(0xFFFF==temp_otp_flag1)){
    //OTPM have valid data
    CDBG("%s: otp_type = 0x%02x, OTPM have valid data\n",__func__,otp_type);
    return 1;
  }else{
    //OTPM have no valid data
    CDBG("%s: otp_type = 0x%02x, OTPM have no valid data\n",__func__,otp_type);
    return 0;
  }
}

/*******************************************************************************
* return value:
*     0, OTPM have no valid data
*     1, OTPM have valid data
*******************************************************************************/
int ar0542_read_otp(struct msm_sensor_ctrl_t *s_ctrl, struct ar0542_otp_struct *p_otp)
{
  uint16_t i = 0;
  int otp_status = 0;//initial OTPM have no valid data status
  if(ar0542_check_otp_status(s_ctrl,0x35)){
    //type 0x35 have valid data, now use type 0x35
    otp_status = 1;
  }else{
    if(ar0542_check_otp_status(s_ctrl,0x34)){
      //type 0x34 have valid data, now use type 0x34
      otp_status = 1;
    }else{
      if(ar0542_check_otp_status(s_ctrl,0x33)){
        //type 0x33 have valid data, now use type 0x33
        otp_status = 1;
      }else{
        if(ar0542_check_otp_status(s_ctrl,0x32)){
          //type 0x32 have valid data, now use type 0x32
          otp_status = 1;
        }else{
          if(ar0542_check_otp_status(s_ctrl,0x31)){
            //type 0x31 have valid data, now use type 0x31
            otp_status = 1;
          }else{
            if(ar0542_check_otp_status(s_ctrl,0x30)){
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
    for (i = 0; i < (sizeof(st_ar0542_otp)/2); i++) {
      msm_camera_i2c_read(s_ctrl->sensor_i2c_client, (0x3800+i*2),
        ((uint16_t *)p_otp +i), MSM_CAMERA_I2C_WORD_DATA);
    }
  }else{
    CDBG("%s: have no valid otp data\n",__func__);
  }
#if 0
  CDBG("%s: Module_info_0 = 0x%04x (%d)\n",__func__,p_otp->Module_info_0,p_otp->Module_info_0);
  CDBG("%s: Module_info_1 = 0x%04x (%d)\n",__func__,p_otp->Module_info_1,p_otp->Module_info_1);
  CDBG("%s: Module_info_2 = 0x%04x (%d)\n",__func__,p_otp->Module_info_2,p_otp->Module_info_2);
  CDBG("%s: Reserve_0     = 0x%04x (%d)\n",__func__,p_otp->Reserve_0,    p_otp->Reserve_0);
  CDBG("%s: R_Gr          = 0x%04x (%d)\n",__func__,p_otp->R_Gr,         p_otp->R_Gr);
  CDBG("%s: B_Gr          = 0x%04x (%d)\n",__func__,p_otp->B_Gr,         p_otp->B_Gr);
  CDBG("%s: Gb_Gr         = 0x%04x (%d)\n",__func__,p_otp->Gb_Gr,        p_otp->Gb_Gr);
  CDBG("%s: Reserve_1     = 0x%04x (%d)\n",__func__,p_otp->Reserve_1,    p_otp->Reserve_1);
  CDBG("%s: LSC_data[0]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[0],  p_otp->LSC_data[0]);
  CDBG("%s: LSC_data[1]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[1],  p_otp->LSC_data[1]);
  CDBG("%s: LSC_data[2]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[2],  p_otp->LSC_data[2]);
  CDBG("%s: LSC_data[3]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[3],  p_otp->LSC_data[3]);
  CDBG("%s: LSC_data[4]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[4],  p_otp->LSC_data[4]);
  CDBG("%s: LSC_data[5]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[5],  p_otp->LSC_data[5]);
  CDBG("%s: LSC_data[6]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[6],  p_otp->LSC_data[6]);
  CDBG("%s: LSC_data[7]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[7],  p_otp->LSC_data[7]);
  CDBG("%s: LSC_data[8]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[8],  p_otp->LSC_data[8]);
  CDBG("%s: LSC_data[9]   = 0x%04x (%d)\n",__func__,p_otp->LSC_data[9],  p_otp->LSC_data[9]);
  CDBG("%s: LSC_data[10]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[10], p_otp->LSC_data[10]);
  CDBG("%s: LSC_data[11]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[11], p_otp->LSC_data[11]);
  CDBG("%s: LSC_data[12]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[12], p_otp->LSC_data[12]);
  CDBG("%s: LSC_data[13]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[13], p_otp->LSC_data[13]);
  CDBG("%s: LSC_data[14]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[14], p_otp->LSC_data[14]);
  CDBG("%s: LSC_data[15]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[15], p_otp->LSC_data[15]);
  CDBG("%s: LSC_data[16]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[16], p_otp->LSC_data[16]);
  CDBG("%s: LSC_data[17]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[17], p_otp->LSC_data[17]);
  CDBG("%s: LSC_data[18]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[18], p_otp->LSC_data[18]);
  CDBG("%s: LSC_data[19]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[19], p_otp->LSC_data[19]);
  CDBG("%s: LSC_data[20]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[20], p_otp->LSC_data[20]);
  CDBG("%s: LSC_data[21]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[21], p_otp->LSC_data[21]);
  CDBG("%s: LSC_data[22]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[22], p_otp->LSC_data[22]);
  CDBG("%s: LSC_data[23]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[23], p_otp->LSC_data[23]);
  CDBG("%s: LSC_data[24]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[24], p_otp->LSC_data[24]);
  CDBG("%s: LSC_data[25]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[25], p_otp->LSC_data[25]);
  CDBG("%s: LSC_data[26]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[26], p_otp->LSC_data[26]);
  CDBG("%s: LSC_data[27]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[27], p_otp->LSC_data[27]);
  CDBG("%s: LSC_data[28]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[28], p_otp->LSC_data[28]);
  CDBG("%s: LSC_data[29]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[29], p_otp->LSC_data[29]);
  CDBG("%s: LSC_data[30]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[30], p_otp->LSC_data[30]);
  CDBG("%s: LSC_data[31]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[31], p_otp->LSC_data[31]);
  CDBG("%s: LSC_data[32]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[32], p_otp->LSC_data[32]);
  CDBG("%s: LSC_data[33]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[33], p_otp->LSC_data[33]);
  CDBG("%s: LSC_data[34]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[34], p_otp->LSC_data[34]);
  CDBG("%s: LSC_data[35]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[35], p_otp->LSC_data[35]);
  CDBG("%s: LSC_data[36]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[36], p_otp->LSC_data[36]);
  CDBG("%s: LSC_data[37]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[37], p_otp->LSC_data[37]);
  CDBG("%s: LSC_data[38]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[38], p_otp->LSC_data[38]);
  CDBG("%s: LSC_data[39]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[39], p_otp->LSC_data[39]);
  CDBG("%s: LSC_data[40]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[40], p_otp->LSC_data[40]);
  CDBG("%s: LSC_data[41]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[41], p_otp->LSC_data[41]);
  CDBG("%s: LSC_data[42]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[42], p_otp->LSC_data[42]);
  CDBG("%s: LSC_data[43]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[43], p_otp->LSC_data[43]);
  CDBG("%s: LSC_data[44]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[44], p_otp->LSC_data[44]);
  CDBG("%s: LSC_data[45]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[45], p_otp->LSC_data[45]);
  CDBG("%s: LSC_data[46]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[46], p_otp->LSC_data[46]);
  CDBG("%s: LSC_data[47]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[47], p_otp->LSC_data[47]);
  CDBG("%s: LSC_data[48]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[48], p_otp->LSC_data[48]);
  CDBG("%s: LSC_data[49]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[49], p_otp->LSC_data[49]);
  CDBG("%s: LSC_data[50]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[50], p_otp->LSC_data[50]);
  CDBG("%s: LSC_data[51]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[51], p_otp->LSC_data[51]);
  CDBG("%s: LSC_data[52]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[52], p_otp->LSC_data[52]);
  CDBG("%s: LSC_data[53]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[53], p_otp->LSC_data[53]);
  CDBG("%s: LSC_data[54]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[54], p_otp->LSC_data[54]);
  CDBG("%s: LSC_data[55]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[55], p_otp->LSC_data[55]);
  CDBG("%s: LSC_data[56]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[56], p_otp->LSC_data[56]);
  CDBG("%s: LSC_data[57]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[57], p_otp->LSC_data[57]);
  CDBG("%s: LSC_data[58]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[58], p_otp->LSC_data[58]);
  CDBG("%s: LSC_data[59]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[59], p_otp->LSC_data[59]);
  CDBG("%s: LSC_data[60]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[60], p_otp->LSC_data[60]);
  CDBG("%s: LSC_data[61]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[61], p_otp->LSC_data[61]);
  CDBG("%s: LSC_data[62]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[62], p_otp->LSC_data[62]);
  CDBG("%s: LSC_data[63]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[63], p_otp->LSC_data[63]);
  CDBG("%s: LSC_data[64]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[64], p_otp->LSC_data[64]);
  CDBG("%s: LSC_data[65]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[65], p_otp->LSC_data[65]);
  CDBG("%s: LSC_data[66]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[66], p_otp->LSC_data[66]);
  CDBG("%s: LSC_data[67]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[67], p_otp->LSC_data[67]);
  CDBG("%s: LSC_data[68]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[68], p_otp->LSC_data[68]);
  CDBG("%s: LSC_data[69]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[69], p_otp->LSC_data[69]);
  CDBG("%s: LSC_data[70]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[70], p_otp->LSC_data[70]);
  CDBG("%s: LSC_data[71]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[71], p_otp->LSC_data[71]);
  CDBG("%s: LSC_data[72]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[72], p_otp->LSC_data[72]);
  CDBG("%s: LSC_data[73]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[73], p_otp->LSC_data[73]);
  CDBG("%s: LSC_data[74]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[74], p_otp->LSC_data[74]);
  CDBG("%s: LSC_data[75]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[75], p_otp->LSC_data[75]);
  CDBG("%s: LSC_data[76]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[76], p_otp->LSC_data[76]);
  CDBG("%s: LSC_data[77]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[77], p_otp->LSC_data[77]);
  CDBG("%s: LSC_data[78]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[78], p_otp->LSC_data[78]);
  CDBG("%s: LSC_data[79]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[79], p_otp->LSC_data[79]);
  CDBG("%s: LSC_data[80]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[80], p_otp->LSC_data[80]);
  CDBG("%s: LSC_data[81]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[81], p_otp->LSC_data[81]);
  CDBG("%s: LSC_data[82]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[82], p_otp->LSC_data[82]);
  CDBG("%s: LSC_data[83]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[83], p_otp->LSC_data[83]);
  CDBG("%s: LSC_data[84]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[84], p_otp->LSC_data[84]);
  CDBG("%s: LSC_data[85]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[85], p_otp->LSC_data[85]);
  CDBG("%s: LSC_data[86]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[86], p_otp->LSC_data[86]);
  CDBG("%s: LSC_data[87]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[87], p_otp->LSC_data[87]);
  CDBG("%s: LSC_data[88]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[88], p_otp->LSC_data[88]);
  CDBG("%s: LSC_data[89]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[89], p_otp->LSC_data[89]);
  CDBG("%s: LSC_data[90]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[90], p_otp->LSC_data[90]);
  CDBG("%s: LSC_data[91]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[91], p_otp->LSC_data[91]);
  CDBG("%s: LSC_data[92]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[92], p_otp->LSC_data[92]);
  CDBG("%s: LSC_data[93]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[93], p_otp->LSC_data[93]);
  CDBG("%s: LSC_data[94]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[94], p_otp->LSC_data[94]);
  CDBG("%s: LSC_data[95]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[95], p_otp->LSC_data[95]);
  CDBG("%s: LSC_data[96]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[96], p_otp->LSC_data[96]);
  CDBG("%s: LSC_data[97]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[97], p_otp->LSC_data[97]);
  CDBG("%s: LSC_data[98]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[98], p_otp->LSC_data[98]);
  CDBG("%s: LSC_data[99]  = 0x%04x (%d)\n",__func__,p_otp->LSC_data[99], p_otp->LSC_data[99]);
  CDBG("%s: LSC_data[100] = 0x%04x (%d)\n",__func__,p_otp->LSC_data[100],p_otp->LSC_data[100]);
  CDBG("%s: LSC_data[101] = 0x%04x (%d)\n",__func__,p_otp->LSC_data[101],p_otp->LSC_data[101]);
  CDBG("%s: LSC_data[102] = 0x%04x (%d)\n",__func__,p_otp->LSC_data[102],p_otp->LSC_data[102]);
  CDBG("%s: LSC_data[103] = 0x%04x (%d)\n",__func__,p_otp->LSC_data[103],p_otp->LSC_data[103]);
  CDBG("%s: LSC_data[104] = 0x%04x (%d)\n",__func__,p_otp->LSC_data[104],p_otp->LSC_data[104]);
  CDBG("%s: LSC_data[105] = 0x%04x (%d)\n",__func__,p_otp->LSC_data[105],p_otp->LSC_data[105]);
  CDBG("%s: Reserve_2     = 0x%04x (%d)\n",__func__,p_otp->Reserve_2,    p_otp->Reserve_2);
  CDBG("%s: AF_infinity   = 0x%04x (%d)\n",__func__,p_otp->AF_infinity,  p_otp->AF_infinity);
  CDBG("%s: AF_macro      = 0x%04x (%d)\n",__func__,p_otp->AF_macro,     p_otp->AF_macro);
  CDBG("%s: AF_data       = 0x%04x (%d)\n",__func__,p_otp->AF_data,      p_otp->AF_data);
  CDBG("%s: OTP_flag_0    = 0x%04x (%d)\n",__func__,p_otp->OTP_flag_0,   p_otp->OTP_flag_0);
  CDBG("%s: OTP_flag_1    = 0x%04x (%d)\n",__func__,p_otp->OTP_flag_1,   p_otp->OTP_flag_1);
#endif
  return otp_status;
}

void ar0542_write_otp(struct msm_sensor_ctrl_t *s_ctrl, struct ar0542_otp_struct *p_otp)
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

void ar0542_update_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
  ar0542_read_otp(s_ctrl, &st_ar0542_otp);
  ar0542_write_otp(s_ctrl, &st_ar0542_otp);
}

int32_t ar0542_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
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
    s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
    rc = msm_sensor_write_init_settings(s_ctrl);
    csi_config = 0;
    ar0542_update_otp(s_ctrl);
  } else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
    CDBG("PERIODIC : %d\n", res);
	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x30f2, &af_reg_dac, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_read(s_ctrl->sensor_i2c_client, 0x30f4, &af_reg_damp, MSM_CAMERA_I2C_WORD_DATA);
	//set to zero to avoid lens crash sound
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f4, 0x0,MSM_CAMERA_I2C_WORD_DATA);
	for(af_step_pos = af_reg_dac&0xff; af_step_pos > 0; af_step_pos-=8){
		msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f2, af_step_pos&0xff,MSM_CAMERA_I2C_WORD_DATA);
		msleep(2);
	}
	s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
    if(res == 0)
      msleep(40);
    else
      msleep(105);
    s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
    rc = msm_sensor_write_conf_array(s_ctrl->sensor_i2c_client,
      s_ctrl->msm_sensor_reg->mode_settings, res);
    if(res == 1){
      msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3012,0x0457,
           MSM_CAMERA_I2C_WORD_DATA);
      msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3014,0x0908,
           MSM_CAMERA_I2C_WORD_DATA);
      msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3010,0x0184,
           MSM_CAMERA_I2C_WORD_DATA);
    }
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
        ar0542_prev_exp_gain,ar0542_prev_exp_line);
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
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f4, 0x01F4, MSM_CAMERA_I2C_WORD_DATA);
	msm_camera_i2c_write(s_ctrl->sensor_i2c_client, 0x30f2, af_reg_dac, MSM_CAMERA_I2C_WORD_DATA);
    msleep(50);
  }
  return rc;
}

static struct msm_sensor_fn_t ar0542_func_tbl = {
  .sensor_start_stream            = ar0542_start_stream,
  .sensor_stop_stream             = ar0542_stop_stream,
  .sensor_group_hold_on           = ar0542_group_hold_on,
  .sensor_group_hold_off          = ar0542_group_hold_off,
  .sensor_set_fps                 = msm_sensor_set_fps,
  .sensor_write_exp_gain          = ar0542_write_prev_exp_gain,
  .sensor_write_snapshot_exp_gain = ar0542_write_pict_exp_gain,
  .sensor_csi_setting             = ar0542_sensor_setting,
  .sensor_set_sensor_mode         = msm_sensor_set_sensor_mode,
  .sensor_mode_init               = msm_sensor_mode_init,
  .sensor_get_output_info         = msm_sensor_get_output_info,
  .sensor_config                  = msm_sensor_config,
  .sensor_power_up                = ar0542_sensor_power_up,
  .sensor_power_down              = ar0542_sensor_power_down,
  .sensor_match_id                = ar0542_sensor_match_id,
  .sensor_get_csi_params          = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t ar0542_regs = {
  .default_data_type              = MSM_CAMERA_I2C_BYTE_DATA,
  .start_stream_conf              = ar0542_start_stream_conf,
  .start_stream_conf_size         = ARRAY_SIZE(ar0542_start_stream_conf),
  .stop_stream_conf               = ar0542_stop_stream_conf,
  .stop_stream_conf_size          = ARRAY_SIZE(ar0542_stop_stream_conf),
  .group_hold_on_conf             = ar0542_group_hold_on_conf,
  .group_hold_on_conf_size        = ARRAY_SIZE(ar0542_group_hold_on_conf),
  .group_hold_off_conf            = ar0542_group_hold_off_conf,
  .group_hold_off_conf_size       = ARRAY_SIZE(ar0542_group_hold_off_conf),
  .init_settings                  = &ar0542_init_settings[0],
  .init_size                      = ARRAY_SIZE(ar0542_init_settings),
  .mode_settings                  = &ar0542_mode_settings[0],
  .output_settings                = &ar0542_output_settings[0],
  .num_conf                       = ARRAY_SIZE(ar0542_mode_settings),
};

static struct msm_sensor_ctrl_t ar0542_s_ctrl = {
  .msm_sensor_reg                 = &ar0542_regs,
  .sensor_i2c_client              = &ar0542_sensor_i2c_client,
  .sensor_i2c_addr                = 0x6C,
  .sensor_output_reg_addr         = &ar0542_output_reg_addr,
  .sensor_id_info                 = &ar0542_id_info,
  .sensor_exp_gain_info           = &ar0542_exp_gain_info,
  .cam_mode                       = MSM_SENSOR_MODE_INVALID,
  .csic_params                    = &ar0542_csi_params_array[0],
  .msm_sensor_mutex               = &ar0542_mut,
  .sensor_i2c_driver              = &ar0542_i2c_driver,
  .sensor_v4l2_subdev_info        = ar0542_subdev_info,
  .sensor_v4l2_subdev_info_size   = ARRAY_SIZE(ar0542_subdev_info),
  .sensor_v4l2_subdev_ops         = &ar0542_subdev_ops,
  .func_tbl                       = &ar0542_func_tbl,
  .clk_rate                       = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Aptina 5M Bayer sensor driver");
MODULE_LICENSE("GPL v2");
