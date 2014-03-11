/*
 * This file is part of the APDS990x sensor driver.
 * Chip is combined proximity and ambient light sensor.
 *
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (c) 2012 The Linux Foundation. All Rights Reserved.
 *
 * Contact: Samu Onkalo <samu.p.onkalo@nokia.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/input.h>
#include <linux/input/apds990x.h>
#include <linux/wakelock.h>

#define APDS990x_DRV_NAME	"apds990x"
#define DRIVER_VERSION		"1.0.4"

#define APDS990x_POWER_ONOFF(pdata, onoff) \
        do { \
                if ((pdata)->power_onoff) { \
                        (pdata)->power_onoff(onoff); \
                } \
        } \
        while(0)

/* Change History
 *
 * 1.0.1	Functions apds990x_show_rev(), apds990x_show_id() and apds990x_show_status()
 *			have missing CMD_BYTE in the i2c_smbus_read_byte_data(). APDS-990x needs
 *			CMD_BYTE for i2c write/read byte transaction.
 *
 *
 * 1.0.2	Include PS switching threshold level when interrupt occurred
 *
 *
 * 1.0.3	Implemented ISR and delay_work, correct PS threshold storing
 *
 * 1.0.4	Added Input Report Event
 */

/*
 * Defines
 */

#define APDS990x_ENABLE_REG	0x00
#define APDS990x_ATIME_REG	0x01
#define APDS990x_PTIME_REG	0x02
#define APDS990x_WTIME_REG	0x03
#define APDS990x_AILTL_REG	0x04
#define APDS990x_AILTH_REG	0x05
#define APDS990x_AIHTL_REG	0x06
#define APDS990x_AIHTH_REG	0x07
#define APDS990x_PILTL_REG	0x08
#define APDS990x_PILTH_REG	0x09
#define APDS990x_PIHTL_REG	0x0A
#define APDS990x_PIHTH_REG	0x0B
#define APDS990x_PERS_REG	0x0C
#define APDS990x_CONFIG_REG	0x0D
#define APDS990x_PPCOUNT_REG	0x0E
#define APDS990x_CONTROL_REG	0x0F
#define APDS990x_REV_REG	0x11
#define APDS990x_ID_REG		0x12
#define APDS990x_STATUS_REG	0x13
#define APDS990x_CDATAL_REG	0x14
#define APDS990x_CDATAH_REG	0x15
#define APDS990x_IRDATAL_REG	0x16
#define APDS990x_IRDATAH_REG	0x17
#define APDS990x_PDATAL_REG	0x18
#define APDS990x_PDATAH_REG	0x19

#define CMD_BYTE	0x80
#define CMD_WORD	0xA0
#define CMD_SPECIAL	0xE0

#define CMD_CLR_PS_INT	0xE5
#define CMD_CLR_ALS_INT	0xE6
#define CMD_CLR_PS_ALS_INT	0xE7

/*
 * Structs
 */

struct apds990x_data {
	//put the platform special resource in platform code
	struct apds990x_platform_data *pdata;
	struct i2c_client *client;
	struct mutex update_lock;
	//use a dedicated spinlock instead of dwork.wait_lock
	spinlock_t wq_lock;
	struct delayed_work	dwork;	/* for PS interrupt */
	struct delayed_work als_dwork; /* for ALS polling */
	struct input_dev *input_dev_als;
	struct input_dev *input_dev_ps;
	struct wake_lock prx_wake_lock;

	unsigned int enable;
	unsigned int atime;
	unsigned int ptime;
	unsigned int wtime;
	unsigned int ailt;
	unsigned int aiht;
	unsigned int pilt;
	unsigned int piht;
	unsigned int pers;
	unsigned int config;
	unsigned int ppcount;
	unsigned int control;

	/* control flag from HAL */
	unsigned int enable_ps_sensor;
	unsigned int enable_als_sensor;

	/* PS parameters */
	unsigned int ps_threshold;
	unsigned int ps_hysteresis_threshold; /* always lower than ps_threshold */
	unsigned int ps_detection;		/* 1 = near-to-far; 0 = far-to-near */
	unsigned int ps_data;			/* to store PS data */

	/* ALS parameters */
	unsigned int als_threshold_l;	/* low threshold */
	unsigned int als_threshold_h;	/* high threshold */
	unsigned int als_data;			/* to store ALS data */

	unsigned int als_gain;			/* needed for Lux calculation */
	unsigned int als_poll_delay;	/* needed for light sensor polling : micro-second (us) */
	unsigned int als_atime;			/* storage for als integratiion time */
};

/*
 * Global data
 */

/*
 * Management functions
 */

static int apds990x_set_command(struct i2c_client *client, int command)
{
	int ret;
	int clearInt;

	if (command == 0)
		clearInt = CMD_CLR_PS_INT;
	else if (command == 1)
		clearInt = CMD_CLR_ALS_INT;
	else
		clearInt = CMD_CLR_PS_ALS_INT;
	//It`s unnecessary to lock mutex before i2c_smbus_write_byte, which is thread-safe and SMP safe.
	ret = i2c_smbus_write_byte(client, clearInt);

	return ret;
}

static int apds990x_set_enable(struct i2c_client *client, int enable)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	//Yes, the mutex is necessary for modify data->data, but I will lock it before call this function
	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, enable);
	data->enable = enable;

	return ret;
}

static int apds990x_set_atime(struct i2c_client *client, int atime)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ATIME_REG, atime);
	data->atime = atime;

	return ret;
}

static int apds990x_set_ptime(struct i2c_client *client, int ptime)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_PTIME_REG, ptime);
	data->ptime = ptime;

	return ret;
}

static int apds990x_set_wtime(struct i2c_client *client, int wtime)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_WTIME_REG, wtime);
	data->wtime = wtime;

	return ret;
}

static int apds990x_set_ailt(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AILTL_REG, threshold);
	data->ailt = threshold;

	return ret;
}

static int apds990x_set_aiht(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AIHTL_REG, threshold);
	data->aiht = threshold;

	return ret;
}

static int apds990x_set_pilt(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, threshold);
	data->pilt = threshold;

	return ret;
}

static int apds990x_set_piht(struct i2c_client *client, int threshold)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, threshold);
	data->piht = threshold;

	return ret;
}

static int apds990x_set_pers(struct i2c_client *client, int pers)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_PERS_REG, pers);
	data->pers = pers;

	return ret;
}

static int apds990x_set_config(struct i2c_client *client, int config)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_CONFIG_REG, config);
	data->config = config;

	return ret;
}

static int apds990x_set_ppcount(struct i2c_client *client, int ppcount)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_PPCOUNT_REG, ppcount);
	data->ppcount = ppcount;

	return ret;
}

static int apds990x_set_control(struct i2c_client *client, int control)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int ret;

	ret = i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_CONTROL_REG, control);
	data->control = control;

	/* obtain ALS gain value */
	if ((control&0x03) == 0x00) /* 1X Gain */
		data->als_gain = 1;
	else if ((control&0x03) == 0x01) /* 8X Gain */
		data->als_gain = 8;
	else if ((control&0x03) == 0x02) /* 16X Gain */
		data->als_gain = 16;
	else  /* 120X Gain */
		data->als_gain = 120;

	return ret;
}

static int LuxCalculation(struct i2c_client *client, int cdata, int irdata)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int luxValue=0;

	int IAC1=0;
	int IAC2=0;
	int IAC=0;
	int GA=1064;			/* 0.48 without glass window */
	int COE_B=210;		/* 2.23 without glass window */
	int COE_C=29;		/* 0.70 without glass window */
	int COE_D=57;		/* 1.42 without glass window */
	int DF=52;

	IAC1 = (cdata - (COE_B*irdata)/100);	// re-adjust COE_B to avoid 2 decimal point
	IAC2 = ((COE_C*cdata)/100 - (COE_D*irdata)/100); // re-adjust COE_C and COE_D to void 2 decimal point

	if (IAC1 > IAC2)
		IAC = IAC1;
	else if (IAC1 <= IAC2)
		IAC = IAC2;
	else
		IAC = 0;

	luxValue = ((IAC*GA*DF)/100)/(((272*(256-data->atime))/100)*data->als_gain);

	return luxValue;
}

static void apds990x_change_ps_threshold(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int retval;

	retval = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_PDATAL_REG);
	if (retval < 0)
		return;

	data->ps_data = retval;
	if ( (data->ps_data > data->pilt) && (data->ps_data >= data->piht) ) {
		/* far-to-near detected */
		data->ps_detection = 1;

		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 0);/* FAR-to-NEAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, data->ps_hysteresis_threshold);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, 1023);

		data->pilt = data->ps_hysteresis_threshold;
		data->piht = 1023;

		pr_debug("far-to-near detected\n");
	}
	else if ( (data->ps_data <= data->pilt) && (data->ps_data < data->piht) ) {
		/* near-to-far detected */
		data->ps_detection = 0;

		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		pr_debug("near-to-far detected\n");
	} else {
		pr_debug("data->ps_data = %d, data->pilt = %d, data->piht = %d\n", data->ps_data, data->pilt, data->piht);
	}
}

static void apds990x_change_als_threshold(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int cdata, irdata;
	int luxValue=0;

	cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_IRDATAL_REG);

	luxValue = LuxCalculation(client, cdata, irdata);

	luxValue = luxValue>0 ? luxValue : 0;
	luxValue = luxValue<10000 ? luxValue : 10000;
	// check PS under sunlight
	if ( (data->ps_detection == 1) && (cdata > (75*(1024*(256-data->atime)))/100))	// PS was previously in far-to-near condition
	{
		// need to inform input event as there will be no interrupt from the PS
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		data->ps_detection = 0;	/* near-to-far detected */

		pr_debug("apds_990x_proximity_handler = FAR\n");
	}

	input_report_abs(data->input_dev_als, ABS_MISC, luxValue); // report the lux level
	input_sync(data->input_dev_als);

	data->als_data = cdata;
	data->als_threshold_l = (data->als_data * (100-data->pdata->als_hsyt_thld) ) /100;
	data->als_threshold_h = (data->als_data * (100+data->pdata->als_hsyt_thld) ) /100;

	if (data->als_threshold_h >= 65535) data->als_threshold_h = 65535;

	i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AILTL_REG, data->als_threshold_l);
	i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_AIHTL_REG, data->als_threshold_h);
}

/* ALS polling routine */
static void apds990x_als_polling_work_handler(struct work_struct *work)
{
	struct apds990x_data *data = container_of(work, struct apds990x_data, als_dwork.work);
	struct i2c_client *client=data->client;
	int cdata, irdata, pdata;
	int luxValue=0;

	//1. work queue is reentrant in SMP.
	//2. serveral function here calls need mutex.
	mutex_lock(&data->update_lock);
	cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
	irdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_IRDATAL_REG);
	pdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_PDATAL_REG);

	if ((cdata < 0) || (irdata < 0) || (pdata < 0)) {
		mutex_unlock(&data->update_lock);
		return;
	}

	luxValue = LuxCalculation(client, cdata, irdata);
	luxValue = luxValue>0 ? luxValue : 0;
	luxValue = luxValue<10000 ? luxValue : 10000;
	//pr_debug("%s: lux = %d cdata = %x  irdata = %x pdata = %x \n", __func__, luxValue, cdata, irdata, pdata);
	// check PS under sunlight
	if ( (data->ps_detection == 1) && (cdata > (75*(1024*(256-data->atime)))/100))	// PS was previously in far-to-near condition
	{
		// need to inform input event as there will be no interrupt from the PS
		input_report_abs(data->input_dev_ps, ABS_DISTANCE, 1);/* NEAR-to-FAR detection */
		input_sync(data->input_dev_ps);

		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PILTL_REG, 0);
		i2c_smbus_write_word_data(client, CMD_WORD|APDS990x_PIHTL_REG, data->ps_threshold);

		data->pilt = 0;
		data->piht = data->ps_threshold;

		data->ps_detection = 0;	/* near-to-far detected */

		pr_debug("apds_990x_proximity_handler = FAR\n");
	}

	input_report_abs(data->input_dev_als, ABS_MISC, luxValue); // report the lux level
	input_sync(data->input_dev_als);

	schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// restart timer
	mutex_unlock(&data->update_lock);
}

/* PS interrupt routine */
static void apds990x_work_handler(struct work_struct *work)
{
	struct apds990x_data *data = container_of(work, struct apds990x_data, dwork.work);
	struct i2c_client *client=data->client;
	int	status;
	int cdata;
	int retry_count = 3;

retry:
	status = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS990x_STATUS_REG);
	if (status < 0) {
		pr_warning("fail to read data,status = %x\n", status);
		if (retry_count--) {
			usleep(50000);
			goto retry;
		} else {
			return;
		}
	}

	i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, 1);	/* disable 990x's ADC first */

	pr_debug("status = %x, enable = %x\n", status, data->enable);
	//Interrupt is not reentrant both in UP and MP. But some variant/function here need to thread-safe.
	mutex_lock(&data->update_lock);

	if ((status & data->enable & 0x30) == 0x30) {
		/* both PS and ALS are interrupted */
		apds990x_change_als_threshold(client);

		cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
		if (cdata < (75*(1024*(256-data->atime)))/100)
			apds990x_change_ps_threshold(client);
		else {
			if (data->ps_detection == 1) {
				apds990x_change_ps_threshold(client);
			}
			else {
				pr_debug("Triggered by background ambient noise\n");
			}
		}

		apds990x_set_command(client, 2);	/* 2 = CMD_CLR_PS_ALS_INT */
	}
	else if ((status & data->enable & 0x20) == 0x20) {
		/* only PS is interrupted */
		/* check if this is triggered by background ambient noise */
		cdata = i2c_smbus_read_word_data(client, CMD_WORD|APDS990x_CDATAL_REG);
		if (cdata < (75*(1024*(256-data->atime)))/100)
			apds990x_change_ps_threshold(client);
		else {
			if (data->ps_detection == 1) {
				apds990x_change_ps_threshold(client);
			}
			else {
				pr_debug("Triggered by background ambient noise\n");
			}
		}

		apds990x_set_command(client, 0);	/* 0 = CMD_CLR_PS_INT */
	}
	else if ((status & data->enable & 0x10) == 0x10) {
		/* only ALS is interrupted */
		apds990x_change_als_threshold(client);
		apds990x_set_command(client, 1);	/* 1 = CMD_CLR_ALS_INT */
	}
	i2c_smbus_write_byte_data(client, CMD_BYTE|APDS990x_ENABLE_REG, data->enable);
	mutex_unlock(&data->update_lock);
}

/* assume this is ISR */
static irqreturn_t apds990x_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	struct apds990x_data *data = i2c_get_clientdata(client);

	pr_debug("==> apds990x_interrupt (timeout)\n");
	wake_lock_timeout(&data->prx_wake_lock, HZ / 2);
	//Seems linux-3.0 trends to use threaded-interrupt, so we can call the work directly.
	apds990x_work_handler(&data->dwork.work);

	return IRQ_HANDLED;
}

/*
 * SysFS support
 */
static ssize_t apds990x_show_ps_sensor_thld(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d %d\n", data->pdata->ps_hsyt_thld, data->pdata->ps_det_thld);
}

static ssize_t apds990x_store_ps_sensor_thld(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);
	char *next_buf;
	unsigned long hsyt_val = simple_strtoul(buf, &next_buf, 10);
	unsigned long det_val = simple_strtoul(++next_buf, NULL, 10);

	if ((det_val < 0) || (det_val > 1023) || (hsyt_val < 0) || (hsyt_val >= det_val)) {
		pr_err("%s:store unvalid det_val=%ld, hsyt_val=%ld\n", __func__, det_val, hsyt_val);
		return -EINVAL;
	}
	mutex_lock(&data->update_lock);
	data->pdata->ps_det_thld = det_val;
	data->pdata->ps_hsyt_thld = hsyt_val;
	mutex_unlock(&data->update_lock);

	return count;
}

static DEVICE_ATTR(ps_sensor_thld, S_IWUGO | S_IRUGO,
				   apds990x_show_ps_sensor_thld, apds990x_store_ps_sensor_thld);

static ssize_t apds990x_show_enable_ps_sensor(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->enable_ps_sensor);
}

static ssize_t apds990x_store_enable_ps_sensor(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	unsigned long flags;
	
	pr_debug("%s: enable ps senosr ( %ld)\n", __func__, val);

	if ((val != 0) && (val != 1)) {
		pr_err("%s:store unvalid value=%ld\n", __func__, val);
		return count;
	}
	//some variant/function here need to thread-safe. And it`s better to finish all the steps in one time.
	mutex_lock(&data->update_lock);

	if(val == 1) {
		//turn on p sensor
		if (data->enable_ps_sensor==0) {
			data->enable_ps_sensor= 1;
			apds990x_set_enable(client,0); /* Power Off */
			apds990x_set_atime(client, 0xf6); /* 27.2ms */
			apds990x_set_ptime(client, 0xff); /* 2.72ms */
			apds990x_set_ppcount(client, 8); /* 8-pulse */
			apds990x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
			apds990x_set_pilt(client, 0);		// init threshold for proximity
			apds990x_set_piht(client, data->pdata->ps_det_thld);
			data->ps_threshold = data->pdata->ps_det_thld;
			data->ps_hysteresis_threshold = data->pdata->ps_hsyt_thld;
			apds990x_set_ailt( client, 0);
			apds990x_set_aiht( client, 0xffff);
			apds990x_set_pers(client, 0x33); /* 3 persistence */

			if (data->enable_als_sensor==0) {

				/* we need this polling timer routine for sunlight canellation */
				spin_lock_irqsave(&data->wq_lock, flags);

				/*
				 * If work is already scheduled then subsequent schedules will not
				 * change the scheduled time that's why we have to cancel it first.
				 */
				__cancel_delayed_work(&data->als_dwork);
				schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}

			apds990x_set_enable(client, 0x27);	 /* only enable PS interrupt */
		}
	} 
	else {
		//turn off p sensor - kk 25 Apr 2011 we can't turn off the entire sensor, the light sensor may be needed by HAL
			data->enable_ps_sensor = 0;
			if (data->enable_als_sensor) {
				// reconfigute light sensor setting
				apds990x_set_enable(client,0); /* Power Off */
				apds990x_set_atime(client, data->als_atime);  /* previous als poll delay */
				apds990x_set_ailt( client, 0);
				apds990x_set_aiht( client, 0xffff);
				apds990x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
				apds990x_set_pers(client, 0x33); /* 3 persistence */
				apds990x_set_enable(client, 0x3);	 /* only enable light sensor */
				/*
				 * If work is already scheduled then subsequent schedules will not
				 * change the scheduled time that's why we have to cancel it first.
				 */
				spin_lock_irqsave(&data->wq_lock, flags);

				__cancel_delayed_work(&data->als_dwork);
				schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}
			else {
				apds990x_set_enable(client, 0);			
				/*
				 * If work is already scheduled then subsequent schedules will not
				 * change the scheduled time that's why we have to cancel it first.
				 */
				spin_lock_irqsave(&data->wq_lock, flags);

				__cancel_delayed_work(&data->als_dwork);

				spin_unlock_irqrestore(&data->wq_lock, flags);
			}
	}

	mutex_unlock(&data->update_lock);

	return count;
}

static DEVICE_ATTR(enable_ps_sensor, S_IWUSR | S_IWGRP | S_IRUGO,
				   apds990x_show_enable_ps_sensor, apds990x_store_enable_ps_sensor);

static ssize_t apds990x_show_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->enable_als_sensor);
}

static ssize_t apds990x_store_enable_als_sensor(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
 	unsigned long flags;

	pr_debug("%s: enable als sensor ( %ld)\n", __func__, val);

	if ((val != 0) && (val != 1))
	{
		pr_err("%s: enable als sensor=%ld\n", __func__, val);
		return count;
	}

	mutex_lock(&data->update_lock);

	if(val == 1) {
		//turn on light  sensor
		if (data->enable_als_sensor==0) {
			data->enable_als_sensor = 1;
			apds990x_set_enable(client,0); /* Power Off */
			apds990x_set_atime(client, data->als_atime);  /* 100.64ms */
			apds990x_set_ailt( client, 0);
			apds990x_set_aiht( client, 0xffff);
			apds990x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
			apds990x_set_pers(client, 0x33); /* 3 persistence */

			if (data->enable_ps_sensor) {
				apds990x_set_ptime(client, 0xff); /* 2.72ms */
				apds990x_set_ppcount(client, 8); /* 8-pulse */
				apds990x_set_enable(client, 0x27);	 /* if prox sensor was activated previously */
			}
			else {
				apds990x_set_enable(client, 0x3);	 /* only enable light sensor */
			}
			/*
			 * If work is already scheduled then subsequent schedules will not
			 * change the scheduled time that's why we have to cancel it first.
			 */
			spin_lock_irqsave(&data->wq_lock, flags);

			__cancel_delayed_work(&data->als_dwork);
			schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));

			spin_unlock_irqrestore(&data->wq_lock, flags);
		}
	}
	else {
		if (data->enable_als_sensor==1) {
			data->enable_als_sensor = 0;
			if (data->enable_ps_sensor) {
				apds990x_set_enable(client,0); /* Power Off */
				apds990x_set_atime(client, 0xf6);  /* 27.2ms */
				apds990x_set_ptime(client, 0xff); /* 2.72ms */
				apds990x_set_ppcount(client, 8); /* 8-pulse */
				apds990x_set_control(client, 0x60); /* 100mA, IR-diode, 1X PGAIN, 1X AGAIN */
				apds990x_set_piht(client, 0);
				apds990x_set_piht(client, data->pdata->ps_det_thld);
				apds990x_set_ailt( client, 0);
				apds990x_set_aiht( client, 0xffff);
				apds990x_set_pers(client, 0x33); /* 3 persistence */
				apds990x_set_enable(client, 0x27);	 /* only enable prox sensor with interrupt */
			}
			else {
				apds990x_set_enable(client, 0);
			}

			/*
			 * If work is already scheduled then subsequent schedules will not
			 * change the scheduled time that's why we have to cancel it first.
			 */
			spin_lock_irqsave(&data->wq_lock, flags);

			__cancel_delayed_work(&data->als_dwork);

			spin_unlock_irqrestore(&data->wq_lock, flags);
		}
	}
	mutex_unlock(&data->update_lock);

	return count;
}

static DEVICE_ATTR(enable_als_sensor, S_IWUSR | S_IWGRP | S_IRUGO,
				   apds990x_show_enable_als_sensor, apds990x_store_enable_als_sensor);

static ssize_t apds990x_show_als_poll_delay(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", data->als_poll_delay*1000);	// return in micro-second
}

static ssize_t apds990x_store_als_poll_delay(struct device *dev,
					struct device_attribute *attr, const char *buf, size_t count)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct apds990x_data *data = i2c_get_clientdata(client);
	unsigned long val = simple_strtoul(buf, NULL, 10);
	int ret;
	int poll_delay=0;
	unsigned long flags;

	if (val<5000)
		val = 5000;	// minimum 5ms

	mutex_lock(&data->update_lock);
	data->als_poll_delay = val/1000;	// convert us => ms

	// val * 2 uses bigger integration time, which will provide more accurate output.
	poll_delay = 256 - (val * 2 / 2720);	// the minimum is 2.72ms = 2720 us, maximum is 696.32ms
	if (poll_delay >= 256)
		data->als_atime = 255;
	else if (poll_delay < 0)
		data->als_atime = 0;
	else
		data->als_atime = poll_delay;

	ret = apds990x_set_atime(client, data->als_atime);

	if (ret < 0)
		return ret;

	/* we need this polling timer routine for sunlight canellation */
	/*
	 * If work is already scheduled then subsequent schedules will not
	 * change the scheduled time that's why we have to cancel it first.
	 */
	spin_lock_irqsave(&data->wq_lock, flags); 

	__cancel_delayed_work(&data->als_dwork);
	schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));	// 100ms

	spin_unlock_irqrestore(&data->wq_lock, flags);

	mutex_unlock(&data->update_lock);

	return count;
}

static DEVICE_ATTR(als_poll_delay, S_IWUSR | S_IWGRP | S_IRUGO,
				   apds990x_show_als_poll_delay, apds990x_store_als_poll_delay);

static struct attribute *apds990x_attributes[] = {
	&dev_attr_enable_ps_sensor.attr,
	&dev_attr_enable_als_sensor.attr,
	&dev_attr_als_poll_delay.attr,
	&dev_attr_ps_sensor_thld.attr,
	NULL
};


static const struct attribute_group apds990x_attr_group = {
	.attrs = apds990x_attributes,
};

/*
 * Initialization function
 */

static int apds990x_init_client(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int err;
	int id;

	err = apds990x_set_enable(client, 0);

	if (err < 0)
		return err;

	id = i2c_smbus_read_byte_data(client, CMD_BYTE|APDS990x_ID_REG);
	if (id == 0x20) {
		pr_debug("APDS-9901\n");
	}
	else if (id == 0x29) {
		pr_debug("APDS-990x\n");
	}
	else {
		pr_err("Neither APDS-9901 nor APDS-9901\n");
		return -EIO;
	}

	apds990x_set_atime(client, 0xDB);	// 100.64ms ALS integration time
	apds990x_set_ptime(client, 0xFF);	// 2.72ms Prox integration time
	apds990x_set_wtime(client, 0xFF);	// 2.72ms Wait time

	apds990x_set_ppcount(client, 0x08);	// 8-Pulse for proximity
	apds990x_set_config(client, 0);		// no long wait
	apds990x_set_control(client, 0x60);	// 100mA, IR-diode, 1X PGAIN, 1X AGAIN

	apds990x_set_pilt(client, 0);		// init threshold for proximity
	apds990x_set_piht(client, data->pdata->ps_det_thld);

	data->ps_threshold = data->pdata->ps_det_thld;
	data->ps_hysteresis_threshold = data->pdata->ps_hsyt_thld;

	apds990x_set_ailt(client, 0);		// init threshold for als
	apds990x_set_aiht(client, 0xFFFF);

	apds990x_set_pers(client, 0x22);	// 2 consecutive Interrupt persistence

	// sensor is in disabled mode but all the configurations are preset

	return 0;
}

/*
 * I2C init/probing/exit functions
 */

static struct i2c_driver apds990x_driver;
static int __devinit apds990x_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct apds990x_data *data;
	int err = 0;
	struct apds990x_platform_data *pdata = client->dev.platform_data;

	if (!pdata || (pdata->irq <= 0)) {
        	pr_err("%s: platform resource is no enough!\n", __func__);
        	return -ENODEV;
    	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE)) {
		err = -EIO;
		goto exit;
	}

	data = kzalloc(sizeof(struct apds990x_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	data->pdata = pdata;
	data->client = client;
	i2c_set_clientdata(client, data);

	data->enable = 0;	/* default mode is standard */
	data->ps_threshold = 0;
	data->ps_hysteresis_threshold = 0;
	data->ps_detection = 0;	/* default to no detection */
	data->enable_als_sensor = 0;	// default to 0
	data->enable_ps_sensor = 0;	// default to 0
	data->als_poll_delay = 100;	// default to 100ms
	data->als_atime	= 0xdb;			// work in conjuction with als_poll_delay

	pr_debug("enable = %x\n", data->enable);

	mutex_init(&data->update_lock);
	spin_lock_init(&data->wq_lock);

	wake_lock_init(&data->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	INIT_DELAYED_WORK(&data->dwork, apds990x_work_handler);
	INIT_DELAYED_WORK(&data->als_dwork, apds990x_als_polling_work_handler);

	pr_debug("%s interrupt is hooked\n", __func__);

	/* Initialize the APDS990x chip */
	err = apds990x_init_client(client);
	if (err)
		goto exit_kfree;

	/* Register to Input Device */
	data->input_dev_als = input_allocate_device();
	if (!data->input_dev_als) {
		err = -ENOMEM;
		pr_err("Failed to allocate input device als\n");
		goto exit_kfree;
	}

	data->input_dev_ps = input_allocate_device();
	if (!data->input_dev_ps) {
		err = -ENOMEM;
		pr_err("Failed to allocate input device ps\n");
		goto exit_free_dev_als;
	}

	set_bit(EV_ABS, data->input_dev_als->evbit);
	set_bit(EV_ABS, data->input_dev_ps->evbit);

	input_set_abs_params(data->input_dev_als, ABS_MISC, 0, 10000, 0, 0);
	input_set_abs_params(data->input_dev_ps, ABS_DISTANCE, 0, 1, 0, 0);

	data->input_dev_als->name = "Avago light sensor";
	data->input_dev_ps->name = "Avago proximity sensor";

	err = input_register_device(data->input_dev_als);
	if (err) {
		err = -ENOMEM;
		pr_err("Unable to register input device als: %s\n",
		       data->input_dev_als->name);
		goto exit_free_dev_ps;
	}

	err = input_register_device(data->input_dev_ps);
	if (err) {
		err = -ENOMEM;
		pr_err("Unable to register input device ps: %s\n",
		       data->input_dev_ps->name);
		goto exit_unregister_dev_als;
	}

	/* Register sysfs hooks */
	err = sysfs_create_group(&client->dev.kobj, &apds990x_attr_group);
	if (err)
		goto exit_unregister_dev_ps;

	err = request_threaded_irq(data->pdata->irq, NULL, apds990x_interrupt, IRQ_TYPE_EDGE_FALLING,
		APDS990x_DRV_NAME, (void *)client);
	if (err) {
		pr_err("%s Could not allocate irq(%d) !\n", __func__, data->pdata->irq);
		goto exit_remove_sysfs;
	}
	device_init_wakeup(&client->dev, 1);
	pr_debug("%s support ver. %s enabled\n", __func__, DRIVER_VERSION);

	return 0;

exit_remove_sysfs:
	sysfs_remove_group(&client->dev.kobj, &apds990x_attr_group);
exit_unregister_dev_ps:
	input_unregister_device(data->input_dev_ps);
exit_unregister_dev_als:
	input_unregister_device(data->input_dev_als);
exit_free_dev_ps:
	input_free_device(data->input_dev_ps);
exit_free_dev_als:
	input_free_device(data->input_dev_als);
exit_kfree:
	wake_lock_destroy(&data->prx_wake_lock);
	kfree(data);
exit:
	return err;
}

static int __devexit apds990x_remove(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);

	disable_irq(data->pdata->irq);
	device_init_wakeup(&client->dev, 0);
	cancel_delayed_work_sync(&data->als_dwork);

	input_unregister_device(data->input_dev_als);
	input_unregister_device(data->input_dev_ps);

	input_free_device(data->input_dev_als);
	input_free_device(data->input_dev_ps);

	free_irq(data->pdata->irq, client);

	sysfs_remove_group(&client->dev.kobj, &apds990x_attr_group);

	/* Power down the device */
	apds990x_set_enable(client, 0);

	wake_lock_destroy(&data->prx_wake_lock);
	kfree(data);

	return 0;
}

#ifdef CONFIG_PM

static int apds990x_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int enable = 0;

	disable_irq(data->pdata->irq);
	if (data->enable_als_sensor)
		cancel_delayed_work_sync(&data->als_dwork);

	if (data->enable_ps_sensor) {
		enable = 0x27;
		enable_irq_wake(data->pdata->irq);
	}

	if (enable != data->enable)
		apds990x_set_enable(client, enable);

	return 0;
}

static int apds990x_resume(struct i2c_client *client)
{
	struct apds990x_data *data = i2c_get_clientdata(client);
	int enable = 0;

	if (data->enable_als_sensor)
		enable |= 0x03;

	if (data->enable_ps_sensor) {
		enable |= 0x27;
		disable_irq_wake(data->pdata->irq);
	}

	if (enable != 0x27)
		apds990x_set_enable(client, enable);

	if (data->enable_als_sensor)
		schedule_delayed_work(&data->als_dwork, msecs_to_jiffies(data->als_poll_delay));

	enable_irq(data->pdata->irq);

	return 0;
}

#else

#define apds990x_suspend	NULL
#define apds990x_resume		NULL

#endif /* CONFIG_PM */

static const struct i2c_device_id apds990x_id[] = {
	{ "apds990x", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, apds990x_id);

static struct i2c_driver apds990x_driver = {
	.driver = {
		.name	= APDS990x_DRV_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = apds990x_suspend,
	.resume	= apds990x_resume,
	.probe	= apds990x_probe,
	.remove	= __devexit_p(apds990x_remove),
	.id_table = apds990x_id,
};

static int __init apds990x_init(void)
{
	return i2c_add_driver(&apds990x_driver);
}

static void __exit apds990x_exit(void)
{
	i2c_del_driver(&apds990x_driver);
}

MODULE_DESCRIPTION("APDS990X combined ALS and proximity sensor");
MODULE_AUTHOR("Samu Onkalo, Nokia Corporation");
MODULE_LICENSE("GPL v2");
MODULE_VERSION(DRIVER_VERSION);

module_init(apds990x_init);
module_exit(apds990x_exit);
