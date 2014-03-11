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
#include "msm_ispif.h"
#include "msm_camera_i2c_mux.h"

#define SENSOR_NAME "s5k3h2_sunny_q8s02e"
#define PLATFORM_DRIVER_NAME "msm_camera_s5k3h2_sunny_q8s02e"
#define s5k3h2_sunny_q8s02e_obj s5k3h2_sunny_q8s02e_##obj

#ifdef CDBG
#undef CDBG
#endif
#ifdef CDBG_HIGH
#undef CDBG_HIGH
#endif

#define S5K3H2_SUNNY_Q8S02E_VERBOSE_DGB

#ifdef S5K3H2_SUNNY_Q8S02E_VERBOSE_DGB
#define LOG_TAG "s5k3h2_sunny_q8s02e-v4l2"
#define CDBG(fmt, args...) printk(fmt, ##args)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#else
#define CDBG(fmt, args...) do { } while (0)
#define CDBG_HIGH(fmt, args...) printk(fmt, ##args)
#endif

#ifndef ABS
  #define ABS(x)            (((x) < 0) ? -(x) : (x))
#endif
#define MSB                             1
#define LSB                             0

#define S5K3H2_SUNNY_Q8S02E_OTP_FEATURE
//#define S5K3H2_SUNNY_Q8S02E_OTP_FEATURE_DGB
#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE
#define S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE 663
#define S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE 654
#define S5K3H2_SUNNY_Q8S02E_Gb_Gr_RATIO_TYPICAL_VALUE 1024
struct s5k3h2_sunny_q8s02e_otp_struct {
  uint8_t LSC_OTP_Disable_Page_09[64];  /* LSC_OTP_Disable */
  uint8_t Chip_ID_Page_0A[64];          /* Chip_ID */
  uint8_t LSC_Page_0B[64];              /* LSC */
  uint8_t LSC_Page_0C[64];
  uint8_t LSC_Page_0D[64];
  uint8_t LSC_Page_0E[64];
  uint8_t LSC_Page_0F[64];
  uint8_t AWB_Page_10[64];              /* AWB */
  uint8_t AWB_Page_11[64];
  uint8_t AWB_Page_12[64];
} st_s5k3h2_sunny_q8s02e_otp = {
  /* Golden Module OTP data */
  {
    /* Page_0x09 */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
   },
  {
    /* Page_0x0a */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x72,0x1b,0x20,0x81,0x8c,0x17,
    0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0xd9,0x4f,0xf6,0xca,0xff,0x27,0xc0,
    0x26,0x7f,0xfd,0x81,0xa0,0x14,0x2e,0xfe,0x5c,0x2f,0xfd,0x1a,0x00,0x85,0x7f,0xf9,
    0x85,0xff,0x6b,0xf0,0x0e,0xdb,0x00,0x8e,0x80,0x07,0x28,0xff,0x3f,0x90,0x0a,0xc3
  },
  {
    /* Page_0x0b */
    0x00,0x6b,0x3f,0xf2,0x78,0x00,0xc9,0x9f,0xf5,0xe6,0x00,0xc9,0x1f,0xf2,0xce,0xff,
    0xe0,0xb0,0x07,0xcd,0xfe,0xef,0xc0,0x07,0xa1,0xff,0x91,0x30,0x07,0xd8,0x00,0x52,
    0xdf,0xf8,0xee,0x00,0xc6,0x9f,0xfd,0x0b,0x00,0x19,0x8f,0xfd,0x72,0xff,0xc4,0xe0,
    0x03,0x6b,0x07,0x6d,0xdf,0xf3,0xb1,0xff,0x6c,0x30,0x1e,0x54,0xfe,0x46,0x70,0x0b
  },
  {
    /* Page_0x0c */
    0xe3,0xfe,0x0e,0x4f,0xfd,0x8f,0x00,0x69,0xef,0xff,0xb6,0xfe,0xd5,0xc0,0x15,0x92,
    0x00,0xc7,0x10,0x01,0x63,0xff,0x73,0xc0,0x08,0x87,0x00,0x74,0xff,0xf2,0x25,0x00,
    0x77,0x50,0x04,0xd7,0x00,0x5e,0x2f,0xf2,0x54,0x00,0x37,0x30,0x05,0x91,0xff,0x8e,
    0xcf,0xf4,0xec,0xff,0xf1,0x00,0x0a,0x17,0x00,0x2e,0x8f,0xf4,0x21,0x00,0x4e,0x80
  },
  {
    /* Page_0x0d */
    0x08,0x13,0xff,0xf2,0xdf,0xfc,0x8b,0xff,0x81,0xe0,0x0d,0x30,0x06,0x6f,0x5f,0xf8,
    0xd2,0xfe,0xf0,0xe0,0x2c,0x32,0xfc,0xf7,0xe0,0x1b,0x4e,0xfe,0x6e,0x80,0x02,0x85,
    0x00,0x05,0x40,0x07,0xae,0xfe,0x59,0x60,0x17,0xb3,0x00,0xb0,0xdf,0xff,0x8f,0xff,
    0x9b,0x80,0x03,0x1c,0x00,0xd3,0x3f,0xf3,0x6b,0x00,0x6f,0xa0,0x02,0x04,0x00,0xc5
  },
  {
    /* Page_0x0e */
    0x9f,0xe7,0xe9,0x01,0x3a,0x8f,0xf1,0x4d,0xff,0x42,0x1f,0xf4,0xd7,0xff,0xca,0xf0,
    0x14,0x6c,0xfe,0xa7,0x40,0x13,0x81,0x00,0xac,0xc0,0x0c,0x20,0xff,0x8b,0x6f,0xff,
    0x5e,0x00,0x1f,0xff,0xfa,0x1a,0x06,0xe1,0xff,0xf7,0x35,0xff,0x3e,0x70,0x22,0x58,
    0xfd,0xd1,0x40,0x11,0x81,0xfe,0x39,0x5f,0xfe,0xad,0x00,0x52,0x0f,0xff,0xfa,0xfe
  },
  {
    /* Page_0x0f */
    0xf9,0x00,0x11,0xa4,0x00,0xd4,0x20,0x02,0xe8,0xff,0x7f,0xd0,0x04,0xcb,0x00,0xb4,
    0xdf,0xf4,0x68,0x00,0x3a,0xdf,0xfe,0x14,0x00,0x96,0x9f,0xf4,0x5d,0xff,0xf9,0x2f,
    0xff,0x34,0xff,0xc5,0x0f,0xfd,0x3d,0xff,0xa0,0x20,0x0a,0xe8,0x00,0x0e,0xa0,0x02,
    0x7f,0x00,0x2d,0x40,0x03,0xad,0x00,0x23,0x9f,0xf9,0xea,0xff,0xe7,0x2f,0xff,0x75
  },
  {
    /* Page_0x10 */
    0x02,0xa4,0x02,0x92,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  },
  {
    /* Page_0x11 */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  },
  {
    /* Page_0x12 */
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
  },
};

#endif

static struct msm_sensor_ctrl_t s5k3h2_sunny_q8s02e_s_ctrl;
static uint16_t s5k3h2_sunny_q8s02e_prev_exp_gain = 0x0020; //same as preview settigns
static uint16_t s5k3h2_sunny_q8s02e_prev_exp_line = 0x4db; //same as preview settigns
DEFINE_MUTEX(s5k3h2_sunny_q8s02e_mut);

static struct msm_camera_i2c_reg_conf s5k3h2_sunny_q8s02e_start_stream_conf[] = {

};

static struct msm_camera_i2c_reg_conf s5k3h2_sunny_q8s02e_stop_stream_conf[] = {

};

static struct msm_camera_i2c_reg_conf s5k3h2_sunny_q8s02e_group_hold_on_conf[] = {
  {0x0104, 0x01},
};

static struct msm_camera_i2c_reg_conf s5k3h2_sunny_q8s02e_group_hold_off_conf[] = {
  {0x0104, 0x00},
};

static struct msm_camera_i2c_reg_conf s5k3h2_sunny_q8s02e_prev_settings[] = {
  {0x0103,0x01},
  {0x0101,0x00}, // Flip/Mirror ON 0x03      OFF 0x00
                 // MIPI Setting
  {0x0105,0X01},//mask corrupted frame enable
  {0x3065,0x35},
  {0x310E,0x00},
  {0x3098,0xAB},
  {0x30C7,0x0A},
  {0x309A,0x01},
  {0x310D,0xC6},
  {0x30c3,0x40},
  {0x30BB,0x02},
  {0x30BC,0x38}, // According to MCLK, these registers should be changed!
  {0x30BD,0x40},
  {0x3110,0x70},
  {0x3111,0x80},
  {0x3112,0x7B},
  {0x3113,0xC0},
  {0x30C7,0x1A},
  {0x3000,0x08},
  {0x3001,0x05},
  {0x3002,0x0D},
  {0x3003,0x21},
  {0x3004,0x62},
  {0x3005,0x0B},
  {0x3006,0x6D},
  {0x3007,0x02},
  {0x3008,0x62},
  {0x3009,0x62},
  {0x300A,0x41},
  {0x300B,0x10},
  {0x300C,0x21},
  {0x300D,0x04},
  {0x307E,0x03},
  {0x307F,0xA5},
  {0x3080,0x04},
  {0x3081,0x29},
  {0x3082,0x03},
  {0x3083,0x21},
  {0x3011,0x5F},
  {0x3156,0xE2},
  {0x3027,0xBE}, // DBR_CLK enable for EMI
  {0x300f,0x02},
  {0x3010,0x10},
  {0x3017,0x74},
  {0x3018,0x00},
  {0x3020,0x02},
  {0x3021,0x00},
  {0x3023,0x80},
  {0x3024,0x08},
  {0x3025,0x08},
  {0x301C,0xD4},
  {0x315D,0x00},
  {0x3054,0x00},
  {0x3055,0x35},
  {0x3062,0x04},
  {0x3063,0x38},
  {0x31A4,0x04},
  {0x3016,0x54},
  {0x3157,0x02},
  {0x3158,0x00},
  {0x315B,0x02},
  {0x315C,0x00},
  {0x301B,0x05},
  {0x3028,0x41},
  {0x302A,0x10},
  {0x3060,0x00},
  {0x302D,0x19},
  {0x302B,0x05},
  {0x3072,0x13},
  {0x3073,0x21},
  {0x3074,0x82},
  {0x3075,0x20},
  {0x3076,0xA2},
  {0x3077,0x02},
  {0x3078,0x91},
  {0x3079,0x91},
  {0x307A,0x61},
  {0x307B,0x28},
  {0x307C,0x31},
  {0x304E,0x40}, // Pedestal
  {0x304F,0x01}, // Pedestal
  {0x3050,0x00}, // Pedestal
  {0x3088,0x01}, // Pedestal
  {0x3089,0x00}, // Pedestal
  {0x3210,0x01}, // Pedestal
  {0x3211,0x00}, // Pedestal
  {0x308E,0x01},
  {0x308F,0x8F},
  {0x3064,0x03},
  {0x31A7,0x0F},

  {0x0305,0x04},// pre_pll_clk_div = 4
  {0x0306,0x00},// pll_multiplier
  {0x0307,0x6C},// pll_multiplier  = 108
  {0x0303,0x01},// vt_sys_clk_div = 1
  {0x0301,0x05},// vt_pix_clk_div = 5
  {0x030B,0x01},// op_sys_clk_div = 1
  {0x0309,0x05},// op_pix_clk_div = 5
  {0x30CC,0xB0},// DPHY_band_ctrl 640бл690MHz
  {0x31A1,0x58},// "DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0])
                // = 648Mhz / 8 = 81Mhz
                // [7:4] must be same as vt_pix_clk_div (0x0301)"
  {0x0344,0x00},// X addr start 8d
  {0x0345,0x08},
  {0x0346,0x00},// Y addr start 8d
  {0x0347,0x08},
  {0x0348,0x0C},// X addr end 3271d
  {0x0349,0xc7},
  {0x034A,0x09},// Y addr end 2455d
  {0x034B,0x97},
  {0x0381,0x01},// x_even_inc = 1
  {0x0383,0x03},// x_odd_inc = 3
  {0x0385,0x01},// y_even_inc = 1
  {0x0387,0x03},// y_odd_inc = 3
  {0x0401,0x00},// Derating_en  = 0 (disable)
  {0x0405,0x10},
  {0x0700,0x05},// fifo_water_mark_pixels = 1328
  {0x0701,0x30},
  {0x034C,0x06},// x_output_size = 1632
  {0x034D,0x60},
  {0x034E,0x04},// y_output_size = 1224
  {0x034F,0xC8},
  {0x0200,0x02},// fine integration time
  {0x0201,0x50},
  {0x0202,0x04},// Coarse integration time
  {0x0203,0xDB},
  {0x0204,0x00},// Analog gain
  {0x0205,0x20},
  {0x0342,0x0D},// Line_length_pck 3470d
  {0x0343,0x8E},
  {0x0340,0x04},// Frame_length_lines 1248d
  {0x0341,0xE0},
  {0x300E,0x2D},// Hbinnning[2] : 1b enale / 0b disable
  {0x31A3,0x40},// Vbinning enable[6] : 1b enale / 0b disable
  {0x301A,0x77},// "In case of using the Vt_Pix_Clk more than 137Mhz,
                // 0xA7h should be adopted! "
  {0x3053,0xCF},// In case of Video Mode, use CBh.
};

static struct msm_camera_i2c_reg_conf s5k3h2_sunny_q8s02e_snap_settings[] = {
  {0x0103,0x01},

  {0x0101,0x00}, // Flip/Mirror ON 0x03      OFF 0x00
                 // MIPI Setting
  {0x0105,0X01},//mask corrupted frame enable
  {0x3065,0x35},
  {0x310E,0x00},
  {0x3098,0xAB},
  {0x30C7,0x0A},
  {0x309A,0x01},
  {0x310D,0xC6},
  {0x30c3,0x40},
  {0x30BB,0x02},
  {0x30BC,0x38}, // According to MCLK, these registers should be changed!
  {0x30BD,0x40},
  {0x3110,0x70},
  {0x3111,0x80},
  {0x3112,0x7B},
  {0x3113,0xC0},
  {0x30C7,0x1A},
  {0x3000,0x08},
  {0x3001,0x05},
  {0x3002,0x0D},
  {0x3003,0x21},
  {0x3004,0x62},
  {0x3005,0x0B},
  {0x3006,0x6D},
  {0x3007,0x02},
  {0x3008,0x62},
  {0x3009,0x62},
  {0x300A,0x41},
  {0x300B,0x10},
  {0x300C,0x21},
  {0x300D,0x04},
  {0x307E,0x03},
  {0x307F,0xA5},
  {0x3080,0x04},
  {0x3081,0x29},
  {0x3082,0x03},
  {0x3083,0x21},
  {0x3011,0x5F},
  {0x3156,0xE2},
  {0x3027,0xBE}, // DBR_CLK enable for EMI
  {0x300f,0x02},
  {0x3010,0x10},
  {0x3017,0x74},
  {0x3018,0x00},
  {0x3020,0x02},
  {0x3021,0x00},
  {0x3023,0x80},
  {0x3024,0x08},
  {0x3025,0x08},
  {0x301C,0xD4},
  {0x315D,0x00},
  {0x3054,0x00},
  {0x3055,0x35},
  {0x3062,0x04},
  {0x3063,0x38},
  {0x31A4,0x04},
  {0x3016,0x54},
  {0x3157,0x02},
  {0x3158,0x00},
  {0x315B,0x02},
  {0x315C,0x00},
  {0x301B,0x05},
  {0x3028,0x41},
  {0x302A,0x10},
  {0x3060,0x00},
  {0x302D,0x19},
  {0x302B,0x05},
  {0x3072,0x13},
  {0x3073,0x21},
  {0x3074,0x82},
  {0x3075,0x20},
  {0x3076,0xA2},
  {0x3077,0x02},
  {0x3078,0x91},
  {0x3079,0x91},
  {0x307A,0x61},
  {0x307B,0x28},
  {0x307C,0x31},
  {0x304E,0x40}, // Pedestal
  {0x304F,0x01}, // Pedestal
  {0x3050,0x00}, // Pedestal
  {0x3088,0x01}, // Pedestal
  {0x3089,0x00}, // Pedestal
  {0x3210,0x01}, // Pedestal
  {0x3211,0x00}, // Pedestal
  {0x308E,0x01},
  {0x308F,0x8F},
  {0x3064,0x03},
  {0x31A7,0x0F},

  {0x0305,0x04},// pre_pll_clk_div = 4
  {0x0306,0x00},// pll_multiplier
  {0x0307,0x6C},// pll_multiplier  = 152
  {0x0303,0x01},// vt_sys_clk_div = 1
  {0x0301,0x05},// vt_pix_clk_div = 5
  {0x030B,0x01},// op_sys_clk_div = 1
  {0x0309,0x05},// op_pix_clk_div = 5
  {0x30CC,0xB0},// DPHY_band_ctrl 640бл690MHz
  {0x31A1,0x5A},// "DBR_CLK = PLL_CLK / DIV_DBR(0x31A1[3:0])
                // = 648Mhz / 10 = 64.8Mhz
                // [7:4] must be same as vt_pix_clk_div (0x0301)"
  {0x0344,0x00},// X addr start 8d
  {0x0345,0x08},
  {0x0346,0x00},// Y addr start 8d
  {0x0347,0x08},
  {0x0348,0x0C},// X addr end 3271d
  {0x0349,0xC7},
  {0x034A,0x09},// Y addr end 2455d
  {0x034B,0x97},
  {0x0381,0x01},// x_even_inc = 1
  {0x0383,0x01},// x_odd_inc = 1
  {0x0385,0x01},// y_even_inc = 1
  {0x0387,0x01},// y_odd_inc = 1
  {0x0401,0x00},// Derating_en  = 0 (disable)
  {0x0405,0x10},
  {0x0700,0x05},// fifo_water_mark_pixels = 1328
  {0x0701,0x30},
  {0x034C,0x0C},// x_output_size = 3264
  {0x034D,0xC0},
  {0x034E,0x09},// y_output_size = 2448
  {0x034F,0x90},
  {0x0200,0x02},// fine integration time
  {0x0201,0x50},
  {0x0202,0x04},// Coarse integration time
  {0x0203,0xE7},
  {0x0204,0x00},// Analog gain
  {0x0205,0x20},
  {0x0342,0x0D},// Line_length_pck 3470d
  {0x0343,0x8E},
  {0x0340,0x09},// Frame_length_lines 2480d
  {0x0341,0xB0},
  {0x300E,0x29},// Hbinnning[2] : 1b enale / 0b disable
  {0x31A3,0x00},// Vbinning enable[6] : 1b enale / 0b disable
  {0x301A,0x77},// "In case of using the Vt_Pix_Clk more than 137Mhz,
                // 0xA7h should be adopted! "
  {0x3053,0xCF},
};

static struct msm_camera_i2c_reg_conf s5k3h2_sunny_q8s02e_init_settings_conf[] = {
#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE
  {0x3210,0x81},
  {0x3200,0x08},
  {0x3201,0x00},
#endif
};

static struct msm_camera_i2c_conf_array s5k3h2_sunny_q8s02e_init_settings[] = {
  {&s5k3h2_sunny_q8s02e_init_settings_conf[0],
  ARRAY_SIZE(s5k3h2_sunny_q8s02e_init_settings_conf), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_i2c_conf_array s5k3h2_sunny_q8s02e_mode_settings[] = {
  {&s5k3h2_sunny_q8s02e_snap_settings[0],
  ARRAY_SIZE(s5k3h2_sunny_q8s02e_snap_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
  {&s5k3h2_sunny_q8s02e_prev_settings[0],
  ARRAY_SIZE(s5k3h2_sunny_q8s02e_prev_settings), 0, MSM_CAMERA_I2C_BYTE_DATA},
};

static struct msm_camera_csi_params s5k3h2_sunny_q8s02e_csi_params = {
  .data_format               = CSI_10BIT,
  .lane_cnt                  = 2,
  .lane_assign               = 0xe4,
  .dpcm_scheme               = 0,
  .settle_cnt                = 12,
};

static struct v4l2_subdev_info s5k3h2_sunny_q8s02e_subdev_info[] = {
  {
    .code                    = V4L2_MBUS_FMT_SGRBG10_1X10,
    .colorspace              = V4L2_COLORSPACE_JPEG,
    .fmt                     = 1,
    .order                   = 0,
  },
};

static struct msm_sensor_output_info_t s5k3h2_sunny_q8s02e_output_settings[] = {
  { /* For SNAPSHOT */
    .x_output                = 3264,
    .y_output                = 2448,
    .line_length_pclk        = 3470,
    .frame_length_lines      = 2480,
    .vt_pixel_clk            = 129600000,/* =15.06*3470*2480 */
    .op_pixel_clk            = 648000000,
    .binning_factor          = 0x0,
  },
  { /* For PREVIEW */
    .x_output                = 1632,
    .y_output                = 1224,
    .line_length_pclk        = 3470,
    .frame_length_lines      = 1248,
    .vt_pixel_clk            = 129600000,/* =29.9*3470*1248 */
    .op_pixel_clk            = 648000000,
    .binning_factor          = 0x1,
  },
};

static struct msm_sensor_output_reg_addr_t s5k3h2_sunny_q8s02e_output_reg_addr = {
  .x_output                  = 0x034C,
  .y_output                  = 0x034E,
  .line_length_pclk          = 0x0342,
  .frame_length_lines        = 0x0340,
};

static struct msm_camera_csi_params *s5k3h2_sunny_q8s02e_csi_params_array[] = {
  &s5k3h2_sunny_q8s02e_csi_params,
  &s5k3h2_sunny_q8s02e_csi_params,
};

static struct msm_sensor_id_info_t s5k3h2_sunny_q8s02e_id_info = {
  .sensor_id_reg_addr        = 0x0000,
  .sensor_id                 = 0x382B,
};

static struct msm_sensor_exp_gain_info_t s5k3h2_sunny_q8s02e_exp_gain_info = {
  .coarse_int_time_addr      = 0x0202,
  .global_gain_addr          = 0x0204,
  .vert_offset               = 8,
};

static void s5k3h2_sunny_q8s02e_start_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0100,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static void s5k3h2_sunny_q8s02e_stop_stream(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0100,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static void s5k3h2_sunny_q8s02e_group_hold_on(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static void s5k3h2_sunny_q8s02e_group_hold_off(struct msm_sensor_ctrl_t *s_ctrl)
{
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0104,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
}

static inline uint8_t s5k3h2_sunny_q8s02e_byte(uint16_t word, uint8_t offset)
{
  return word >> (offset * BITS_PER_BYTE);
}

static int32_t s5k3h2_sunny_q8s02e_write_prev_exp_gain(
  struct msm_sensor_ctrl_t *s_ctrl, uint16_t gain, uint32_t line)
{
  uint16_t max_legal_gain = 0x0400;
  uint16_t min_legal_line = 0x03;

  int32_t rc = 0;
  static uint32_t fl_lines, offset;
  s5k3h2_sunny_q8s02e_prev_exp_gain = gain;
  s5k3h2_sunny_q8s02e_prev_exp_line = line;

  CDBG("%s: gain = %d,line = %d\n",__func__, gain, line);
  offset = s_ctrl->sensor_exp_gain_info->vert_offset;
  if (gain > max_legal_gain) {
    CDBG("%s: gain > max_legal_gain\n",__func__);
    gain = max_legal_gain;
  }
  if (line < min_legal_line) {
    CDBG("%s: line < min_legal_line\n",__func__);
    line = min_legal_line;
  }

  s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
  if ((line < (1248-offset))||(line == (1248-offset))){
    fl_lines = 1248;
    CDBG("%s: line <= 1248 - 8\n",__func__);
  }else{
    fl_lines = line + offset;
    CDBG("%s: line > 1248 - 8\n",__func__);
  }
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_output_reg_addr->frame_length_lines,
    s5k3h2_sunny_q8s02e_byte(fl_lines, MSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
    s5k3h2_sunny_q8s02e_byte(fl_lines, LSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  /* Coarse Integration Time */
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
    s5k3h2_sunny_q8s02e_byte(line, MSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
    s5k3h2_sunny_q8s02e_byte(line, LSB),
    MSM_CAMERA_I2C_BYTE_DATA);

  /* Analogue Gain */
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->global_gain_addr,
    s5k3h2_sunny_q8s02e_byte(gain, MSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
    s5k3h2_sunny_q8s02e_byte(gain, LSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
  return rc;
}

static int32_t s5k3h2_sunny_q8s02e_write_pict_exp_gain(
  struct msm_sensor_ctrl_t *s_ctrl,    uint16_t gain, uint32_t line)
{
  uint16_t max_legal_gain = 0x0400;
  uint16_t min_legal_line = 0x03;
  int32_t rc = 0;
  static uint32_t fl_lines, offset;

  CDBG("%s: gain = %d,line = %d\n",__func__, gain, line);
  offset = s_ctrl->sensor_exp_gain_info->vert_offset;
  if (gain > max_legal_gain) {
    CDBG("%s: gain > max_legal_gain\n",__func__);
    gain = max_legal_gain;
  }
  if (line < min_legal_line) {
    CDBG("%s: line < min_legal_line\n",__func__);
    line = min_legal_line;
  }

  s_ctrl->func_tbl->sensor_group_hold_on(s_ctrl);
  if ((line < (2480-offset))||(line == (2480-offset))){
    fl_lines = 2480;
    CDBG("%s: line <= 2480 - 8\n",__func__);
  }else{
    fl_lines = line + offset;
    CDBG("%s: line > 2480 - 8\n",__func__);
  }
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_output_reg_addr->frame_length_lines,
    s5k3h2_sunny_q8s02e_byte(fl_lines, MSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_output_reg_addr->frame_length_lines + 1,
    s5k3h2_sunny_q8s02e_byte(fl_lines, LSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  /* Coarse Integration Time */
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->coarse_int_time_addr,
    s5k3h2_sunny_q8s02e_byte(line, MSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->coarse_int_time_addr + 1,
    s5k3h2_sunny_q8s02e_byte(line, LSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  /* Analogue Gain */
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->global_gain_addr,
    s5k3h2_sunny_q8s02e_byte(gain, MSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,
    s_ctrl->sensor_exp_gain_info->global_gain_addr + 1,
    s5k3h2_sunny_q8s02e_byte(gain, LSB),
    MSM_CAMERA_I2C_BYTE_DATA);
  s_ctrl->func_tbl->sensor_group_hold_off(s_ctrl);
  return rc;
}

static const struct i2c_device_id s5k3h2_sunny_q8s02e_i2c_id[] = {
  {SENSOR_NAME, (kernel_ulong_t)&s5k3h2_sunny_q8s02e_s_ctrl},
  { }
};

int32_t s5k3h2_sunny_q8s02e_sensor_i2c_probe(struct i2c_client *client,
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
  CDBG("%s X\n", __func__);
  return rc;
}

static struct i2c_driver s5k3h2_sunny_q8s02e_i2c_driver = {
  .id_table                  = s5k3h2_sunny_q8s02e_i2c_id,
  .probe                     = s5k3h2_sunny_q8s02e_sensor_i2c_probe,
  .driver                    = {
    .name                    = SENSOR_NAME,
  },
};

static struct msm_camera_i2c_client s5k3h2_sunny_q8s02e_sensor_i2c_client = {
  .addr_type                 = MSM_CAMERA_I2C_WORD_ADDR,
};

static int __init msm_sensor_init_module(void)
{
  return i2c_add_driver(&s5k3h2_sunny_q8s02e_i2c_driver);
}

static struct v4l2_subdev_core_ops s5k3h2_sunny_q8s02e_subdev_core_ops = {
  .ioctl                     = msm_sensor_subdev_ioctl,
  .s_power                   = msm_sensor_power,
};

static struct v4l2_subdev_video_ops s5k3h2_sunny_q8s02e_subdev_video_ops = {
  .enum_mbus_fmt             = msm_sensor_v4l2_enum_fmt,
};

static struct v4l2_subdev_ops s5k3h2_sunny_q8s02e_subdev_ops = {
  .core                      = &s5k3h2_sunny_q8s02e_subdev_core_ops,
  .video                     = &s5k3h2_sunny_q8s02e_subdev_video_ops,
};

int32_t s5k3h2_sunny_q8s02e_sensor_power_down(struct msm_sensor_ctrl_t *s_ctrl)
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
  lcd_camera_power_l5_onoff(0);
  msleep(40);
  s5k3h2_sunny_q8s02e_prev_exp_gain = 0x0020; //same as preview settigns
  s5k3h2_sunny_q8s02e_prev_exp_line = 0x4db; //same as preview settigns
  CDBG("%s: X\n",__func__);
  return 0;
}

int32_t s5k3h2_sunny_q8s02e_sensor_power_up(struct msm_sensor_ctrl_t *s_ctrl)
{
  int32_t rc = 0;
  struct msm_camera_sensor_info *info = NULL;
  info = s_ctrl->sensordata;
  CDBG("%s, sensor_pwd:%d, sensor_reset:%d\r\n",__func__,
    info->sensor_pwd,info->sensor_reset);
  gpio_direction_output(info->sensor_pwd, 0);
  gpio_direction_output(info->sensor_reset, 0);
  usleep_range(10000, 11000);
  lcd_camera_power_l5_onoff(1);
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

int32_t s5k3h2_sunny_q8s02e_sensor_match_id(struct msm_sensor_ctrl_t *s_ctrl)
{
  int32_t rc = 0;
  uint16_t chipid_high_byte = 0;
  uint16_t chipid_low_byte = 0;
  uint16_t chipid = 0;
  rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0000, &chipid_high_byte,
    MSM_CAMERA_I2C_BYTE_DATA);
  rc = msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0001, &chipid_low_byte,
    MSM_CAMERA_I2C_BYTE_DATA);
  if (rc < 0) {
    CDBG("%s: %s: read id failed\n", __func__,
      s_ctrl->sensordata->sensor_name);
    return rc;
  }
  chipid = (chipid_high_byte <<8) | chipid_low_byte ;
  CDBG("%s: chipid = 0x%x\n", __func__,chipid);
  if (chipid != s_ctrl->sensor_id_info->sensor_id) {
    CDBG("%s: chip id mismatch\n",__func__);
    return -ENODEV;
  }
  return rc;
}

#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE
void s5k3h2_sunny_q8s02e_otp_init_setting(struct msm_sensor_ctrl_t *s_ctrl)
{
  msleep(10);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3200,0x28,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3a2d,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3a2d,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
}

void s5k3h2_sunny_q8s02e_read_otp_data_by_page(struct msm_sensor_ctrl_t *s_ctrl,
  int page, struct s5k3h2_sunny_q8s02e_otp_struct *p_otp)
{
  uint16_t i = 0;
  uint16_t temp = 0;
#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE_DGB
  CDBG("%s: page = 0x%02x\n\n",__func__,page);
#endif
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3a1c,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x04,
    MSM_CAMERA_I2C_BYTE_DATA);
  page = page & 0xff;
  if(page > 0x1f){
    CDBG("%s: page number is out of range\n",__func__);
  }
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a02,page,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
  msleep(1);
  for(;i<64;i++){
    msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a04+i,&temp,
      MSM_CAMERA_I2C_BYTE_DATA);
#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE_DGB
    CDBG("%s: Reg_0x%04x = 0x%04x\n",__func__,(0x0a04+i),temp);
#endif
    ((uint8_t *)p_otp)[(page - 9)*64 + i] = temp & 0xff;
  }
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x04,
    MSM_CAMERA_I2C_BYTE_DATA);
}

void s5k3h2_sunny_q8s02e_calc_otp(uint16_t r_ratio, uint16_t b_ratio,
  uint16_t *r_target, uint16_t *b_target, uint16_t r_offset, uint16_t b_offset)
{
  if ((b_offset * ABS(S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - r_ratio))
    < (r_offset * ABS(S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - b_ratio))) {
    if (b_ratio < S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE)
      *b_target = S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - b_offset;
    else
      *b_target = S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE + b_offset;

    if (r_ratio < S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE) {
      *r_target = S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE
        - ABS(S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - *b_target)
        * ABS(S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - r_ratio)
        / ABS(S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - b_ratio);
    } else {
      *r_target = S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE
        + ABS(S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - *b_target)
        * ABS(S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - r_ratio)
        / ABS(S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - b_ratio);
    }
  } else {
    if (r_ratio < S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE)
      *r_target = S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - r_offset;
    else
      *r_target = S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE + r_offset;

    if (b_ratio < S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE) {
      *b_target = S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE
        - ABS(S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - *r_target)
        * ABS(S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - b_ratio)
        / ABS(S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - r_ratio);
    } else {
      *b_target = S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE
        + ABS(S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - *r_target)
        * ABS(S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE - b_ratio)
        / ABS(S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE - r_ratio);
    }
  }
}

void s5k3h2_sunny_q8s02e_update_awb_otp_data(struct msm_sensor_ctrl_t *s_ctrl,
  struct s5k3h2_sunny_q8s02e_otp_struct *p_otp)
{
  uint16_t temp = 0;
  uint16_t AWB_R_Gr_Ratio = 0;
  uint16_t AWB_B_Gr_Ratio = 0;
  uint16_t AWB_Gb_Gr_Ratio = 0;
  uint16_t R_gain, G_gain, B_gain;
  uint16_t R_offset_outside, B_offset_outside;
  uint16_t R_offset_inside, B_offset_inside;
  uint16_t R_target, B_target;

  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3a1c,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x04,
    MSM_CAMERA_I2C_BYTE_DATA);

  //page 0x10
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a02,0x10,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
  msleep(1);
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a04,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_10[0] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a05,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_10[1] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a06,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_10[2] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a07,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_10[3] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a08,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_10[4] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a09,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_10[5] = temp & 0xff;
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x04,
    MSM_CAMERA_I2C_BYTE_DATA);

  //page 0x11
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a02,0x11,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
  msleep(1);
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a04,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_11[0] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a05,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_11[1] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a06,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_11[2] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a07,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_11[3] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a08,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_11[4] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a09,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_11[5] = temp & 0xff;
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x04,
    MSM_CAMERA_I2C_BYTE_DATA);

  //page 0x12
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a02,0x12,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x01,
    MSM_CAMERA_I2C_BYTE_DATA);
  msleep(1);
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a04,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_12[0] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a05,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_12[1] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a06,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_12[2] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a07,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_12[3] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a08,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_12[4] = temp & 0xff;
  msm_camera_i2c_read(s_ctrl->sensor_i2c_client,0x0a09,&temp,
    MSM_CAMERA_I2C_BYTE_DATA);
  p_otp->AWB_Page_12[5] = temp & 0xff;
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x04,
    MSM_CAMERA_I2C_BYTE_DATA);

  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0a00,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE_DGB
  CDBG("%s: AWB_Page_10[0]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_10[0] ,p_otp->AWB_Page_10[0] );
  CDBG("%s: AWB_Page_10[1]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_10[1] ,p_otp->AWB_Page_10[1] );
  CDBG("%s: AWB_Page_10[2]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_10[2] ,p_otp->AWB_Page_10[2] );
  CDBG("%s: AWB_Page_10[3]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_10[3] ,p_otp->AWB_Page_10[3] );
  CDBG("%s: AWB_Page_10[4]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_10[4] ,p_otp->AWB_Page_10[4] );
  CDBG("%s: AWB_Page_10[5]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_10[5] ,p_otp->AWB_Page_10[5] );

  CDBG("%s: AWB_Page_11[0]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_11[0] ,p_otp->AWB_Page_11[0] );
  CDBG("%s: AWB_Page_11[1]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_11[1] ,p_otp->AWB_Page_11[1] );
  CDBG("%s: AWB_Page_11[2]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_11[2] ,p_otp->AWB_Page_11[2] );
  CDBG("%s: AWB_Page_11[3]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_11[3] ,p_otp->AWB_Page_11[3] );
  CDBG("%s: AWB_Page_11[4]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_11[4] ,p_otp->AWB_Page_11[4] );
  CDBG("%s: AWB_Page_11[5]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_11[5] ,p_otp->AWB_Page_11[5] );

  CDBG("%s: AWB_Page_12[0]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_12[0] ,p_otp->AWB_Page_12[0] );
  CDBG("%s: AWB_Page_12[1]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_12[1] ,p_otp->AWB_Page_12[1] );
  CDBG("%s: AWB_Page_12[2]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_12[2] ,p_otp->AWB_Page_12[2] );
  CDBG("%s: AWB_Page_12[3]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_12[3] ,p_otp->AWB_Page_12[3] );
  CDBG("%s: AWB_Page_12[4]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_12[4] ,p_otp->AWB_Page_12[4] );
  CDBG("%s: AWB_Page_12[5]  = 0x%02x (%d)\n",__func__,
    p_otp->AWB_Page_12[5] ,p_otp->AWB_Page_12[5] );
#endif
  AWB_R_Gr_Ratio = ((p_otp->AWB_Page_10[0])<<8) | (p_otp->AWB_Page_10[1]);
  AWB_B_Gr_Ratio = ((p_otp->AWB_Page_10[2])<<8) | (p_otp->AWB_Page_10[3]);
  AWB_Gb_Gr_Ratio = ((p_otp->AWB_Page_10[4])<<8) | (p_otp->AWB_Page_10[5]);

  AWB_R_Gr_Ratio = AWB_R_Gr_Ratio
    * S5K3H2_SUNNY_Q8S02E_Gb_Gr_RATIO_TYPICAL_VALUE / AWB_Gb_Gr_Ratio;
  AWB_B_Gr_Ratio = AWB_B_Gr_Ratio
    * S5K3H2_SUNNY_Q8S02E_Gb_Gr_RATIO_TYPICAL_VALUE / AWB_Gb_Gr_Ratio;

  R_offset_outside = 70;
  B_offset_outside = 70;
  R_offset_inside = 10;
  B_offset_inside = 10;

  if ((ABS(AWB_R_Gr_Ratio - S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE)
    < R_offset_inside)
    && (ABS(AWB_B_Gr_Ratio - S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE)
    < B_offset_inside)) {/* in inside range */
    R_gain = 0x100;
    G_gain = 0x100;
    B_gain = 0x100;
  } else {
    if ((ABS(AWB_R_Gr_Ratio - S5K3H2_SUNNY_Q8S02E_R_Gr_RATIO_TYPICAL_VALUE)
      < R_offset_outside)
      && (ABS(AWB_B_Gr_Ratio - S5K3H2_SUNNY_Q8S02E_B_Gr_RATIO_TYPICAL_VALUE)
      < B_offset_outside)) {
      s5k3h2_sunny_q8s02e_calc_otp(AWB_R_Gr_Ratio, AWB_B_Gr_Ratio
        , &R_target, &B_target,/* between inside and outside range */
        R_offset_inside, B_offset_inside);
    } else {
      s5k3h2_sunny_q8s02e_calc_otp(AWB_R_Gr_Ratio, AWB_B_Gr_Ratio
        , &R_target, &B_target,/* out of outside range */
        R_offset_outside, B_offset_outside);
    }

    /* 0x100 = 1x gain */
    if (AWB_B_Gr_Ratio < B_target) {
      if (AWB_R_Gr_Ratio < R_target) {
        G_gain = 0x100;
        B_gain = 0x100 *
          B_target /
          AWB_B_Gr_Ratio;
        R_gain = 0x100 *
          R_target /
          AWB_R_Gr_Ratio;
      } else {
        R_gain = 0x100;
        G_gain = 0x100 *
          AWB_R_Gr_Ratio /
          R_target;
        B_gain = 0x100 * AWB_R_Gr_Ratio * B_target
          / (AWB_B_Gr_Ratio * R_target);
      }
    } else {
      if (AWB_R_Gr_Ratio < R_target) {
        B_gain = 0x100;
        G_gain = 0x100 *
          AWB_B_Gr_Ratio /
          B_target;
        R_gain = 0x100 * AWB_B_Gr_Ratio * R_target
          / (AWB_R_Gr_Ratio * B_target);
      } else {
        if (B_target * AWB_R_Gr_Ratio < R_target * AWB_B_Gr_Ratio) {
          B_gain = 0x100;
          G_gain = 0x100 *
            AWB_B_Gr_Ratio /
            B_target;
          R_gain = 0x100 * AWB_B_Gr_Ratio * R_target
          / (AWB_R_Gr_Ratio * B_target);
        } else {
          R_gain = 0x100;
          G_gain = 0x100 *
            AWB_R_Gr_Ratio /
            R_target;
          B_gain = 0x100 * AWB_R_Gr_Ratio * B_target
          / (AWB_B_Gr_Ratio * R_target);
        }
      }
    }
  }

  //R
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0210,((R_gain && 0xff00)>>8),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0211,(R_gain && 0xff),
    MSM_CAMERA_I2C_BYTE_DATA);

  //B
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0212,((B_gain && 0xff00)>>8),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0213,(B_gain && 0xff),
    MSM_CAMERA_I2C_BYTE_DATA);

  //Gb
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0214,((G_gain && 0xff00)>>8),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x0215,(G_gain && 0xff),
    MSM_CAMERA_I2C_BYTE_DATA);

  //Gr
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x020e,((G_gain && 0xff00)>>8),
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x020f,(G_gain && 0xff),
    MSM_CAMERA_I2C_BYTE_DATA);
}

int s5k3h2_sunny_q8s02e_check_lsc_otp_status(struct msm_sensor_ctrl_t *s_ctrl,
  struct s5k3h2_sunny_q8s02e_otp_struct *p_otp)
{
  s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x09,p_otp);
  if((p_otp->LSC_OTP_Disable_Page_09[0] == 0)
    &&(p_otp->LSC_OTP_Disable_Page_09[1] == 0)
    &&(p_otp->LSC_OTP_Disable_Page_09[2] == 0)){
    //OTPM have valid data
#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE_DGB
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x0a,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x0b,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x0c,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x0d,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x0e,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x0f,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x10,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x11,p_otp);
    s5k3h2_sunny_q8s02e_read_otp_data_by_page(s_ctrl,0x12,p_otp);

    CDBG("%s: LSC_OTP_Disable_Page_09[0]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[0] ,p_otp->LSC_OTP_Disable_Page_09[0] );
    CDBG("%s: LSC_OTP_Disable_Page_09[1]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[1] ,p_otp->LSC_OTP_Disable_Page_09[1] );
    CDBG("%s: LSC_OTP_Disable_Page_09[2]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[2] ,p_otp->LSC_OTP_Disable_Page_09[2] );
    CDBG("%s: LSC_OTP_Disable_Page_09[3]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[3] ,p_otp->LSC_OTP_Disable_Page_09[3] );
    CDBG("%s: LSC_OTP_Disable_Page_09[4]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[4] ,p_otp->LSC_OTP_Disable_Page_09[4] );
    CDBG("%s: LSC_OTP_Disable_Page_09[5]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[5] ,p_otp->LSC_OTP_Disable_Page_09[5] );
    CDBG("%s: LSC_OTP_Disable_Page_09[6]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[6] ,p_otp->LSC_OTP_Disable_Page_09[6] );
    CDBG("%s: LSC_OTP_Disable_Page_09[7]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[7] ,p_otp->LSC_OTP_Disable_Page_09[7] );
    CDBG("%s: LSC_OTP_Disable_Page_09[8]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[8] ,p_otp->LSC_OTP_Disable_Page_09[8] );
    CDBG("%s: LSC_OTP_Disable_Page_09[9]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[9] ,p_otp->LSC_OTP_Disable_Page_09[9] );
    CDBG("%s: LSC_OTP_Disable_Page_09[10] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[10],p_otp->LSC_OTP_Disable_Page_09[10]);
    CDBG("%s: LSC_OTP_Disable_Page_09[11] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[11],p_otp->LSC_OTP_Disable_Page_09[11]);
    CDBG("%s: LSC_OTP_Disable_Page_09[12] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[12],p_otp->LSC_OTP_Disable_Page_09[12]);
    CDBG("%s: LSC_OTP_Disable_Page_09[13] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[13],p_otp->LSC_OTP_Disable_Page_09[13]);
    CDBG("%s: LSC_OTP_Disable_Page_09[14] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[14],p_otp->LSC_OTP_Disable_Page_09[14]);
    CDBG("%s: LSC_OTP_Disable_Page_09[15] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[15],p_otp->LSC_OTP_Disable_Page_09[15]);
    CDBG("%s: LSC_OTP_Disable_Page_09[16] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[16],p_otp->LSC_OTP_Disable_Page_09[16]);
    CDBG("%s: LSC_OTP_Disable_Page_09[17] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[17],p_otp->LSC_OTP_Disable_Page_09[17]);
    CDBG("%s: LSC_OTP_Disable_Page_09[18] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[18],p_otp->LSC_OTP_Disable_Page_09[18]);
    CDBG("%s: LSC_OTP_Disable_Page_09[19] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[19],p_otp->LSC_OTP_Disable_Page_09[19]);
    CDBG("%s: LSC_OTP_Disable_Page_09[20] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[20],p_otp->LSC_OTP_Disable_Page_09[20]);
    CDBG("%s: LSC_OTP_Disable_Page_09[21] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[21],p_otp->LSC_OTP_Disable_Page_09[21]);
    CDBG("%s: LSC_OTP_Disable_Page_09[22] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[22],p_otp->LSC_OTP_Disable_Page_09[22]);
    CDBG("%s: LSC_OTP_Disable_Page_09[23] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[23],p_otp->LSC_OTP_Disable_Page_09[23]);
    CDBG("%s: LSC_OTP_Disable_Page_09[24] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[24],p_otp->LSC_OTP_Disable_Page_09[24]);
    CDBG("%s: LSC_OTP_Disable_Page_09[25] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[25],p_otp->LSC_OTP_Disable_Page_09[25]);
    CDBG("%s: LSC_OTP_Disable_Page_09[26] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[26],p_otp->LSC_OTP_Disable_Page_09[26]);
    CDBG("%s: LSC_OTP_Disable_Page_09[27] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[27],p_otp->LSC_OTP_Disable_Page_09[27]);
    CDBG("%s: LSC_OTP_Disable_Page_09[28] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[28],p_otp->LSC_OTP_Disable_Page_09[28]);
    CDBG("%s: LSC_OTP_Disable_Page_09[29] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[29],p_otp->LSC_OTP_Disable_Page_09[29]);
    CDBG("%s: LSC_OTP_Disable_Page_09[30] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[30],p_otp->LSC_OTP_Disable_Page_09[30]);
    CDBG("%s: LSC_OTP_Disable_Page_09[31] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[31],p_otp->LSC_OTP_Disable_Page_09[31]);
    CDBG("%s: LSC_OTP_Disable_Page_09[32] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[32],p_otp->LSC_OTP_Disable_Page_09[32]);
    CDBG("%s: LSC_OTP_Disable_Page_09[33] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[33],p_otp->LSC_OTP_Disable_Page_09[33]);
    CDBG("%s: LSC_OTP_Disable_Page_09[34] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[34],p_otp->LSC_OTP_Disable_Page_09[34]);
    CDBG("%s: LSC_OTP_Disable_Page_09[35] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[35],p_otp->LSC_OTP_Disable_Page_09[35]);
    CDBG("%s: LSC_OTP_Disable_Page_09[36] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[36],p_otp->LSC_OTP_Disable_Page_09[36]);
    CDBG("%s: LSC_OTP_Disable_Page_09[37] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[37],p_otp->LSC_OTP_Disable_Page_09[37]);
    CDBG("%s: LSC_OTP_Disable_Page_09[38] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[38],p_otp->LSC_OTP_Disable_Page_09[38]);
    CDBG("%s: LSC_OTP_Disable_Page_09[39] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[39],p_otp->LSC_OTP_Disable_Page_09[39]);
    CDBG("%s: LSC_OTP_Disable_Page_09[40] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[40],p_otp->LSC_OTP_Disable_Page_09[40]);
    CDBG("%s: LSC_OTP_Disable_Page_09[41] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[41],p_otp->LSC_OTP_Disable_Page_09[41]);
    CDBG("%s: LSC_OTP_Disable_Page_09[42] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[42],p_otp->LSC_OTP_Disable_Page_09[42]);
    CDBG("%s: LSC_OTP_Disable_Page_09[43] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[43],p_otp->LSC_OTP_Disable_Page_09[43]);
    CDBG("%s: LSC_OTP_Disable_Page_09[44] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[44],p_otp->LSC_OTP_Disable_Page_09[44]);
    CDBG("%s: LSC_OTP_Disable_Page_09[45] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[45],p_otp->LSC_OTP_Disable_Page_09[45]);
    CDBG("%s: LSC_OTP_Disable_Page_09[46] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[46],p_otp->LSC_OTP_Disable_Page_09[46]);
    CDBG("%s: LSC_OTP_Disable_Page_09[47] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[47],p_otp->LSC_OTP_Disable_Page_09[47]);
    CDBG("%s: LSC_OTP_Disable_Page_09[48] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[48],p_otp->LSC_OTP_Disable_Page_09[48]);
    CDBG("%s: LSC_OTP_Disable_Page_09[49] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[49],p_otp->LSC_OTP_Disable_Page_09[49]);
    CDBG("%s: LSC_OTP_Disable_Page_09[50] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[50],p_otp->LSC_OTP_Disable_Page_09[50]);
    CDBG("%s: LSC_OTP_Disable_Page_09[51] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[51],p_otp->LSC_OTP_Disable_Page_09[51]);
    CDBG("%s: LSC_OTP_Disable_Page_09[52] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[52],p_otp->LSC_OTP_Disable_Page_09[52]);
    CDBG("%s: LSC_OTP_Disable_Page_09[53] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[53],p_otp->LSC_OTP_Disable_Page_09[53]);
    CDBG("%s: LSC_OTP_Disable_Page_09[54] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[54],p_otp->LSC_OTP_Disable_Page_09[54]);
    CDBG("%s: LSC_OTP_Disable_Page_09[55] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[55],p_otp->LSC_OTP_Disable_Page_09[55]);
    CDBG("%s: LSC_OTP_Disable_Page_09[56] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[56],p_otp->LSC_OTP_Disable_Page_09[56]);
    CDBG("%s: LSC_OTP_Disable_Page_09[57] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[57],p_otp->LSC_OTP_Disable_Page_09[57]);
    CDBG("%s: LSC_OTP_Disable_Page_09[58] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[58],p_otp->LSC_OTP_Disable_Page_09[58]);
    CDBG("%s: LSC_OTP_Disable_Page_09[59] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[59],p_otp->LSC_OTP_Disable_Page_09[59]);
    CDBG("%s: LSC_OTP_Disable_Page_09[60] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[60],p_otp->LSC_OTP_Disable_Page_09[60]);
    CDBG("%s: LSC_OTP_Disable_Page_09[61] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[61],p_otp->LSC_OTP_Disable_Page_09[61]);
    CDBG("%s: LSC_OTP_Disable_Page_09[62] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[62],p_otp->LSC_OTP_Disable_Page_09[62]);
    CDBG("%s: LSC_OTP_Disable_Page_09[63] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_OTP_Disable_Page_09[63],p_otp->LSC_OTP_Disable_Page_09[63]);

    CDBG("%s: Chip_ID_Page_0A[0]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[0] ,p_otp->Chip_ID_Page_0A[0] );
    CDBG("%s: Chip_ID_Page_0A[1]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[1] ,p_otp->Chip_ID_Page_0A[1] );
    CDBG("%s: Chip_ID_Page_0A[2]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[2] ,p_otp->Chip_ID_Page_0A[2] );
    CDBG("%s: Chip_ID_Page_0A[3]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[3] ,p_otp->Chip_ID_Page_0A[3] );
    CDBG("%s: Chip_ID_Page_0A[4]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[4] ,p_otp->Chip_ID_Page_0A[4] );
    CDBG("%s: Chip_ID_Page_0A[5]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[5] ,p_otp->Chip_ID_Page_0A[5] );
    CDBG("%s: Chip_ID_Page_0A[6]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[6] ,p_otp->Chip_ID_Page_0A[6] );
    CDBG("%s: Chip_ID_Page_0A[7]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[7] ,p_otp->Chip_ID_Page_0A[7] );
    CDBG("%s: Chip_ID_Page_0A[8]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[8] ,p_otp->Chip_ID_Page_0A[8] );
    CDBG("%s: Chip_ID_Page_0A[9]  = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[9] ,p_otp->Chip_ID_Page_0A[9] );
    CDBG("%s: Chip_ID_Page_0A[10] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[10],p_otp->Chip_ID_Page_0A[10]);
    CDBG("%s: Chip_ID_Page_0A[11] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[11],p_otp->Chip_ID_Page_0A[11]);
    CDBG("%s: Chip_ID_Page_0A[12] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[12],p_otp->Chip_ID_Page_0A[12]);
    CDBG("%s: Chip_ID_Page_0A[13] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[13],p_otp->Chip_ID_Page_0A[13]);
    CDBG("%s: Chip_ID_Page_0A[14] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[14],p_otp->Chip_ID_Page_0A[14]);
    CDBG("%s: Chip_ID_Page_0A[15] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[15],p_otp->Chip_ID_Page_0A[15]);
    CDBG("%s: Chip_ID_Page_0A[16] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[16],p_otp->Chip_ID_Page_0A[16]);
    CDBG("%s: Chip_ID_Page_0A[17] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[17],p_otp->Chip_ID_Page_0A[17]);
    CDBG("%s: Chip_ID_Page_0A[18] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[18],p_otp->Chip_ID_Page_0A[18]);
    CDBG("%s: Chip_ID_Page_0A[19] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[19],p_otp->Chip_ID_Page_0A[19]);
    CDBG("%s: Chip_ID_Page_0A[20] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[20],p_otp->Chip_ID_Page_0A[20]);
    CDBG("%s: Chip_ID_Page_0A[21] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[21],p_otp->Chip_ID_Page_0A[21]);
    CDBG("%s: Chip_ID_Page_0A[22] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[22],p_otp->Chip_ID_Page_0A[22]);
    CDBG("%s: Chip_ID_Page_0A[23] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[23],p_otp->Chip_ID_Page_0A[23]);
    CDBG("%s: Chip_ID_Page_0A[24] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[24],p_otp->Chip_ID_Page_0A[24]);
    CDBG("%s: Chip_ID_Page_0A[25] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[25],p_otp->Chip_ID_Page_0A[25]);
    CDBG("%s: Chip_ID_Page_0A[26] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[26],p_otp->Chip_ID_Page_0A[26]);
    CDBG("%s: Chip_ID_Page_0A[27] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[27],p_otp->Chip_ID_Page_0A[27]);
    CDBG("%s: Chip_ID_Page_0A[28] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[28],p_otp->Chip_ID_Page_0A[28]);
    CDBG("%s: Chip_ID_Page_0A[29] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[29],p_otp->Chip_ID_Page_0A[29]);
    CDBG("%s: Chip_ID_Page_0A[30] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[30],p_otp->Chip_ID_Page_0A[30]);
    CDBG("%s: Chip_ID_Page_0A[31] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[31],p_otp->Chip_ID_Page_0A[31]);
    CDBG("%s: Chip_ID_Page_0A[32] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[32],p_otp->Chip_ID_Page_0A[32]);
    CDBG("%s: Chip_ID_Page_0A[33] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[33],p_otp->Chip_ID_Page_0A[33]);
    CDBG("%s: Chip_ID_Page_0A[34] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[34],p_otp->Chip_ID_Page_0A[34]);
    CDBG("%s: Chip_ID_Page_0A[35] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[35],p_otp->Chip_ID_Page_0A[35]);
    CDBG("%s: Chip_ID_Page_0A[36] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[36],p_otp->Chip_ID_Page_0A[36]);
    CDBG("%s: Chip_ID_Page_0A[37] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[37],p_otp->Chip_ID_Page_0A[37]);
    CDBG("%s: Chip_ID_Page_0A[38] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[38],p_otp->Chip_ID_Page_0A[38]);
    CDBG("%s: Chip_ID_Page_0A[39] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[39],p_otp->Chip_ID_Page_0A[39]);
    CDBG("%s: Chip_ID_Page_0A[40] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[40],p_otp->Chip_ID_Page_0A[40]);
    CDBG("%s: Chip_ID_Page_0A[41] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[41],p_otp->Chip_ID_Page_0A[41]);
    CDBG("%s: Chip_ID_Page_0A[42] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[42],p_otp->Chip_ID_Page_0A[42]);
    CDBG("%s: Chip_ID_Page_0A[43] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[43],p_otp->Chip_ID_Page_0A[43]);
    CDBG("%s: Chip_ID_Page_0A[44] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[44],p_otp->Chip_ID_Page_0A[44]);
    CDBG("%s: Chip_ID_Page_0A[45] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[45],p_otp->Chip_ID_Page_0A[45]);
    CDBG("%s: Chip_ID_Page_0A[46] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[46],p_otp->Chip_ID_Page_0A[46]);
    CDBG("%s: Chip_ID_Page_0A[47] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[47],p_otp->Chip_ID_Page_0A[47]);
    CDBG("%s: Chip_ID_Page_0A[48] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[48],p_otp->Chip_ID_Page_0A[48]);
    CDBG("%s: Chip_ID_Page_0A[49] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[49],p_otp->Chip_ID_Page_0A[49]);
    CDBG("%s: Chip_ID_Page_0A[50] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[50],p_otp->Chip_ID_Page_0A[50]);
    CDBG("%s: Chip_ID_Page_0A[51] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[51],p_otp->Chip_ID_Page_0A[51]);
    CDBG("%s: Chip_ID_Page_0A[52] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[52],p_otp->Chip_ID_Page_0A[52]);
    CDBG("%s: Chip_ID_Page_0A[53] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[53],p_otp->Chip_ID_Page_0A[53]);
    CDBG("%s: Chip_ID_Page_0A[54] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[54],p_otp->Chip_ID_Page_0A[54]);
    CDBG("%s: Chip_ID_Page_0A[55] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[55],p_otp->Chip_ID_Page_0A[55]);
    CDBG("%s: Chip_ID_Page_0A[56] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[56],p_otp->Chip_ID_Page_0A[56]);
    CDBG("%s: Chip_ID_Page_0A[57] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[57],p_otp->Chip_ID_Page_0A[57]);
    CDBG("%s: Chip_ID_Page_0A[58] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[58],p_otp->Chip_ID_Page_0A[58]);
    CDBG("%s: Chip_ID_Page_0A[59] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[59],p_otp->Chip_ID_Page_0A[59]);
    CDBG("%s: Chip_ID_Page_0A[60] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[60],p_otp->Chip_ID_Page_0A[60]);
    CDBG("%s: Chip_ID_Page_0A[61] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[61],p_otp->Chip_ID_Page_0A[61]);
    CDBG("%s: Chip_ID_Page_0A[62] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[62],p_otp->Chip_ID_Page_0A[62]);
    CDBG("%s: Chip_ID_Page_0A[63] = 0x%02x (%d)\n",__func__,
      p_otp->Chip_ID_Page_0A[63],p_otp->Chip_ID_Page_0A[63]);

    CDBG("%s: LSC_Page_0B[0]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[0] ,p_otp->LSC_Page_0B[0] );
    CDBG("%s: LSC_Page_0B[1]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[1] ,p_otp->LSC_Page_0B[1] );
    CDBG("%s: LSC_Page_0B[2]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[2] ,p_otp->LSC_Page_0B[2] );
    CDBG("%s: LSC_Page_0B[3]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[3] ,p_otp->LSC_Page_0B[3] );
    CDBG("%s: LSC_Page_0B[4]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[4] ,p_otp->LSC_Page_0B[4] );
    CDBG("%s: LSC_Page_0B[5]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[5] ,p_otp->LSC_Page_0B[5] );
    CDBG("%s: LSC_Page_0B[6]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[6] ,p_otp->LSC_Page_0B[6] );
    CDBG("%s: LSC_Page_0B[7]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[7] ,p_otp->LSC_Page_0B[7] );
    CDBG("%s: LSC_Page_0B[8]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[8] ,p_otp->LSC_Page_0B[8] );
    CDBG("%s: LSC_Page_0B[9]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[9] ,p_otp->LSC_Page_0B[9] );
    CDBG("%s: LSC_Page_0B[10] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[10],p_otp->LSC_Page_0B[10]);
    CDBG("%s: LSC_Page_0B[11] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[11],p_otp->LSC_Page_0B[11]);
    CDBG("%s: LSC_Page_0B[12] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[12],p_otp->LSC_Page_0B[12]);
    CDBG("%s: LSC_Page_0B[13] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[13],p_otp->LSC_Page_0B[13]);
    CDBG("%s: LSC_Page_0B[14] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[14],p_otp->LSC_Page_0B[14]);
    CDBG("%s: LSC_Page_0B[15] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[15],p_otp->LSC_Page_0B[15]);
    CDBG("%s: LSC_Page_0B[16] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[16],p_otp->LSC_Page_0B[16]);
    CDBG("%s: LSC_Page_0B[17] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[17],p_otp->LSC_Page_0B[17]);
    CDBG("%s: LSC_Page_0B[18] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[18],p_otp->LSC_Page_0B[18]);
    CDBG("%s: LSC_Page_0B[19] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[19],p_otp->LSC_Page_0B[19]);
    CDBG("%s: LSC_Page_0B[20] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[20],p_otp->LSC_Page_0B[20]);
    CDBG("%s: LSC_Page_0B[21] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[21],p_otp->LSC_Page_0B[21]);
    CDBG("%s: LSC_Page_0B[22] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[22],p_otp->LSC_Page_0B[22]);
    CDBG("%s: LSC_Page_0B[23] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[23],p_otp->LSC_Page_0B[23]);
    CDBG("%s: LSC_Page_0B[24] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[24],p_otp->LSC_Page_0B[24]);
    CDBG("%s: LSC_Page_0B[25] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[25],p_otp->LSC_Page_0B[25]);
    CDBG("%s: LSC_Page_0B[26] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[26],p_otp->LSC_Page_0B[26]);
    CDBG("%s: LSC_Page_0B[27] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[27],p_otp->LSC_Page_0B[27]);
    CDBG("%s: LSC_Page_0B[28] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[28],p_otp->LSC_Page_0B[28]);
    CDBG("%s: LSC_Page_0B[29] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[29],p_otp->LSC_Page_0B[29]);
    CDBG("%s: LSC_Page_0B[30] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[30],p_otp->LSC_Page_0B[30]);
    CDBG("%s: LSC_Page_0B[31] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[31],p_otp->LSC_Page_0B[31]);
    CDBG("%s: LSC_Page_0B[32] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[32],p_otp->LSC_Page_0B[32]);
    CDBG("%s: LSC_Page_0B[33] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[33],p_otp->LSC_Page_0B[33]);
    CDBG("%s: LSC_Page_0B[34] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[34],p_otp->LSC_Page_0B[34]);
    CDBG("%s: LSC_Page_0B[35] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[35],p_otp->LSC_Page_0B[35]);
    CDBG("%s: LSC_Page_0B[36] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[36],p_otp->LSC_Page_0B[36]);
    CDBG("%s: LSC_Page_0B[37] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[37],p_otp->LSC_Page_0B[37]);
    CDBG("%s: LSC_Page_0B[38] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[38],p_otp->LSC_Page_0B[38]);
    CDBG("%s: LSC_Page_0B[39] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[39],p_otp->LSC_Page_0B[39]);
    CDBG("%s: LSC_Page_0B[40] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[40],p_otp->LSC_Page_0B[40]);
    CDBG("%s: LSC_Page_0B[41] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[41],p_otp->LSC_Page_0B[41]);
    CDBG("%s: LSC_Page_0B[42] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[42],p_otp->LSC_Page_0B[42]);
    CDBG("%s: LSC_Page_0B[43] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[43],p_otp->LSC_Page_0B[43]);
    CDBG("%s: LSC_Page_0B[44] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[44],p_otp->LSC_Page_0B[44]);
    CDBG("%s: LSC_Page_0B[45] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[45],p_otp->LSC_Page_0B[45]);
    CDBG("%s: LSC_Page_0B[46] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[46],p_otp->LSC_Page_0B[46]);
    CDBG("%s: LSC_Page_0B[47] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[47],p_otp->LSC_Page_0B[47]);
    CDBG("%s: LSC_Page_0B[48] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[48],p_otp->LSC_Page_0B[48]);
    CDBG("%s: LSC_Page_0B[49] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[49],p_otp->LSC_Page_0B[49]);
    CDBG("%s: LSC_Page_0B[50] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[50],p_otp->LSC_Page_0B[50]);
    CDBG("%s: LSC_Page_0B[51] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[51],p_otp->LSC_Page_0B[51]);
    CDBG("%s: LSC_Page_0B[52] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[52],p_otp->LSC_Page_0B[52]);
    CDBG("%s: LSC_Page_0B[53] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[53],p_otp->LSC_Page_0B[53]);
    CDBG("%s: LSC_Page_0B[54] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[54],p_otp->LSC_Page_0B[54]);
    CDBG("%s: LSC_Page_0B[55] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[55],p_otp->LSC_Page_0B[55]);
    CDBG("%s: LSC_Page_0B[56] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[56],p_otp->LSC_Page_0B[56]);
    CDBG("%s: LSC_Page_0B[57] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[57],p_otp->LSC_Page_0B[57]);
    CDBG("%s: LSC_Page_0B[58] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[58],p_otp->LSC_Page_0B[58]);
    CDBG("%s: LSC_Page_0B[59] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[59],p_otp->LSC_Page_0B[59]);
    CDBG("%s: LSC_Page_0B[60] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[60],p_otp->LSC_Page_0B[60]);
    CDBG("%s: LSC_Page_0B[61] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[61],p_otp->LSC_Page_0B[61]);
    CDBG("%s: LSC_Page_0B[62] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[62],p_otp->LSC_Page_0B[62]);
    CDBG("%s: LSC_Page_0B[63] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0B[63],p_otp->LSC_Page_0B[63]);

    CDBG("%s: LSC_Page_0C[0]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[0] ,p_otp->LSC_Page_0C[0] );
    CDBG("%s: LSC_Page_0C[1]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[1] ,p_otp->LSC_Page_0C[1] );
    CDBG("%s: LSC_Page_0C[2]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[2] ,p_otp->LSC_Page_0C[2] );
    CDBG("%s: LSC_Page_0C[3]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[3] ,p_otp->LSC_Page_0C[3] );
    CDBG("%s: LSC_Page_0C[4]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[4] ,p_otp->LSC_Page_0C[4] );
    CDBG("%s: LSC_Page_0C[5]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[5] ,p_otp->LSC_Page_0C[5] );
    CDBG("%s: LSC_Page_0C[6]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[6] ,p_otp->LSC_Page_0C[6] );
    CDBG("%s: LSC_Page_0C[7]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[7] ,p_otp->LSC_Page_0C[7] );
    CDBG("%s: LSC_Page_0C[8]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[8] ,p_otp->LSC_Page_0C[8] );
    CDBG("%s: LSC_Page_0C[9]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[9] ,p_otp->LSC_Page_0C[9] );
    CDBG("%s: LSC_Page_0C[10] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[10],p_otp->LSC_Page_0C[10]);
    CDBG("%s: LSC_Page_0C[11] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[11],p_otp->LSC_Page_0C[11]);
    CDBG("%s: LSC_Page_0C[12] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[12],p_otp->LSC_Page_0C[12]);
    CDBG("%s: LSC_Page_0C[13] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[13],p_otp->LSC_Page_0C[13]);
    CDBG("%s: LSC_Page_0C[14] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[14],p_otp->LSC_Page_0C[14]);
    CDBG("%s: LSC_Page_0C[15] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[15],p_otp->LSC_Page_0C[15]);
    CDBG("%s: LSC_Page_0C[16] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[16],p_otp->LSC_Page_0C[16]);
    CDBG("%s: LSC_Page_0C[17] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[17],p_otp->LSC_Page_0C[17]);
    CDBG("%s: LSC_Page_0C[18] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[18],p_otp->LSC_Page_0C[18]);
    CDBG("%s: LSC_Page_0C[19] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[19],p_otp->LSC_Page_0C[19]);
    CDBG("%s: LSC_Page_0C[20] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[20],p_otp->LSC_Page_0C[20]);
    CDBG("%s: LSC_Page_0C[21] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[21],p_otp->LSC_Page_0C[21]);
    CDBG("%s: LSC_Page_0C[22] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[22],p_otp->LSC_Page_0C[22]);
    CDBG("%s: LSC_Page_0C[23] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[23],p_otp->LSC_Page_0C[23]);
    CDBG("%s: LSC_Page_0C[24] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[24],p_otp->LSC_Page_0C[24]);
    CDBG("%s: LSC_Page_0C[25] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[25],p_otp->LSC_Page_0C[25]);
    CDBG("%s: LSC_Page_0C[26] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[26],p_otp->LSC_Page_0C[26]);
    CDBG("%s: LSC_Page_0C[27] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[27],p_otp->LSC_Page_0C[27]);
    CDBG("%s: LSC_Page_0C[28] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[28],p_otp->LSC_Page_0C[28]);
    CDBG("%s: LSC_Page_0C[29] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[29],p_otp->LSC_Page_0C[29]);
    CDBG("%s: LSC_Page_0C[30] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[30],p_otp->LSC_Page_0C[30]);
    CDBG("%s: LSC_Page_0C[31] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[31],p_otp->LSC_Page_0C[31]);
    CDBG("%s: LSC_Page_0C[32] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[32],p_otp->LSC_Page_0C[32]);
    CDBG("%s: LSC_Page_0C[33] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[33],p_otp->LSC_Page_0C[33]);
    CDBG("%s: LSC_Page_0C[34] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[34],p_otp->LSC_Page_0C[34]);
    CDBG("%s: LSC_Page_0C[35] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[35],p_otp->LSC_Page_0C[35]);
    CDBG("%s: LSC_Page_0C[36] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[36],p_otp->LSC_Page_0C[36]);
    CDBG("%s: LSC_Page_0C[37] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[37],p_otp->LSC_Page_0C[37]);
    CDBG("%s: LSC_Page_0C[38] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[38],p_otp->LSC_Page_0C[38]);
    CDBG("%s: LSC_Page_0C[39] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[39],p_otp->LSC_Page_0C[39]);
    CDBG("%s: LSC_Page_0C[40] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[40],p_otp->LSC_Page_0C[40]);
    CDBG("%s: LSC_Page_0C[41] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[41],p_otp->LSC_Page_0C[41]);
    CDBG("%s: LSC_Page_0C[42] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[42],p_otp->LSC_Page_0C[42]);
    CDBG("%s: LSC_Page_0C[43] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[43],p_otp->LSC_Page_0C[43]);
    CDBG("%s: LSC_Page_0C[44] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[44],p_otp->LSC_Page_0C[44]);
    CDBG("%s: LSC_Page_0C[45] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[45],p_otp->LSC_Page_0C[45]);
    CDBG("%s: LSC_Page_0C[46] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[46],p_otp->LSC_Page_0C[46]);
    CDBG("%s: LSC_Page_0C[47] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[47],p_otp->LSC_Page_0C[47]);
    CDBG("%s: LSC_Page_0C[48] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[48],p_otp->LSC_Page_0C[48]);
    CDBG("%s: LSC_Page_0C[49] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[49],p_otp->LSC_Page_0C[49]);
    CDBG("%s: LSC_Page_0C[50] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[50],p_otp->LSC_Page_0C[50]);
    CDBG("%s: LSC_Page_0C[51] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[51],p_otp->LSC_Page_0C[51]);
    CDBG("%s: LSC_Page_0C[52] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[52],p_otp->LSC_Page_0C[52]);
    CDBG("%s: LSC_Page_0C[53] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[53],p_otp->LSC_Page_0C[53]);
    CDBG("%s: LSC_Page_0C[54] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[54],p_otp->LSC_Page_0C[54]);
    CDBG("%s: LSC_Page_0C[55] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[55],p_otp->LSC_Page_0C[55]);
    CDBG("%s: LSC_Page_0C[56] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[56],p_otp->LSC_Page_0C[56]);
    CDBG("%s: LSC_Page_0C[57] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[57],p_otp->LSC_Page_0C[57]);
    CDBG("%s: LSC_Page_0C[58] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[58],p_otp->LSC_Page_0C[58]);
    CDBG("%s: LSC_Page_0C[59] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[59],p_otp->LSC_Page_0C[59]);
    CDBG("%s: LSC_Page_0C[60] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[60],p_otp->LSC_Page_0C[60]);
    CDBG("%s: LSC_Page_0C[61] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[61],p_otp->LSC_Page_0C[61]);
    CDBG("%s: LSC_Page_0C[62] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[62],p_otp->LSC_Page_0C[62]);
    CDBG("%s: LSC_Page_0C[63] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0C[63],p_otp->LSC_Page_0C[63]);

    CDBG("%s: LSC_Page_0D[0]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[0] ,p_otp->LSC_Page_0D[0] );
    CDBG("%s: LSC_Page_0D[1]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[1] ,p_otp->LSC_Page_0D[1] );
    CDBG("%s: LSC_Page_0D[2]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[2] ,p_otp->LSC_Page_0D[2] );
    CDBG("%s: LSC_Page_0D[3]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[3] ,p_otp->LSC_Page_0D[3] );
    CDBG("%s: LSC_Page_0D[4]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[4] ,p_otp->LSC_Page_0D[4] );
    CDBG("%s: LSC_Page_0D[5]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[5] ,p_otp->LSC_Page_0D[5] );
    CDBG("%s: LSC_Page_0D[6]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[6] ,p_otp->LSC_Page_0D[6] );
    CDBG("%s: LSC_Page_0D[7]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[7] ,p_otp->LSC_Page_0D[7] );
    CDBG("%s: LSC_Page_0D[8]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[8] ,p_otp->LSC_Page_0D[8] );
    CDBG("%s: LSC_Page_0D[9]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[9] ,p_otp->LSC_Page_0D[9] );
    CDBG("%s: LSC_Page_0D[10] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[10],p_otp->LSC_Page_0D[10]);
    CDBG("%s: LSC_Page_0D[11] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[11],p_otp->LSC_Page_0D[11]);
    CDBG("%s: LSC_Page_0D[12] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[12],p_otp->LSC_Page_0D[12]);
    CDBG("%s: LSC_Page_0D[13] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[13],p_otp->LSC_Page_0D[13]);
    CDBG("%s: LSC_Page_0D[14] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[14],p_otp->LSC_Page_0D[14]);
    CDBG("%s: LSC_Page_0D[15] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[15],p_otp->LSC_Page_0D[15]);
    CDBG("%s: LSC_Page_0D[16] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[16],p_otp->LSC_Page_0D[16]);
    CDBG("%s: LSC_Page_0D[17] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[17],p_otp->LSC_Page_0D[17]);
    CDBG("%s: LSC_Page_0D[18] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[18],p_otp->LSC_Page_0D[18]);
    CDBG("%s: LSC_Page_0D[19] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[19],p_otp->LSC_Page_0D[19]);
    CDBG("%s: LSC_Page_0D[20] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[20],p_otp->LSC_Page_0D[20]);
    CDBG("%s: LSC_Page_0D[21] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[21],p_otp->LSC_Page_0D[21]);
    CDBG("%s: LSC_Page_0D[22] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[22],p_otp->LSC_Page_0D[22]);
    CDBG("%s: LSC_Page_0D[23] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[23],p_otp->LSC_Page_0D[23]);
    CDBG("%s: LSC_Page_0D[24] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[24],p_otp->LSC_Page_0D[24]);
    CDBG("%s: LSC_Page_0D[25] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[25],p_otp->LSC_Page_0D[25]);
    CDBG("%s: LSC_Page_0D[26] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[26],p_otp->LSC_Page_0D[26]);
    CDBG("%s: LSC_Page_0D[27] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[27],p_otp->LSC_Page_0D[27]);
    CDBG("%s: LSC_Page_0D[28] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[28],p_otp->LSC_Page_0D[28]);
    CDBG("%s: LSC_Page_0D[29] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[29],p_otp->LSC_Page_0D[29]);
    CDBG("%s: LSC_Page_0D[30] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[30],p_otp->LSC_Page_0D[30]);
    CDBG("%s: LSC_Page_0D[31] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[31],p_otp->LSC_Page_0D[31]);
    CDBG("%s: LSC_Page_0D[32] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[32],p_otp->LSC_Page_0D[32]);
    CDBG("%s: LSC_Page_0D[33] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[33],p_otp->LSC_Page_0D[33]);
    CDBG("%s: LSC_Page_0D[34] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[34],p_otp->LSC_Page_0D[34]);
    CDBG("%s: LSC_Page_0D[35] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[35],p_otp->LSC_Page_0D[35]);
    CDBG("%s: LSC_Page_0D[36] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[36],p_otp->LSC_Page_0D[36]);
    CDBG("%s: LSC_Page_0D[37] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[37],p_otp->LSC_Page_0D[37]);
    CDBG("%s: LSC_Page_0D[38] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[38],p_otp->LSC_Page_0D[38]);
    CDBG("%s: LSC_Page_0D[39] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[39],p_otp->LSC_Page_0D[39]);
    CDBG("%s: LSC_Page_0D[40] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[40],p_otp->LSC_Page_0D[40]);
    CDBG("%s: LSC_Page_0D[41] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[41],p_otp->LSC_Page_0D[41]);
    CDBG("%s: LSC_Page_0D[42] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[42],p_otp->LSC_Page_0D[42]);
    CDBG("%s: LSC_Page_0D[43] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[43],p_otp->LSC_Page_0D[43]);
    CDBG("%s: LSC_Page_0D[44] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[44],p_otp->LSC_Page_0D[44]);
    CDBG("%s: LSC_Page_0D[45] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[45],p_otp->LSC_Page_0D[45]);
    CDBG("%s: LSC_Page_0D[46] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[46],p_otp->LSC_Page_0D[46]);
    CDBG("%s: LSC_Page_0D[47] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[47],p_otp->LSC_Page_0D[47]);
    CDBG("%s: LSC_Page_0D[48] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[48],p_otp->LSC_Page_0D[48]);
    CDBG("%s: LSC_Page_0D[49] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[49],p_otp->LSC_Page_0D[49]);
    CDBG("%s: LSC_Page_0D[50] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[50],p_otp->LSC_Page_0D[50]);
    CDBG("%s: LSC_Page_0D[51] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[51],p_otp->LSC_Page_0D[51]);
    CDBG("%s: LSC_Page_0D[52] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[52],p_otp->LSC_Page_0D[52]);
    CDBG("%s: LSC_Page_0D[53] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[53],p_otp->LSC_Page_0D[53]);
    CDBG("%s: LSC_Page_0D[54] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[54],p_otp->LSC_Page_0D[54]);
    CDBG("%s: LSC_Page_0D[55] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[55],p_otp->LSC_Page_0D[55]);
    CDBG("%s: LSC_Page_0D[56] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[56],p_otp->LSC_Page_0D[56]);
    CDBG("%s: LSC_Page_0D[57] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[57],p_otp->LSC_Page_0D[57]);
    CDBG("%s: LSC_Page_0D[58] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[58],p_otp->LSC_Page_0D[58]);
    CDBG("%s: LSC_Page_0D[59] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[59],p_otp->LSC_Page_0D[59]);
    CDBG("%s: LSC_Page_0D[60] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[60],p_otp->LSC_Page_0D[60]);
    CDBG("%s: LSC_Page_0D[61] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[61],p_otp->LSC_Page_0D[61]);
    CDBG("%s: LSC_Page_0D[62] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[62],p_otp->LSC_Page_0D[62]);
    CDBG("%s: LSC_Page_0D[63] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0D[63],p_otp->LSC_Page_0D[63]);

    CDBG("%s: LSC_Page_0E[0]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[0] ,p_otp->LSC_Page_0E[0] );
    CDBG("%s: LSC_Page_0E[1]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[1] ,p_otp->LSC_Page_0E[1] );
    CDBG("%s: LSC_Page_0E[2]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[2] ,p_otp->LSC_Page_0E[2] );
    CDBG("%s: LSC_Page_0E[3]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[3] ,p_otp->LSC_Page_0E[3] );
    CDBG("%s: LSC_Page_0E[4]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[4] ,p_otp->LSC_Page_0E[4] );
    CDBG("%s: LSC_Page_0E[5]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[5] ,p_otp->LSC_Page_0E[5] );
    CDBG("%s: LSC_Page_0E[6]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[6] ,p_otp->LSC_Page_0E[6] );
    CDBG("%s: LSC_Page_0E[7]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[7] ,p_otp->LSC_Page_0E[7] );
    CDBG("%s: LSC_Page_0E[8]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[8] ,p_otp->LSC_Page_0E[8] );
    CDBG("%s: LSC_Page_0E[9]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[9] ,p_otp->LSC_Page_0E[9] );
    CDBG("%s: LSC_Page_0E[10] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[10],p_otp->LSC_Page_0E[10]);
    CDBG("%s: LSC_Page_0E[11] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[11],p_otp->LSC_Page_0E[11]);
    CDBG("%s: LSC_Page_0E[12] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[12],p_otp->LSC_Page_0E[12]);
    CDBG("%s: LSC_Page_0E[13] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[13],p_otp->LSC_Page_0E[13]);
    CDBG("%s: LSC_Page_0E[14] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[14],p_otp->LSC_Page_0E[14]);
    CDBG("%s: LSC_Page_0E[15] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[15],p_otp->LSC_Page_0E[15]);
    CDBG("%s: LSC_Page_0E[16] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[16],p_otp->LSC_Page_0E[16]);
    CDBG("%s: LSC_Page_0E[17] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[17],p_otp->LSC_Page_0E[17]);
    CDBG("%s: LSC_Page_0E[18] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[18],p_otp->LSC_Page_0E[18]);
    CDBG("%s: LSC_Page_0E[19] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[19],p_otp->LSC_Page_0E[19]);
    CDBG("%s: LSC_Page_0E[20] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[20],p_otp->LSC_Page_0E[20]);
    CDBG("%s: LSC_Page_0E[21] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[21],p_otp->LSC_Page_0E[21]);
    CDBG("%s: LSC_Page_0E[22] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[22],p_otp->LSC_Page_0E[22]);
    CDBG("%s: LSC_Page_0E[23] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[23],p_otp->LSC_Page_0E[23]);
    CDBG("%s: LSC_Page_0E[24] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[24],p_otp->LSC_Page_0E[24]);
    CDBG("%s: LSC_Page_0E[25] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[25],p_otp->LSC_Page_0E[25]);
    CDBG("%s: LSC_Page_0E[26] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[26],p_otp->LSC_Page_0E[26]);
    CDBG("%s: LSC_Page_0E[27] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[27],p_otp->LSC_Page_0E[27]);
    CDBG("%s: LSC_Page_0E[28] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[28],p_otp->LSC_Page_0E[28]);
    CDBG("%s: LSC_Page_0E[29] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[29],p_otp->LSC_Page_0E[29]);
    CDBG("%s: LSC_Page_0E[30] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[30],p_otp->LSC_Page_0E[30]);
    CDBG("%s: LSC_Page_0E[31] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[31],p_otp->LSC_Page_0E[31]);
    CDBG("%s: LSC_Page_0E[32] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[32],p_otp->LSC_Page_0E[32]);
    CDBG("%s: LSC_Page_0E[33] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[33],p_otp->LSC_Page_0E[33]);
    CDBG("%s: LSC_Page_0E[34] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[34],p_otp->LSC_Page_0E[34]);
    CDBG("%s: LSC_Page_0E[35] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[35],p_otp->LSC_Page_0E[35]);
    CDBG("%s: LSC_Page_0E[36] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[36],p_otp->LSC_Page_0E[36]);
    CDBG("%s: LSC_Page_0E[37] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[37],p_otp->LSC_Page_0E[37]);
    CDBG("%s: LSC_Page_0E[38] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[38],p_otp->LSC_Page_0E[38]);
    CDBG("%s: LSC_Page_0E[39] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[39],p_otp->LSC_Page_0E[39]);
    CDBG("%s: LSC_Page_0E[40] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[40],p_otp->LSC_Page_0E[40]);
    CDBG("%s: LSC_Page_0E[41] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[41],p_otp->LSC_Page_0E[41]);
    CDBG("%s: LSC_Page_0E[42] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[42],p_otp->LSC_Page_0E[42]);
    CDBG("%s: LSC_Page_0E[43] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[43],p_otp->LSC_Page_0E[43]);
    CDBG("%s: LSC_Page_0E[44] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[44],p_otp->LSC_Page_0E[44]);
    CDBG("%s: LSC_Page_0E[45] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[45],p_otp->LSC_Page_0E[45]);
    CDBG("%s: LSC_Page_0E[46] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[46],p_otp->LSC_Page_0E[46]);
    CDBG("%s: LSC_Page_0E[47] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[47],p_otp->LSC_Page_0E[47]);
    CDBG("%s: LSC_Page_0E[48] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[48],p_otp->LSC_Page_0E[48]);
    CDBG("%s: LSC_Page_0E[49] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[49],p_otp->LSC_Page_0E[49]);
    CDBG("%s: LSC_Page_0E[50] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[50],p_otp->LSC_Page_0E[50]);
    CDBG("%s: LSC_Page_0E[51] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[51],p_otp->LSC_Page_0E[51]);
    CDBG("%s: LSC_Page_0E[52] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[52],p_otp->LSC_Page_0E[52]);
    CDBG("%s: LSC_Page_0E[53] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[53],p_otp->LSC_Page_0E[53]);
    CDBG("%s: LSC_Page_0E[54] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[54],p_otp->LSC_Page_0E[54]);
    CDBG("%s: LSC_Page_0E[55] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[55],p_otp->LSC_Page_0E[55]);
    CDBG("%s: LSC_Page_0E[56] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[56],p_otp->LSC_Page_0E[56]);
    CDBG("%s: LSC_Page_0E[57] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[57],p_otp->LSC_Page_0E[57]);
    CDBG("%s: LSC_Page_0E[58] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[58],p_otp->LSC_Page_0E[58]);
    CDBG("%s: LSC_Page_0E[59] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[59],p_otp->LSC_Page_0E[59]);
    CDBG("%s: LSC_Page_0E[60] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[60],p_otp->LSC_Page_0E[60]);
    CDBG("%s: LSC_Page_0E[61] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[61],p_otp->LSC_Page_0E[61]);
    CDBG("%s: LSC_Page_0E[62] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[62],p_otp->LSC_Page_0E[62]);
    CDBG("%s: LSC_Page_0E[63] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0E[63],p_otp->LSC_Page_0E[63]);

    CDBG("%s: LSC_Page_0F[0]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[0] ,p_otp->LSC_Page_0F[0] );
    CDBG("%s: LSC_Page_0F[1]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[1] ,p_otp->LSC_Page_0F[1] );
    CDBG("%s: LSC_Page_0F[2]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[2] ,p_otp->LSC_Page_0F[2] );
    CDBG("%s: LSC_Page_0F[3]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[3] ,p_otp->LSC_Page_0F[3] );
    CDBG("%s: LSC_Page_0F[4]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[4] ,p_otp->LSC_Page_0F[4] );
    CDBG("%s: LSC_Page_0F[5]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[5] ,p_otp->LSC_Page_0F[5] );
    CDBG("%s: LSC_Page_0F[6]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[6] ,p_otp->LSC_Page_0F[6] );
    CDBG("%s: LSC_Page_0F[7]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[7] ,p_otp->LSC_Page_0F[7] );
    CDBG("%s: LSC_Page_0F[8]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[8] ,p_otp->LSC_Page_0F[8] );
    CDBG("%s: LSC_Page_0F[9]  = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[9] ,p_otp->LSC_Page_0F[9] );
    CDBG("%s: LSC_Page_0F[10] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[10],p_otp->LSC_Page_0F[10]);
    CDBG("%s: LSC_Page_0F[11] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[11],p_otp->LSC_Page_0F[11]);
    CDBG("%s: LSC_Page_0F[12] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[12],p_otp->LSC_Page_0F[12]);
    CDBG("%s: LSC_Page_0F[13] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[13],p_otp->LSC_Page_0F[13]);
    CDBG("%s: LSC_Page_0F[14] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[14],p_otp->LSC_Page_0F[14]);
    CDBG("%s: LSC_Page_0F[15] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[15],p_otp->LSC_Page_0F[15]);
    CDBG("%s: LSC_Page_0F[16] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[16],p_otp->LSC_Page_0F[16]);
    CDBG("%s: LSC_Page_0F[17] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[17],p_otp->LSC_Page_0F[17]);
    CDBG("%s: LSC_Page_0F[18] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[18],p_otp->LSC_Page_0F[18]);
    CDBG("%s: LSC_Page_0F[19] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[19],p_otp->LSC_Page_0F[19]);
    CDBG("%s: LSC_Page_0F[20] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[20],p_otp->LSC_Page_0F[20]);
    CDBG("%s: LSC_Page_0F[21] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[21],p_otp->LSC_Page_0F[21]);
    CDBG("%s: LSC_Page_0F[22] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[22],p_otp->LSC_Page_0F[22]);
    CDBG("%s: LSC_Page_0F[23] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[23],p_otp->LSC_Page_0F[23]);
    CDBG("%s: LSC_Page_0F[24] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[24],p_otp->LSC_Page_0F[24]);
    CDBG("%s: LSC_Page_0F[25] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[25],p_otp->LSC_Page_0F[25]);
    CDBG("%s: LSC_Page_0F[26] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[26],p_otp->LSC_Page_0F[26]);
    CDBG("%s: LSC_Page_0F[27] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[27],p_otp->LSC_Page_0F[27]);
    CDBG("%s: LSC_Page_0F[28] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[28],p_otp->LSC_Page_0F[28]);
    CDBG("%s: LSC_Page_0F[29] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[29],p_otp->LSC_Page_0F[29]);
    CDBG("%s: LSC_Page_0F[30] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[30],p_otp->LSC_Page_0F[30]);
    CDBG("%s: LSC_Page_0F[31] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[31],p_otp->LSC_Page_0F[31]);
    CDBG("%s: LSC_Page_0F[32] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[32],p_otp->LSC_Page_0F[32]);
    CDBG("%s: LSC_Page_0F[33] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[33],p_otp->LSC_Page_0F[33]);
    CDBG("%s: LSC_Page_0F[34] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[34],p_otp->LSC_Page_0F[34]);
    CDBG("%s: LSC_Page_0F[35] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[35],p_otp->LSC_Page_0F[35]);
    CDBG("%s: LSC_Page_0F[36] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[36],p_otp->LSC_Page_0F[36]);
    CDBG("%s: LSC_Page_0F[37] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[37],p_otp->LSC_Page_0F[37]);
    CDBG("%s: LSC_Page_0F[38] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[38],p_otp->LSC_Page_0F[38]);
    CDBG("%s: LSC_Page_0F[39] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[39],p_otp->LSC_Page_0F[39]);
    CDBG("%s: LSC_Page_0F[40] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[40],p_otp->LSC_Page_0F[40]);
    CDBG("%s: LSC_Page_0F[41] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[41],p_otp->LSC_Page_0F[41]);
    CDBG("%s: LSC_Page_0F[42] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[42],p_otp->LSC_Page_0F[42]);
    CDBG("%s: LSC_Page_0F[43] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[43],p_otp->LSC_Page_0F[43]);
    CDBG("%s: LSC_Page_0F[44] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[44],p_otp->LSC_Page_0F[44]);
    CDBG("%s: LSC_Page_0F[45] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[45],p_otp->LSC_Page_0F[45]);
    CDBG("%s: LSC_Page_0F[46] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[46],p_otp->LSC_Page_0F[46]);
    CDBG("%s: LSC_Page_0F[47] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[47],p_otp->LSC_Page_0F[47]);
    CDBG("%s: LSC_Page_0F[48] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[48],p_otp->LSC_Page_0F[48]);
    CDBG("%s: LSC_Page_0F[49] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[49],p_otp->LSC_Page_0F[49]);
    CDBG("%s: LSC_Page_0F[50] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[50],p_otp->LSC_Page_0F[50]);
    CDBG("%s: LSC_Page_0F[51] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[51],p_otp->LSC_Page_0F[51]);
    CDBG("%s: LSC_Page_0F[52] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[52],p_otp->LSC_Page_0F[52]);
    CDBG("%s: LSC_Page_0F[53] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[53],p_otp->LSC_Page_0F[53]);
    CDBG("%s: LSC_Page_0F[54] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[54],p_otp->LSC_Page_0F[54]);
    CDBG("%s: LSC_Page_0F[55] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[55],p_otp->LSC_Page_0F[55]);
    CDBG("%s: LSC_Page_0F[56] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[56],p_otp->LSC_Page_0F[56]);
    CDBG("%s: LSC_Page_0F[57] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[57],p_otp->LSC_Page_0F[57]);
    CDBG("%s: LSC_Page_0F[58] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[58],p_otp->LSC_Page_0F[58]);
    CDBG("%s: LSC_Page_0F[59] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[59],p_otp->LSC_Page_0F[59]);
    CDBG("%s: LSC_Page_0F[60] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[60],p_otp->LSC_Page_0F[60]);
    CDBG("%s: LSC_Page_0F[61] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[61],p_otp->LSC_Page_0F[61]);
    CDBG("%s: LSC_Page_0F[62] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[62],p_otp->LSC_Page_0F[62]);
    CDBG("%s: LSC_Page_0F[63] = 0x%02x (%d)\n",__func__,
      p_otp->LSC_Page_0F[63],p_otp->LSC_Page_0F[63]);
#endif
    CDBG("%s: OTPM have valid data\n",__func__);
    return 1;
  }else{
    //OTPM have no valid data
    CDBG("%s: OTPM have no valid data\n",__func__);
    return 0;
  }
}

void s5k3h2_sunny_q8s02e_write_default_lsc_otp_data(
  struct msm_sensor_ctrl_t *s_ctrl,
  struct s5k3h2_sunny_q8s02e_otp_struct *p_otp)
{
  uint16_t i = 0;
  uint16_t temp = 0;

  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3200,0x08,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3a2d,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3210,0x81,
    MSM_CAMERA_I2C_BYTE_DATA);

  for(i=0;i<72;i++){
    temp = ((((uint8_t *)p_otp)[88 + (5*i)])  & 0xf0)>>4;
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3214,temp,
      MSM_CAMERA_I2C_BYTE_DATA);
    temp = (((((uint8_t *)p_otp)[88 + (5*i)])  & 0x0f)<<4)
      |(((((uint8_t *)p_otp)[88 + 5*i+1])  & 0xf0)>>4);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3215,temp,
      MSM_CAMERA_I2C_BYTE_DATA);
    temp = (((((uint8_t *)p_otp)[88 + (5*i)+1])  & 0x0f)<<4)
      |(((((uint8_t *)p_otp)[88 + (5*i)+2])  & 0xf0)>>4);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3216,temp,
      MSM_CAMERA_I2C_BYTE_DATA);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3217,(2*i),
      MSM_CAMERA_I2C_BYTE_DATA);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3218,0x01,
      MSM_CAMERA_I2C_BYTE_DATA);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3218,0x00,
      MSM_CAMERA_I2C_BYTE_DATA);

    temp = ((((uint8_t *)p_otp)[88 + (5*i)+2])  & 0x0f);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3214,temp,
      MSM_CAMERA_I2C_BYTE_DATA);
    temp = ((uint8_t *)p_otp)[88 + (5*i)+3];
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3215,temp,
      MSM_CAMERA_I2C_BYTE_DATA);
    temp = ((uint8_t *)p_otp)[88 + (5*i)+4];
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3216,temp,
      MSM_CAMERA_I2C_BYTE_DATA);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3217,(2*i+1),
      MSM_CAMERA_I2C_BYTE_DATA);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3218,0x01,
      MSM_CAMERA_I2C_BYTE_DATA);
    msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3218,0x00,
      MSM_CAMERA_I2C_BYTE_DATA);
  }
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3200,0x18,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3200,0x08,
    MSM_CAMERA_I2C_BYTE_DATA);
  msm_camera_i2c_write(s_ctrl->sensor_i2c_client,0x3201,0x00,
    MSM_CAMERA_I2C_BYTE_DATA);
}

void s5k3h2_sunny_q8s02e_update_otp(struct msm_sensor_ctrl_t *s_ctrl)
{
  if(s5k3h2_sunny_q8s02e_check_lsc_otp_status(s_ctrl,
    &st_s5k3h2_sunny_q8s02e_otp)){
    ;
  }else{
    CDBG("%s: have no valid lsc otp data, write default value\n",__func__);
    s5k3h2_sunny_q8s02e_write_default_lsc_otp_data(s_ctrl,
      &st_s5k3h2_sunny_q8s02e_otp);
  }
  s5k3h2_sunny_q8s02e_update_awb_otp_data(s_ctrl,&st_s5k3h2_sunny_q8s02e_otp);
}
#endif

int32_t s5k3h2_sunny_q8s02e_sensor_setting(struct msm_sensor_ctrl_t *s_ctrl,
  int update_type, int res)
{
  int32_t rc = 0;
  static int csi_config;

  s_ctrl->func_tbl->sensor_stop_stream(s_ctrl);
  if (update_type == MSM_SENSOR_REG_INIT) {
    CDBG("Register INIT\n");
    s_ctrl->curr_csi_params = NULL;
    msm_sensor_enable_debugfs(s_ctrl);
    msm_sensor_write_init_settings(s_ctrl);
#ifdef S5K3H2_SUNNY_Q8S02E_OTP_FEATURE
    s5k3h2_sunny_q8s02e_otp_init_setting(s_ctrl);
    s5k3h2_sunny_q8s02e_update_otp(s_ctrl);
#endif
    csi_config = 0;
  } else if (update_type == MSM_SENSOR_UPDATE_PERIODIC) {
    CDBG("PERIODIC : %d\n", res);
    if(res == 0){
      msleep(105);// preview -> capture
    }else{
      msleep(205);// capture -> preview
    }
    msm_sensor_write_conf_array(
      s_ctrl->sensor_i2c_client,
      s_ctrl->msm_sensor_reg->mode_settings, res);
    if(res == 0){
      // preview -> capture
      ;
    }else{
      // capture -> preview
      s_ctrl->func_tbl->sensor_write_exp_gain(s_ctrl,
        s5k3h2_sunny_q8s02e_prev_exp_gain,s5k3h2_sunny_q8s02e_prev_exp_line);
    }
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
    }
    v4l2_subdev_notify(&s_ctrl->sensor_v4l2_subdev,
      NOTIFY_PCLK_CHANGE,
      &s_ctrl->sensordata->pdata->ioclk.vfe_clk_rate);
    s_ctrl->func_tbl->sensor_start_stream(s_ctrl);
    msleep(50);
  }
  return rc;
}

static struct msm_sensor_fn_t s5k3h2_sunny_q8s02e_func_tbl = {
  .sensor_start_stream            = s5k3h2_sunny_q8s02e_start_stream,
  .sensor_stop_stream             = s5k3h2_sunny_q8s02e_stop_stream,
  .sensor_group_hold_on           = s5k3h2_sunny_q8s02e_group_hold_on,
  .sensor_group_hold_off          = s5k3h2_sunny_q8s02e_group_hold_off,
  .sensor_set_fps                 = msm_sensor_set_fps,
  .sensor_write_exp_gain          = s5k3h2_sunny_q8s02e_write_prev_exp_gain,
  .sensor_write_snapshot_exp_gain = s5k3h2_sunny_q8s02e_write_pict_exp_gain,
  .sensor_csi_setting             = s5k3h2_sunny_q8s02e_sensor_setting,
  .sensor_set_sensor_mode         = msm_sensor_set_sensor_mode,
  .sensor_mode_init               = msm_sensor_mode_init,
  .sensor_get_output_info         = msm_sensor_get_output_info,
  .sensor_config                  = msm_sensor_config,
  .sensor_power_up                = s5k3h2_sunny_q8s02e_sensor_power_up,
  .sensor_power_down              = s5k3h2_sunny_q8s02e_sensor_power_down,
  .sensor_match_id                = s5k3h2_sunny_q8s02e_sensor_match_id,
  .sensor_get_csi_params          = msm_sensor_get_csi_params,
};

static struct msm_sensor_reg_t s5k3h2_sunny_q8s02e_regs = {
  .default_data_type              = MSM_CAMERA_I2C_BYTE_DATA,
  .start_stream_conf              = s5k3h2_sunny_q8s02e_start_stream_conf,
  .start_stream_conf_size         = ARRAY_SIZE(s5k3h2_sunny_q8s02e_start_stream_conf),
  .stop_stream_conf               = s5k3h2_sunny_q8s02e_stop_stream_conf,
  .stop_stream_conf_size          = ARRAY_SIZE(s5k3h2_sunny_q8s02e_stop_stream_conf),
  .group_hold_on_conf             = s5k3h2_sunny_q8s02e_group_hold_on_conf,
  .group_hold_on_conf_size        = ARRAY_SIZE(s5k3h2_sunny_q8s02e_group_hold_on_conf),
  .group_hold_off_conf            = s5k3h2_sunny_q8s02e_group_hold_off_conf,
  .group_hold_off_conf_size       = ARRAY_SIZE(s5k3h2_sunny_q8s02e_group_hold_off_conf),
  .init_settings                  = &s5k3h2_sunny_q8s02e_init_settings[0],
  .init_size                      = ARRAY_SIZE(s5k3h2_sunny_q8s02e_init_settings),
  .mode_settings                  = &s5k3h2_sunny_q8s02e_mode_settings[0],
  .output_settings                = &s5k3h2_sunny_q8s02e_output_settings[0],
  .num_conf                       = ARRAY_SIZE(s5k3h2_sunny_q8s02e_mode_settings),
};

static struct msm_sensor_ctrl_t s5k3h2_sunny_q8s02e_s_ctrl = {
  .msm_sensor_reg                 = &s5k3h2_sunny_q8s02e_regs,
  .sensor_i2c_client              = &s5k3h2_sunny_q8s02e_sensor_i2c_client,
  .sensor_i2c_addr                = 0x20,
  .sensor_output_reg_addr         = &s5k3h2_sunny_q8s02e_output_reg_addr,
  .sensor_id_info                 = &s5k3h2_sunny_q8s02e_id_info,
  .sensor_exp_gain_info           = &s5k3h2_sunny_q8s02e_exp_gain_info,
  .cam_mode                       = MSM_SENSOR_MODE_INVALID,
  .csic_params                    = &s5k3h2_sunny_q8s02e_csi_params_array[0],
  .msm_sensor_mutex               = &s5k3h2_sunny_q8s02e_mut,
  .sensor_i2c_driver              = &s5k3h2_sunny_q8s02e_i2c_driver,
  .sensor_v4l2_subdev_info        = s5k3h2_sunny_q8s02e_subdev_info,
  .sensor_v4l2_subdev_info_size   = ARRAY_SIZE(s5k3h2_sunny_q8s02e_subdev_info),
  .sensor_v4l2_subdev_ops         = &s5k3h2_sunny_q8s02e_subdev_ops,
  .func_tbl                       = &s5k3h2_sunny_q8s02e_func_tbl,
  .clk_rate                       = MSM_SENSOR_MCLK_24HZ,
};

module_init(msm_sensor_init_module);
MODULE_DESCRIPTION("Samsung 8MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
