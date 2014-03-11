/* Lite-On LTR-559ALS Android / Linux Driver
 *
 * Copyright (C) 2013 Lite-On Technology Corp (Singapore)
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */


#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/gfp.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/pm.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/wakelock.h>
#include <linux/workqueue.h>
#include <asm/uaccess.h>
#include <asm/mach-types.h>
#include <asm/setup.h>
#include <linux/version.h>


/* LTR-559 Registers */
#define LTR559_ALS_CONTR	0x80
#define LTR559_PS_CONTR		0x81
#define LTR559_PS_LED		0x82
#define LTR559_PS_N_PULSES	0x83
#define LTR559_PS_MEAS_RATE	0x84
#define LTR559_ALS_MEAS_RATE	0x85
#define LTR559_PART_ID		0x86
#define LTR559_MANUFACTURER_ID	0x87
#define LTR559_ALS_DATA_CH1_0	0x88
#define LTR559_ALS_DATA_CH1_1	0x89
#define LTR559_ALS_DATA_CH0_0	0x8A
#define LTR559_ALS_DATA_CH0_1	0x8B
#define LTR559_ALS_PS_STATUS	0x8C
#define LTR559_PS_DATA_0	0x8D
#define LTR559_PS_DATA_1	0x8E
#define LTR559_INTERRUPT	0x8F
#define LTR559_PS_THRES_UP_0	0x90
#define LTR559_PS_THRES_UP_1	0x91
#define LTR559_PS_THRES_LOW_0	0x92
#define LTR559_PS_THRES_LOW_1	0x93
#define LTR559_PS_OFFSET_1	0x94
#define LTR559_PS_OFFSET_0	0x95
#define LTR559_ALS_THRES_UP_0	0x97
#define LTR559_ALS_THRES_UP_1	0x98
#define LTR559_ALS_THRES_LOW_0	0x99
#define LTR559_ALS_THRES_LOW_1	0x9A
#define LTR559_INTERRUPT_PRST	0x9E
/* LTR-559 Registers */


#define SET_BIT 1
#define CLR_BIT 0

#define ALS 0
#define PS 1
#define ALSPS 2
#define PS_W_SATURATION_BIT	3

/* address 0x80 */
#define ALS_MODE_ACTIVE	(1 << 0)
#define ALS_MODE_STDBY		(0 << 0)
#define ALS_SW_RESET		(1 << 1)
#define ALS_SW_NRESET		(0 << 1)
#define ALS_GAIN_1x		(0 << 2)
#define ALS_GAIN_2x		(1 << 2)
#define ALS_GAIN_4x		(2 << 2)
#define ALS_GAIN_8x		(3 << 2)
#define ALS_GAIN_48x	(6 << 2)
#define ALS_GAIN_96x	(7 << 2)
#define ALS_MODE_RDBCK			0
#define ALS_SWRT_RDBCK			1
#define ALS_GAIN_RDBCK			2
#define ALS_CONTR_RDBCK		3

/* address 0x81 */
#define PS_MODE_ACTIVE		(3 << 0)
#define PS_MODE_STDBY		(0 << 0)
#define PS_GAIN_16x			(0 << 2)
#define PS_GAIN_32x			(2 << 2)
#define PS_GAIN_64x			(3 << 2)
#define PS_SATUR_INDIC_EN	(1 << 5)
#define PS_SATU_INDIC_DIS	(0 << 5)
#define PS_MODE_RDBCK		0
#define PS_GAIN_RDBCK		1
#define PS_SATUR_RDBCK		2
#define PS_CONTR_RDBCK		3

/* address 0x82 */
#define LED_CURR_5MA		(0 << 0)
#define LED_CURR_10MA		(1 << 0)
#define LED_CURR_20MA		(2 << 0)
#define LED_CURR_50MA		(3 << 0)
#define LED_CURR_100MA		(4 << 0)
#define LED_CURR_DUTY_25PC		(0 << 3)
#define LED_CURR_DUTY_50PC		(1 << 3)
#define LED_CURR_DUTY_75PC		(2 << 3)
#define LED_CURR_DUTY_100PC	(3 << 3)
#define LED_PUL_FREQ_30KHZ		(0 << 5)
#define LED_PUL_FREQ_40KHZ		(1 << 5)
#define LED_PUL_FREQ_50KHZ		(2 << 5)
#define LED_PUL_FREQ_60KHZ		(3 << 5)
#define LED_PUL_FREQ_70KHZ		(4 << 5)
#define LED_PUL_FREQ_80KHZ		(5 << 5)
#define LED_PUL_FREQ_90KHZ		(6 << 5)
#define LED_PUL_FREQ_100KHZ	(7 << 5)
#define LED_CURR_RDBCK			0
#define LED_CURR_DUTY_RDBCK	1
#define LED_PUL_FREQ_RDBCK		2
#define PS_LED_RDBCK			3

/* address 0x84 */
#define PS_MEAS_RPT_RATE_50MS		(0 << 0)
#define PS_MEAS_RPT_RATE_70MS		(1 << 0)
#define PS_MEAS_RPT_RATE_100MS	(2 << 0)
#define PS_MEAS_RPT_RATE_200MS	(3 << 0)
#define PS_MEAS_RPT_RATE_500MS	(4 << 0)
#define PS_MEAS_RPT_RATE_1000MS	(5 << 0)
#define PS_MEAS_RPT_RATE_2000MS	(6 << 0)
#define PS_MEAS_RPT_RATE_10MS		(8 << 0)

/* address 0x85 */
#define ALS_MEAS_RPT_RATE_50MS	(0 << 0)
#define ALS_MEAS_RPT_RATE_100MS	(1 << 0)
#define ALS_MEAS_RPT_RATE_200MS	(2 << 0)
#define ALS_MEAS_RPT_RATE_500MS	(3 << 0)
#define ALS_MEAS_RPT_RATE_1000MS	(4 << 0)
#define ALS_MEAS_RPT_RATE_2000MS	(5 << 0)
#define ALS_INTEG_TM_100MS		(0 << 3)
#define ALS_INTEG_TM_50MS			(1 << 3)
#define ALS_INTEG_TM_200MS		(2 << 3)
#define ALS_INTEG_TM_400MS		(3 << 3)
#define ALS_INTEG_TM_150MS		(4 << 3)
#define ALS_INTEG_TM_250MS		(5 << 3)
#define ALS_INTEG_TM_300MS		(6 << 3)
#define ALS_INTEG_TM_350MS		(7 << 3)
#define ALS_MEAS_RPT_RATE_RDBCK	0
#define ALS_INTEG_TM_RDBCK			1
#define ALS_MEAS_RATE_RDBCK		2

/* address 0x86 */
#define PART_NUM_ID_RDBCK		0
#define REVISION_ID_RDBCK		1
#define PART_ID_REG_RDBCK		2

/* address 0x8C */
#define PS_DATA_STATUS_RDBCK		0
#define PS_INTERR_STATUS_RDBCK	1
#define ALS_DATA_STATUS_RDBCK		2
#define ALS_INTERR_STATUS_RDBCK	3
#define ALS_GAIN_STATUS_RDBCK		4
#define ALS_VALID_STATUS_RDBCK	5
#define ALS_PS_STATUS_RDBCK		6

/* address 0x8F */
#define INT_MODE_00					(0 << 0)
#define INT_MODE_PS_TRIG			(1 << 0)
#define INT_MODE_ALS_TRIG			(2 << 0)
#define INT_MODE_ALSPS_TRIG		(3 << 0)
#define INT_POLAR_ACT_LO			(0 << 2)
#define INT_POLAR_ACT_HI			(1 << 2)
#define INT_MODE_RDBCK				0
#define INT_POLAR_RDBCK			1
#define INT_INTERRUPT_RDBCK		2

/* address 0x9E */
#define ALS_PERSIST_SHIFT	0
#define PS_PERSIST_SHIFT	4
#define ALS_PRST_RDBCK		0
#define PS_PRST_RDBCK		1
#define ALSPS_PRST_RDBCK	2

#define PON_DELAY		600

#define ALS_MIN_MEASURE_VAL	0
#define ALS_MAX_MEASURE_VAL	65535
#define ALS_VALID_MEASURE_MASK	ALS_MAX_MEASURE_VAL
#define PS_MIN_MEASURE_VAL	0
#define PS_MAX_MEASURE_VAL	2047
#define PS_VALID_MEASURE_MASK   PS_MAX_MEASURE_VAL
#define LO_LIMIT			0
#define HI_LIMIT			1
#define LO_N_HI_LIMIT	2
#define PS_OFFSET_MIN_VAL		0
#define PS_OFFSET_MAX_VAL		1023

#define DRIVER_VERSION "1.00"
#define PARTID 0x92
#define MANUID 0x05

#define I2C_RETRY 5

#define DEVICE_NAME "ltr559alsps"
#define GPIO17_PLSENSOR_INT 17
#define DEBUG

#define PS_SET_LOWTHRESH           (1150)	
#define PS_SET_HIGHTHRESH          (1200)


/*
 * Magic Number
 * ============
 * Refer to file ioctl-number.txt for allocation
 */
#define LTR559_IOCTL_MAGIC      'c'

/* IOCTLs for ltr559 device */
#define LTR559_IOCTL_PS_ENABLE		_IOR(LTR559_IOCTL_MAGIC, 1, int *)
#define LTR559_IOCTL_PS_GET_ENABLED	_IOW(LTR559_IOCTL_MAGIC, 2, int *)
#define LTR559_IOCTL_ALS_ENABLE		_IOR(LTR559_IOCTL_MAGIC, 3, int *)
#define LTR559_IOCTL_ALS_GET_ENABLED	_IOW(LTR559_IOCTL_MAGIC, 4, int *)

//pjn add for psensor calibration
uint16_t  ps_set_lowthresh = PS_SET_LOWTHRESH;
uint16_t  ps_set_highthresh = PS_SET_HIGHTHRESH; 

//(Linux RTOS)>
#if 0
struct ltr559_platform_data {
	/* ALS */
	uint16_t pfd_levels[5];
	uint16_t pfd_als_lowthresh;
	uint16_t pfd_als_highthresh;
	int pfd_disable_als_on_suspend;

	/* PS */
	uint16_t pfd_ps_lowthresh;
	uint16_t pfd_ps_highthresh;
	int pfd_disable_ps_on_suspend;

	/* Interrupt */
	int pfd_gpio_int_no;
};
#endif
//(Linux RTOS)<


struct ltr559_data {
	/* Device */
	struct i2c_client *i2c_client;
	struct input_dev *als_input_dev;
	struct input_dev *ps_input_dev;
	struct workqueue_struct *workqueue;
	struct early_suspend early_suspend;
	struct wake_lock ps_wake_lock;

	/* Device mode
	 * 0 = ALS
	 * 1 = PS
	 */
	uint8_t mode;

	/* ALS */
	uint8_t als_enable_flag;
	uint8_t als_suspend_enable_flag;
	uint8_t als_irq_flag;
	uint8_t als_opened;
	uint16_t als_lowthresh;
	uint16_t als_highthresh;
	uint16_t default_als_lowthresh;
	uint16_t default_als_highthresh;
	uint16_t *adc_levels;
	/* Flag to suspend ALS on suspend or not */
	uint8_t disable_als_on_suspend;

	/* PS */
	uint8_t ps_enable_flag;
	uint8_t ps_suspend_enable_flag;
	uint8_t ps_irq_flag;
	uint8_t ps_opened;
	uint16_t ps_lowthresh;
	uint16_t ps_highthresh;
	uint16_t default_ps_lowthresh;
	uint16_t default_ps_highthresh;
	/* Flag to suspend PS on suspend or not */
	uint8_t disable_ps_on_suspend;
	unsigned int ps_direction;		/* 0 = near-to-far; 1 = far-to-near */

	/* LED */
	int led_pulse_freq;
	int led_duty_cyc;
	int led_peak_curr;
	int led_pulse_count;

	/* Interrupt */
	int irq;
	int gpio_int_no;
	int is_suspend;
};

struct ltr559_data *sensor_info;


/* I2C Read */
static int8_t I2C_Read(uint8_t *rxData, uint8_t length)
{
	int8_t index;
	struct i2c_msg data[] = {
		{
			.addr = sensor_info->i2c_client->addr,
			.flags = 0,
			.len = 1,
			.buf = rxData,
		},
		{
			.addr = sensor_info->i2c_client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = rxData,
		},
	};

	for (index = 0; index < I2C_RETRY; index++) {
		if (i2c_transfer(sensor_info->i2c_client->adapter, data, 2) > 0)
			break;

		mdelay(10);
	}

	if (index >= I2C_RETRY) {
		pr_alert("%s I2C Read Fail !!!!\n",__func__);
		return -EIO;
	}

	return 0;
}


/* I2C Write */
static int8_t I2C_Write(uint8_t *txData, uint8_t length)
{
	int8_t index;
	struct i2c_msg data[] = {
		{
			.addr = sensor_info->i2c_client->addr,
			.flags = 0,
			.len = length,
			.buf = txData,
		},
	};

	for (index = 0; index < I2C_RETRY; index++) {
		if (i2c_transfer(sensor_info->i2c_client->adapter, data, 1) > 0)
			break;

		mdelay(10);
	}

	if (index >= I2C_RETRY) {
		pr_alert("%s I2C Write Fail !!!!\n", __func__);
		return -EIO;
	}

	return 0;
}


/* Set register bit */
static int8_t _ltr559_set_bit(struct i2c_client *client, uint8_t set, uint8_t cmd, uint8_t data)
{
	uint8_t buffer[2];
	uint8_t value;
	int8_t ret = 0;

	buffer[0] = cmd;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (set)
		value |= data;
	else
		value &= ~data;

	buffer[0] = cmd;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return -EIO;
	}

	return ret;
}


static uint16_t lux_formula(uint16_t ch0_adc, uint16_t ch1_adc)
{
	uint16_t luxval = 0;
	int ch0_coeff = 0;
	int ch1_coeff = 0;
	uint16_t ch0_calc;
	int ratio;
	int8_t ret; 
	uint8_t buffer[2];

	buffer[0] = LTR559_ALS_CONTR;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		pr_alert("%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	ch0_calc = ch0_adc;
#if 0
	if ((buffer[0] & 0x20) == 0x20) {
		ch0_calc = ch0_adc - ch1_adc;
	}
#endif
	if ((ch1_adc + ch0_calc) == 0) {
		ratio = 100;
	} else {
		ratio = (100 * ch1_adc)/(ch1_adc + ch0_calc);		
	}

	if (ratio < 45) {
		ch0_coeff = 17743;
		ch1_coeff = 11059;
	} else if ((ratio >= 45) && (ratio < 64)) {
		ch0_coeff = 42785;
		ch1_coeff = -19548;
	} else if ((ratio >= 64) && (ratio < 85)) {
		ch0_coeff = 5926;
		ch1_coeff = 1185;
	} else if (ratio >= 85) {
		ch0_coeff = 0;
		ch1_coeff = 0;
	}

	luxval = ((ch0_calc * ch0_coeff) + (ch1_adc * ch1_coeff))/10000;
	return luxval;
}


/* Read ADC Value */
static uint16_t read_adc_value(struct ltr559_data *ltr559)
{

	int8_t ret = -99;
	uint16_t value = -99;
	uint16_t ps_val;
	int ch0_val;
	int ch1_val;
	
	uint8_t buffer[4];

	switch (ltr559->mode) {
		case 0 :
			/* ALS */
			buffer[0] = LTR559_ALS_DATA_CH1_0;

			/* read data bytes from data regs */
			ret = I2C_Read(buffer, 4);
			break;

		case 1 :
		case 3 : /* PS with saturation bit */
			/* PS */
			buffer[0] = LTR559_PS_DATA_0;

			/* read data bytes from data regs */
			ret = I2C_Read(buffer, 2);
			break;
	}

	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}


	switch (ltr559->mode) {
		case 0 :
			/* ALS Ch0 */
		 	ch0_val = (uint16_t)buffer[2] | ((uint16_t)buffer[3] << 8);
				dev_dbg(&ltr559->i2c_client->dev, 
					"%s | als_ch0 value = 0x%04X\n", __func__, 
					ch0_val);

			if (ch0_val > ALS_MAX_MEASURE_VAL) {
				dev_err(&ltr559->i2c_client->dev,
				        "%s: ALS Value Error: 0x%X\n", __func__,
				        ch0_val);
			}
			ch0_val &= ALS_VALID_MEASURE_MASK;

			/* ALS Ch1 */
		 	ch1_val = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
				dev_dbg(&ltr559->i2c_client->dev, 
					"%s | als_ch1 value = 0x%04X\n", __func__, 
					ch1_val);

			if (ch1_val > ALS_MAX_MEASURE_VAL) {
				dev_err(&ltr559->i2c_client->dev,
				        "%s: ALS Value Error: 0x%X\n", __func__,
				        ch1_val);
			}
			ch1_val &= ALS_VALID_MEASURE_MASK;

			/* ALS Lux Conversion */
			value = lux_formula(ch0_val, ch1_val);

			break;

		case 1 :	
		case 3 : /* PS with saturation bit */
			/* PS */
			ps_val = (uint16_t)buffer[0] | ((uint16_t)buffer[1] << 8);
				dev_dbg(&ltr559->i2c_client->dev, 
					"%s | ps value = 0x%04X\n", __func__, 
					ps_val);

			if (ltr559->mode == 1) {
				if (ps_val > PS_MAX_MEASURE_VAL) {
					dev_err(&ltr559->i2c_client->dev,
					        "%s: PS Value Error: 0x%X\n", __func__,
					        ps_val);
				}
				ps_val &= PS_VALID_MEASURE_MASK;				
			} else if (ltr559->mode == 3) {
				ps_val &= 0x87FF;
			}			
			
			value = ps_val;

			break;		
			
	}

	return value;
}

static int8_t als_mode_setup (uint8_t alsMode_set_reset, struct ltr559_data *ltr559)
{
	int8_t ret = 0;

	ret = _ltr559_set_bit(ltr559->i2c_client, alsMode_set_reset, LTR559_ALS_CONTR, ALS_MODE_ACTIVE);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s ALS mode setup fail...\n", __func__);
		return ret;
	}

	return ret;
}

static int8_t als_contr_setup(uint8_t als_contr_val, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR559_ALS_CONTR;

	/* Default settings used for now. */
	buffer[1] = als_contr_val;
	buffer[1] &= 0x1F;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | ALS_CONTR (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}


static int8_t als_contr_readback (uint8_t rdbck_type, uint8_t *retVal, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR559_ALS_CONTR;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (rdbck_type == ALS_MODE_RDBCK) {
		*retVal = value & 0x01;
	} else if (rdbck_type == ALS_SWRT_RDBCK) {
		*retVal = (value & 0x02) >> 1;
	} else if (rdbck_type == ALS_GAIN_RDBCK) {
		*retVal = (value & 0x1C) >> 2;
	} else if (rdbck_type == ALS_CONTR_RDBCK) {
		*retVal = value & 0x1F;
	}

	return ret;
}

static int8_t ps_mode_setup (uint8_t psMode_set_reset, struct ltr559_data *ltr559)
{
	int8_t ret = 0;

	ret = _ltr559_set_bit(ltr559->i2c_client, psMode_set_reset, LTR559_PS_CONTR, PS_MODE_ACTIVE);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s PS mode setup fail...\n", __func__);
		return ret;
	}

	return ret;
}

static int8_t ps_contr_setup(uint8_t ps_contr_val, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR559_PS_CONTR;

	/* Default settings used for now. */
	buffer[1] = ps_contr_val;
	buffer[1] &= 0x2F;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | PS_CONTR (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}


static int8_t ps_contr_readback (uint8_t rdbck_type, uint8_t *retVal, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR559_PS_CONTR;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (rdbck_type == PS_MODE_RDBCK) {
		*retVal = (value & 0x03);
	} else if (rdbck_type == PS_GAIN_RDBCK) {
		*retVal = (value & 0x0C) >> 2;
	} else if (rdbck_type == PS_SATUR_RDBCK) {
		*retVal = (value & 0x20) >> 5;
	} else if (rdbck_type == PS_CONTR_RDBCK) {
		*retVal = value & 0x2F;
	}

	return ret;
}


/* LED Setup */
static int8_t ps_led_setup(uint8_t ps_led_val, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR559_PS_LED;

	/* Default settings used for now. */
	buffer[1] = ps_led_val;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | PS_LED (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}

static int8_t ps_ledPulseCount_setup(uint8_t pspulsecount_val, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR559_PS_N_PULSES;

	/* Default settings used for now. */
	if (pspulsecount_val > 15) {
		pspulsecount_val = 15;
	}
	buffer[1] = pspulsecount_val;
	buffer[1] &= 0x0F;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | PS_LED_COUNT (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}

static int8_t ps_meas_rate_setup(uint16_t meas_rate_val, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR559_PS_MEAS_RATE;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	value &= 0xF0;

	if (meas_rate_val == 50) {
		value |= PS_MEAS_RPT_RATE_50MS;
	} else if (meas_rate_val == 70) {
		value |= PS_MEAS_RPT_RATE_70MS;
	} else if (meas_rate_val == 100) {
		value |= PS_MEAS_RPT_RATE_100MS;
	} else if (meas_rate_val == 200) {
		value |= PS_MEAS_RPT_RATE_200MS;
	} else if (meas_rate_val == 500) {
		value |= PS_MEAS_RPT_RATE_500MS;
	} else if (meas_rate_val == 1000) {
		value |= PS_MEAS_RPT_RATE_1000MS;
	} else if (meas_rate_val == 2000) {
		value |= PS_MEAS_RPT_RATE_2000MS;
	} else if (meas_rate_val == 10) {
		value |= PS_MEAS_RPT_RATE_10MS;		
	}

	buffer[0] = LTR559_PS_MEAS_RATE;
	buffer[1] = value;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s PS measurement rate setup fail...\n", __func__);
		return ret;
	}

	return ret;
}

static int8_t als_meas_rate_reg_setup(uint8_t als_meas_rate_reg_val, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[3];

	buffer[0] = LTR559_ALS_MEAS_RATE;

	buffer[1] = als_meas_rate_reg_val;
	buffer[1] &= 0x3F;
	ret = I2C_Write(buffer, 2);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | ALS_MEAS_RATE (0x%02X) setup fail...", __func__, buffer[0]);
	}

	return ret;
}

static int8_t part_ID_reg_readback (uint8_t rdbck_type, uint8_t *retVal, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[1], value;

	buffer[0] = LTR559_PART_ID;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (rdbck_type == PART_NUM_ID_RDBCK) {
		*retVal = (value & 0xF0) >> 4;
	} else if (rdbck_type == REVISION_ID_RDBCK) {
		*retVal = value & 0x0F;
	} else if (rdbck_type == PART_ID_REG_RDBCK) {
		*retVal = value;
	}

	return ret;
}


static int8_t manu_ID_reg_readback (uint8_t *retVal, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[1], value;

	buffer[0] = LTR559_MANUFACTURER_ID;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];
	*retVal = value;

	return ret;
}


static int8_t als_ps_status_reg (uint8_t data_status_type, uint8_t *retVal, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[2], value;

	buffer[0] = LTR559_ALS_PS_STATUS;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value = buffer[0];

	if (data_status_type == PS_DATA_STATUS_RDBCK) {
		*retVal = (value & 0x01);
	} else if (data_status_type == PS_INTERR_STATUS_RDBCK) {
		*retVal = (value & 0x02) >> 1;
	} else if (data_status_type == ALS_DATA_STATUS_RDBCK) {
		*retVal = (value & 0x04) >> 2;
	} else if (data_status_type == ALS_INTERR_STATUS_RDBCK) {
		*retVal = (value & 0x08) >> 3;
	} else if (data_status_type == ALS_GAIN_STATUS_RDBCK) {
		*retVal = (value & 0x70) >> 4;
	} else if (data_status_type == ALS_VALID_STATUS_RDBCK) {
		*retVal = (value & 0x80) >> 7;
	} else if (data_status_type == ALS_PS_STATUS_RDBCK) {
		*retVal = value;
	}

	return ret;
}

/* Set ALS range */
static int8_t set_als_range(uint16_t lt, uint16_t ht, uint8_t lo_hi)
{
	int8_t ret;
	uint8_t buffer[5], num_data = 0;

	if (lo_hi == LO_LIMIT) {
		buffer[0] = LTR559_ALS_THRES_LOW_0;
		buffer[1] = lt & 0xFF;
		buffer[2] = (lt >> 8) & 0xFF;
		num_data = 3;
	} else if (lo_hi == HI_LIMIT) {
		buffer[0] = LTR559_ALS_THRES_UP_0;
		buffer[1] = ht & 0xFF;
		buffer[2] = (ht >> 8) & 0xFF;
		num_data = 3;
	} else if (lo_hi == LO_N_HI_LIMIT) {
		buffer[0] = LTR559_ALS_THRES_UP_0;
		buffer[1] = ht & 0xFF;
		buffer[2] = (ht >> 8) & 0xFF;
		buffer[3] = lt & 0xFF;
		buffer[4] = (lt >> 8) & 0xFF;
		num_data = 5;
	}	

	ret = I2C_Write(buffer, num_data);
	if (ret <0) {
		pr_alert("%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}
	dev_dbg(&sensor_info->i2c_client->dev, "%s Set als range:0x%04x"
	                                       " - 0x%04x\n", __func__, lt, ht);

	return ret;
}


static int8_t als_range_readback (uint16_t *lt, uint16_t *ht, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[5];
	uint16_t value_lo, value_hi;

	buffer[0] = LTR559_ALS_THRES_UP_0;
	ret = I2C_Read(buffer, 4);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value_lo = buffer[3];
	value_lo <<= 8;
	value_lo += buffer[2];
	*lt = value_lo;

	value_hi = buffer[1];
	value_hi <<= 8;
	value_hi += buffer[0];
	*ht = value_hi;

	return ret;
}


/* Set PS range */
static int8_t set_ps_range(uint16_t lt, uint16_t ht, uint8_t lo_hi)
{
	int8_t ret;
	uint8_t buffer[5], num_data = 0;

	if (lo_hi == LO_LIMIT) {
		buffer[0] = LTR559_PS_THRES_LOW_0;
		buffer[1] = lt & 0xFF;
		buffer[2] = (lt >> 8) & 0x07;
		num_data = 3;
	} else if (lo_hi == HI_LIMIT) {
		buffer[0] = LTR559_PS_THRES_UP_0;
		buffer[1] = ht & 0xFF;
		buffer[2] = (ht >> 8) & 0x07;
		num_data = 3;
	} else if (lo_hi == LO_N_HI_LIMIT) {
		buffer[0] = LTR559_PS_THRES_UP_0;
		buffer[1] = ht & 0xFF;
		buffer[2] = (ht >> 8) & 0x07;
		buffer[3] = lt & 0xFF;
		buffer[4] = (lt >> 8) & 0x07;
		num_data = 5;
	}	

	ret = I2C_Write(buffer, num_data);
	if (ret <0) {
		pr_alert("%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}
	dev_dbg(&sensor_info->i2c_client->dev, "%s Set ps range:0x%04x"
	                                       " - 0x%04x\n", __func__, lt, ht);

	return ret;
}


static int8_t ps_range_readback (uint16_t *lt, uint16_t *ht, struct ltr559_data *ltr559)
{
	int8_t ret = 0;
	uint8_t buffer[5];
	uint16_t value_lo, value_hi;

	buffer[0] = LTR559_PS_THRES_UP_0;
	ret = I2C_Read(buffer, 4);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return ret;
	}

	value_lo = buffer[3];
	value_lo <<= 8;
	value_lo += buffer[2];
	*lt = value_lo;

	value_hi = buffer[1];
	value_hi <<= 8;
	value_hi += buffer[0];
	*ht = value_hi;

	return ret;
}


//(Linux RTOS)>
/* Report PS input event */
static void report_ps_input_event(struct ltr559_data *ltr559)
{
	//int8_t ret;
	uint16_t adc_value;
	//int thresh_hi, thresh_lo, thresh_delta;

	ltr559->mode = PS;
	adc_value = read_adc_value (ltr559);

	if (adc_value > ps_set_highthresh){
		ltr559->ps_direction = 0;//1;               //far to near
		/*
		ret = set_ps_range(PS_SET_LOWTHRESH, PS_MAX_MEASURE_VAL, LO_N_HI_LIMIT);
		if (ret < 0) {
			dev_err(&ltr559->i2c_client->dev, "%s : PS thresholds setting Fail...\n", __func__);
		}*/
	}else if (adc_value < ps_set_lowthresh){
		ltr559->ps_direction = 1;//0;               //near to far 
		/*
		ret = set_ps_range(0, PS_SET_HIGHTHRESH, LO_N_HI_LIMIT);
		if (ret < 0) {
			dev_err(&ltr559->i2c_client->dev, "%s : PS thresholds setting Fail...\n", __func__);
		}*/
	}
	//printk("%s,adc_val:%d,hithresh:%d,lowthresh:%d,report_val:%d\n",__func__,
	//	adc_value,ps_set_highthresh,ps_set_lowthresh, ltr559->ps_direction);	
	input_report_abs(ltr559->ps_input_dev, ABS_DISTANCE, ltr559->ps_direction);
	input_sync(ltr559->ps_input_dev);	
	
	//input_report_abs(ltr559->ps_input_dev, ABS_DISTANCE, adc_value);
	//input_sync(ltr559->ps_input_dev);
	
	/* Adjust measurement range using a crude filter to prevent interrupt jitter */
	/*
	thresh_delta = (adc_value >> 10)+2;
	thresh_lo = adc_value - thresh_delta;
	thresh_hi = adc_value + thresh_delta;
	if (thresh_lo < PS_MIN_MEASURE_VAL) {
		thresh_lo = PS_MIN_MEASURE_VAL;
	}
	if (thresh_hi > PS_MAX_MEASURE_VAL) {
		thresh_hi = PS_MAX_MEASURE_VAL;
	}

	ret = set_ps_range((uint16_t)thresh_lo, (uint16_t)thresh_hi, LO_N_HI_LIMIT);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s : PS thresholds setting Fail...\n", __func__);
	}
	*/

	

}


/* Report ALS input event */
static void report_als_input_event(struct ltr559_data *ltr559)
{
	int8_t ret;
	uint16_t adc_value;
	int thresh_hi, thresh_lo, thresh_delta;

	ltr559->mode = ALS;
	adc_value = read_adc_value (ltr559);
	input_report_abs(ltr559->als_input_dev, ABS_MISC, adc_value);
	input_sync(ltr559->als_input_dev);

	/* Adjust measurement range using a crude filter to prevent interrupt jitter */
	thresh_delta = (adc_value >> 12)+2;
	thresh_lo = adc_value - thresh_delta;
	thresh_hi = adc_value + thresh_delta;
	if (thresh_lo < ALS_MIN_MEASURE_VAL) {
		thresh_lo = ALS_MIN_MEASURE_VAL;
	}
	if (thresh_hi > ALS_MAX_MEASURE_VAL) {
		thresh_hi = ALS_MAX_MEASURE_VAL;
	}

	ret = set_als_range((uint16_t)thresh_lo, (uint16_t)thresh_hi, LO_N_HI_LIMIT);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s : ALS thresholds setting Fail...\n", __func__);
	}
}


/* Work when interrupt */
static void ltr559_schedwork(struct work_struct *work)
{
	int8_t ret;
	uint8_t status, i_ctr = 0;
	uint8_t	interrupt_stat, newdata;
	struct ltr559_data *ltr559 = sensor_info;
	uint8_t buffer[2], buf[40];

	buffer[0] = LTR559_ALS_PS_STATUS;	
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s | 0x%02X", __func__, buffer[0]);
		return;
	}
	status = buffer[0];
	interrupt_stat = status & 0x0A;
	newdata = status & 0x05;

	if (!interrupt_stat) {
		/* there is an interrupt with no work to do */
		dev_info(&ltr559->i2c_client->dev,"%s Unexpected received interrupt with no work to do status:0x%02x\n", __func__, status);
		buf[0] = 0x80;
		I2C_Read(buf, sizeof(buf));
		for (i_ctr = 0; i_ctr < sizeof(buf); i_ctr++) {
			dev_info(&ltr559->i2c_client->dev, "%s reg:0x%02x val:0x%02x\n", __func__, 0x80+i_ctr, buf[i_ctr]);
		}
	} else {
		// PS interrupt and PS with new data
		if ((interrupt_stat & 0x02) && (newdata & 0x01)) {
			ltr559->ps_irq_flag = 1;
			report_ps_input_event(ltr559);
		}

		// ALS interrupt and ALS with new data
		if ((interrupt_stat & 0x08) && (newdata & 0x04)) {
			ltr559->als_irq_flag = 1;
			report_als_input_event(ltr559);
		}
	}
	enable_irq(ltr559->irq);
}

static DECLARE_WORK(irq_workqueue, ltr559_schedwork);


/* IRQ Handler */
static irqreturn_t ltr559_irq_handler(int irq, void *data)
{
	struct ltr559_data *ltr559 = data;

	/* disable an irq without waiting */
	disable_irq_nosync(ltr559->irq);

	schedule_work(&irq_workqueue);

	return IRQ_HANDLED;
}


#if 1
static int ltr559_gpio_irq(struct ltr559_data *ltr559)
{
	int rc = 0;

	rc = gpio_request(ltr559->gpio_int_no, DEVICE_NAME);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev,"%s: GPIO %d Request Fail (%d)\n", __func__, ltr559->gpio_int_no, rc);
		return rc;
	}

	gpio_tlmm_config(GPIO_CFG(ltr559->gpio_int_no,0,GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	rc = gpio_direction_input(ltr559->gpio_int_no);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Set GPIO %d as Input Fail (%d)\n", __func__, ltr559->gpio_int_no, rc);
		goto out1;
	}

	/* Configure an active low trigger interrupt for the device */
	rc = request_irq(ltr559->irq, ltr559_irq_handler, IRQF_TRIGGER_LOW, DEVICE_NAME, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Request IRQ (%d) for GPIO %d Fail (%d)\n", __func__, ltr559->irq,
		        ltr559->gpio_int_no, rc);
		goto out1;
	}
	
	return rc;

out1:
	gpio_free(ltr559->gpio_int_no);

	return rc;
}
#endif
//(Linux RTOS)<


/* PS Enable */
static int8_t ps_device_init(struct ltr559_data *ltr559)
{
	int8_t rc = 0;
	
	if (ltr559->ps_enable_flag) {
		dev_info(&ltr559->i2c_client->dev, "%s: already enabled\n", __func__);
		return 0;
	}

	/* Set thresholds*/
	rc = set_ps_range(PS_SET_LOWTHRESH, PS_SET_HIGHTHRESH, LO_N_HI_LIMIT);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s : PS Thresholds Write Fail...\n", __func__);
		return rc;
	}

	//(Linux RTOS)>
#if 0
	/* Allows this interrupt to wake the system */
	//rc = irq_set_irq_wake(ltr559->irq, 1);
	rc = set_irq_wake(ltr559->irq, 1);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: IRQ-%d WakeUp Enable Fail...\n", __func__, ltr559->irq);
		return rc;
	}
#endif
	//(Linux RTOS)<

	rc = ps_led_setup(0x7F, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS LED Setup Fail...\n", __func__);
		return rc;
	}

	rc = ps_ledPulseCount_setup(0x08, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS LED pulse count setup Fail...\n", __func__);
	}

	rc = ps_meas_rate_setup(100, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS MeasRate Setup Fail...\n", __func__);
		return rc;
	}

	return rc;
}

/*ps enable*/
static int8_t ltr559_ps_enable(struct ltr559_data *ltr559)
{
	int8_t rc = 0;
	uint8_t buffer[1];

	if (ltr559->ps_enable_flag) {
		//dev_info(&ltr559->i2c_client->dev, "%s: already enabled\n", __func__);
		return 0;
	}
	
	rc = ps_contr_setup(0x03, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS Enable Fail...\n", __func__);
		return rc;
	}
	buffer[0] = LTR559_PS_CONTR;
	rc = I2C_Read(buffer, 1);
	//printk("PS Enable Statte, 0x81 = %d...\n", buffer[0]);

	ltr559->ps_enable_flag = 1;
	wake_lock(&(ltr559->ps_wake_lock));

	return rc;
}


/* ps disable */
static int8_t ps_disable(struct ltr559_data *ltr559)
{
	int8_t rc = 0;

	if (ltr559->ps_enable_flag == 0) {
		//dev_info(&ltr559->i2c_client->dev, "%s: already disabled\n", __func__);
		return 0;
	}

	//(Linux RTOS)>
#if 0
	/* Don't allow this interrupt to wake the system anymore */
	//rc = irq_set_irq_wake(ltr559->irq, 0);
	rc = set_irq_wake(ltr559->irq, 0);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: IRQ-%d WakeUp Disable Fail...\n", __func__, ltr559->irq);
		return rc;
	}
#endif
	//(Linux RTOS)<

	//rc = _ltr559_set_bit(ltr559->i2c_client, CLR_BIT, LTR559_PS_CONTR, PS_MODE);
	rc = ps_mode_setup(CLR_BIT, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS Disable Fail...\n", __func__);
		return rc;
	}

	ltr559->ps_enable_flag = 0;
	wake_unlock(&(ltr559->ps_wake_lock));

	return rc;
}


/* PS open fops */
ssize_t ps_open(struct inode *inode, struct file *file)
{
	struct ltr559_data *ltr559 = sensor_info;

	if (ltr559->ps_opened)
		return -EBUSY;

	ltr559->ps_opened = 1;

	return 0;
}


/* PS release fops */
ssize_t ps_release(struct inode *inode, struct file *file)
{
	struct ltr559_data *ltr559 = sensor_info;

	ltr559->ps_opened = 0;

	return ps_disable(ltr559);	
}


/* PS IOCTL */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int ps_ioctl (struct inode *ino, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long ps_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	int rc = 0, val = 0;
	struct ltr559_data *ltr559 = sensor_info;

	pr_info("%s cmd %d\n", __func__, _IOC_NR(cmd));

	switch (cmd) {
		case LTR559_IOCTL_PS_ENABLE:
			if (get_user(val, (unsigned long __user *)arg)) {
				rc = -EFAULT;
				break;
			}
			pr_info("%s value = %d\n", __func__, val);
			rc = val ? ltr559_ps_enable(ltr559) : ps_disable(ltr559);

			break;
		case LTR559_IOCTL_PS_GET_ENABLED:
			rc = put_user(ltr559->ps_enable_flag, (unsigned long __user *)arg);

			break;
		default:
			pr_err("%s: INVALID COMMAND %d\n", __func__, _IOC_NR(cmd));
			rc = -EINVAL;
	}

	return rc;
}

static const struct file_operations ps_fops = {
	.owner = THIS_MODULE,
	.open = ps_open,
	.release = ps_release,
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = ps_ioctl
	#else
	.unlocked_ioctl = ps_ioctl
	#endif
};

struct miscdevice ps_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ltr559_ps",
	.fops = &ps_fops
};


static int8_t als_device_init(struct ltr559_data *ltr559)
{
	int8_t rc = 0;

	rc = als_meas_rate_reg_setup(0x03, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS_Meas_Rate register Setup Fail...\n", __func__);
		return rc;
	}

	/* Set minimummax thresholds where interrupt will *not* be generated */
	//rc = set_als_range(ALS_MIN_MEASURE_VAL, ALS_MAX_MEASURE_VAL, LO_N_HI_LIMIT);
	rc = set_als_range(ALS_MIN_MEASURE_VAL, ALS_MIN_MEASURE_VAL, LO_N_HI_LIMIT);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s : ALS Thresholds Write Fail...\n", __func__);
		return rc;
	}
	return rc;
}

//als enable
static int8_t ltr559_als_enable(struct ltr559_data *ltr559)
{
	int8_t rc = 0;
	uint8_t buffer[1];

	/* if device not enabled, enable it */
	if (ltr559->als_enable_flag) {
		//dev_err(&ltr559->i2c_client->dev, "%s: ALS already enabled...\n", __func__);
		return rc;
	}
	rc = als_contr_setup(0x0D, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS Enable Fail...\n", __func__);
		return rc;
	}
	buffer[0] = LTR559_ALS_CONTR;
	rc = I2C_Read(buffer, 1);
	//printk("ALS Enable Statte, 0x80 = %d...\n", buffer[0]);
	
	ltr559->als_enable_flag = 1;
	
	return rc;
}


static int8_t als_disable(struct ltr559_data *ltr559)
{
	int8_t rc = 0;

	if (ltr559->als_enable_flag == 0) {
		//dev_err(&ltr559->i2c_client->dev, "%s : ALS already disabled...\n", __func__);
		return rc;
	}

	//rc = _ltr559_set_bit(ltr559->i2c_client, CLR_BIT, LTR559_ALS_CONTR, ALS_MODE);
	rc = als_mode_setup(CLR_BIT, ltr559);
	if (rc < 0) {
		dev_err(&ltr559->i2c_client->dev,"%s: ALS Disable Fail...\n", __func__);
		return rc;
	}
	ltr559->als_enable_flag = 0;

	return rc;
}


ssize_t als_open(struct inode *inode, struct file *file)
{
	struct ltr559_data *ltr559 = sensor_info;
	int8_t rc = 0;

	if (ltr559->als_opened) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS already Opened...\n", __func__);
		rc = -EBUSY;
	}
	ltr559->als_opened = 1;

	return rc;
}


ssize_t als_release(struct inode *inode, struct file *file)
{
	struct ltr559_data *ltr559 = sensor_info;

	ltr559->als_opened = 0;

	//return 0;
	return als_disable(ltr559);
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
static int als_ioctl (struct inode *ino, struct file *file, unsigned int cmd, unsigned long arg)
#else
static long als_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
#endif
{
	int rc = 0, val = 0;
	struct ltr559_data *ltr559 = sensor_info;

	pr_debug("%s cmd %d\n", __func__, _IOC_NR(cmd));

	switch (cmd) {
		case LTR559_IOCTL_ALS_ENABLE:
			if (get_user(val, (unsigned long __user *)arg)) {
				rc = -EFAULT;
				break;
			}
			/*pr_info("%s value = %d\n", __func__, val);*/
			rc = val ? ltr559_als_enable(ltr559) : als_disable(ltr559);

			break;
		case LTR559_IOCTL_ALS_GET_ENABLED:
			val = ltr559->als_enable_flag;
			/*pr_info("%s enabled %d\n", __func__, val);*/
			rc = put_user(val, (unsigned long __user *)arg);

			break;
		default:
			pr_err("%s: INVALID COMMAND %d\n", __func__, _IOC_NR(cmd));
			rc = -EINVAL;
	}

	return rc;
}


static const struct file_operations als_fops = {
	.owner = THIS_MODULE,
	.open = als_open,
	.release = als_release,
	#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl = als_ioctl
	#else
	.unlocked_ioctl = als_ioctl
	#endif
};

static struct miscdevice als_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ltr559_ls",
	.fops = &als_fops
};


static ssize_t als_adc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t value;
	int ret;
	struct ltr559_data *ltr559 = sensor_info;

	ltr559->mode = ALS;
	value = read_adc_value(ltr559);
	//ret = sprintf(buf, "%d\n", value);
	ret = sprintf(buf, "%d", value);

	return ret;
}

static DEVICE_ATTR(als_adc, 0444, als_adc_show, NULL);


static ssize_t ps_adc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	uint16_t value;
	int ret;
	struct ltr559_data *ltr559 = sensor_info;

	ltr559->mode = PS;
	value = read_adc_value(ltr559);
	//ret = sprintf(buf, "%d\n", value);
	ret = sprintf(buf, "%d", value);
	
	return ret;
}

static DEVICE_ATTR(ps_adc, 0444, ps_adc_show, NULL);

static ssize_t alscontrsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr559_data *ltr559 = sensor_info;	

	ret = als_contr_readback(ALS_CONTR_RDBCK, &rdback_val, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS_CONTR_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static ssize_t alscontrsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint8_t param;
	//int *param_temp = buf;
	int param_temp[2];

	struct ltr559_data *ltr559 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];

	if (count <= 1) {
		param_temp[0] = 48;
		param_temp[1] = 48;
	} else if (count == 2) {
		param_temp[1] = param_temp[0];
		param_temp[0] = 48;
	}


	if (param_temp[0] >= 65 && param_temp[0] <= 70) {
		param_temp[0] -= 55;
	} else if (param_temp[0] >= 97 && param_temp[0] <= 102) {
		param_temp[0] -= 87;
	} else if (param_temp[0] >= 48 && param_temp[0] <= 57) {
		param_temp[0] -= 48;
	} else {
		param_temp[0] = 0;
	}

	if (param_temp[1] >= 65 && param_temp[1] <= 70) {
		param_temp[1] -= 55;
	} else if (param_temp[1] >= 97 && param_temp[1] <= 102) {
		param_temp[1] -= 87;
	} else if (param_temp[1] >= 48 && param_temp[1] <= 57) {
		param_temp[1] -= 48;
	} else {
		param_temp[1] = 0;
	}

	param = ((param_temp[0] << 4) + (param_temp[1]));
	dev_dbg(&ltr559->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = als_contr_setup(param, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS contr setup Fail...\n", __func__);
		return (-1);
	}

	return count;	
}

static DEVICE_ATTR(alscontrsetup, 0664, alscontrsetup_show, alscontrsetup_store);

static ssize_t pscontrsetup_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr559_data *ltr559 = sensor_info;	

	ret = ps_contr_readback(PS_CONTR_RDBCK, &rdback_val, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS_CONTR_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}


static ssize_t pscontrsetup_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint8_t param;
	//int *param_temp = buf;
	int param_temp[2];

	struct ltr559_data *ltr559 = sensor_info;

	//sscanf(buf, "%d", param_temp);
	param_temp[0] = buf[0];
	param_temp[1] = buf[1];

	if (count <= 1) {
		param_temp[0] = 48;
		param_temp[1] = 48;
	} else if (count == 2) {
		param_temp[1] = param_temp[0];
		param_temp[0] = 48;
	}

	if (param_temp[0] >= 65 && param_temp[0] <= 70) {
		param_temp[0] -= 55;
	} else if (param_temp[0] >= 97 && param_temp[0] <= 102) {
		param_temp[0] -= 87;
	} else if (param_temp[0] >= 48 && param_temp[0] <= 57) {
		param_temp[0] -= 48;
	} else {
		param_temp[0] = 0;
	}

	if (param_temp[1] >= 65 && param_temp[1] <= 70) {
		param_temp[1] -= 55;
	} else if (param_temp[1] >= 97 && param_temp[1] <= 102) {
		param_temp[1] -= 87;
	} else if (param_temp[1] >= 48 && param_temp[1] <= 57) {
		param_temp[1] -= 48;
	} else {
		param_temp[1] = 0;
	}

	param = ((param_temp[0] << 4) + (param_temp[1]));
	dev_dbg(&ltr559->i2c_client->dev, "%s: store value = %d\n", __func__, param);

	ret = ps_contr_setup(param, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS contr setup Fail...\n", __func__);
		return (-1);
	}

	return count;	
}

static DEVICE_ATTR(pscontrsetup, 0664, pscontrsetup_show, pscontrsetup_store);

static ssize_t partidreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr559_data *ltr559 = sensor_info;

	ret = part_ID_reg_readback(PART_ID_REG_RDBCK, &rdback_val, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PART_ID_REG_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(partidreg, 0444, partidreg_show, NULL);


static ssize_t manuid_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr559_data *ltr559 = sensor_info;

	ret = manu_ID_reg_readback(&rdback_val, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Manufacturing ID readback Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
}

static DEVICE_ATTR(manuid, 0444, manuid_show, NULL);

static ssize_t reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	int i = 0;
	uint8_t bufdata[1];
	int count=0;

	for(i = 0;i < 31 ;i++)
	{
		bufdata[0] = 0x80+i;
		ret = I2C_Read(bufdata, 1);
		count+= sprintf(buf+count,"[%x] = (%x)\n",0x80+i,bufdata[0]);
	}
	
	return count;
}

static DEVICE_ATTR(reg, 0444, reg_show, NULL);

static ssize_t alspsstatusreg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint8_t rdback_val = 0;
	struct ltr559_data *ltr559 = sensor_info;	

	ret = als_ps_status_reg(ALS_PS_STATUS_RDBCK, &rdback_val, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS_PS_STATUS_RDBCK Fail...\n", __func__);
		return (-1);
	}

	ret = sprintf(buf, "%d\n", rdback_val);

	return ret;
	
}

static DEVICE_ATTR(alspsstatusreg, 0444, alspsstatusreg_show, NULL);

static ssize_t setalslothrerange_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	int lo_thr = 0;
	uint8_t param_temp[5];
	struct ltr559_data *ltr559 = sensor_info;

	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];
	param_temp[3] = buf[3];
	param_temp[4] = buf[4];

	if (count <= 1) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;
	} else if (count == 2) { // 1 digit
		param_temp[0] -= 48;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[0];
		param_temp[3] = 0;
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 3) { // 2 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[1];
		param_temp[3] = param_temp[0];
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 4) { // 3 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[2];
		param_temp[3] = param_temp[1];
		param_temp[2] = param_temp[0];
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 5) { // 4 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] = 0;

		param_temp[4] = param_temp[3];
		param_temp[3] = param_temp[2];
		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count >= 6) { // 5 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] -= 48;
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	lo_thr = ((param_temp[0] * 10000) + (param_temp[1] * 1000) + (param_temp[2] * 100) + (param_temp[3] * 10) + param_temp[4]);
	if (lo_thr > 65535) {
		lo_thr = 65535;
	}
	dev_dbg(&ltr559->i2c_client->dev, "%s: store value = %d\n", __func__, lo_thr);

	ret = set_als_range((uint16_t)lo_thr, 0, LO_LIMIT);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: set ALS lo threshold Fail...\n", __func__);
		return (-1);
	}
	
	return count;
}

static DEVICE_ATTR(setalslothrerange, 0660, NULL, setalslothrerange_store);


static ssize_t setalshithrerange_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	int hi_thr = 0;
	uint8_t param_temp[5];
	struct ltr559_data *ltr559 = sensor_info;

	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];
	param_temp[3] = buf[3];
	param_temp[4] = buf[4];

	if (count <= 1) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;
	} else if (count == 2) { // 1 digit
		param_temp[0] -= 48;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[0];
		param_temp[3] = 0;
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 3) { // 2 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[1];
		param_temp[3] = param_temp[0];
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 4) { // 3 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] = 0;
		param_temp[4] = 0;

		param_temp[4] = param_temp[2];
		param_temp[3] = param_temp[1];
		param_temp[2] = param_temp[0];
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 5) { // 4 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] = 0;

		param_temp[4] = param_temp[3];
		param_temp[3] = param_temp[2];
		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count >= 6) { // 5 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;
		param_temp[4] -= 48;
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	hi_thr = ((param_temp[0] * 10000) + (param_temp[1] * 1000) + (param_temp[2] * 100) + (param_temp[3] * 10) + param_temp[4]);
	if (hi_thr > 65535) {
		hi_thr = 65535;
	}
	dev_dbg(&ltr559->i2c_client->dev, "%s: store value = %d\n", __func__, hi_thr);

	ret = set_als_range(0, (uint16_t)hi_thr, HI_LIMIT);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: set ALS hi threshold Fail...\n", __func__);
		return (-1);
	}
	
	return count;
}

static DEVICE_ATTR(setalshithrerange, 0660, NULL, setalshithrerange_store);


static ssize_t dispalsthrerange_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint16_t rdback_lo, rdback_hi;
	struct ltr559_data *ltr559 = sensor_info;

	ret = als_range_readback(&rdback_lo, &rdback_hi, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS threshold range readback Fail...\n", __func__);
		return (-1);
	}
	
	ret = sprintf(buf, "%d %d\n", rdback_lo, rdback_hi);

	return ret;
}

static DEVICE_ATTR(dispalsthrerange, 0444, dispalsthrerange_show, NULL);


static ssize_t setpslothrerange_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint16_t lo_thr = 0;
	uint8_t param_temp[4];
	struct ltr559_data *ltr559 = sensor_info;

	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];
	param_temp[3] = buf[3];

	if (count <= 1) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
	} else if (count == 2) { // 1 digit
		param_temp[0] -= 48;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;

		param_temp[3] = param_temp[0];
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 3) { // 2 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;
		param_temp[3] = 0;

		param_temp[3] = param_temp[1];
		param_temp[2] = param_temp[0];
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 4) { // 3 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] = 0;

		param_temp[3] = param_temp[2];
		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count >= 5) { // 4 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;		
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	lo_thr = ((param_temp[0] * 1000) + (param_temp[1] * 100) + (param_temp[2] * 10) + param_temp[3]);
	if (lo_thr > 2047) {
		lo_thr = 2047;
	}
	dev_dbg(&ltr559->i2c_client->dev, "%s: store value = %d\n", __func__, lo_thr);
	ps_set_lowthresh = lo_thr;
	printk("%s,lo_thr:%d,ps_set_lowthresh:%d\n",__func__,lo_thr,ps_set_lowthresh);	

	ret = set_ps_range(lo_thr, 0, LO_LIMIT);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: set PS lo threshold Fail...\n", __func__);
		return (-1);
	}
	
	return count;
}

static DEVICE_ATTR(setpslothrerange, 0660, NULL, setpslothrerange_store);


static ssize_t setpshithrerange_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int8_t ret;
	uint16_t hi_thr = 0;
	uint8_t param_temp[4];
	struct ltr559_data *ltr559 = sensor_info;

	param_temp[0] = buf[0];
	param_temp[1] = buf[1];
	param_temp[2] = buf[2];
	param_temp[3] = buf[3];

	if (count <= 1) {
		param_temp[0] = 0;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;
	} else if (count == 2) { // 1 digit
		param_temp[0] -= 48;
		param_temp[1] = 0;
		param_temp[2] = 0;
		param_temp[3] = 0;

		param_temp[3] = param_temp[0];
		param_temp[2] = 0;
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 3) { // 2 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] = 0;
		param_temp[3] = 0;

		param_temp[3] = param_temp[1];
		param_temp[2] = param_temp[0];
		param_temp[1] = 0;
		param_temp[0] = 0;
	} else if (count == 4) { // 3 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] = 0;

		param_temp[3] = param_temp[2];
		param_temp[2] = param_temp[1];
		param_temp[1] = param_temp[0];
		param_temp[0] = 0;
	} else if (count >= 5) { // 4 digits
		param_temp[0] -= 48;
		param_temp[1] -= 48;
		param_temp[2] -= 48;
		param_temp[3] -= 48;		
	}

	//for (i_ctr = 0; i_ctr < sizeof(param_temp); i_ctr++) {
	//	if ((param_temp[i_ctr] < 0) ||(param_temp[i_ctr] > 9)) {
	//		param_temp[i_ctr] = 0;
	//	}
	//}

	hi_thr = ((param_temp[0] * 1000) + (param_temp[1] * 100) + (param_temp[2] * 10) + param_temp[3]);
	if (hi_thr > 2047) {
		hi_thr = 2047;
	}
	dev_dbg(&ltr559->i2c_client->dev, "%s: store value = %d\n", __func__, hi_thr);
	ps_set_highthresh = hi_thr;
	printk("%s,hi_thr:%d,ps_set_highthresh:%d\n",__func__,hi_thr,ps_set_highthresh);
	
	ret = set_ps_range(0, hi_thr, HI_LIMIT);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: set PS hi threshold Fail...\n", __func__);
		return (-1);
	}
	
	return count;
}

static DEVICE_ATTR(setpshithrerange, 0660, NULL, setpshithrerange_store);


static ssize_t disppsthrerange_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int8_t ret = 0;
	uint16_t rdback_lo, rdback_hi;
	struct ltr559_data *ltr559 = sensor_info;

	ret = ps_range_readback(&rdback_lo, &rdback_hi, ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS threshold range readback Fail...\n", __func__);
		return (-1);
	}
	
	ret = sprintf(buf, "%d %d\n", rdback_lo, rdback_hi);

	return ret;
}

static DEVICE_ATTR(disppsthrerange, 0444, disppsthrerange_show, NULL);

static ssize_t als_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ltr559_data *ltr559 = sensor_info;
	
	return sprintf(buf, "%d\n", ltr559->als_enable_flag);			
}

static ssize_t als_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct ltr559_data *ltr559 = sensor_info;
	
	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	
	if (data) 
		error = ltr559_als_enable(ltr559);
	else
		error = als_disable(ltr559);

	if(error){
		dev_err(&ltr559->i2c_client->dev, "%s: als set enable fail...\n", __func__);
		return error;
	}
	
	return count;
}

static DEVICE_ATTR(als_enable, 0664, als_enable_show, als_enable_store);

static ssize_t ps_enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ltr559_data *ltr559 = sensor_info;
	
	return sprintf(buf, "%d\n", ltr559->ps_enable_flag);	
}

static ssize_t ps_enable_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct ltr559_data *ltr559 = sensor_info;
	
	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	
	if (data) 
		error = ltr559_ps_enable(ltr559);
	else
		error = ps_disable(ltr559);

	if(error){
		dev_err(&ltr559->i2c_client->dev, "%s: ps set enable fail...\n", __func__);
		return error;
	}

	return count;	
}

static DEVICE_ATTR(ps_enable, 0664, ps_enable_show, ps_enable_store);

static void sysfs_register_device(struct i2c_client *client) 
{
	int rc = 0;

	rc += device_create_file(&client->dev, &dev_attr_als_adc);
	rc += device_create_file(&client->dev, &dev_attr_ps_adc);	
	rc += device_create_file(&client->dev, &dev_attr_alscontrsetup);             //0X80
	rc += device_create_file(&client->dev, &dev_attr_pscontrsetup);	          //0X81
	rc += device_create_file(&client->dev, &dev_attr_partidreg);              //0x86
	rc += device_create_file(&client->dev, &dev_attr_manuid);	           //0x87
	rc += device_create_file(&client->dev, &dev_attr_reg);	           
	rc += device_create_file(&client->dev, &dev_attr_alspsstatusreg);        //0x8c
	rc += device_create_file(&client->dev, &dev_attr_setalslothrerange);
	rc += device_create_file(&client->dev, &dev_attr_setalshithrerange);
	rc += device_create_file(&client->dev, &dev_attr_dispalsthrerange);
	rc += device_create_file(&client->dev, &dev_attr_setpslothrerange);
	rc += device_create_file(&client->dev, &dev_attr_setpshithrerange);
	rc += device_create_file(&client->dev, &dev_attr_disppsthrerange);
	rc += device_create_file(&client->dev, &dev_attr_als_enable);
	rc += device_create_file(&client->dev, &dev_attr_ps_enable);
	
	if (rc) {
		dev_err(&client->dev, "%s Unable to create sysfs files\n", __func__);
	} else {
		dev_dbg(&client->dev, "%s Created sysfs files\n", __func__);
	}
}


static int als_setup(struct ltr559_data *ltr559)
{
	int ret;

	ltr559->als_input_dev = input_allocate_device();
	if (!ltr559->als_input_dev) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS Input Allocate Device Fail...\n", __func__);
		return -ENOMEM;
	}
	ltr559->als_input_dev->name = "ltr559_als";
	set_bit(EV_ABS, ltr559->als_input_dev->evbit);
	input_set_abs_params(ltr559->als_input_dev, ABS_MISC, ALS_MIN_MEASURE_VAL, ALS_MAX_MEASURE_VAL, 0, 0);

	ret = input_register_device(ltr559->als_input_dev);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS Register Input Device Fail...\n", __func__);
		goto err_als_register_input_device;
	}

	ret = misc_register(&als_misc);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS Register Misc Device Fail...\n", __func__);
		goto err_als_register_misc_device;
	}

	return ret;

err_als_register_misc_device:
	input_unregister_device(ltr559->als_input_dev);
err_als_register_input_device:
	input_free_device(ltr559->als_input_dev);

	return ret;
}


static int ps_setup(struct ltr559_data *ltr559)
{
	int ret;

	ltr559->ps_input_dev = input_allocate_device();
	if (!ltr559->ps_input_dev) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS Input Allocate Device Fail...\n", __func__);
		return -ENOMEM;
	}
	ltr559->ps_input_dev->name = "ltr559_ps";
	set_bit(EV_ABS, ltr559->ps_input_dev->evbit);
	input_set_abs_params(ltr559->ps_input_dev, ABS_DISTANCE, PS_MIN_MEASURE_VAL, PS_MAX_MEASURE_VAL, 0, 0);

	ret = input_register_device(ltr559->ps_input_dev);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS Register Input Device Fail...\n", __func__);
		goto err_ps_register_input_device;
	}

	ret = misc_register(&ps_misc);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS Register Misc Device Fail...\n", __func__);
		goto err_ps_register_misc_device;
	}

	return ret;

err_ps_register_misc_device:
	input_unregister_device(ltr559->ps_input_dev);
err_ps_register_input_device:
	input_free_device(ltr559->ps_input_dev);

	return ret;
}


static int _check_part_id(struct ltr559_data *ltr559)
{
	uint8_t ret;
	uint8_t buffer[2];

	buffer[0] = LTR559_PART_ID;
	ret = I2C_Read(buffer, 1);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Read failure :0x%02X",
		        __func__, buffer[0]);
		return -1;
	}

	if (buffer[0] != PARTID) {
		dev_err(&ltr559->i2c_client->dev, "%s: Part failure miscompare"
		        " act:0x%02x exp:0x%02x\n", __func__, buffer[0], PARTID);
		return -2;
	}

	return 0;
}


static int ltr559_setup(struct ltr559_data *ltr559)
{
	int ret = 0;

	/* Reset the devices */
	ret = _ltr559_set_bit(ltr559->i2c_client, SET_BIT, LTR559_ALS_CONTR, ALS_SW_RESET);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS reset fail...\n", __func__);
		goto err_out1;
	}

	ret = _ltr559_set_bit(ltr559->i2c_client, CLR_BIT, LTR559_PS_CONTR, PS_MODE_ACTIVE);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS reset fail...\n", __func__);
		goto err_out1;
	}

	msleep(PON_DELAY);
	dev_dbg(&ltr559->i2c_client->dev, "%s: Reset ltr559 device\n", __func__);

	/* Do another part read to ensure we have exited reset */
	if (_check_part_id(ltr559) < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Part ID Read Fail after reset...\n", __func__);
		goto err_out1;
	}
		
	//(Linux RTOS)>
#if 1
	ret = ltr559_gpio_irq(ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: GPIO Request Fail...\n", __func__);
		goto err_out1;
	}
	dev_dbg(&ltr559->i2c_client->dev, "%s Requested interrupt\n", __func__);
#endif
	//(Linux RTOS)<

	/* Set count of measurements outside data range before interrupt is generated */
	ret = _ltr559_set_bit(ltr559->i2c_client, SET_BIT, LTR559_INTERRUPT_PRST, 0x11);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: ALS Set Persist Fail...\n", __func__);
		goto err_out2;
	}

	/* Enable interrupts on the device and clear only when status is read */
	ret = _ltr559_set_bit(ltr559->i2c_client, SET_BIT, LTR559_INTERRUPT, INT_MODE_ALSPS_TRIG);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Enabled interrupts failed...\n", __func__);
		goto err_out2;
	}
	dev_dbg(&ltr559->i2c_client->dev, "%s Enabled interrupt to device\n", __func__);

	/* Turn on ALS and PS */
	ret = als_device_init(ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s Unable to enable ALS", __func__);
		goto err_out2;
	}
	dev_info(&ltr559->i2c_client->dev, "%s Turned on ambient light sensor\n", __func__);

	ret = ps_device_init(ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s Unable to enable PS", __func__);
		goto err_out2;
	}
	dev_info(&ltr559->i2c_client->dev, "%s Turned on proximity sensor\n", __func__);

	return ret;

err_out2:
	free_irq(ltr559->irq, ltr559);
	gpio_free(ltr559->gpio_int_no);

err_out1:
	dev_err(&ltr559->i2c_client->dev, "%s Unable to setup device\n", __func__);

	return ret;
}


//(Linux RTOS)>
#if 1
static void ltr559_early_suspend(struct early_suspend *h)
{
	int ret = 0;
	struct ltr559_data *ltr559 = sensor_info;

	ltr559->is_suspend = 1;

	ret = als_disable(ltr559);
#if 0
	/* Save away the state of the devices at suspend point */
	ltr559->als_suspend_enable_flag = ltr559->als_enable_flag;
	ltr559->ps_suspend_enable_flag = ltr559->ps_enable_flag;

	/* Disable the devices for suspend if configured */
	if (ltr559->disable_als_on_suspend && ltr559->als_enable_flag) {
		ret += als_disable(ltr559);
	}
#endif

	if (ret) {
		dev_err(&ltr559->i2c_client->dev, "%s Unable to complete suspend\n", __func__);
	} else {
		dev_info(&ltr559->i2c_client->dev, "%s Suspend completed\n", __func__);
	}
}


static void ltr559_late_resume(struct early_suspend *h)
{
	struct ltr559_data *ltr559 = sensor_info;
	int ret = 0;

	ltr559->is_suspend = 0;
	ret = ltr559_als_enable(ltr559);

#if 0
	/* If PS was enbled before suspend, enable during resume */
	if (ltr559->ps_suspend_enable_flag) {
		ret += ltr559_ps_enable(ltr559);
		ltr559->ps_suspend_enable_flag = 0;
	}
#endif

	if (ret) {
		dev_err(&ltr559->i2c_client->dev, "%s Unable to complete resume\n", __func__);
	} else {
		dev_info(&ltr559->i2c_client->dev, "%s Resume completed\n", __func__);
	}
}
#endif
//(Linux RTOS)<


static int ltr559_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct ltr559_data *ltr559;
	
	printk("entry %s\n",__func__);
//(Linux RTOS)>
#if 0
	struct ltr559_platform_data *platdata;
#endif
//(Linux RTOS)<
	
	ltr559 = kzalloc(sizeof(struct ltr559_data), GFP_KERNEL);
	if (!ltr559)
	{
		dev_err(&ltr559->i2c_client->dev, "%s: Mem Alloc Fail...\n", __func__);
		return -ENOMEM;
	}

	/* Global pointer for this device */
	sensor_info = ltr559;

	/* Set initial defaults */
	ltr559->als_enable_flag = 0;
	ltr559->ps_enable_flag = 0;

	ltr559->i2c_client = client;
	ltr559->irq = client->irq;
	ltr559->gpio_int_no = GPIO17_PLSENSOR_INT;
	
	i2c_set_clientdata(client, ltr559);
	
	/* Parse the platform data */
	//(Linux RTOS)>
#if 0
	platdata = client->dev.platform_data;
	if (!platdata) {
		dev_err(&ltr559->i2c_client->dev, "%s: Platform Data assign Fail...\n", __func__);
		ret = -EBUSY;
		goto err_out;
	}

	ltr559->gpio_int_no = platdata->pfd_gpio_int_no;
	//ltr559->adc_levels = platdata->pfd_levels;
	ltr559->default_ps_lowthresh = platdata->pfd_ps_lowthresh;
	ltr559->default_ps_highthresh = platdata->pfd_ps_highthresh;

	/* Configuration to set or disable devices upon suspend */
	//ltr559->disable_als_on_suspend = platdata->pfd_disable_als_on_suspend;
	//ltr559->disable_ps_on_suspend = platdata->pfd_disable_ps_on_suspend;
#endif
	//(Linux RTOS)<

	if (_check_part_id(ltr559) < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Part ID Read Fail...\n", __func__);
		goto err_out;
	}

	/* Setup the input subsystem for the ALS */
	ret = als_setup(ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev,"%s: ALS Setup Fail...\n", __func__);
		goto err_out;
	}

	/* Setup the input subsystem for the PS */
	ret = ps_setup(ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: PS Setup Fail...\n", __func__);
		goto err_out;
	}

	/* Create the workqueue for the interrup handler */
	//(Linux RTOS)>
#if 1
	ltr559->workqueue = create_singlethread_workqueue("ltr559_wq");
	if (!ltr559->workqueue) {
		dev_err(&ltr559->i2c_client->dev, "%s: Create WorkQueue Fail...\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	/* Wake lock option for promity sensor */
	wake_lock_init(&(ltr559->ps_wake_lock), WAKE_LOCK_SUSPEND, "proximity");
#endif
	//(Linux RTOS)<

	/* Setup and configure both the ALS and PS on the ltr559 device */
	ret = ltr559_setup(ltr559);
	if (ret < 0) {
		dev_err(&ltr559->i2c_client->dev, "%s: Setup Fail...\n", __func__);
		goto err_ltr559_setup;
	}

	/* Setup the suspend and resume functionality */
	//(Linux RTOS)>
#if 1
	ltr559->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ltr559->early_suspend.suspend = ltr559_early_suspend;
	ltr559->early_suspend.resume = ltr559_late_resume;
	register_early_suspend(&ltr559->early_suspend);
#endif
	//(Linux RTOS)<
	

	/* Register the sysfs files */
	sysfs_register_device(client);
	//sysfs_register_als_device(client, &ltr559->als_input_dev->dev);
	//sysfs_register_ps_device(client, &ltr559->ps_input_dev->dev);

	dev_dbg(&ltr559->i2c_client->dev, "%s: probe complete\n", __func__);
	return ret;

err_ltr559_setup:
	destroy_workqueue(ltr559->workqueue);
err_out:
	kfree(ltr559);

	return ret;
}


static const struct i2c_device_id ltr559_id[] = {
	{ DEVICE_NAME, 0 },
	{}
};

static struct i2c_driver ltr559_driver = {
	.probe = ltr559_probe,
	.id_table = ltr559_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = DEVICE_NAME,
	},
};


static int __init ltr559_init(void)
{
	return i2c_add_driver(&ltr559_driver);
}

static void __exit ltr559_exit(void)
{
	i2c_del_driver(&ltr559_driver);
}


module_init(ltr559_init)
module_exit(ltr559_exit)

MODULE_AUTHOR("Lite-On Technology Corp");
MODULE_DESCRIPTION("LTR-559ALSPS Driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRIVER_VERSION);


