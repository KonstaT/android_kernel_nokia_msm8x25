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
 */
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <linux/i2c.h>
#include <devices-msm7x2xa.h>

#ifdef CONFIG_AVAGO_APDS990X
#include <linux/input/apds990x.h>
#endif

#ifdef CONFIG_SENSORS_TMD2771X
#include <linux/input/tmd2771x.h>
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050
#include <linux/mpu.h>
#endif

#ifdef  CONFIG_INPUT_LTR502
#include <linux/input/ltr502.h>
#endif

#ifdef CONFIG_SENSORS_AK8975
#include <linux/akm8975.h>
#endif

#ifdef CONFIG_INPUT_LTR558
#include <linux/input/ltr5xx.h>

#ifndef LTR558_IRQ_GPIO
#define LTR558_IRQ_GPIO 17
#endif

#endif /* !CONFIG_INPUT_LTR558 */

#ifdef CONFIG_INPUT_ISL29028
#include <linux/input/isl29028.h>

#ifndef ISL29028_IRQ_GPIO
#define ISL29028_IRQ_GPIO 17
#endif

#endif /* !CONFIG_INPUT_ISL29028 */

#ifdef CONFIG_AVAGO_APDS990X
#ifndef APDS990X_IRQ_GPIO
#define APDS990X_IRQ_GPIO 17
#endif

#ifndef APDS990x_PS_DETECTION_THRESHOLD
#define APDS990x_PS_DETECTION_THRESHOLD         600
#endif

#ifndef APDS990x_PS_HSYTERESIS_THRESHOLD
#define APDS990x_PS_HSYTERESIS_THRESHOLD        500
#endif

#ifndef APDS990x_ALS_THRESHOLD_HSYTERESIS
#define APDS990x_ALS_THRESHOLD_HSYTERESIS       20
#endif

#if defined(CONFIG_INPUT_KXTJ9)
#include <linux/input/kxtj9.h>
#endif

static struct msm_gpio apds990x_cfg_data[] = {
        {GPIO_CFG(APDS990X_IRQ_GPIO, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),"apds990x_irq"},
};

static struct apds990x_platform_data apds990x_platformdata = {
        .irq            = MSM_GPIO_TO_INT(APDS990X_IRQ_GPIO),
        .ps_det_thld    = APDS990x_PS_DETECTION_THRESHOLD,
        .ps_hsyt_thld   = APDS990x_PS_HSYTERESIS_THRESHOLD,
        .als_hsyt_thld  = APDS990x_ALS_THRESHOLD_HSYTERESIS,
};

static struct i2c_board_info i2c_info_apds990x = {
        I2C_BOARD_INFO("apds990x", 0x39),
        .platform_data = &apds990x_platformdata,
};

static int apds990x_setup(void)
{
        int retval = 0;

        retval = msm_gpios_request_enable(apds990x_cfg_data, sizeof(apds990x_cfg_data)/sizeof(struct msm_gpio));
        if(retval) {
                printk(KERN_ERR "%s: Failed to obtain L/P sensor interrupt. Code: %d.", __func__, retval);
        }

        i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
                        &i2c_info_apds990x, 1);

        return retval;
}
#endif

#ifdef CONFIG_SENSORS_TMD2771X
#ifndef TMD2771X_IRQ_GPIO
#define TMD2771X_IRQ_GPIO 17
#endif

#ifndef TMD2771X_PS_DETECTION_THRESHOLD
#define TMD2771X_PS_DETECTION_THRESHOLD         600
#endif

#ifndef TMD2771X_PS_HSYTERESIS_THRESHOLD
#define TMD2771X_PS_HSYTERESIS_THRESHOLD        500
#endif

#ifndef TMD2771X_ALS_THRESHOLD_HSYTERESIS
#define TMD2771X_ALS_THRESHOLD_HSYTERESIS       20
#endif

#if defined(CONFIG_INPUT_KXTJ9)
#include <linux/input/kxtj9.h>
#endif

static struct msm_gpio tmd2771x_cfg_data[] = {
        {GPIO_CFG(TMD2771X_IRQ_GPIO, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA),"tmd2771x_irq"},
};

static struct tmd2771x_platform_data tmd2771x_platformdata = {
        .irq            = MSM_GPIO_TO_INT(TMD2771X_IRQ_GPIO),
        .ps_det_thld    = TMD2771X_PS_DETECTION_THRESHOLD,
        .ps_hsyt_thld   = TMD2771X_PS_HSYTERESIS_THRESHOLD,
        .als_hsyt_thld  = TMD2771X_ALS_THRESHOLD_HSYTERESIS,
};

static struct i2c_board_info i2c_info_tmd2771x = {
        I2C_BOARD_INFO("tmd2771x", 0x39),
        .platform_data = &tmd2771x_platformdata,
};

static int tmd2771x_setup(void)
{
        int retval = 0;
		int irbi;
        retval = msm_gpios_request_enable(tmd2771x_cfg_data, sizeof(tmd2771x_cfg_data)/sizeof(struct msm_gpio));
        if(retval) {
                printk(KERN_ERR "%s: Failed to obtain L/P sensor interrupt. Code: %d.", __func__, retval);
        }

        irbi = i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
                        &i2c_info_tmd2771x, 1);
        return retval;
}
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050

#define GPIO_ACC_INT 28
#define GPIO_GYRO_INT 27

/* gyro x and z axis invert for EVB*/
static struct mpu_platform_data mpu3050_data = {
	.int_config  = 0x10,
	.orientation = { -1, 0, 0,
			0, 1, 0,
			0, 0, -1 },
};

/* accel x and z axis invert for EVB */
static struct ext_slave_platform_data inv_mpu_bma250_data = {
	.bus         = EXT_SLAVE_BUS_SECONDARY,
	.orientation = { -1, 0, 0,
			0, 1, 0,
			0, 0, -1 },
};
/* compass  */
static struct ext_slave_platform_data inv_mpu_mmc328xms_data = {
	.bus         = EXT_SLAVE_BUS_PRIMARY,
	.orientation = { -1, 0, 0,
			0, 1, 0,
			0, 0, 1 },
};

/* gyro x and z axis invert for EVT*/
static struct mpu_platform_data mpu3050_data_qrd5 = {
	.int_config  = 0x10,
	.orientation = { 1, 0, 0,
			0, 1, 0,
			0, 0, 1 },
};

/* accel x and z axis invert for EVT */
static struct ext_slave_platform_data inv_mpu_bma250_data_qrd5 = {
	.bus         = EXT_SLAVE_BUS_SECONDARY,
	.orientation = { 1, 0, 0,
			0, 1, 0,
			0, 0, 1 },
};
/* compass for EVT  */
static struct ext_slave_platform_data inv_mpu_mmc328xms_data_qrd5 = {
	.bus         = EXT_SLAVE_BUS_PRIMARY,
	.orientation = { 1, 0, 0,
			0, 1, 0,
			0, 0, -1 },
};

static struct i2c_board_info __initdata mpu3050_boardinfo[] = {
	{
		I2C_BOARD_INFO("mpu3050", 0x68),
		.irq = MSM_GPIO_TO_INT(GPIO_GYRO_INT),
		.platform_data = &mpu3050_data,
	},
	{
		I2C_BOARD_INFO("bma250", 0x18),
		.irq = MSM_GPIO_TO_INT(GPIO_ACC_INT),
		.platform_data = &inv_mpu_bma250_data,
	},
	{
		I2C_BOARD_INFO("mmc328xms", 0x30),
		//.irq = (IH_GPIO_BASE + COMPASS_IRQ_GPIO),
		.platform_data = &inv_mpu_mmc328xms_data,
	},
};

static struct i2c_board_info __initdata mpu3050_boardinfo_qrd5[] = {
	{
		I2C_BOARD_INFO("mpu3050", 0x68),
		.irq = MSM_GPIO_TO_INT(GPIO_GYRO_INT),
		.platform_data = &mpu3050_data_qrd5,
	},
	{
		I2C_BOARD_INFO("bma250", 0x18),
		.irq = MSM_GPIO_TO_INT(GPIO_ACC_INT),
		.platform_data = &inv_mpu_bma250_data_qrd5,
	},
	{
		I2C_BOARD_INFO("mmc328xms", 0x30),
		//.irq = (IH_GPIO_BASE + COMPASS_IRQ_GPIO),
		.platform_data = &inv_mpu_mmc328xms_data_qrd5,
	},
};

static struct msm_gpio mpu3050_gpio_cfg_data[] = {
	{ GPIO_CFG(GPIO_GYRO_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"mpu3050_gyroint" },
	{ GPIO_CFG(GPIO_ACC_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"mpu3050_accint" },
};

static int mpu3050_gpio_setup(void) {
	int ret = 0;
	ret = msm_gpios_request_enable(mpu3050_gpio_cfg_data,
				 sizeof(mpu3050_gpio_cfg_data)/sizeof(struct msm_gpio));
	if( ret<0 )
		printk(KERN_ERR "Failed to obtain mpu3050 int GPIO!\n");
	else
		printk("mpu3050 int GPIO request!\n");
	if(machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a() ) {
		if (ARRAY_SIZE(mpu3050_boardinfo_qrd5))
			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
						mpu3050_boardinfo_qrd5,
						ARRAY_SIZE(mpu3050_boardinfo_qrd5));
	} else {
		if (ARRAY_SIZE(mpu3050_boardinfo))
			i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
						mpu3050_boardinfo,
						ARRAY_SIZE(mpu3050_boardinfo));
	}
	printk("i2c_register_board_info for MPU3050\n");

	return ret;
}
#endif

#ifdef CONFIG_BOSCH_BMA250
static struct i2c_board_info bma250_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("bma250", 0x18),
	},
};
#endif

#ifdef CONFIG_INPUT_ISL29028
static struct isl29028_platform_data isl29028_pdata = {
	.int_gpio = MSM_GPIO_TO_INT(ISL29028_IRQ_GPIO),
};

/* ISL29028 BUS0 ID 0x44 */
static struct i2c_board_info isl29028_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("isl29028", 0x44),
		.platform_data =  &isl29028_pdata
	},
};

static struct msm_gpio isl29028_gpio_cfg_data[] = {
	{GPIO_CFG(ISL29028_IRQ_GPIO, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "isl29028_ALS_PS_int"},
};

static int isl29028_gpio_setup(void) {
	int ret = 0;
	ret = msm_gpios_request_enable(isl29028_gpio_cfg_data, 1);
	if(ret < 0)
		printk(KERN_ERR "%s: Failed to obtain acc int GPIO %d. Code: %d\n",
				__func__, isl29028_pdata.int_gpio, ret);

	return ret;
}

#endif

#ifdef CONFIG_INPUT_LTR558
static struct ltr5xx_platform_data ltr558_pdata = {
	.int_gpio = MSM_GPIO_TO_INT(LTR558_IRQ_GPIO),
};

/* LTR558 BUS ID 0x23 */
static struct i2c_board_info ltr558_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("ltr558", 0x23),
		.platform_data =  &ltr558_pdata
	},
};

static struct msm_gpio ltr558_gpio_cfg_data[] = {
	{GPIO_CFG(LTR558_IRQ_GPIO, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "ltr558_ALS_PS_int"},
};

static int ltr558_gpio_setup(void) {
	int ret = 0;
	ret = msm_gpios_request_enable(ltr558_gpio_cfg_data, 1);
	if(ret < 0)
		printk(KERN_ERR "%s: Failed to obtain acc int GPIO %d. Code: %d\n",
				__func__, ltr558_pdata.int_gpio, ret);

	return ret;
}

#endif /* !CONFIG_INPUT_LTR558 */

#if defined(CONFIG_I2C) && defined(CONFIG_INPUT_LTR502)

static struct ltr502_platform_data ltr502_pdata = {
	.int_gpio = -1,
};

static struct i2c_board_info ltr502_light_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("ltr502", 0x1c),
		.platform_data =  &ltr502_pdata,
	},
};

static struct msm_gpio ltr502_light_gpio_cfg_data[] = {
	{GPIO_CFG(-1, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA), "ltr502_light_int"},
};

static int ltr502_light_gpio_setup(void) {
	int ret = 0;
	ltr502_pdata.int_gpio = 17;
	ltr502_light_gpio_cfg_data[0].gpio_cfg =
                                GPIO_CFG(ltr502_pdata.int_gpio, 0,
                                GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_6MA);
	ret = msm_gpios_request_enable(ltr502_light_gpio_cfg_data, 1);
	if(ret < 0)
		printk(KERN_ERR "%s: Failed to obtain acc int GPIO %d. Code: %d\n",
				__func__, ltr502_pdata.int_gpio, ret);

	return ret;
}
#endif

#ifdef  CONFIG_INPUT_LIS3DH

#define GPIO_ACC_INT 28

static struct i2c_board_info lis3dh_acc_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("lis3dh", 0x19),
		.irq = -1,
	},
};

static struct msm_gpio lis3dh_acc_gpio_cfg_data[] = {
	{
		GPIO_CFG(GPIO_ACC_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"lis3dh_acc_int"
	},
};

static int lis3dh_acc_gpio_setup(void) {
	int ret = 0;
	ret = msm_gpios_request_enable(lis3dh_acc_gpio_cfg_data,
				 sizeof(lis3dh_acc_gpio_cfg_data)/sizeof(struct msm_gpio));
	if( ret<0 )
		printk(KERN_ERR "%s: Failed to obtain acc int GPIO %d. Code: %d\n",
				__func__, GPIO_ACC_INT, ret);
	//lis3dh_acc_i2c_info[0].irq = gpio_to_irq(GPIO_ACC_INT);
	return ret;
}
#endif

#ifdef CONFIG_SENSORS_AK8975

static struct msm_gpio akm_gpio_cfg_data[] = {
	{
		GPIO_CFG(-1, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"akm_int"
	},
};

static int akm_gpio_setup(void) {
	int ret = 0;
	akm_gpio_cfg_data[0].gpio_cfg =
				GPIO_CFG(18, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA);
	ret = msm_gpios_request_enable(akm_gpio_cfg_data,
				 sizeof(akm_gpio_cfg_data)/sizeof(struct msm_gpio));
	return ret;
}

static struct akm8975_platform_data akm_platform_data_8975 = {
		.gpio_DRDY = -1,
};

static struct i2c_board_info akm8975_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("akm8975", 0x0e),
		.platform_data =  &akm_platform_data_8975,
		.flags = I2C_CLIENT_WAKE,
		.irq = -1,//MSM_GPIO_TO_INT(GPIO_COMPASS_DRDY_INDEX),
	},
};
#endif

#if defined(CONFIG_INPUT_KXTJ9)
#define KXTJ9_DEVICE_MAP	7
#define KXTJ9_MAP_X		((KXTJ9_DEVICE_MAP-1)%2)
#define KXTJ9_MAP_Y		(KXTJ9_DEVICE_MAP%2)
#define KXTJ9_NEG_X		(((KXTJ9_DEVICE_MAP+1)/2)%2)
#define KXTJ9_NEG_Y		(((KXTJ9_DEVICE_MAP+5)/4)%2)
#define KXTJ9_NEG_Z		((KXTJ9_DEVICE_MAP-1)/4)

struct kxtj9_platform_data kxtj9_pdata = {
	.min_interval 	= 5,
	.poll_interval 	= 200,

	.axis_map_x 	= KXTJ9_MAP_X,
	.axis_map_y 	= KXTJ9_MAP_Y,
	.axis_map_z 	= 2,

	.negate_x 		= KXTJ9_NEG_X,
	.negate_y 		= KXTJ9_NEG_Y,
	.negate_z 		= KXTJ9_NEG_Z,

	.res_12bit 		= RES_12BIT,
	.g_range  		= KXTJ9_G_2G,
};

static struct i2c_board_info accel_kxtj9_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("kxtik", 0x0F),
		.platform_data = &kxtj9_pdata,
	},
};
#endif // CONFIG_INPUT_KXTJ9

//pjn add for gsensor  
#if defined(CONFIG_SENSORS_BMA2X2)
#define GPIO_ACC_INT 28

static struct msm_gpio bma2x2_gpio_cfg_data[] = {
	{ GPIO_CFG(GPIO_ACC_INT, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_6MA),
		"bma2x2_int" },
};

static struct i2c_board_info accel_bma2x2_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("bma2x2", 0x18),
		.irq=MSM_GPIO_TO_INT(GPIO_ACC_INT),
	},
};

static int bma2x2_gpio_setup(void) {
	int ret = 0;
	ret= msm_gpios_request_enable(bma2x2_gpio_cfg_data,
				 sizeof(bma2x2_gpio_cfg_data)/sizeof(struct msm_gpio));
	if( ret<0 )
		printk(KERN_ERR "%s: Failed to obtain acc int GPIO %d. Code: %d\n",
				__func__, GPIO_ACC_INT, ret);
	return ret;
}

#endif
//end
//pjn add for p/l sensor
#if defined(CONFIG_SENSORS_LTR559)
#define GPIO17_PLSENSOR_INT 17
static struct i2c_board_info ltr559_i2c_info[] __initdata = {
	{
		I2C_BOARD_INFO("ltr559alsps", 0x23),
		.irq = MSM_GPIO_TO_INT(GPIO17_PLSENSOR_INT),
	},
};
#endif

void __init msm7627a_sensor_init(void)
{
#ifdef CONFIG_AVAGO_APDS990X
	if ( machine_is_msm7627a_evb() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {
		pr_info("i2c_register_board_info APDS990X\n");
		apds990x_setup();
	}
#endif

#ifdef CONFIG_SENSORS_TMD2771X
	if (machine_is_msm8625q_skue()) {
		pr_info("i2c_register_board_info TMD2771X\n");
		tmd2771x_setup();
	}
#endif

#ifdef CONFIG_MPU_SENSORS_MPU3050
	if (machine_is_msm7627a_evb() || machine_is_msm8625_evb() || machine_is_msm8625_qrd5() || machine_is_msm7x27a_qrd5a()) {
		pr_info("i2c_register_board_info MPU3050\n");
		mpu3050_gpio_setup();
	}
#endif

#ifdef CONFIG_BOSCH_BMA250
	if (machine_is_msm8625_qrd7() || machine_is_msm7627a_qrd3() || machine_is_msm8625q_skud()) {
		pr_info("i2c_register_board_info BMA250 ACC\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					bma250_i2c_info,
					ARRAY_SIZE(bma250_i2c_info));
	}
#endif

#ifdef CONFIG_INPUT_ISL29028
	if (machine_is_msm8625q_skud()) {
		pr_info("i2c_register_board_info ISL29028 ALP sensor!\n");
		isl29028_gpio_setup();
		i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
					isl29028_i2c_info,
					ARRAY_SIZE(isl29028_i2c_info));
	}
#endif

#ifdef CONFIG_INPUT_LTR558
	if (machine_is_msm8625q_skue()) {
		pr_info("i2c_register_board_info LTR558 ALP sensor!\n");
		ltr558_gpio_setup();
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					ltr558_i2c_info,
					ARRAY_SIZE(ltr558_i2c_info));
	}
#endif

#ifdef CONFIG_INPUT_LIS3DH
	if (machine_is_msm8625q_skue()) {
		lis3dh_acc_gpio_setup();
		pr_info("i2c_register_board_info LIS3DH ACC\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
					lis3dh_acc_i2c_info,
					ARRAY_SIZE(lis3dh_acc_i2c_info));
	}
#endif

#ifdef CONFIG_INPUT_LTR502
	if (machine_is_msm8625_qrd7() || machine_is_msm7627a_qrd3()) {
		pr_info("i2c_register_board_info LTR502\n");
		ltr502_light_gpio_setup();
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				ltr502_light_i2c_info,
				ARRAY_SIZE(ltr502_light_i2c_info));
	}
#endif

#ifdef CONFIG_SENSORS_AK8975
	if (machine_is_msm8625_qrd7() || machine_is_msm7627a_qrd3() || machine_is_msm8625q_skud()) {
		pr_info("i2c_register_board_info AKM8975\n");
		akm_gpio_setup();
		akm_platform_data_8975.gpio_DRDY = 18;
		akm8975_i2c_info[0].irq = gpio_to_irq(akm_platform_data_8975.gpio_DRDY);
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
				akm8975_i2c_info,
				ARRAY_SIZE(akm8975_i2c_info));
	}
#endif

#ifdef CONFIG_INPUT_KXTJ9
	if(machine_is_msm8625_skua()) {
	pr_info("i2c_register_board_info KXTJ9\n");
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			accel_kxtj9_i2c_info,
			ARRAY_SIZE(accel_kxtj9_i2c_info));
	}
#endif

#ifdef CONFIG_SENSORS_BMA2X2
	//if (machine_is_msm8625_qrd7() || machine_is_msm7627a_qrd3()) {
	pr_info("i2c_register_board_info BMA2X2\n");
        bma2x2_gpio_setup	();
		i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			accel_bma2x2_i2c_info,
			ARRAY_SIZE(accel_bma2x2_i2c_info));
	//}
#endif

#ifdef CONFIG_SENSORS_LTR559
	pr_info("i2c_register_board_info LTR559\n");
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			ltr559_i2c_info,
			ARRAY_SIZE(ltr559_i2c_info));
#endif

}
