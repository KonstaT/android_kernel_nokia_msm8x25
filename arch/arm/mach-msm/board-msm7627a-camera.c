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
 */

#include <linux/i2c.h>
#include <linux/i2c/sx150x.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <asm/mach-types.h>
#include <mach/msm_iomap.h>
#include <mach/board.h>
#include <mach/irqs-7xxx.h>
#include "devices-msm7x2xa.h"
#include "board-msm7627a.h"
#include <mach/vreg.h>

#define GPIO_SKU1_CAM_VGA_SHDN    18
#define GPIO_SKU1_CAM_VGA_RESET_N 29
#define GPIO_SKU3_CAM_5MP_SHDN_N   5         /* PWDN */
#define GPIO_SKU3_CAM_5MP_CAMIF_RESET   6    /* (board_is(EVT))?123:121 RESET */
#define GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN 30

#define GPIO_SKUD_CAM_5MP_SHDN_N   23      /* PWDN */
#define GPIO_SKUD_CAM_5MP_CAMIF_RESET   79
#define GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN 16
#define GPIO_SKUD_CAM_1MP_PWDN 85
#define GPIO_SKUD_CAM_LED_EN 34
#define GPIO_SKUD_CAM_LED_FLASH_EN 48
#define GPIO_SKUD_CAM_5MP_CAMID 127 /*CAMID*/

 #ifndef	CONFIG_ODMM 
//static int camera_gpio_front_qrd7[] = {91, 29};//PWDN, RESET
#endif


#define GPIO_SKUE_CAM_5MP_SHDN_N 23
#define GPIO_SKUE_CAM_5MP_CAMIF_RESET   79
#define GPIO_SKUE_CAM_LED_EN 34
#define GPIO_SKUE_CAM_LED_FLASH_EN 48
#define GPIO_SKUE_CAM_PWR_EN 33


#define GPIO_SKU7_CAM_VGA_SHDN    91
#define GPIO_SKU7_CAM_5MP_SHDN_N   93         /* PWDN */
#define GPIO_SKU7_CAM_5MP_CAMIF_RESET   23   /* (board_is(EVT))?123:121 RESET */

#define GPIO_SKUE_CAM_VGA_SHDN    85

#define GPIO_NOT_CONFIGURED -1
#define MOUNT_ANGLE_NOT_CONFIGURED -1
//for qrd7 camera gpio
#ifdef CONFIG_OV5647
static int camera_gpio_rear_qrd7[] = {93, 23, 30};//PWDN, RESET, DRIVER_PWDN
#endif

#ifndef ODMM_PROJECT_STAGE_EVB1
#define GPIO_SENSOR_ID	29 //add by lwl for sensor id detected
static int camera_id_status = 0;

#endif
extern int sku3_lcdc_lcd_camera_power_onoff(int on);

#ifdef CONFIG_MSM_CAMERA_V4L2
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifdef CONFIG_OV5647_DALING_OVBD631AC
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
#endif
//lwl modify 
#ifdef CONFIG_S5K5CA
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
#endif

#ifdef CONFIG_OV5647_PARTRON_CM500
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
#endif
#ifdef CONFIG_OV5647_HQ_O9B5_DG806T
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
#endif
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
#ifdef CONFIG_OV5647_DALING_OVBD631AC//shao.wenqi@byd.com; for daling ov5647
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),	// gpio 98 is EBI2_CS2 , to control flash strobe pin
#endif

#ifdef CONFIG_OV5647_PARTRON_CM500//shao.wenqi@byd.com;add; for partrom module
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),	// gpio 98 is EBI2_CS2 , to control flash strobe pin
#endif

#ifdef CONFIG_OV5647_HQ_O9B5_DG806T//shao.wenqi@byd.com; add ; for daling moduel
    GPIO_CFG(35, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(40, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(5, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(6, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
    GPIO_CFG(13, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),	// gpio 98 is EBI2_CS2 , to control flash strobe pin
#endif
};

#ifdef CONFIG_S5K4E1 // #suwg
static struct gpio s5k4e1_cam_req_gpio[] = {
	{GPIO_CAM_GP_CAMIF_RESET_N, GPIOF_DIR_OUT, "CAM_RESET"},
};

static struct msm_gpio_set_tbl s5k4e1_cam_gpio_set_tbl[] = {
	{GPIO_CAM_GP_CAMIF_RESET_N, GPIOF_OUT_INIT_LOW, 1000},
	{GPIO_CAM_GP_CAMIF_RESET_N, GPIOF_OUT_INIT_HIGH, 4000},
};

static struct msm_camera_gpio_conf gpio_conf_s5k4e1 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_off_table_size = ARRAY_SIZE(camera_off_gpio_table),
	.camera_on_table = camera_on_gpio_table,
	.camera_on_table_size = ARRAY_SIZE(camera_on_gpio_table),
	.cam_gpio_req_tbl = s5k4e1_cam_req_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(s5k4e1_cam_req_gpio),
	.cam_gpio_set_tbl = s5k4e1_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(s5k4e1_cam_gpio_set_tbl),
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_MT9E013 // #suwg
static struct msm_camera_gpio_conf gpio_conf_mt9e013 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_WEBCAM_OV9726 // #suwg
static struct msm_camera_gpio_conf gpio_conf_ov9726 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_WEBCAM_OV7692_QRD
static struct msm_camera_gpio_conf gpio_conf_ov7692 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif


#ifdef CONFIG_OV7692
static struct gpio ov7692_cam_req_gpio[] = {
	{GPIO_SKU1_CAM_VGA_RESET_N, GPIOF_DIR_OUT, "CAM_VGA_RESET"},
};

static struct gpio ov7692_cam_req_gpio_sku7[] = {
	{GPIO_SKU7_CAM_VGA_SHDN, GPIOF_DIR_OUT, "CAM_VGA_SHDN"},
	{GPIO_SKU1_CAM_VGA_RESET_N, GPIOF_DIR_OUT, "CAM_VGA_RESET"},
};

static struct msm_gpio_set_tbl ov7692_cam_gpio_set_tbl[] = {
	{GPIO_SKU1_CAM_VGA_SHDN, GPIOF_OUT_INIT_HIGH, 5000},
	{GPIO_SKU1_CAM_VGA_SHDN, GPIOF_OUT_INIT_LOW, 5000},
	{GPIO_SKU1_CAM_VGA_RESET_N, GPIOF_OUT_INIT_HIGH, 5000},
	{GPIO_SKU1_CAM_VGA_RESET_N, GPIOF_OUT_INIT_LOW, 5000},
};

static struct msm_gpio_set_tbl ov7692_cam_gpio_set_tbl_sku7[] = {
	{GPIO_SKU7_CAM_VGA_SHDN, GPIOF_OUT_INIT_HIGH, 5000},
	{GPIO_SKU7_CAM_VGA_SHDN, GPIOF_OUT_INIT_LOW, 5000},
	{GPIO_SKU1_CAM_VGA_RESET_N, GPIOF_OUT_INIT_HIGH, 5000},
	{GPIO_SKU1_CAM_VGA_RESET_N, GPIOF_OUT_INIT_LOW, 5000},
};

static struct msm_camera_gpio_conf gpio_conf_ov7692 = {
	.cam_gpio_req_tbl = ov7692_cam_req_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(ov7692_cam_req_gpio),
	.cam_gpio_set_tbl = ov7692_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(ov7692_cam_gpio_set_tbl),
	.gpio_no_mux = 1,
};

static struct msm_camera_gpio_conf sku7_gpio_conf_ov7692 = {
	.cam_gpio_req_tbl = ov7692_cam_req_gpio_sku7,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(ov7692_cam_req_gpio_sku7),
	.cam_gpio_set_tbl = ov7692_cam_gpio_set_tbl_sku7,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(ov7692_cam_gpio_set_tbl_sku7),
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_OV5647
static struct msm_camera_gpio_conf gpio_conf_ov5647 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif
#ifdef CONFIG_OV5647_DALING_OVBD631AC
static struct msm_camera_gpio_conf gpio_conf_ov5647_dl_bd631ac = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif
#ifdef CONFIG_OV5647_PARTRON_CM500
static struct msm_camera_gpio_conf gpio_conf_ov5647_partron_cm500 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif
#ifdef CONFIG_OV5647_HQ_O9B5_DG806T
static struct msm_camera_gpio_conf gpio_conf_ov5647_hq_dg806t = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif
#ifdef CONFIG_OV5647_TRULY_CM6868
static struct msm_camera_gpio_conf gpio_conf_ov5647_truly_cm6868 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif

//lwl modify
#ifdef CONFIG_S5K5CA
static struct msm_camera_gpio_conf gpio_conf_s5k5ca = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_HI351_SUNNY_Q3H01B
static struct msm_camera_gpio_conf gpio_conf_hi351_sunny_q3h01b = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif


#ifdef CONFIG_AR0543
static struct msm_camera_gpio_conf gpio_conf_ar0543 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif
#ifdef CONFIG_S5K3H2_SUNNY_Q8S02E
static struct msm_camera_gpio_conf gpio_conf_s5k3h2_sunny_q8s02e = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_AR0542
static struct msm_camera_gpio_conf gpio_conf_ar0542 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif

#ifdef CONFIG_OV5648_TRULY_CM8352
static struct msm_camera_gpio_conf gpio_conf_ov5648_truly_cm8352 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif
#ifdef CONFIG_A8140
static struct msm_camera_gpio_conf gpio_conf_a8140 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};
#endif
#if 0 // #suwg
#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = GPIO_CAM_GP_LED_EN1,
	._fsrc.ext_driver_src.led_flash_en = GPIO_CAM_GP_LED_EN2,
};
#endif
#endif

static struct camera_vreg_t msm_cam_vreg[] = {
#ifdef ODMM_PROJECT_STAGE_EVB2
#ifdef CONFIG_S5K5CA
	{"wlan3", REG_LDO, 1500000, 1500000, 0},// lwl modify lco6
	{"wlan2", REG_LDO, 1800000, 1800000, 0},//lwl modify lco19
#endif
#ifdef CONFIG_HI351_SUNNY_Q3H01B
	{"wlan3", REG_LDO, 1200000, 1200000, 0},// lwl modify lco6
	{"wlan2", REG_LDO, 1800000, 1800000, 0},//lwl modify lco19
#endif
#else
	{"msme1", REG_LDO, 1800000, 1800000, 0},
	{"gp2", REG_LDO, 2850000, 2850000, 0},
	{"usb", REG_LDO, 1200000, 1200000, 0},// lwl modify lco6
	{"wlan2", REG_LDO, 1800000, 1800000, 0},//lwl modify lco19
#endif
};
#ifdef CONFIG_OV5647
static struct camera_vreg_t ov5647_gpio_vreg[] = {
	{"cam_ov5647_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ov5647_vdd", REG_GPIO, 0, 0, 0},
};
#endif
#ifdef CONFIG_OV5647_TRULY_CM6868
static struct camera_vreg_t ov5647_truly_cm6868_gpio_vreg[] = {
	{"cam_ov5647_truly_cm6868_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ov5647_truly_cm6868_vdd", REG_GPIO, 0, 0, 0},
};
#endif
#ifdef CONFIG_OV5648_TRULY_CM8352
static struct camera_vreg_t ov5648_truly_cm8352_gpio_vreg[] = {
	{"ldo12", REG_LDO, 2700000, 3300000, 0},
	{"smps3", REG_LDO, 1800000, 1800000, 0},
};

static struct camera_vreg_t ov5648_gpio_vreg_evbd[] = {
	{"ldo12", REG_LDO, 2700000, 3300000, 0},
	{"smps3", REG_LDO, 1800000, 1800000, 0},
	{"cam_ov5648_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ov5648_vdd", REG_GPIO, 0, 0, 0},
};
#endif

#ifdef CONFIG_OV8825
static struct camera_vreg_t ov8825_gpio_vreg[] = {
	{"cam_ov8825_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ov8825_vdd", REG_GPIO, 0, 0, 0},
};
static struct camera_vreg_t ov8825_gpio_vreg_evbd[] = {
	{"ldo12", REG_LDO, 2700000, 3300000, 0},
	{"smps3", REG_LDO, 1800000, 1800000, 0},
	{"cam_ov8825_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ov8825_vdd", REG_GPIO, 0, 0, 0},
};
#endif
#ifdef CONFIG_OV7692
static struct camera_vreg_t ov7692_gpio_vreg[] = {
	{"cam_ov7692_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ov7692_vdd", REG_GPIO, 0, 0, 0},
};
#endif

#ifdef CONFIG_AR0543
static struct camera_vreg_t ar0543_gpio_vreg[] = {
	{"cam_ar0543_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ar0543_vdd",  REG_GPIO, 0, 0, 0},
};
#endif
#ifdef CONFIG_A8140
static struct camera_vreg_t a8140_gpio_vreg[] = {
	{"cam_a8140_avdd", REG_GPIO, 0, 0, 0},
	{"cam_a8140_vdd",  REG_GPIO, 0, 0, 0},
};
#endif
#ifdef CONFIG_S5K3H2_SUNNY_Q8S02E
static struct camera_vreg_t s5k3h2_sunny_q8s02e_gpio_vreg[] = {
	{"cam_s5k3h2_sunny_q8s02e_avdd", REG_GPIO, 0, 0, 0},
	{"cam_s5k3h2_sunny_q8s02e_vdd",  REG_GPIO, 0, 0, 0},
};
#endif
#ifdef CONFIG_AR0542
static struct camera_vreg_t ar0542_gpio_vreg[] = {
//	{"cam_ar0542_avdd", REG_GPIO, 0, 0, 0},
//	{"cam_ar0542_vdd",  REG_GPIO, 0, 0, 0},
};
#endif

// #suwg:static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1_data;

struct msm_camera_device_platform_data msm_camera_device_data_csi1[] = {
	{
		.csid_core = 1,
		.ioclk = {
			.vfe_clk_rate = 192000000,
		},
	},
	{
		.csid_core = 1,
		.ioclk = {
			.vfe_clk_rate = 266667000,
		},
	},
};

struct msm_camera_device_platform_data msm_camera_device_data_csi0[] = {
	{
		.csid_core = 0,
		.ioclk = {
			.vfe_clk_rate = 192000000,
		},
	},
	{
		.csid_core = 0,
		.ioclk = {
			.vfe_clk_rate = 266667000,
		},
	},
};

static struct i2c_board_info msm_act_main_cam_i2c_info = {
	I2C_BOARD_INFO("msm_actuator", 0x11),
};

#if 0 // #suwg
static struct msm_actuator_info msm_act_main_cam_4_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_4,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_CAM_GP_CAM_PWDN,
	.vcm_enable     = 1,
};
#endif
#ifdef CONFIG_S5K4E1
static struct msm_camera_sensor_flash_data flash_s5k4e1 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k4e1 = {
	.mount_angle	= 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_s5k4e1,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1_data = {
	.sensor_name    = "s5k4e1",
	.sensor_reset_enable = 1,
	.pdata                  = &msm_camera_device_data_csi1[0],
	.flash_data             = &flash_s5k4e1,
	.sensor_platform_info   = &sensor_board_info_s5k4e1,
	.csi_if                 = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_4_info,
};
#endif



#ifdef CONFIG_OV7692
static struct msm_camera_sensor_platform_info sensor_board_info_ov7692 = {
	.mount_angle = 270,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov7692,
};

static struct msm_camera_sensor_flash_data flash_ov7692 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7692_data = {
	.sensor_name	    = "ov7692",
	.sensor_reset_enable    = 0,
	.sensor_reset	   = GPIO_NOT_CONFIGURED,
	.sensor_pwd	     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi0[0],
	.flash_data	     = &flash_ov7692,
	.sensor_platform_info   = &sensor_board_info_ov7692,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
#endif

#ifdef CONFIG_WEBCAM_OV7692_QRD

//[ start ] lixujie 20120601 modify it for eg808t front camera orientation
#ifdef	CONFIG_ODMM	
static struct msm_camera_sensor_platform_info sensor_board_info_ov7692 = {
	/* 	li.xujie@byd.com 20120813 modify it for EG808T front Camera  ov7692 
		preview/snap Orientation (EG808T_P004990) 	*/
	
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = 0,
	.gpio_conf = &gpio_conf_ov7692,
};
#else
static struct msm_camera_sensor_platform_info sensor_board_info_ov7692 = {
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov7692,
};
#endif 	
//[ end ] 

static struct msm_camera_sensor_flash_data flash_ov7692 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7692_data = {
	.sensor_name	    = "ov7692",
	.sensor_reset_enable    = 0,
	 
        #ifdef	CONFIG_ODMM  // [ start ] lixujie 20120511 modify it for EG808T front Camera 
	.pmic_gpio_enable  = 0,
	 #else
	.pmic_gpio_enable  = 1,
	 #endif 	// [ end ] 
	.sensor_reset	   = GPIO_SKU1_CAM_VGA_RESET_N,
	.sensor_pwd	     = GPIO_SKU1_CAM_VGA_SHDN,
	.pdata			= &msm_camera_device_data_csi0[0],
	.flash_data	     = &flash_ov7692,
	.sensor_platform_info   = &sensor_board_info_ov7692,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};

// [ start ] lixujie 20120511 add it for EG808T front Camera 

static void ov7692_camera_gpio_cfg(void)
{
	int32_t	rc = 0;
	int32_t	ov7692_pwrdwn_rst_pin ;

       printk(" %s: L(%d) start \n",__func__,__LINE__);
	   
	if(msm_camera_sensor_ov7692_data.sensor_reset_enable){

		ov7692_pwrdwn_rst_pin = msm_camera_sensor_ov7692_data.sensor_reset;
			
		rc = gpio_request(ov7692_pwrdwn_rst_pin, "ov7692");
		if (rc < 0){
			pr_err("%s: gpio_request---ov7692_rst_pin failed!", __func__);			
		}
		
		rc = gpio_tlmm_config(GPIO_CFG(ov7692_pwrdwn_rst_pin, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		
		if (rc < 0) {
			pr_err("%s:unable to enable ov7692_rst_pin gpio for frnt camera!\n", __func__);
			gpio_free(ov7692_pwrdwn_rst_pin);			
		}
	       gpio_direction_output(ov7692_pwrdwn_rst_pin, 1);
				
	}

       ov7692_pwrdwn_rst_pin = msm_camera_sensor_ov7692_data.sensor_pwd;
	rc = gpio_request(ov7692_pwrdwn_rst_pin, "ov7692");
	
	if (rc < 0){
		
		pr_err("%s: gpio_request---ov7692_pwdn_gpio failed!", __func__);
		
	}
	
	rc = gpio_tlmm_config(GPIO_CFG(ov7692_pwrdwn_rst_pin, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	
	if (rc < 0) {
		
		pr_err("%s:unable to enable Powr Dwn gpio for frnt camera!\n",__func__);
		gpio_free(ov7692_pwrdwn_rst_pin);
		
	}

	 gpio_direction_output(ov7692_pwrdwn_rst_pin, 1);

}

// [ end ]

#endif


#ifdef CONFIG_OV7695
static struct gpio ov7695_cam_req_gpio_skud[] = {
	{GPIO_SKUD_CAM_1MP_PWDN, GPIOF_DIR_OUT, "CAM_VGA_SHDN"},
};

static struct msm_gpio_set_tbl ov7695_cam_gpio_set_tbl_skud[] = {
	{GPIO_SKUD_CAM_1MP_PWDN, GPIOF_OUT_INIT_LOW, 5000},
	{GPIO_SKUD_CAM_1MP_PWDN, GPIOF_OUT_INIT_HIGH, 5000},
};
static struct msm_camera_gpio_conf skud_gpio_conf_ov7695 = {
	.cam_gpio_req_tbl = ov7695_cam_req_gpio_skud,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(ov7695_cam_req_gpio_skud),
	.cam_gpio_set_tbl = ov7695_cam_gpio_set_tbl_skud,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(ov7695_cam_gpio_set_tbl_skud),
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

static struct camera_vreg_t ov7695_gpio_vreg[] = {
	{"ldo12", REG_LDO, 2700000, 3300000, 0},
	{"smps3", REG_LDO, 1800000, 1800000, 0},
};

static struct camera_vreg_t ov7695_gpio_vreg_evbd[] = {
	{"ldo12", REG_LDO, 2700000, 3300000, 0},
	{"smps3", REG_LDO, 1800000, 1800000, 0},
	{"cam_ov7695_avdd", REG_GPIO, 0, 0, 0},
	{"cam_ov7695_vdd", REG_GPIO, 0, 0, 0},
};
static struct msm_camera_sensor_platform_info sensor_board_info_ov7695 = {
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &skud_gpio_conf_ov7695,
};

static struct msm_camera_sensor_flash_data flash_ov7695 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7695_data = {
	.sensor_name	    = "ov7695",
	.sensor_reset_enable    = 0,
	.sensor_reset	   = GPIO_NOT_CONFIGURED,
	.sensor_pwd	     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi0[0],
	.flash_data	     = &flash_ov7695,
	.sensor_platform_info   = &sensor_board_info_ov7695,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
#endif

#ifdef CONFIG_OV7695_RAW
static struct msm_camera_gpio_conf skud_gpio_conf_ov7695_raw = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

static struct camera_vreg_t ov7695_raw_gpio_vreg[] = {
	{"ldo12", REG_LDO, 2700000, 3300000, 0},
	{"smps3", REG_LDO, 1800000, 1800000, 0},
};
static struct msm_camera_sensor_platform_info sensor_board_info_ov7695_raw = {
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &skud_gpio_conf_ov7695_raw,
};

static struct msm_camera_sensor_flash_data flash_ov7695_raw = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7695_raw_data = {
	.sensor_name	    = "ov7695_raw",
	.sensor_reset_enable    = 0,
	.sensor_reset	   = GPIO_NOT_CONFIGURED,
	.sensor_pwd	     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi1[0],//&msm_camera_device_data_csi0[0],
	.flash_data	     = &flash_ov7695_raw,
	.sensor_platform_info   = &sensor_board_info_ov7695_raw,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};
#endif

#ifdef CONFIG_MT9V115
static struct gpio mt9v115_cam_req_gpio[] = {
	{GPIO_SKUE_CAM_VGA_SHDN, GPIOF_DIR_OUT, "CAM_VGA_SHDN"},
};

static struct msm_gpio_set_tbl mt9v115_cam_gpio_set_tbl[] = {
	{GPIO_SKUE_CAM_VGA_SHDN, GPIOF_OUT_INIT_HIGH, 5000},
	{GPIO_SKUE_CAM_VGA_SHDN, GPIOF_OUT_INIT_LOW, 10000},
};
static struct msm_camera_gpio_conf gpio_conf_mt9v115 = {
	.cam_gpio_req_tbl = mt9v115_cam_req_gpio,
	.cam_gpio_req_tbl_size = ARRAY_SIZE(mt9v115_cam_req_gpio),
	.cam_gpio_set_tbl = mt9v115_cam_gpio_set_tbl,
	.cam_gpio_set_tbl_size = ARRAY_SIZE(mt9v115_cam_gpio_set_tbl),
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

static struct camera_vreg_t mt9v115_gpio_vreg[] = {
	{"ldo12", REG_LDO, 2700000, 3300000, 0},
	{"smps3", REG_LDO, 1800000, 1800000, 0},
};
static struct msm_camera_sensor_platform_info sensor_board_info_mt9v115 = {
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_mt9v115,
};

static struct msm_camera_sensor_flash_data flash_mt9v115 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9v115_data = {
	.sensor_name	    = "mt9v115",
	.sensor_reset_enable    = 0,
	.sensor_reset	   = GPIO_NOT_CONFIGURED,
	.sensor_pwd	     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi0[0],
	.flash_data	     = &flash_mt9v115,
	.sensor_platform_info   = &sensor_board_info_mt9v115,
	.csi_if		 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
#endif

#ifdef CONFIG_OV5647

static struct msm_actuator_info msm_act_main_cam_5_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_5,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_NOT_CONFIGURED,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov5647 = {
	.mount_angle = MOUNT_ANGLE_NOT_CONFIGURED,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov5647,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5647 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED1,
	._fsrc.ext_driver_src.led_en = 13,
	._fsrc.ext_driver_src.led_flash_en = 32,
};

static struct msm_camera_sensor_flash_data flash_ov5647 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_ov5647,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_data = {
	.sensor_name    = "ov5647",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = GPIO_NOT_CONFIGURED,
	.sensor_pwd     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_ov5647,
	.sensor_platform_info   = &sensor_board_info_ov5647,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_5_info,
};
#endif
//lwl modify
#ifdef CONFIG_S5K5CA
static struct msm_camera_sensor_platform_info sensor_board_info_s5k5ca = {
	.mount_angle = 90,//270,//90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_s5k5ca,
};

#if 0
static struct msm_camera_sensor_flash_src msm_flash_src_s5k5ca = {
	#if 0
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED,
	._fsrc.led_src.led_name = "flashlight",
	._fsrc.led_src.led_name_len = 10,
	#endif
	//.flash_sr_type = ODMM_CAMERA_FLASH_SRC_LED,
	 ._fsrc.led_src.led_name = "ktd265",
     ._fsrc.ext_driver_src = {
     	//.odmm_led_name = "ktd265",
        .led_en = 98,
        .led_flash_en = 98,
    }
};
#endif

static struct msm_camera_sensor_flash_data flash_s5k5ca = {
	.flash_type     =MSM_CAMERA_FLASH_NONE,// MSM_CAMERA_FLASH_LED,
	//.flash_src      = &msm_flash_src_s5k5ca,
};

 
static struct msm_camera_sensor_info msm_camera_sensor_s5k5ca_data = {
	.sensor_name    = "s5k5ca",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = 5,//GPIO_NOT_CONFIGURED,
	.sensor_pwd     = 6,//GPIO_NOT_CONFIGURED,
	.pdata          = &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_s5k5ca,
	.sensor_platform_info   = &sensor_board_info_s5k5ca,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
 #endif

 #ifdef CONFIG_HI351_SUNNY_Q3H01B
static struct msm_camera_sensor_platform_info sensor_board_info_hi351_sunny_q3h01b = {
	.mount_angle = 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_hi351_sunny_q3h01b,
};
#if 0
static struct msm_camera_sensor_flash_src msm_flash_src_hi351_sunny_q3h01b = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED,
	._fsrc.led_src.led_name = "flashlight",
	._fsrc.led_src.led_name_len = 10,
};
#endif
static struct msm_camera_sensor_flash_data flash_hi351_sunny_q3h01b = {
	.flash_type             = MSM_CAMERA_FLASH_NONE,//MSM_CAMERA_FLASH_LED,
	//.flash_src              = &msm_flash_src_hi351_sunny_q3h01b,
};

static struct msm_camera_sensor_info msm_camera_sensor_hi351_sunny_q3h01b_data = {
	.sensor_name    = "hi351",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = 5,//GPIO_NOT_CONFIGURED,
	.sensor_pwd     = 6,//GPIO_NOT_CONFIGURED,
	.pdata          = &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_hi351_sunny_q3h01b,
	.sensor_platform_info   = &sensor_board_info_hi351_sunny_q3h01b,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = YUV_SENSOR,
};
#endif

#ifdef CONFIG_OV5647_TRULY_CM6868
static struct msm_actuator_info msm_act_main_cam_6_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_6,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_NOT_CONFIGURED,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov5647_truly_cm6868 = {
	.mount_angle = MOUNT_ANGLE_NOT_CONFIGURED,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov5647_truly_cm6868,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5647_truly_cm6868 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED1,
	._fsrc.ext_driver_src.led_en = 13,
	._fsrc.ext_driver_src.led_flash_en = 32,
};

static struct msm_camera_sensor_flash_data flash_ov5647_truly_cm6868 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_ov5647_truly_cm6868,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_truly_cm6868_data = {
	.sensor_name    = "ov5647_truly_cm6868",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = GPIO_NOT_CONFIGURED,
	.sensor_pwd     = GPIO_NOT_CONFIGURED,
	.pdata          = &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_ov5647_truly_cm6868,
	.sensor_platform_info   = &sensor_board_info_ov5647_truly_cm6868,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_6_info,
};
#endif

#ifdef CONFIG_OV5648_TRULY_CM8352
static struct msm_actuator_info msm_act_main_cam_7_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_7,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_NOT_CONFIGURED,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov5648_truly_cm8352 = {
	.mount_angle = MOUNT_ANGLE_NOT_CONFIGURED,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov5648_truly_cm8352,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5648_truly_cm8352 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED1,
	._fsrc.ext_driver_src.led_en =  GPIO_SKUD_CAM_LED_EN,
	._fsrc.ext_driver_src.led_flash_en = GPIO_SKUD_CAM_LED_FLASH_EN,
};

static struct msm_camera_sensor_flash_data flash_ov5648_truly_cm8352 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_ov5648_truly_cm8352,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5648_truly_cm8352_data = {
	.sensor_name    = "ov5648_truly_cm8352",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = GPIO_NOT_CONFIGURED,
	.sensor_pwd     = GPIO_NOT_CONFIGURED,
	.pdata          = &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_ov5648_truly_cm8352,
	.sensor_platform_info   = &sensor_board_info_ov5648_truly_cm8352,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_7_info,
};
#endif


#ifdef CONFIG_OV8825
static struct msm_camera_gpio_conf gpio_conf_ov8825 = {
	.camera_off_table = camera_off_gpio_table,
	.camera_on_table = camera_on_gpio_table,
	.gpio_no_mux = 1,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov8825 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED1,
	._fsrc.ext_driver_src.led_en = 13,
	._fsrc.ext_driver_src.led_flash_en = 32,
};

static struct msm_camera_sensor_flash_data flash_ov8825 = {
	.flash_type     = MSM_CAMERA_FLASH_LED,
	.flash_src      = &msm_flash_src_ov8825,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov8825 = {
	.mount_angle  = MOUNT_ANGLE_NOT_CONFIGURED,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov8825,
};

static struct msm_actuator_info msm_act_main_cam_3_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = MSM_ACTUATOR_MAIN_CAM_3,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_NOT_CONFIGURED,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov8825_data = {
	.sensor_name    = "ov8825",
	.sensor_reset_enable    = 1,
	.pmic_gpio_enable = 1,
	.sensor_reset           = GPIO_NOT_CONFIGURED,
	.sensor_pwd     = GPIO_NOT_CONFIGURED,
	.pdata  = &msm_camera_device_data_csi1[1],
	.flash_data     = &flash_ov8825,
	.sensor_platform_info = &sensor_board_info_ov8825,
	.csi_if = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &msm_act_main_cam_3_info,
};
#endif


#ifdef CONFIG_OV5647_DALING_OVBD631AC//#ifndef CONFIG_ODMM comment by zy 20120712

static struct msm_actuator_info odmm_msm_act_main_cam_dl_bd631ac_5_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = ODMM_MSM_ACTUATOR_MAIN_CAM_DALING_OVBD631AC_5,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_NOT_CONFIGURED,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov5647_dl_bd631ac = {
	.mount_angle = 270, /*mod by zy 20120712 adjust angle*/
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov5647_dl_bd631ac,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5647_dl_bd631ac = {
	 //.flash_sr_type = ODMM_CAMERA_FLASH_SRC_LED,
	 ._fsrc.led_src.led_name = "ktd265",
     ._fsrc.ext_driver_src = {
     	//.odmm_led_name = "ktd265",
        .led_en = 98,
        .led_flash_en = 98,
    }
};

static struct msm_camera_sensor_flash_data flash_ov5647_dl_bd631ac = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_ov5647_dl_bd631ac,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_dl_bd631ac_data = {
	.sensor_name    = "ov5647_dl_bd631ac",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = GPIO_NOT_CONFIGURED,
	.sensor_pwd     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_ov5647_dl_bd631ac,
	.sensor_platform_info   = &sensor_board_info_ov5647_dl_bd631ac,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &odmm_msm_act_main_cam_dl_bd631ac_5_info,
};

#endif
#ifdef CONFIG_OV5647_PARTRON_CM500

static struct msm_actuator_info odmm_msm_act_main_cam_partron_cm500_5_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name   = ODMM_MSM_ACTUATOR_MAIN_CAM_PARTRON_CM500_5,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_NOT_CONFIGURED,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov5647_partron_cm500 = {
	.mount_angle = 270, /*mod by zy 20120712 adjust angle*/
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov5647_partron_cm500,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5647_partron_cm500 = {
	 //.flash_sr_type = ODMM_CAMERA_FLASH_SRC_LED,
	 ._fsrc.led_src.led_name = "ktd265",
     ._fsrc.ext_driver_src = {
     	//.odmm_led_name = "ktd265",
        .led_en = 98,
        .led_flash_en = 98,
    }
};

static struct msm_camera_sensor_flash_data flash_ov5647_partron_cm500 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_ov5647_partron_cm500,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_partron_cm500_data = {
	.sensor_name    = "ov5647_partr_cm500",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = GPIO_NOT_CONFIGURED,
	.sensor_pwd     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_ov5647_partron_cm500,
	.sensor_platform_info   = &sensor_board_info_ov5647_partron_cm500,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &odmm_msm_act_main_cam_partron_cm500_5_info,
};

#endif

#ifdef CONFIG_OV5647_HQ_O9B5_DG806T

static struct msm_actuator_info odmm_msm_act_main_cam_hq_dg806t_5_info = {
	.board_info     = &msm_act_main_cam_i2c_info,
	.cam_name       = ODMM_MSM_ACTUATOR_MAIN_CAM_HQ_DG806T_5,
	.bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
	.vcm_pwd        = GPIO_NOT_CONFIGURED,
	.vcm_enable     = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov5647_hq_dg806t = {
	.mount_angle = 270, /*mod by zy 20120712 adjust angle*/
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov5647_hq_dg806t,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5647_hq_dg806t = {
	 //.flash_sr_type = ODMM_CAMERA_FLASH_SRC_LED,
	 ._fsrc.led_src.led_name = "ktd265",
     ._fsrc.ext_driver_src = {
     	//.odmm_led_name = "ktd265",
        .led_en = 98,
        .led_flash_en = 98,
    }
};

static struct msm_camera_sensor_flash_data flash_ov5647_hq_dg806t = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_ov5647_hq_dg806t,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_hq_dg806t_data = {
	.sensor_name    = "ov5647_hq_dg806t",
	.sensor_reset_enable = 1,
	.pmic_gpio_enable  = 1,
	.sensor_reset   = GPIO_NOT_CONFIGURED,
	.sensor_pwd     = GPIO_NOT_CONFIGURED,
	.pdata			= &msm_camera_device_data_csi1[0],
	.flash_data     = &flash_ov5647_hq_dg806t,
	.sensor_platform_info   = &sensor_board_info_ov5647_hq_dg806t,
	.csi_if                 = 1,
	.camera_type	= BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
	.actuator_info = &odmm_msm_act_main_cam_hq_dg806t_5_info,
};

#endif
#ifdef CONFIG_AR0543
static struct msm_camera_sensor_platform_info sensor_board_info_ar0543 = {
  .mount_angle            = MOUNT_ANGLE_NOT_CONFIGURED,
  .cam_vreg               = msm_cam_vreg,
  .num_vreg               = ARRAY_SIZE(msm_cam_vreg),
  .gpio_conf              = &gpio_conf_ar0543,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ar0543 = {
  .flash_sr_type                     = MSM_CAMERA_FLASH_SRC_LED1,
  ._fsrc.ext_driver_src.led_en       = 13,
  ._fsrc.ext_driver_src.led_flash_en = 32,
};

static struct msm_camera_sensor_flash_data flash_ar0543 = {
  .flash_type             = MSM_CAMERA_FLASH_LED,
  .flash_src              = &msm_flash_src_ar0543,
};

static struct msm_camera_sensor_info msm_camera_sensor_ar0543_data = {
  .sensor_name            = "ar0543",
  .sensor_reset_enable    = 1,
  .pmic_gpio_enable       = 1,
  .sensor_reset           = GPIO_NOT_CONFIGURED,
  .sensor_pwd             = GPIO_NOT_CONFIGURED,
  .pdata                  = &msm_camera_device_data_csi1[0],
  .flash_data             = &flash_ar0543,
  .sensor_platform_info   = &sensor_board_info_ar0543,
  .csi_if                 = 1,
  .camera_type            = BACK_CAMERA_2D,
  .sensor_type            = BAYER_SENSOR,
};
#endif

#ifdef CONFIG_S5K3H2_SUNNY_Q8S02E
static struct msm_actuator_info msm_act_main_cam_9_info = {
  .board_info             = &msm_act_main_cam_i2c_info,
  .cam_name               = MSM_ACTUATOR_MAIN_CAM_9,
  .bus_id                 = MSM_GSBI0_QUP_I2C_BUS_ID,
  .vcm_pwd                = GPIO_NOT_CONFIGURED,
  .vcm_enable             = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_s5k3h2_sunny_q8s02e = {
  .mount_angle            = MOUNT_ANGLE_NOT_CONFIGURED,
  .cam_vreg               = msm_cam_vreg,
  .num_vreg               = ARRAY_SIZE(msm_cam_vreg),
  .gpio_conf              = &gpio_conf_s5k3h2_sunny_q8s02e,
};

static struct msm_camera_sensor_flash_src msm_flash_src_s5k3h2_sunny_q8s02e = {
  .flash_sr_type                     = MSM_CAMERA_FLASH_SRC_LED1,
  ._fsrc.ext_driver_src.led_en       = 13,
  ._fsrc.ext_driver_src.led_flash_en = 32,
};

static struct msm_camera_sensor_flash_data flash_s5k3h2_sunny_q8s02e = {
  .flash_type             = MSM_CAMERA_FLASH_LED,
  .flash_src              = &msm_flash_src_s5k3h2_sunny_q8s02e,
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k3h2_sunny_q8s02e_data = {
  .sensor_name            = "s5k3h2_sunny_q8s02e",
  .sensor_reset_enable    = 1,
  .pmic_gpio_enable       = 1,
  .sensor_reset           = GPIO_NOT_CONFIGURED,
  .sensor_pwd             = GPIO_NOT_CONFIGURED,
  .pdata                  = &msm_camera_device_data_csi1[0],
  .flash_data             = &flash_s5k3h2_sunny_q8s02e,
  .sensor_platform_info   = &sensor_board_info_s5k3h2_sunny_q8s02e,
  .csi_if                 = 1,
  .camera_type            = BACK_CAMERA_2D,
  .sensor_type            = BAYER_SENSOR,
  .actuator_info          = &msm_act_main_cam_9_info,
};
#endif
#ifdef CONFIG_AR0542
static struct msm_camera_sensor_platform_info sensor_board_info_ar0542 = {
  .mount_angle            = MOUNT_ANGLE_NOT_CONFIGURED,
  .cam_vreg               = msm_cam_vreg,
  .num_vreg               = ARRAY_SIZE(msm_cam_vreg),
  .gpio_conf              = &gpio_conf_ar0542,
};

static struct msm_camera_sensor_flash_src msm_flash_src_ar0542 = {
  .flash_sr_type                     = MSM_CAMERA_FLASH_SRC_LED1,
  ._fsrc.ext_driver_src.led_en       = 13,
  ._fsrc.ext_driver_src.led_flash_en = 32,
};

static struct msm_camera_sensor_flash_data flash_ar0542 = {
  .flash_type             = MSM_CAMERA_FLASH_LED,
  .flash_src              = &msm_flash_src_ar0542,
};

static struct msm_actuator_info msm_act_main_cam_10_info = {
        .board_info     = &msm_act_main_cam_i2c_info,
        .cam_name   = MSM_ACTUATOR_MAIN_CAM_10,
        .bus_id         = MSM_GSBI0_QUP_I2C_BUS_ID,
        .vcm_pwd        = GPIO_NOT_CONFIGURED,
        .vcm_enable     = 0,
};

static struct msm_camera_sensor_info msm_camera_sensor_ar0542_data = {
  .sensor_name            = "ar0542",
  .sensor_reset_enable    = 1,
  .pmic_gpio_enable       = 1,
  .sensor_reset           = GPIO_NOT_CONFIGURED,
  .sensor_pwd             = GPIO_NOT_CONFIGURED,
  .pdata                  = &msm_camera_device_data_csi1[0],
  .flash_data             = &flash_ar0542,
  .sensor_platform_info   = &sensor_board_info_ar0542,
  .csi_if                 = 1,
  .camera_type            = BACK_CAMERA_2D,
  .sensor_type            = BAYER_SENSOR,
  .actuator_info = &msm_act_main_cam_10_info,
};
#endif

#ifdef CONFIG_MT9E013
static struct msm_camera_sensor_flash_data flash_mt9e013 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src
};

static struct msm_camera_sensor_platform_info sensor_board_info_mt9e013 = {
	.mount_angle	= 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_mt9e013,
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9e013_data = {
	.sensor_name    = "mt9e013",
	.sensor_reset_enable = 1,
	.pdata                  = &msm_camera_device_data_csi1[1],
	.flash_data             = &flash_mt9e013,
	.sensor_platform_info   = &sensor_board_info_mt9e013,
	.csi_if                 = 1,
	.camera_type = BACK_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};
#endif
#ifdef CONFIG_A8140
static struct msm_actuator_info msm_act_main_cam_8_info = {
  .board_info             = &msm_act_main_cam_i2c_info,
  .cam_name               = MSM_ACTUATOR_MAIN_CAM_8,
  .bus_id                 = MSM_GSBI0_QUP_I2C_BUS_ID,
  .vcm_pwd                = GPIO_NOT_CONFIGURED,
  .vcm_enable             = 0,
};

static struct msm_camera_sensor_platform_info sensor_board_info_a8140 = {
  .mount_angle            = MOUNT_ANGLE_NOT_CONFIGURED,
  .cam_vreg               = msm_cam_vreg,
  .num_vreg               = ARRAY_SIZE(msm_cam_vreg),
  .gpio_conf              = &gpio_conf_a8140,
};

static struct msm_camera_sensor_flash_src msm_flash_src_a8140 = {
  .flash_sr_type                     = MSM_CAMERA_FLASH_SRC_LED1,
  ._fsrc.ext_driver_src.led_en       = 13,
  ._fsrc.ext_driver_src.led_flash_en = 32,
};

static struct msm_camera_sensor_flash_data flash_a8140 = {
  .flash_type             = MSM_CAMERA_FLASH_LED,
  .flash_src              = &msm_flash_src_a8140,
};

static struct msm_camera_sensor_info msm_camera_sensor_a8140_data = {
  .sensor_name            = "a8140",
  .sensor_reset_enable    = 1,
  .pmic_gpio_enable       = 1,
  .sensor_reset           = GPIO_NOT_CONFIGURED,
  .sensor_pwd             = GPIO_NOT_CONFIGURED,
  .pdata                  = &msm_camera_device_data_csi1[0],
  .flash_data             = &flash_a8140,
  .sensor_platform_info   = &sensor_board_info_a8140,
  .csi_if                 = 1,
  .camera_type            = BACK_CAMERA_2D,
  .sensor_type            = BAYER_SENSOR,
  .actuator_info          = &msm_act_main_cam_8_info,
};
#endif
#ifdef CONFIG_WEBCAM_OV9726
static struct msm_camera_sensor_flash_data flash_ov9726 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src
};

static struct msm_camera_sensor_platform_info sensor_board_info_ov9726 = {
	.mount_angle	= 90,
	.cam_vreg = msm_cam_vreg,
	.num_vreg = ARRAY_SIZE(msm_cam_vreg),
	.gpio_conf = &gpio_conf_ov9726,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov9726_data = {
	.sensor_name    = "ov9726",
	.sensor_reset_enable = 0,
	.pdata                  = &msm_camera_device_data_csi0[0],
	.flash_data             = &flash_ov9726,
	.sensor_platform_info   = &sensor_board_info_ov9726,
	.csi_if                 = 1,
	.camera_type = FRONT_CAMERA_2D,
	.sensor_type = BAYER_SENSOR,
};
#endif

static struct platform_device msm_camera_server = {
	.name = "msm_cam_server",
	.id = 0,
};

static void __init msm7x27a_init_cam(void)
{

	if (!(machine_is_msm7x27a_ffa() ||
			machine_is_msm7625a_ffa() ||
			machine_is_msm7627a_qrd1() ||
			machine_is_msm8625_qrd7() ||
			machine_is_msm7627a_qrd3() ||
			machine_is_msm8625_evb() ||
			machine_is_msm8625_qrd5() ||
			machine_is_msm7x27a_qrd5a() ||
			machine_is_msm8625q_evbd() ||
			machine_is_msm8625q_skud() ||
			machine_is_msm8625q_skue() ||
			machine_is_msm8625_ffa())){
#ifdef CONFIG_S5K4E1
		sensor_board_info_s5k4e1.cam_vreg = NULL;
		sensor_board_info_s5k4e1.num_vreg = 0;
#endif
#ifdef CONFIG_MT9E013
		sensor_board_info_mt9e013.cam_vreg = NULL;
		sensor_board_info_mt9e013.num_vreg = 0;
#endif
#ifdef CONFIG_WEBCAM_OV9726
		sensor_board_info_ov9726.cam_vreg = NULL;
		sensor_board_info_ov9726.num_vreg = 0;
#endif
//#ifdef CONFIG_OV7692
#ifdef CONFIG_WEBCAM_OV7692_QRD
		sensor_board_info_ov7692.cam_vreg = NULL;
		sensor_board_info_ov7692.num_vreg = 0;
#endif
#ifdef CONFIG_OV7695
		sensor_board_info_ov7695.cam_vreg = NULL;
		sensor_board_info_ov7695.num_vreg = 0;
#endif
#ifdef CONFIG_MT9V115
		sensor_board_info_mt9v115.cam_vreg = NULL;
		sensor_board_info_mt9v115.num_vreg = 0;
#endif
#ifdef CONFIG_OV7695_RAW
		sensor_board_info_ov7695_raw.cam_vreg = NULL;
		sensor_board_info_ov7695_raw.num_vreg = 0;
#endif
#ifdef CONFIG_OV5647
		sensor_board_info_ov5647.cam_vreg = NULL;
		sensor_board_info_ov5647.num_vreg = 0;
#endif
// lwl modify
#ifdef CONFIG_S5K5CA
		sensor_board_info_s5k5ca.cam_vreg = NULL;
		sensor_board_info_s5k5ca.num_vreg = 0;
#endif
#ifdef CONFIG_HI351_SUNNY_Q3H01B
		sensor_board_info_hi351_sunny_q3h01b.cam_vreg = NULL;
		sensor_board_info_hi351_sunny_q3h01b.num_vreg = 0;
#endif
#ifdef CONFIG_OV8825
		sensor_board_info_ov8825.cam_vreg = NULL;
		sensor_board_info_ov8825.num_vreg = 0;
#endif
#ifdef CONFIG_OV5647_DALING_OVBD631AC
		sensor_board_info_ov5647_dl_bd631ac.cam_vreg = NULL;
		sensor_board_info_ov5647_dl_bd631ac.num_vreg = 0;
#endif
#ifdef CONFIG_OV5647_PARTRON_CM500
		sensor_board_info_ov5647_partron_cm500.cam_vreg = NULL;
		sensor_board_info_ov5647_partron_cm500.num_vreg = 0;
#endif
#ifdef CONFIG_OV5647_HQ_O9B5_DG806T
		sensor_board_info_ov5647_hq_dg806t.cam_vreg = NULL;
		sensor_board_info_ov5647_hq_dg806t.num_vreg = 0;
#endif
	}else if (machine_is_msm8625_qrd7() || machine_is_msm7627a_qrd3())
	{
		//Add SKU7 specific settings
#ifdef CONFIG_OV5647
		msm_camera_sensor_ov5647_data.sensor_reset = camera_gpio_rear_qrd7[1];
		msm_camera_sensor_ov5647_data.sensor_pwd = camera_gpio_rear_qrd7[0];
		msm_camera_sensor_ov5647_data.vcm_pwd = 0;
		msm_camera_sensor_ov5647_data.vcm_enable = 0;
		msm_flash_src_ov5647.flash_sr_type = MSM_CAMERA_FLASH_LED;
		msm_flash_src_ov5647._fsrc.ext_driver_src.led_en = 96;
		msm_camera_sensor_ov5647_data.sensor_platform_info->ext_power_ctrl = sku3_lcdc_lcd_camera_power_onoff;
		sensor_board_info_ov5647.mount_angle = 90;
#endif
#ifdef CONFIG_OV7692
		msm_camera_sensor_ov7692_data.sensor_platform_info->ext_power_ctrl = sku3_lcdc_lcd_camera_power_onoff;
		msm_camera_sensor_ov7692_data.vcm_pwd = 0;
		msm_camera_sensor_ov7692_data.vcm_enable = 0;
		sensor_board_info_ov7692.gpio_conf = &sku7_gpio_conf_ov7692;
		sensor_board_info_ov7692.mount_angle = 90;
#endif
#ifdef CONFIG_WEBCAM_OV7692_QRD

      // [ start ] lixujie 20120511 modify it for EG808T front Camera 
        #ifndef	CONFIG_ODMM  
		msm_camera_sensor_ov7692_data.sensor_reset = camera_gpio_front_qrd7[1];
		msm_camera_sensor_ov7692_data.sensor_pwd = camera_gpio_front_qrd7[0];
	#endif	
#endif

	}else if (machine_is_msm8625_evb())
	{
#ifdef CONFIG_OV7692
		sensor_board_info_ov7692.cam_vreg = ov7692_gpio_vreg;
		sensor_board_info_ov7692.num_vreg = ARRAY_SIZE(ov7692_gpio_vreg);
#endif
#ifdef CONFIG_WEBCAM_OV7692_QRD

      // [ start ] lixujie 20120511 modify it for EG808T front Camera 
        #ifndef	CONFIG_ODMM  
		sensor_board_info_ov7692.mount_angle = 270;
		msm_camera_sensor_ov7692_data.sensor_reset = GPIO_SKU1_CAM_VGA_RESET_N;
		msm_camera_sensor_ov7692_data.sensor_pwd = GPIO_SKU1_CAM_VGA_SHDN;
	#endif
#endif

#ifdef CONFIG_OV5647
		sensor_board_info_ov5647.cam_vreg = ov5647_gpio_vreg;
		sensor_board_info_ov5647.num_vreg = ARRAY_SIZE(ov5647_gpio_vreg);
		msm_act_main_cam_5_info.vcm_pwd = GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN;
		msm_act_main_cam_5_info.vcm_enable = 1;
		msm_camera_sensor_ov5647_data.sensor_reset=GPIO_SKU3_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_ov5647_data.sensor_pwd = GPIO_SKU3_CAM_5MP_SHDN_N;
		sensor_board_info_ov5647.mount_angle = 90;
#endif
#ifdef CONFIG_OV8825
		sensor_board_info_ov8825.cam_vreg = ov8825_gpio_vreg;
		sensor_board_info_ov8825.num_vreg = ARRAY_SIZE(ov8825_gpio_vreg);
		msm_act_main_cam_3_info.vcm_pwd = GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN;
		msm_act_main_cam_3_info.vcm_enable = 0;
		msm_camera_sensor_ov8825_data.sensor_reset = GPIO_SKU3_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_ov8825_data.sensor_pwd = GPIO_SKU3_CAM_5MP_SHDN_N;
		sensor_board_info_ov8825.mount_angle = 90;
#endif
	}
	else if(machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a())
	{
#ifdef CONFIG_OV5647_TRULY_CM6868
		sensor_board_info_ov5647_truly_cm6868.cam_vreg = ov5647_truly_cm6868_gpio_vreg;
		sensor_board_info_ov5647_truly_cm6868.num_vreg = ARRAY_SIZE(ov5647_truly_cm6868_gpio_vreg);
		msm_act_main_cam_6_info.vcm_pwd = GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN;
		msm_act_main_cam_6_info.vcm_enable = 1;
		msm_camera_sensor_ov5647_truly_cm6868_data.sensor_reset=GPIO_SKU3_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_ov5647_truly_cm6868_data.sensor_pwd = GPIO_SKU3_CAM_5MP_SHDN_N;
		sensor_board_info_ov5647_truly_cm6868.mount_angle = 90;
#endif
#ifdef CONFIG_OV7692
		sensor_board_info_ov7692.cam_vreg = ov7692_gpio_vreg;
		sensor_board_info_ov7692.num_vreg = ARRAY_SIZE(ov7692_gpio_vreg);
#endif
#ifdef CONFIG_WEBCAM_OV7692_QRD

      // [ start ] lixujie 20120511 modify it for EG808T front Camera 
        #ifndef	CONFIG_ODMM  
		sensor_board_info_ov7692.mount_angle = 90;
		msm_camera_sensor_ov7692_data.sensor_reset = GPIO_SKU1_CAM_VGA_RESET_N;
		msm_camera_sensor_ov7692_data.sensor_pwd = GPIO_SKU1_CAM_VGA_SHDN;
		#endif
#endif
#ifdef CONFIG_OV8825
		sensor_board_info_ov8825.cam_vreg = ov8825_gpio_vreg;
		sensor_board_info_ov8825.num_vreg = ARRAY_SIZE(ov8825_gpio_vreg);
		msm_act_main_cam_3_info.vcm_pwd = GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN;
		msm_act_main_cam_3_info.vcm_enable = 0;
		msm_camera_sensor_ov8825_data.sensor_reset = GPIO_SKU3_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_ov8825_data.sensor_pwd = GPIO_SKU3_CAM_5MP_SHDN_N;
		sensor_board_info_ov8825.mount_angle = 90;
#endif
#ifdef CONFIG_AR0543
		sensor_board_info_ar0543.cam_vreg = ar0543_gpio_vreg;
		sensor_board_info_ar0543.num_vreg = ARRAY_SIZE(ar0543_gpio_vreg);
		msm_camera_sensor_ar0543_data.sensor_reset=GPIO_SKU3_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_ar0543_data.sensor_pwd = GPIO_SKU3_CAM_5MP_SHDN_N;
		sensor_board_info_ar0543.mount_angle = 90;
#endif
#ifdef CONFIG_A8140
		sensor_board_info_a8140.cam_vreg = a8140_gpio_vreg;
		sensor_board_info_a8140.num_vreg = ARRAY_SIZE(a8140_gpio_vreg);
		msm_act_main_cam_8_info.vcm_pwd = GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN;
		msm_act_main_cam_8_info.vcm_enable = 1;
		msm_camera_sensor_a8140_data.sensor_reset=GPIO_SKU3_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_a8140_data.sensor_pwd = GPIO_SKU3_CAM_5MP_SHDN_N;
		sensor_board_info_a8140.mount_angle = 90;
#endif
#ifdef CONFIG_S5K3H2_SUNNY_Q8S02E
      sensor_board_info_s5k3h2_sunny_q8s02e.cam_vreg = s5k3h2_sunny_q8s02e_gpio_vreg;
      sensor_board_info_s5k3h2_sunny_q8s02e.num_vreg = ARRAY_SIZE(s5k3h2_sunny_q8s02e_gpio_vreg);
      msm_act_main_cam_9_info.vcm_pwd = GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN;
      msm_act_main_cam_9_info.vcm_enable = 1;
      msm_camera_sensor_s5k3h2_sunny_q8s02e_data.sensor_reset=GPIO_SKU3_CAM_5MP_CAMIF_RESET;
      msm_camera_sensor_s5k3h2_sunny_q8s02e_data.sensor_pwd = GPIO_SKU3_CAM_5MP_SHDN_N;
      sensor_board_info_s5k3h2_sunny_q8s02e.mount_angle = 90;
#endif
	}
	else if(machine_is_msm8625q_evbd()||
		machine_is_msm8625q_skud())
	{  //for SKUD
#ifdef CONFIG_OV5648_TRULY_CM8352
		if(machine_is_msm8625q_evbd()) {
			sensor_board_info_ov5648_truly_cm8352.cam_vreg = ov5648_gpio_vreg_evbd;
			sensor_board_info_ov5648_truly_cm8352.num_vreg = ARRAY_SIZE(ov5648_gpio_vreg_evbd);
			sensor_board_info_ov5648_truly_cm8352.mount_angle = 270;
		} else {
			sensor_board_info_ov5648_truly_cm8352.cam_vreg = ov5648_truly_cm8352_gpio_vreg;
			sensor_board_info_ov5648_truly_cm8352.num_vreg = ARRAY_SIZE(ov5648_truly_cm8352_gpio_vreg);
			sensor_board_info_ov5648_truly_cm8352.mount_angle = 90;
		}
		msm_act_main_cam_7_info.vcm_pwd = GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN;
		msm_act_main_cam_7_info.vcm_enable = 1;
		msm_camera_sensor_ov5648_truly_cm8352_data.sensor_reset=GPIO_SKUD_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_ov5648_truly_cm8352_data.sensor_pwd = GPIO_SKUD_CAM_5MP_SHDN_N;
		msm_flash_src_ov5648_truly_cm8352._fsrc.ext_driver_src.led_en = GPIO_SKUD_CAM_LED_EN;
		msm_flash_src_ov5648_truly_cm8352._fsrc.ext_driver_src.led_flash_en = GPIO_SKUD_CAM_LED_FLASH_EN;
#endif

#ifdef CONFIG_OV8825
		if(machine_is_msm8625q_evbd())
        {
			sensor_board_info_ov8825.cam_vreg = ov8825_gpio_vreg_evbd;
			sensor_board_info_ov8825.num_vreg = ARRAY_SIZE(ov8825_gpio_vreg_evbd);
			msm_act_main_cam_3_info.vcm_pwd = GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN;
			msm_act_main_cam_3_info.vcm_enable = 1;
			msm_camera_sensor_ov8825_data.sensor_reset=GPIO_SKUD_CAM_5MP_CAMIF_RESET;
			msm_camera_sensor_ov8825_data.sensor_pwd = GPIO_SKUD_CAM_5MP_SHDN_N;
			msm_flash_src_ov8825._fsrc.ext_driver_src.led_en = GPIO_SKUD_CAM_LED_EN;
			msm_flash_src_ov8825._fsrc.ext_driver_src.led_flash_en = GPIO_SKUD_CAM_LED_FLASH_EN;
		}
#endif

#ifdef CONFIG_OV7695
		if(machine_is_msm8625q_evbd()) {
			sensor_board_info_ov7695.cam_vreg = ov7695_gpio_vreg_evbd;
			sensor_board_info_ov7695.num_vreg = ARRAY_SIZE(ov7695_gpio_vreg_evbd);
		} else {
			sensor_board_info_ov7695.cam_vreg = ov7695_gpio_vreg;
			sensor_board_info_ov7695.num_vreg = ARRAY_SIZE(ov7695_gpio_vreg);
		}
		msm_camera_sensor_ov7695_data.vcm_pwd = 0;
		msm_camera_sensor_ov7695_data.vcm_enable = 0;
		sensor_board_info_ov7695.gpio_conf = &skud_gpio_conf_ov7695;
		sensor_board_info_ov7695.mount_angle = 90;
#endif
#ifdef CONFIG_OV7695_RAW
		sensor_board_info_ov7695_raw.cam_vreg = ov7695_raw_gpio_vreg;
		sensor_board_info_ov7695_raw.num_vreg = ARRAY_SIZE(ov7695_raw_gpio_vreg);
		msm_camera_sensor_ov7695_raw_data.vcm_pwd = 0;
		msm_camera_sensor_ov7695_raw_data.vcm_enable = 0;
		msm_camera_sensor_ov7695_raw_data.sensor_pwd = GPIO_SKUD_CAM_1MP_PWDN;
		sensor_board_info_ov7695_raw.gpio_conf = &skud_gpio_conf_ov7695_raw;
		sensor_board_info_ov7695_raw.mount_angle = 270;
#endif
	}else if(machine_is_msm8625q_skue()) {
#ifdef CONFIG_MT9V115
		sensor_board_info_mt9v115.cam_vreg = mt9v115_gpio_vreg;
		sensor_board_info_mt9v115.num_vreg = ARRAY_SIZE(mt9v115_gpio_vreg);
		msm_camera_sensor_mt9v115_data.vcm_pwd = 0;
		msm_camera_sensor_mt9v115_data.vcm_enable = 0;
		msm_camera_sensor_mt9v115_data.sensor_pwd = GPIO_SKUE_CAM_VGA_SHDN;
		sensor_board_info_mt9v115.gpio_conf = &gpio_conf_mt9v115;
		sensor_board_info_mt9v115.mount_angle = 270;
#endif
#ifdef CONFIG_AR0542
		sensor_board_info_ar0542.cam_vreg = ar0542_gpio_vreg;
		sensor_board_info_ar0542.num_vreg = ARRAY_SIZE(ar0542_gpio_vreg);
		msm_camera_sensor_ar0542_data.sensor_reset=GPIO_SKUE_CAM_5MP_CAMIF_RESET;
		msm_camera_sensor_ar0542_data.sensor_pwd = GPIO_SKUE_CAM_5MP_SHDN_N;
		sensor_board_info_ar0542.mount_angle = 90;
		msm_flash_src_ar0542._fsrc.ext_driver_src.led_en = GPIO_SKUE_CAM_LED_EN;
		msm_flash_src_ar0542._fsrc.ext_driver_src.led_flash_en = GPIO_SKUE_CAM_LED_FLASH_EN;
#endif
	}

	platform_device_register(&msm_camera_server);
	if (machine_is_msm8625_surf() || machine_is_msm8625_evb()
			|| machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a() 
			|| machine_is_msm8625_qrd7()
			|| machine_is_msm8625q_evbd()
			|| machine_is_msm8625q_skud()
			|| machine_is_msm8625q_skue()) {
		platform_device_register(&msm8625_device_csic0);
		platform_device_register(&msm8625_device_csic1);
	} else {
		platform_device_register(&msm7x27a_device_csic0);
		platform_device_register(&msm7x27a_device_csic1);
	}
	if (machine_is_msm8625_evb()
			|| machine_is_msm8625_qrd5()
			|| machine_is_msm7627a_qrd3()
			|| machine_is_msm8625_qrd7()
			|| machine_is_msm7x27a_qrd5a()
			|| machine_is_msm8625q_evbd()
			|| machine_is_msm8625q_skud()
			|| machine_is_msm8625q_skue())
		*(int *) msm7x27a_device_clkctl.dev.platform_data = 1;
	platform_device_register(&msm7x27a_device_clkctl);
	platform_device_register(&msm7x27a_device_vfe);
}

static struct i2c_board_info i2c_camera_devices[] = {
#ifdef CONFIG_S5K4E1
	{
		I2C_BOARD_INFO("s5k4e1", 0x36),
		.platform_data = &msm_camera_sensor_s5k4e1_data,
	},
#endif
#ifdef CONFIG_WEBCAM_OV9726
	{
		I2C_BOARD_INFO("ov9726", 0x10),
		.platform_data = &msm_camera_sensor_ov9726_data,
	},
#endif
#ifdef CONFIG_MT9E013
	{
		I2C_BOARD_INFO("mt9e013", 0x6C >> 2),
		.platform_data = &msm_camera_sensor_mt9e013_data,
	},
#endif
//#ifdef CONFIG_OV7692
#ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
		.platform_data = &msm_camera_sensor_ov7692_data,
	},
#endif
#ifdef CONFIG_OV5647
	{
		I2C_BOARD_INFO("ov5647", 0x36 << 1),
		.platform_data = &msm_camera_sensor_ov5647_data,
	},
#endif
#ifdef CONFIG_OV8825
	{
		I2C_BOARD_INFO("ov8825", 0x6C >> 3),
		.platform_data = &msm_camera_sensor_ov8825_data,
	},
#endif
	{
		I2C_BOARD_INFO("sc628a", 0x6E),
	},
};

static struct i2c_board_info i2c_camera_devices_sku5[] = {
#ifdef CONFIG_OV5647_TRULY_CM6868
	{
		I2C_BOARD_INFO("ov5647_truly_cm6868", 0x36 << 1),//original
		.platform_data = &msm_camera_sensor_ov5647_truly_cm6868_data,
	},
#endif
#ifdef CONFIG_OV8825
	{
		I2C_BOARD_INFO("ov8825", 0x6C >> 3),
		.platform_data = &msm_camera_sensor_ov8825_data,
	},
#endif
//#ifdef CONFIG_OV7692
#ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
		.platform_data = &msm_camera_sensor_ov7692_data,
	},
#endif
#ifdef CONFIG_AR0543
	{
		I2C_BOARD_INFO("ar0543", 0x64),
		.platform_data = &msm_camera_sensor_ar0543_data,
	},
#endif
#ifdef CONFIG_A8140
	{
		I2C_BOARD_INFO("a8140", 0x62),
		.platform_data = &msm_camera_sensor_a8140_data,
	},
#endif
#ifdef CONFIG_S5K3H2_SUNNY_Q8S02E
	{
		I2C_BOARD_INFO("s5k3h2_sunny_q8s02e", 0x32),
		.platform_data = &msm_camera_sensor_s5k3h2_sunny_q8s02e_data,
	},
#endif
};

static struct i2c_board_info i2c_camera_devices_skud[] = {
#ifdef CONFIG_OV8825
	{
		I2C_BOARD_INFO("ov8825", 0x6C >> 3), // 8MP sensor
		.platform_data = &msm_camera_sensor_ov8825_data,
	},
#endif
#ifdef CONFIG_OV7695
	{
		I2C_BOARD_INFO("ov7695", 0x21 << 1),
		.platform_data = &msm_camera_sensor_ov7695_data,
	},
#endif
#ifdef CONFIG_OV7695_RAW
	{
		I2C_BOARD_INFO("ov7695_raw", 0x21 << 1),
		.platform_data = &msm_camera_sensor_ov7695_raw_data,
	},
#endif
#ifdef CONFIG_OV5648_TRULY_CM8352
	{
		I2C_BOARD_INFO("ov5648_truly_cm8352", 0x36 << 1),//original
		.platform_data = &msm_camera_sensor_ov5648_truly_cm8352_data,
	},
#endif
};

static struct i2c_board_info i2c_camera_devices_skue[] = {
#ifdef CONFIG_AR0542
	{
		I2C_BOARD_INFO("ar0542", 0x64),
		.platform_data = &msm_camera_sensor_ar0542_data,
	},
#endif
#ifdef CONFIG_MT9V115
	{
		I2C_BOARD_INFO("mt9v115", 0x7a),
		.platform_data = &msm_camera_sensor_mt9v115_data,
	},
#endif
};

static struct i2c_board_info i2c_camera_devices_sku7[] = {
#ifdef CONFIG_OV5647
	{
		I2C_BOARD_INFO("ov5647", 0x36 << 1),
		.platform_data = &msm_camera_sensor_ov5647_data,
	},
#endif
//#ifdef CONFIG_OV7692
#ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
		.platform_data = &msm_camera_sensor_ov7692_data,
	},
#endif
};

#else
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_EXT,
	._fsrc.ext_driver_src.led_en = GPIO_CAM_GP_LED_EN1,
	._fsrc.ext_driver_src.led_flash_en = GPIO_CAM_GP_LED_EN2,
};
#endif

static struct regulator_bulk_data regs_camera[] = {
	{ .supply = "msme1", .min_uV = 1800000, .max_uV = 1800000 },
	{ .supply = "gp2",   .min_uV = 2850000, .max_uV = 2850000 },
	{ .supply = "usb2",  .min_uV = 1800000, .max_uV = 1800000 },
};

static void qrd1_camera_gpio_cfg(void)
{

	int rc = 0;

	rc = gpio_request(QRD_GPIO_CAM_5MP_SHDN_EN, "ov5640");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_CAM_5MP_SHDN_EN failed!",
				__func__);


	rc = gpio_tlmm_config(GPIO_CFG(QRD_GPIO_CAM_5MP_SHDN_EN, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable Power Down gpio for main"
				"camera!\n", __func__);
		gpio_free(QRD_GPIO_CAM_5MP_SHDN_EN);
	}


	rc = gpio_request(QRD_GPIO_CAM_5MP_RESET, "ov5640");
	if (rc < 0) {
		pr_err("%s: gpio_request---GPIO_CAM_5MP_RESET failed!",
				__func__);
		gpio_free(QRD_GPIO_CAM_5MP_SHDN_EN);
	}


	rc = gpio_tlmm_config(GPIO_CFG(QRD_GPIO_CAM_5MP_RESET, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
				__func__);
		gpio_free(QRD_GPIO_CAM_5MP_RESET);
	}

	rc = gpio_request(QRD_GPIO_CAM_3MP_PWDN, "ov7692");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_CAM_3MP_PWDN failed!",
				__func__);

	rc = gpio_tlmm_config(GPIO_CFG(QRD_GPIO_CAM_3MP_PWDN, 0,
				GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable Power Down gpio for front"
				"camera!\n", __func__);
		gpio_free(QRD_GPIO_CAM_3MP_PWDN);
	}

	gpio_direction_output(QRD_GPIO_CAM_5MP_SHDN_EN, 1);
	gpio_direction_output(QRD_GPIO_CAM_5MP_RESET, 1);
	gpio_direction_output(QRD_GPIO_CAM_3MP_PWDN, 1);
}
#endif

static void evb_camera_gpio_cfg(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_SKU3_CAM_5MP_SHDN_N, "ov5647");
	if (rc < 0)
		pr_err("%s: gpio_request OV5647 sensor_pwd: %d failed!",
			 __func__, GPIO_SKU3_CAM_5MP_SHDN_N);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_SHDN_N,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_SHDN_N);
	}

	rc = gpio_direction_output(GPIO_SKU3_CAM_5MP_SHDN_N, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5647 camera\n",
			__func__, GPIO_SKU3_CAM_5MP_SHDN_N);

	rc = gpio_request(GPIO_SKU3_CAM_5MP_CAMIF_RESET, "ov5647");
	if (rc < 0)
		pr_err("%s: gpio_request OV5647 sensor_reset: %d failed!",
			 __func__, GPIO_SKU3_CAM_5MP_CAMIF_RESET);

	rc = gpio_tlmm_config(GPIO_CFG(
				GPIO_SKU3_CAM_5MP_CAMIF_RESET,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	}

	rc = gpio_direction_output(
			GPIO_SKU3_CAM_5MP_CAMIF_RESET, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5647 camera\n",
			__func__, GPIO_SKU3_CAM_5MP_CAMIF_RESET);

	/*OV7692 GPIO Config*/
	/*michael cong delete
	rc = gpio_request(msm_camera_sensor_ov7692_data.sensor_pwd, "ov7692");
	if (rc < 0)
		pr_err("%s: gpio_request OV7692 sensor_pwd: %d failed!",
			 __func__, msm_camera_sensor_ov7692_data.sensor_pwd);

	rc = gpio_tlmm_config(GPIO_CFG(msm_camera_sensor_ov7692_data.sensor_pwd,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(msm_camera_sensor_ov7692_data.sensor_pwd);
	}

	rc = gpio_direction_output(msm_camera_sensor_ov7692_data.sensor_pwd, 0);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov7692 camera\n",
			__func__, msm_camera_sensor_ov7692_data.sensor_pwd);
	*/		
	// [ start ] lixujie 20120511 modify it for EG808T front Camera  
   #ifndef	CONFIG_ODMM  
	rc = gpio_request(GPIO_SKU1_CAM_VGA_SHDN, "ov7692");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_SKU1_CAM_VGA_SHDN failed!",
			 __func__);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU1_CAM_VGA_SHDN, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for frnt camera!\n",
			 __func__);
		gpio_free(GPIO_SKU1_CAM_VGA_SHDN);
	}

	gpio_direction_output(GPIO_SKU1_CAM_VGA_SHDN, 1);

	rc = gpio_request(GPIO_SKU1_CAM_VGA_RESET_N, "ov7692");
	if (rc < 0)
		pr_err("%s: gpio_request---GPIO_SKU1_CAM_VGA_RESET_N failed!",
			 __func__);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU1_CAM_VGA_RESET_N, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for front camera!\n",
			 __func__);
		gpio_free(GPIO_SKU1_CAM_VGA_RESET_N);
	}
	gpio_direction_output(GPIO_SKU1_CAM_VGA_RESET_N, 1);
  #endif
// [ end ] 			

}

static void sku5_camera_gpio_cfg(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_SKU3_CAM_5MP_SHDN_N, "ov5647");
	if (rc < 0)
		pr_err("%s: gpio_request OV5647 sensor_pwd: %d failed!",
			 __func__, GPIO_SKU3_CAM_5MP_SHDN_N);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKU3_CAM_5MP_SHDN_N,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_SHDN_N);
	}

	rc = gpio_direction_output(GPIO_SKU3_CAM_5MP_SHDN_N, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5647 camera\n",
			__func__, GPIO_SKU3_CAM_5MP_SHDN_N);

	rc = gpio_request(GPIO_SKU3_CAM_5MP_CAMIF_RESET, "ov5647");
	if (rc < 0)
		pr_err("%s: gpio_request OV5647 sensor_reset: %d failed!",
			 __func__, GPIO_SKU3_CAM_5MP_CAMIF_RESET);

	rc = gpio_tlmm_config(GPIO_CFG(
				GPIO_SKU3_CAM_5MP_CAMIF_RESET,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKU3_CAM_5MP_CAMIF_RESET);
	}

	rc = gpio_direction_output(
			GPIO_SKU3_CAM_5MP_CAMIF_RESET, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5647 camera\n",
			__func__, GPIO_SKU3_CAM_5MP_CAMIF_RESET);

}

static void skud_camera_gpio_cfg(void)
{
	int rc = 0;
	printk("skuD_camera_gpio_cfg in, cfg gpio\n");
	printk("gpio request: GPIO_SKUD_CAM_5MP_SHDN_N is %d\n", GPIO_SKUD_CAM_5MP_SHDN_N);
	rc = gpio_request(GPIO_SKUD_CAM_5MP_SHDN_N, "ov5648");
	if (rc < 0)
		printk("%s: gpio_request OV5648 sensor_pwd: %d failed!",
			 __func__, GPIO_SKUD_CAM_5MP_SHDN_N);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKUD_CAM_5MP_SHDN_N,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		printk("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKUD_CAM_5MP_SHDN_N);
	}

	rc = gpio_direction_output(GPIO_SKUD_CAM_5MP_SHDN_N, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5648 camera\n",
			__func__, GPIO_SKUD_CAM_5MP_SHDN_N);

	printk("gpio request: GPIO_SKUD_CAM_5MP_CAMIF_RESET is %d\n", GPIO_SKUD_CAM_5MP_CAMIF_RESET);
	rc = gpio_request(GPIO_SKUD_CAM_5MP_CAMIF_RESET, "ov5648");
	if (rc < 0)
		pr_err("%s: gpio_request OV5648 sensor_reset: %d failed!",
			 __func__, GPIO_SKUD_CAM_5MP_CAMIF_RESET);

	rc = gpio_tlmm_config(GPIO_CFG(
				GPIO_SKUD_CAM_5MP_CAMIF_RESET,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKUD_CAM_5MP_CAMIF_RESET);
	}

	rc = gpio_direction_output(
			GPIO_SKUD_CAM_5MP_CAMIF_RESET, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5648 camera\n",
			__func__, GPIO_SKUD_CAM_5MP_CAMIF_RESET);

	printk("gpio request: GPIO_SKUD_CAM_1MP_PWDN is %d\n", GPIO_SKUD_CAM_1MP_PWDN);
		rc = gpio_request(GPIO_SKUD_CAM_1MP_PWDN, "OV7695");
	if (rc < 0)
	pr_err("%s: gpio_request OV7695 sensor_pwdn: %d failed!",
		__func__, GPIO_SKUD_CAM_1MP_PWDN);

	rc = gpio_tlmm_config(GPIO_CFG(
		GPIO_SKUD_CAM_1MP_PWDN,
		0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for front camera!\n", __func__);
		gpio_free(GPIO_SKUD_CAM_1MP_PWDN);
	}

	rc = gpio_direction_output(GPIO_SKUD_CAM_1MP_PWDN, 0);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov7695 camera\n",
		__func__, GPIO_SKUD_CAM_1MP_PWDN);

	// ================================
	printk("gpio request: GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN is %d\n", GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN);
	rc = gpio_request(GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN, "msm_actuator");
	if (rc < 0)
		printk("%s: gpio_request msm_actuator : %d failed!",
		__func__, GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN);

	rc = gpio_tlmm_config(GPIO_CFG(
		GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN,
		0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		printk("%s:unable to enable Powr Dwn gpio for msm_actuator!\n", __func__);
		gpio_free(GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN);
		}
	rc = gpio_direction_output(GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN, 0);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5648 camera\n",
		__func__, GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN);

	//Free the gpio for the actuator will request it again, only do tlmm config here.
	gpio_free(GPIO_SKUD_CAM_5MP_CAM_VCM_PWDN);
	//=================================

	//Fix CAMID leak current issue when phone sleep.
	printk("gpio request: set CAMID GPIO to INPUT, now the gpio is %d\n", GPIO_SKUD_CAM_5MP_CAMID);
	rc = gpio_request(GPIO_SKUD_CAM_5MP_CAMID, "ov5648");
	if (rc < 0)
		printk("%s: gpio_request GPIO %d : failed!", __func__, GPIO_SKUD_CAM_5MP_CAMID);

	gpio_tlmm_config(GPIO_CFG(
		GPIO_SKUD_CAM_5MP_CAMID,
		0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);

	//Free the gpio, only do tlmm config to input pull high here.
	gpio_free(GPIO_SKUD_CAM_5MP_CAMID);
}


static void skue_camera_gpio_cfg(void)
{
	int rc = 0;
	printk("skuE_camera_gpio_cfg in, cfg gpio\n");
	printk("gpio request: GPIO_SKUE_CAM_5MP_SHDN_N is %d\n", GPIO_SKUE_CAM_5MP_SHDN_N);
	rc = gpio_request(GPIO_SKUE_CAM_5MP_SHDN_N, "AR0542");
	if (rc < 0)
		printk("%s: gpio_request AR0542 sensor_pwd: %d failed!",
			 __func__, GPIO_SKUE_CAM_5MP_SHDN_N);

	rc = gpio_tlmm_config(GPIO_CFG(GPIO_SKUE_CAM_5MP_SHDN_N,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		printk("%s:unable to enable Powr Dwn gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKUE_CAM_5MP_SHDN_N);
	}

	rc = gpio_direction_output(GPIO_SKUE_CAM_5MP_SHDN_N, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for ov5648 camera\n",
			__func__, GPIO_SKUE_CAM_5MP_SHDN_N);

	printk("gpio request: GPIO_SKUE_CAM_5MP_CAMIF_RESET is %d\n", GPIO_SKUE_CAM_5MP_CAMIF_RESET);
	rc = gpio_request(GPIO_SKUE_CAM_5MP_CAMIF_RESET, "AR0542");
	if (rc < 0)
		pr_err("%s: gpio_request AR0542 sensor_reset: %d failed!",
			 __func__, GPIO_SKUE_CAM_5MP_CAMIF_RESET);

	rc = gpio_tlmm_config(GPIO_CFG(
				GPIO_SKUE_CAM_5MP_CAMIF_RESET,
				0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for main camera!\n",
			 __func__);
		gpio_free(GPIO_SKUE_CAM_5MP_CAMIF_RESET);
	}

	rc = gpio_direction_output(
			GPIO_SKUE_CAM_5MP_CAMIF_RESET, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for AR0542 camera\n",
			__func__, GPIO_SKUE_CAM_5MP_CAMIF_RESET);

	printk("gpio request: GPIO_SKUE_CAM_PWR_EN is %d\n", GPIO_SKUE_CAM_PWR_EN);
		rc = gpio_request(GPIO_SKUE_CAM_PWR_EN, "AR0542");
	if (rc < 0)
	pr_err("%s: gpio_request AR0542 sensor_pwdn: %d failed!",
		__func__, GPIO_SKUE_CAM_PWR_EN);

	rc = gpio_tlmm_config(GPIO_CFG(
		GPIO_SKUE_CAM_PWR_EN,
		0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable reset gpio for front camera!\n", __func__);
		gpio_free(GPIO_SKUE_CAM_PWR_EN);
	}

	rc = gpio_direction_output(GPIO_SKUE_CAM_PWR_EN, 1);
	if (rc < 0)
		pr_err("%s: unable to set gpio: %d direction for AR0542 camera\n",
		__func__, GPIO_SKUE_CAM_PWR_EN);

}

#ifndef CONFIG_MSM_CAMERA_V4L2

static void msm_camera_vreg_config(int vreg_en)
{
	int rc = vreg_en ?
		regulator_bulk_enable(ARRAY_SIZE(regs_camera), regs_camera) :
		regulator_bulk_disable(ARRAY_SIZE(regs_camera), regs_camera);

	if (rc)
		pr_err("%s: could not %sable regulators: %d\n",
				__func__, vreg_en ? "en" : "dis", rc);
}

static int config_gpio_table(uint32_t *table, int len)
{
	int rc = 0, i = 0;

	for (i = 0; i < len; i++) {
		rc = gpio_tlmm_config(table[i], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s not able to get gpio\n", __func__);
			for (i--; i >= 0; i--)
				gpio_tlmm_config(camera_off_gpio_table[i],
							GPIO_CFG_ENABLE);
			break;
		}
	}
	return rc;
}

static int config_camera_on_gpios_rear(void)
{
	int rc = 0;

	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
				|| machine_is_msm7627a_qrd1()
				|| machine_is_msm8625_ffa())
		msm_camera_vreg_config(1);

	rc = config_gpio_table(camera_on_gpio_table,
			ARRAY_SIZE(camera_on_gpio_table));
	if (rc < 0) {
		pr_err("%s: CAMSENSOR gpio table request"
		"failed\n", __func__);
		return rc;
	}

	return rc;
}

static void config_camera_off_gpios_rear(void)
{
	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
				|| machine_is_msm7627a_qrd1()
				|| machine_is_msm8625_ffa())
		msm_camera_vreg_config(0);

	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
}

static int config_camera_on_gpios_front(void)
{
	int rc = 0;

	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
				|| machine_is_msm7627a_qrd1()
				|| machine_is_msm8625_ffa())
		msm_camera_vreg_config(1);

	rc = config_gpio_table(camera_on_gpio_table,
			ARRAY_SIZE(camera_on_gpio_table));
	if (rc < 0) {
		pr_err("%s: CAMSENSOR gpio table request"
			"failed\n", __func__);
		return rc;
	}

	return rc;
}

static void config_camera_off_gpios_front(void)
{
	if (machine_is_msm7x27a_ffa() || machine_is_msm7625a_ffa()
				|| machine_is_msm7627a_qrd1()
				|| machine_is_msm8625_ffa())
		msm_camera_vreg_config(0);

	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
}

struct msm_camera_device_platform_data msm_camera_device_data_rear = {
	.camera_gpio_on		= config_camera_on_gpios_rear,
	.camera_gpio_off	= config_camera_off_gpios_rear,
	.ioext.csiphy		= 0xA1000000,
	.ioext.csisz		= 0x00100000,
	.ioext.csiirq		= INT_CSI_IRQ_1,
	.ioclk.mclk_clk_rate	= 24000000,
	.ioclk.vfe_clk_rate	= 192000000,
	.ioext.appphy		= MSM7XXX_CLK_CTL_PHYS,
	.ioext.appsz		= MSM7XXX_CLK_CTL_SIZE,
};

struct msm_camera_device_platform_data msm_camera_device_data_front = {
	.camera_gpio_on		= config_camera_on_gpios_front,
	.camera_gpio_off	= config_camera_off_gpios_front,
	.ioext.csiphy		= 0xA0F00000,
	.ioext.csisz		= 0x00100000,
	.ioext.csiirq		= INT_CSI_IRQ_0,
	.ioclk.mclk_clk_rate	= 24000000,
	.ioclk.vfe_clk_rate	= 192000000,
	.ioext.appphy		= MSM7XXX_CLK_CTL_PHYS,
	.ioext.appsz		= MSM7XXX_CLK_CTL_SIZE,
};

#ifdef CONFIG_OV5647

static struct msm_camera_sensor_platform_info ov5647_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5647 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED,
	._fsrc.led_src.led_name = "flashlight",
	._fsrc.led_src.led_name_len = 10,
};

static struct msm_camera_sensor_flash_data flash_ov5647 = {
	.flash_type             = MSM_CAMERA_FLASH_LED,
	.flash_src              = &msm_flash_src_ov5647,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5647_data = {
	.sensor_name    = "ov5647",
	.sensor_reset_enable = 1,
	.sensor_reset   = GPIO_SKU3_CAM_5MP_CAMIF_RESET,
	.pmic_gpio_enable  = 1,
	.sensor_pwd     = GPIO_SKU3_CAM_5MP_SHDN_N,
	.vcm_pwd        = GPIO_SKU3_CAM_5MP_CAM_DRIVER_PWDN,
	.vcm_enable     = 1,
	.pdata          = &msm_camera_device_data_rear,
	.flash_data     = &flash_ov5647,
	.sensor_platform_info   = &ov5647_sensor_7627a_info,
	.csi_if                 = 1
};

static struct platform_device msm_camera_sensor_ov5647 = {
	.name      = "msm_camera_ov5647",
	.dev       = {
		.platform_data = &msm_camera_sensor_ov5647_data,
	},
};
#endif

#ifdef CONFIG_S5K4E1
static struct msm_camera_sensor_platform_info s5k4e1_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_s5k4e1 = {
	.flash_type	     = MSM_CAMERA_FLASH_LED,
	.flash_src	      = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_s5k4e1_data = {
	.sensor_name		= "s5k4e1",
	.sensor_reset_enable	= 1,
	.sensor_reset		= GPIO_CAM_GP_CAMIF_RESET_N,
	.pmic_gpio_enable       = 0,
	.sensor_pwd	     = 85,
	.vcm_pwd		= GPIO_CAM_GP_CAM_PWDN,
	.vcm_enable	     = 1,
	.pdata			= &msm_camera_device_data_rear,
	.flash_data	     = &flash_s5k4e1,
	.sensor_platform_info   = &s5k4e1_sensor_7627a_info,
	.csi_if			= 1
};

static struct platform_device msm_camera_sensor_s5k4e1 = {
	.name   = "msm_camera_s5k4e1",
	.dev    = {
		.platform_data = &msm_camera_sensor_s5k4e1_data,
	},
};
#endif

#ifdef CONFIG_IMX072
static struct msm_camera_sensor_platform_info imx072_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_imx072 = {
	.flash_type	     = MSM_CAMERA_FLASH_LED,
	.flash_src	      = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_imx072_data = {
	.sensor_name		= "imx072",
	.sensor_reset_enable	= 1,
	.sensor_reset		= GPIO_CAM_GP_CAMIF_RESET_N, /* TODO 106,*/
	.pmic_gpio_enable       = 0,
	.sensor_pwd	     = 85,
	.vcm_pwd		= GPIO_CAM_GP_CAM_PWDN,
	.vcm_enable	     = 1,
	.pdata			= &msm_camera_device_data_rear,
	.flash_data	     = &flash_imx072,
	.sensor_platform_info	= &imx072_sensor_7627a_info,
	.csi_if			= 1
};

static struct platform_device msm_camera_sensor_imx072 = {
	.name   = "msm_camera_imx072",
	.dev    = {
		.platform_data = &msm_camera_sensor_imx072_data,
	},
};
#endif

#ifdef CONFIG_WEBCAM_OV9726
static struct msm_camera_sensor_info msm_camera_sensor_ov9726_data;
static struct msm_camera_sensor_platform_info ov9726_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_ov9726 = {
	.flash_type	     = MSM_CAMERA_FLASH_NONE,
	.flash_src	      = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_ov9726_data = {
	.sensor_name		= "ov9726",
	.sensor_reset_enable	= 0,
	.sensor_reset		= GPIO_CAM_GP_CAM1MP_XCLR,
	.pmic_gpio_enable       = 0,
	.sensor_pwd	     = 85,
	.vcm_pwd		= 1,
	.vcm_enable	     = 0,
	.pdata			= &msm_camera_device_data_front,
	.flash_data	     = &flash_ov9726,
	.sensor_platform_info   = &ov9726_sensor_7627a_info,
	.csi_if			= 1
};

static struct platform_device msm_camera_sensor_ov9726 = {
	.name   = "msm_camera_ov9726",
	.dev    = {
		.platform_data = &msm_camera_sensor_ov9726_data,
	},
};
#else
static inline void msm_camera_vreg_init(void) { }
#endif

#ifdef CONFIG_MT9E013
static struct msm_camera_sensor_platform_info mt9e013_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_mt9e013 = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9e013_data = {
	.sensor_name		= "mt9e013",
	.sensor_reset		= 0,
	.sensor_reset_enable	= 1,
	.pmic_gpio_enable       = 0,
	.sensor_pwd		= 85,
	.vcm_pwd		= 1,
	.vcm_enable		= 0,
	.pdata		= &msm_camera_device_data_rear,
	.flash_data		= &flash_mt9e013,
	.sensor_platform_info   = &mt9e013_sensor_7627a_info,
	.csi_if		= 1
};

static struct platform_device msm_camera_sensor_mt9e013 = {
	.name      = "msm_camera_mt9e013",
	.dev       = {
		.platform_data = &msm_camera_sensor_mt9e013_data,
	},
};
#endif

#ifdef CONFIG_OV5640
static struct msm_camera_sensor_platform_info ov5640_sensor_info = {
	.mount_angle    = 90
};

static struct msm_camera_sensor_flash_src msm_flash_src_ov5640 = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_LED,
	._fsrc.led_src.led_name = "flashlight",
	._fsrc.led_src.led_name_len = 10,
};

static struct msm_camera_sensor_flash_data flash_ov5640 = {
	.flash_type     = MSM_CAMERA_FLASH_LED,
	.flash_src      = &msm_flash_src_ov5640,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov5640_data = {
	.sensor_name	    = "ov5640",
	.sensor_reset_enable    = 1,
	.pmic_gpio_enable  = 0,
	.sensor_reset	   = QRD_GPIO_CAM_5MP_RESET,
	.sensor_pwd	     = QRD_GPIO_CAM_5MP_SHDN_EN,
	.vcm_pwd		= 0,
	.vcm_enable	     = 0,
	.pdata			= &msm_camera_device_data_rear,
	.flash_data	     = &flash_ov5640,
	.sensor_platform_info   = &ov5640_sensor_info,
	.csi_if		 = 1,
};

static struct platform_device msm_camera_sensor_ov5640 = {
	.name   = "msm_camera_ov5640",
	.dev    = {
		.platform_data = &msm_camera_sensor_ov5640_data,
	},
};
#endif

#ifdef CONFIG_WEBCAM_OV7692_QRD
static struct msm_camera_sensor_platform_info ov7692_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_flash_data flash_ov7692 = {
	.flash_type     = MSM_CAMERA_FLASH_NONE,
};

static struct msm_camera_sensor_info msm_camera_sensor_ov7692_data = {
	.sensor_name	    = "ov7692",
	.sensor_reset_enable    = 0,
	.pmic_gpio_enable  = 1,
	.sensor_reset	   = GPIO_SKU1_CAM_VGA_RESET_N,
	.sensor_pwd	     = GPIO_SKU1_CAM_VGA_SHDN,
	.vcm_pwd		= 0,
	.vcm_enable	     = 0,
	.pdata			= &msm_camera_device_data_front,
	.flash_data	     = &flash_ov7692,
	.sensor_platform_info   = &ov7692_sensor_7627a_info,
	.csi_if		 = 1,
};

static struct platform_device msm_camera_sensor_ov7692 = {
	.name   = "msm_camera_ov7692",
	.dev    = {
		.platform_data = &msm_camera_sensor_ov7692_data,
	},
};
#endif

static struct i2c_board_info i2c_camera_devices[] = {
	#ifdef CONFIG_S5K4E1
	{
		I2C_BOARD_INFO("s5k4e1", 0x36),
	},
	{
		I2C_BOARD_INFO("s5k4e1_af", 0x8c >> 1),
	},
	#endif
	#ifdef CONFIG_WEBCAM_OV9726
	{
		I2C_BOARD_INFO("ov9726", 0x10),
	},
	#endif
	#ifdef CONFIG_IMX072
	{
		I2C_BOARD_INFO("imx072", 0x34),
	},
	#endif
	#ifdef CONFIG_MT9E013
	{
		I2C_BOARD_INFO("mt9e013", 0x6C >> 2),
	},
	#endif
	{
		I2C_BOARD_INFO("sc628a", 0x6E),
	},
	#ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
		.platform_data = &msm_camera_sensor_ov7692_data,
	},
#endif
};

static struct i2c_board_info i2c_camera_devices_qrd[] = {
	#ifdef CONFIG_OV5640
	{
		I2C_BOARD_INFO("ov5640", 0x78 >> 1),
	},
	#endif
	#ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
	},
	#endif
};

static struct i2c_board_info i2c_camera_devices_evb[] = {
	#ifdef CONFIG_OV5647
	{
		I2C_BOARD_INFO("ov5647", 0x36 << 1),
	},
	{
		I2C_BOARD_INFO("ov5647_af", 0x18 >> 1),
	},
	#endif
	#ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
	},
	#endif
};

static struct platform_device *camera_devices_msm[] __initdata = {
#ifdef CONFIG_S5K4E1
	&msm_camera_sensor_s5k4e1,
#endif
#ifdef CONFIG_IMX072
	&msm_camera_sensor_imx072,
#endif
#ifdef CONFIG_WEBCAM_OV9726
	&msm_camera_sensor_ov9726,
#endif
#ifdef CONFIG_MT9E013
	&msm_camera_sensor_mt9e013,
#endif
};

static struct platform_device *camera_devices_qrd[] __initdata = {
#ifdef CONFIG_OV5640
	&msm_camera_sensor_ov5640,
#endif
#ifdef CONFIG_WEBCAM_OV7692_QRD
	&msm_camera_sensor_ov7692,
#endif
};

static struct platform_device *camera_devices_evb[] __initdata = {
#ifdef CONFIG_OV5647
	&msm_camera_sensor_ov5647,
#endif
#ifdef CONFIG_WEBCAM_OV7692_QRD
	&msm_camera_sensor_ov7692,
#endif
#ifdef CONFIG_OV8825
	&msm_camera_sensor_ov8825,
#endif
};
#endif

enum {
	SX150X_CAM,
};

static struct sx150x_platform_data sx150x_data[] __initdata = {
	[SX150X_CAM]    = {
		.gpio_base	      = GPIO_CAM_EXPANDER_BASE,
		.oscio_is_gpo	   = false,
		.io_pullup_ena	  = 0,
		.io_pulldn_ena	  = 0,
		.io_open_drain_ena      = 0x23,
		.irq_summary	    = -1,
	},
};

static struct i2c_board_info cam_exp_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("sx1508q", 0x22),
		.platform_data  = &sx150x_data[SX150X_CAM],
	},
};

static void __init register_i2c_devices(void)
{
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				cam_exp_i2c_info,
				ARRAY_SIZE(cam_exp_i2c_info));
}

#ifndef CONFIG_MSM_CAMERA_V4L2
#define LCD_CAMERA_LDO_2V8 35 /* SKU1&SKU3 2.8V LDO */
#define SKU3_LCD_CAMERA_LDO_1V8 40 /* SKU3 1.8V LDO */
#define SKU7_LCD_CAMERA_LDO_1V8 58 /* SKU7 1.8V LDO */

static int lcd_camera_ldo_1v8 = SKU3_LCD_CAMERA_LDO_1V8;

static void lcd_camera_power_init(void)
{
	int rc = 0;

	pr_debug("lcd_camera_power_init\n");

	if (machine_is_msm7627a_qrd3() || machine_is_msm8625_qrd7())
		lcd_camera_ldo_1v8 = SKU7_LCD_CAMERA_LDO_1V8;
	else
		lcd_camera_ldo_1v8 = SKU3_LCD_CAMERA_LDO_1V8;

	/* LDO_EXT2V8 */
	if (gpio_request(LCD_CAMERA_LDO_2V8, "lcd_camera_ldo_2v8")) {
		pr_err("failed to request gpio lcd_camera_ldo_2v8\n");
		return;
	}

	rc = gpio_tlmm_config(GPIO_CFG(LCD_CAMERA_LDO_2V8, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable lcd_camera_ldo_2v8!\n", __func__);
		goto fail_gpio2;
	}

	/* LDO_EVT1V8 */
	if (gpio_request(lcd_camera_ldo_1v8, "lcd_camera_ldo_1v8")) {
		pr_err("failed to request gpio lcd_camera_ldo_1v8\n");
		goto fail_gpio2;
	}

	rc = gpio_tlmm_config(GPIO_CFG(lcd_camera_ldo_1v8, 0,
		GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
		GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	if (rc < 0) {
		pr_err("%s: unable to enable lcd_camera_ldo_1v8!\n", __func__);
		goto fail_gpio1;
	}

	return;

fail_gpio1:
	gpio_free(lcd_camera_ldo_1v8);
fail_gpio2:
	gpio_free(LCD_CAMERA_LDO_2V8);

	return;
}

static int lcd_camera_power_on_sku3(void)
{
	int rc = 0;

	pr_debug("turn on sku3 lcd_camera_ldo_1v8\n");
	gpio_set_value_cansleep(lcd_camera_ldo_1v8, 1);

	pr_debug("turn on sku3 lcd_camera_ldo\n");
	gpio_set_value_cansleep(LCD_CAMERA_LDO_2V8, 1);

	return rc;
}

static int lcd_camera_power_off_sku3(void)
{
	int rc = 0;

	pr_debug("turn off sku3 lcd_camera_ldo_1v8\n");
	gpio_set_value_cansleep(lcd_camera_ldo_1v8, 0);

	pr_debug("turn off sku3 lcd_camera_ldo\n");
	gpio_set_value_cansleep(LCD_CAMERA_LDO_2V8, 0);

	gpio_free(lcd_camera_ldo_1v8);
	gpio_free(LCD_CAMERA_LDO_2V8);

	return rc;
}

int lcd_camera_power_onoff(int on)
{
	int rc = 0;

	pr_debug("lcd_camera_power_onoff on = %d,\n", on);

	if (on)
		rc = lcd_camera_power_on_sku3();
	else
		rc = lcd_camera_power_off_sku3();

	return rc;
}
EXPORT_SYMBOL(lcd_camera_power_onoff);
#endif

void camera_af_software_powerdown(struct i2c_client *client)
{
	int rc = 0;
	unsigned char txdata[] = {0x80, 0x0};
	struct i2c_msg msg[] = {
		{
			.addr = 0x18>>1,
			.flags = 0,
			.len = 2,
			.buf = txdata,
		 },
	};
	printk("camera_af_software_powerdown IN...\n");
	/* send software powerdown cmd to AF motor, avoid current leak */
	if (machine_is_msm8625_qrd5()|| machine_is_msm7x27a_qrd5a() || machine_is_msm8625q_evbd() ||
	machine_is_msm8625q_skud() || machine_is_msm8625q_skue()){
		printk("SKUA AF Motor software powerdown, write i2c saddr:0x18, waddr:0x80, wdata:0x00\n");
		rc = i2c_transfer(client->adapter, msg, 1);
		if (rc < 0)
			printk("AF software powerdown faild\n");
		return;
	}
}
EXPORT_SYMBOL(camera_af_software_powerdown);
#ifdef CONFIG_S5K3H2_SUNNY_Q8S02E
int lcd_camera_power_l5_onoff(int on)
{
	static struct regulator *ldo5;
	int rc;

	printk("%s: on = %d\n", __func__, on);
	ldo5 = regulator_get(NULL, "ldo5");
	if (IS_ERR(ldo5)) {
		rc = PTR_ERR(ldo5);
		pr_err("%s: could not get ldo5: %d\n", __func__, rc);
		goto out;
	}

	rc = regulator_set_voltage(ldo5, 1200000, 1200000);
	if (rc) {
		pr_err("%s: could not set ldo5 voltage: %d\n", __func__, rc);
		goto ldo5_free;
	}

	rc = regulator_enable(ldo5);
	if (rc) {
		pr_err("%s: could not enable ldo5: %d\n", __func__, rc);
		goto ldo5_free;
	}

	if (on) {
		rc = regulator_enable(ldo5);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"ldo5", rc);
      goto ldo5_free;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(ldo5);
		if (rc){
			pr_warning("'%s' regulator disable failed, rc=%d\n",
				"ldo5", rc);
      goto ldo5_free;
    }
		pr_debug("%s(off): success\n", __func__);
	}

ldo5_free:
	regulator_put(ldo5);
out:
	ldo5 = NULL;
	return rc;
}
EXPORT_SYMBOL(lcd_camera_power_l5_onoff);
#endif
static struct i2c_board_info odmm_i2c_camera_devices[] = {

  #ifdef CONFIG_WEBCAM_OV7692_QRD
	{
		I2C_BOARD_INFO("ov7692", 0x78),
		.platform_data = &msm_camera_sensor_ov7692_data,
	},
#endif


#ifdef CONFIG_OV5647    //add by zy 20120712
    {
		I2C_BOARD_INFO("ov5647", 0x6c),
		.platform_data = &msm_camera_sensor_ov5647_data,
	},
#endif
#ifdef CONFIG_OV5647_DALING_OVBD631AC   
    {
		I2C_BOARD_INFO("ov5647_dl_bd631ac", 0x6c-1),
		.platform_data = &msm_camera_sensor_ov5647_dl_bd631ac_data,
	},
#endif


#ifdef CONFIG_OV5647_PARTRON_CM500    
    {
		I2C_BOARD_INFO("ov5647_partr_cm500", 0x6c-2),
		.platform_data = &msm_camera_sensor_ov5647_partron_cm500_data,
	},
#endif
#ifdef CONFIG_OV5647_HQ_O9B5_DG806T   
    {
		I2C_BOARD_INFO("ov5647_hq_dg806t", 0x6c-3),
		.platform_data = &msm_camera_sensor_ov5647_hq_dg806t_data,
	},
#endif

};

// lwl modify for EVB1 and other HW board

#ifdef ODMM_PROJECT_STAGE_EVB1
static struct i2c_board_info odmm_i2c_camera_devices_evb1[] = {

#ifdef CONFIG_S5K5CA
	{
		I2C_BOARD_INFO("s5k5ca", 0x78 ),
		.platform_data = &msm_camera_sensor_s5k5ca_data,
	},
#endif
#ifdef CONFIG_HI351_SUNNY_Q3H01B
	{
		I2C_BOARD_INFO("hi351", 0x40),
		.platform_data = &msm_camera_sensor_hi351_sunny_q3h01b_data,
	},
#endif

};

#else


static struct i2c_board_info odmm_i2c_camera_s5k5caga[] = {
	
#ifdef CONFIG_S5K5CA
	{
		I2C_BOARD_INFO("s5k5ca", 0x78 ),
		.platform_data = &msm_camera_sensor_s5k5ca_data,
	},
#endif
	
};

static struct i2c_board_info odmm_i2c_camera_s5k5caga_darling[] = {
	
#ifdef CONFIG_S5K5CA_DARLING
	{
		I2C_BOARD_INFO("s5k5ca_darling", 0x78 ),
		.platform_data = &msm_camera_sensor_s5k5ca_data,
	},
#endif
	
};

#ifdef CONFIG_HI351_SUNNY_Q3H01B
static struct i2c_board_info odmm_i2c_camera_hi351[] = {

	{
		I2C_BOARD_INFO("hi351", 0x40),
		.platform_data = &msm_camera_sensor_hi351_sunny_q3h01b_data,
	},

};
#endif
#endif


void __init msm7627a_camera_init(void)
{

#ifndef CONFIG_MSM_CAMERA_V4L2
	int rc;
#endif

	printk("msm7627a_camera_init Entered\n");

	/* LCD and camera power (VREG & LDO) init */
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb()) {
#ifndef CONFIG_MSM_CAMERA_V4L2
		lcd_camera_power_init();
#endif
		evb_camera_gpio_cfg();
	}else if(machine_is_msm8625_qrd5()|| machine_is_msm7x27a_qrd5a()){
		sku5_camera_gpio_cfg();
	}else if(machine_is_msm8625q_skud()||
			machine_is_msm8625q_evbd())
	{
		skud_camera_gpio_cfg();
	}else if(machine_is_msm8625q_skue())
	{
		skue_camera_gpio_cfg();
	}
#ifdef CONFIG_WEBCAM_OV7692_QRD
	ov7692_camera_gpio_cfg();
#endif
#ifndef CONFIG_MSM_CAMERA_V4L2
	if (machine_is_msm7627a_qrd1()) {
		qrd1_camera_gpio_cfg();
		platform_add_devices(camera_devices_qrd,
				ARRAY_SIZE(camera_devices_qrd));
	} else if (machine_is_msm7627a_evb()
			|| machine_is_msm8625_evb()
			|| machine_is_msm8625_qrd5()
			|| machine_is_msm7x27a_qrd5a()
			|| machine_is_msm7627a_qrd3()
			|| machine_is_msm8625_qrd7()
			|| machine_is_msm8625q_evbd()
			|| machine_is_msm8625q_skud()
			|| machine_is_msm8625q_skue()) {
		platform_add_devices(camera_devices_evb,
				ARRAY_SIZE(camera_devices_evb));
	} else if (machine_is_msm7627a_qrd3())
		return;
	else
		platform_add_devices(camera_devices_msm,
				ARRAY_SIZE(camera_devices_msm));
#endif
	if (!machine_is_msm7627a_qrd1() || !machine_is_msm7627a_evb()
					|| !machine_is_msm8625_evb()
					|| !machine_is_msm8625_qrd5()
					|| !machine_is_msm7x27a_qrd5a()
					|| !machine_is_msm7627a_qrd3()
					|| !machine_is_msm8625_qrd7()
					|| machine_is_msm8625q_evbd()
					|| !machine_is_msm8625q_skud()
					|| !machine_is_msm8625q_skue())
		register_i2c_devices();
#ifndef CONFIG_MSM_CAMERA_V4L2
	rc = regulator_bulk_get(NULL, ARRAY_SIZE(regs_camera), regs_camera);

	if (rc) {
		pr_err("%s: could not get regulators: %d\n", __func__, rc);
		return;
	}

	rc = regulator_bulk_set_voltage(ARRAY_SIZE(regs_camera), regs_camera);

	if (rc) {
		pr_err("%s: could not set voltages: %d\n", __func__, rc);
		return;
	}
#endif

#if defined(CONFIG_MSM_CAMERA_V4L2)
	msm7x27a_init_cam();
#endif
#ifndef CONFIG_MSM_CAMERA_V4L2
	if (machine_is_msm7627a_qrd1()) {
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				i2c_camera_devices_qrd,
				ARRAY_SIZE(i2c_camera_devices_qrd));
	} else if (machine_is_msm7627a_evb()
			|| machine_is_msm8625_evb()
			|| machine_is_msm8625_qrd5()
			|| machine_is_msm7x27a_qrd5a()
			|| machine_is_msm7627a_qrd3()
			|| machine_is_msm8625_qrd7()
			|| machine_is_msm8625q_evbd()
			|| machine_is_msm8625q_skud()
			|| machine_is_msm8625q_skue()) {
		pr_debug("machine_is_msm7627a_evb i2c_register_board_info\n");
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				i2c_camera_devices_evb,
				ARRAY_SIZE(i2c_camera_devices_evb));
	} else
#endif
		pr_debug("i2c_register_board_info\n");
		if (machine_is_msm7627a_evb()|| machine_is_msm8625_evb()){
			pr_debug("i2c_camera_devices_evb registered\n");
			i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
					i2c_camera_devices,
					ARRAY_SIZE(i2c_camera_devices));
		}else if(machine_is_msm8625_qrd5()|| machine_is_msm7x27a_qrd5a()){
			pr_debug("i2c_camera_devices_sku5 registered\n");
			i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
					i2c_camera_devices_sku5,
					ARRAY_SIZE(i2c_camera_devices_sku5));
		}else if(machine_is_msm7627a_qrd3()|| machine_is_msm8625_qrd7()){
			pr_debug("i2c_camera_devices_sku7 registered\n");
			i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
					i2c_camera_devices_sku7,
					ARRAY_SIZE(i2c_camera_devices_sku7));
		}else if(machine_is_msm8625q_skud() || machine_is_msm8625q_evbd())
		{
			printk("i2c_camera_devices_SKUD registered\n");
			i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
					i2c_camera_devices_skud,
					ARRAY_SIZE(i2c_camera_devices_skud));
		}else if(machine_is_msm8625q_skue())
		{
			printk("i2c_camera_devices_SKUE registered\n");
			i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
					i2c_camera_devices_skue,
					ARRAY_SIZE(i2c_camera_devices_skue));
		}
		
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				odmm_i2c_camera_devices,
				ARRAY_SIZE(odmm_i2c_camera_devices));

#ifdef ODMM_PROJECT_STAGE_EVB1
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				odmm_i2c_camera_devices_evb1,
				ARRAY_SIZE(odmm_i2c_camera_devices_evb1));
#else




	gpio_tlmm_config(GPIO_CFG(109, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_4MA), GPIO_CFG_ENABLE);
	 gpio_request(109, "board-camera-id");
	gpio_direction_output(109, 1);
	msleep(20);
	camera_id_status = gpio_get_value(GPIO_SENSOR_ID);
	gpio_direction_output(109, 0);
	gpio_free(109);

	gpio_tlmm_config(GPIO_CFG(GPIO_SENSOR_ID, 0, GPIO_CFG_INPUT, GPIO_CFG_KEEPER,GPIO_CFG_4MA), GPIO_CFG_ENABLE);

	printk("langwenlong&&&&&&&&&&&& #######camera_id_status = %d\n",camera_id_status);

	if  ( camera_id_status ==1) {
			i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				odmm_i2c_camera_s5k5caga,
				ARRAY_SIZE(odmm_i2c_camera_s5k5caga));
		}
		else if ( camera_id_status == 0) {
			i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
				odmm_i2c_camera_s5k5caga_darling,
				ARRAY_SIZE(odmm_i2c_camera_s5k5caga_darling));
		}
		else {

			printk("langwenlong none camera will be registed\n");
		}


#endif

}
