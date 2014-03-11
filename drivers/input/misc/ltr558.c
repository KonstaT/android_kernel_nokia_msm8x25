/*
 * Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/earlysuspend.h>
#include <linux/input/ltr5xx.h>


//#define DEBUG_LTR558
#ifdef DEBUG_LTR558
#define LTR558_DBGMSG(str, args...) printk("%s: " str, __func__, ##args)
#else
#define LTR558_DBGMSG(str, args...)
#endif

/* LTR-558 Registers */
#define LTR558_ALS_CONTR	0x80
#define LTR558_PS_CONTR		0x81
#define LTR558_PS_LED		0x82
#define LTR558_PS_N_PULSES	0x83
#define LTR558_PS_MEAS_RATE	0x84
#define LTR558_ALS_MEAS_RATE	0x85
#define LTR558_MANUFACTURER_ID	0x87

#define LTR558_INTERRUPT	0x8F
#define LTR558_PS_THRES_UP_0	0x90
#define LTR558_PS_THRES_UP_1	0x91
#define LTR558_PS_THRES_LOW_0	0x92
#define LTR558_PS_THRES_LOW_1	0x93

#define LTR558_ALS_THRES_UP_0	0x97
#define LTR558_ALS_THRES_UP_1	0x98
#define LTR558_ALS_THRES_LOW_0	0x99
#define LTR558_ALS_THRES_LOW_1	0x9A

#define LTR558_INTERRUPT_PERSIST 0x9E

/* 558's Read Only Registers */
#define LTR558_ALS_DATA_CH1_0	0x88
#define LTR558_ALS_DATA_CH1_1	0x89
#define LTR558_ALS_DATA_CH0_0	0x8A
#define LTR558_ALS_DATA_CH0_1	0x8B
#define LTR558_ALS_PS_STATUS	0x8C
#define LTR558_PS_DATA_0	0x8D
#define LTR558_PS_DATA_1	0x8E

/* Basic Operating Modes */
#define MODE_ALS_ON_Range1	0x0b
#define MODE_ALS_ON_Range2	0x03
#define MODE_ALS_StdBy		0x00
#define MODE_ALS_ON_FLAG	0x02


#define MODE_PS_ON_Gain1	0x03
#define MODE_PS_ON_Gain4	0x07
#define MODE_PS_ON_Gain8	0x0B
#define MODE_PS_ON_Gain16	0x0f
#define MODE_PS_StdBy		0x00
#define MODE_ACTIVE			0x02

#define INTERRUPT_OP_MODE	0x08	/* updated after every measurement*/
#define INTERRUPT_POLARITY	0x00	/* low active */
#define ENABLE_PS_INTERRUPT	0x01
#define ENABLE_ALS_INTERRUPT	0x02

#define DEFAULT_INT_MODE	(INTERRUPT_OP_MODE | INTERRUPT_POLARITY)

#define PS_RANGE1 		1
#define PS_RANGE2		2
#define PS_RANGE4 		4
#define PS_RANGE8		8

#define ALS_RANGE1_320		1
#define ALS_RANGE2_64K 		2

#define PON_DELAY		600
#define WAKEUP_DELAY		10
#define ALS_INTEGRATION_TIME 	100

#define SENSOR_NAME		"ltr558"

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* driver data */
struct ltr558_data {
	bool on;
	u8 power_state;		/* sensor power control state */
	struct ltr5xx_platform_data *pdata;
	struct i2c_client 		*i2c_client;
	struct mutex 			lock;
	struct workqueue_struct *wq;
	struct early_suspend 	early_suspend;
	struct wake_lock 		prx_wake_lock;

	struct input_dev 		*light_input_dev;
	struct work_struct 		work_light;
	struct hrtimer 			light_timer;
	ktime_t 				light_poll_delay;

	struct input_dev 		*proximity_input_dev;
	struct work_struct 		work_proximity;
	bool					ps_is_near_state;
};

static int ps_gainrange;
static int als_gainrange;
static int als_integration_time;
static struct i2c_client *ltr588_client = NULL;
static int prox_threshold_hi = 200;	//Default value
static int prox_threshold_lo = 160;

static void ltr558_early_suspend(struct early_suspend *h);
static void ltr558_late_resume(struct early_suspend *h);

static int ltr558_i2c_read_reg(u8 regnum)
{
	int readdata;
	readdata = i2c_smbus_read_byte_data(ltr588_client, regnum);
	return readdata;
}

static int ltr558_i2c_write_reg(u8 regnum, u8 value)
{
	int writeerror;
	writeerror = i2c_smbus_write_byte_data(ltr588_client, regnum, value);
	if (writeerror < 0)
		return writeerror;
	else
		return 0;
}

static int ltr558_light_enable(struct ltr558_data *ltr558)
{
	int error=0;
	LTR558_DBGMSG("ltr558_light_enable\n");
	/* ===============
	* ** IMPORTANT **
	* ===============
	* Other settings like timing and threshold to be set here, if required.
	* Not set and kept as device default for now.
	*/
        ltr558_i2c_write_reg(LTR558_ALS_THRES_LOW_0, 0xff); //0xff
        ltr558_i2c_write_reg(LTR558_ALS_THRES_LOW_1, 0xff); //0xff
        ltr558_i2c_write_reg(LTR558_ALS_THRES_UP_0, 0); //0
        ltr558_i2c_write_reg(LTR558_ALS_THRES_UP_1, 0); //0

	if (als_gainrange == 1)
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_ON_Range1);
	else if (als_gainrange == 2)
		error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_ON_Range2);//03
	else
		error = 1;//flase arg value

	msleep(WAKEUP_DELAY);
	return error;
}

static int ltr558_light_disable(struct ltr558_data *ltr558)
{
	int error;
	error = ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_StdBy);
	return error;
}

static ssize_t light_poll_delay_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ltr558_data *ltr558 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(ltr558->light_poll_delay));
}

static ssize_t light_poll_delay_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct ltr558_data *ltr558 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	LTR558_DBGMSG("new delay = %lldns, old delay = %lldns\n",
		      new_delay, ktime_to_ns(ltr558->light_poll_delay));
	mutex_lock(&ltr558->lock);
	if (new_delay != ktime_to_ns(ltr558->light_poll_delay)) {
		ltr558->light_poll_delay = ns_to_ktime(new_delay);
		if (ltr558->power_state & LIGHT_ENABLED) {
			hrtimer_cancel(&ltr558->light_timer);
			cancel_work_sync(&ltr558->work_light);
			hrtimer_start(&ltr558->light_timer,
				      ltr558->light_poll_delay,
				      HRTIMER_MODE_REL);
		}
	}
	mutex_unlock(&ltr558->lock);
	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct ltr558_data *ltr558 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (ltr558->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct ltr558_data *ltr558 = dev_get_drvdata(dev);
	bool new_value;
	int ret = -EINVAL;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return ret;
	}

	mutex_lock(&ltr558->lock);
	LTR558_DBGMSG("new_value = %d, old state = %d\n",
		      new_value, (ltr558->power_state & LIGHT_ENABLED) ? 1 : 0);
	if (new_value && !(ltr558->power_state & LIGHT_ENABLED)) {
		if (!ltr558_light_enable(ltr558)) {
			ret = size;
			ltr558->power_state |= LIGHT_ENABLED;
			hrtimer_start(&ltr558->light_timer,
				      ltr558->light_poll_delay,
				      HRTIMER_MODE_REL);
		}
	} else if (!new_value && (ltr558->power_state & LIGHT_ENABLED)) {
		if (!ltr558_light_disable(ltr558)) {
			ret = size;
			ltr558->power_state &= ~LIGHT_ENABLED;
			hrtimer_cancel(&ltr558->light_timer);
			cancel_work_sync(&ltr558->work_light);
		}
	}
	mutex_unlock(&ltr558->lock);

	return ret;
}

static int ltr558_proximity_enable(struct ltr558_data *ltr558)
{
	int setgain;
	int read_val;
	char buf[4] = {0};

	LTR558_DBGMSG("ltr558_proximity_enable\n");

	buf[0] = prox_threshold_lo & 0x0ff;
	buf[1] = (prox_threshold_lo >> 8) & 0x07;
	buf[2] = prox_threshold_hi & 0x0ff;  //up
	buf[3] = (prox_threshold_hi >> 8) & 0x07;
	ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_0, buf[0]);
	ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_1, buf[1]);
	ltr558_i2c_write_reg(LTR558_PS_THRES_UP_0,  buf[2]);
	ltr558_i2c_write_reg(LTR558_PS_THRES_UP_1,  buf[3]);

	switch (ps_gainrange) {
		case PS_RANGE1:
			setgain = MODE_PS_ON_Gain1;//03
			break;
		case PS_RANGE2:
			setgain = MODE_PS_ON_Gain4;
			break;
		case PS_RANGE4:
			setgain = MODE_PS_ON_Gain8;
			break;
		case PS_RANGE8:
			setgain = MODE_PS_ON_Gain16;
			break;
		default:
			setgain = MODE_PS_ON_Gain1;
			break;
	}

	setgain |= MODE_ACTIVE;
	if(ltr558_i2c_write_reg(LTR558_PS_CONTR, setgain)){
		pr_err("%s: write PS control error .\n", __func__);
	}
	/* enable ALS sensor to ensure PS working proprely */
	read_val = ltr558_i2c_read_reg(LTR558_ALS_CONTR);
	if(read_val < 0 ){
		pr_err("%s: read PS control error .\n", __func__);
	}
	else if(!(read_val & MODE_ALS_ON_FLAG)){
		if(ltr558_i2c_write_reg(LTR558_ALS_CONTR, (read_val | MODE_ALS_ON_FLAG))){
			pr_err("%s: turn on  ALS control error .\n", __func__);
		}
	}

	msleep(WAKEUP_DELAY);

	LTR558_DBGMSG("WAKEUP_DELAY 10ms after write cmd register.\n");
	return 0; /* always return true since error state is not handled by caller */
}

static int ltr558_proximity_disable(struct ltr558_data *ltr558)
{
	int err;

	if(!(ltr558->power_state & LIGHT_ENABLED)){
		/* turn off ALS if light is not enable by uplayer*/
		if(ltr558_i2c_write_reg(LTR558_ALS_CONTR, MODE_ALS_StdBy)){
			pr_err("%s: turn off ALS control error .\n", __func__);
		}
	}

	err = ltr558_i2c_write_reg(LTR558_PS_CONTR, MODE_PS_StdBy);
	if(err){
		pr_err("%s: write PS interrupt control error %d.\n", __func__, err);
	}
	return err;
}


static ssize_t proximity_enable_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ltr558_data *ltr558 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (ltr558->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct ltr558_data *ltr558 = dev_get_drvdata(dev);
	bool new_value;
	int ret = -EINVAL;

	if (sysfs_streq(buf, "1"))
		new_value = true;
	else if (sysfs_streq(buf, "0"))
		new_value = false;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return ret;
	}

	mutex_lock(&ltr558->lock);
	LTR558_DBGMSG("%s, new_value = %d, old state = %d\n",
		      __func__, new_value,
		      (ltr558->power_state & PROXIMITY_ENABLED) ? 1 : 0);
	if (new_value && !(ltr558->power_state & PROXIMITY_ENABLED)) {
		if (!ltr558_proximity_enable(ltr558)) {
			ret = size;
			ltr558->power_state |= PROXIMITY_ENABLED;
		}
	} else if (!new_value && (ltr558->power_state & PROXIMITY_ENABLED)) {
		if (!ltr558_proximity_disable(ltr558)) {
			ret = size;
			ltr558->power_state &= ~PROXIMITY_ENABLED;
			cancel_work_sync(&ltr558->work_proximity);
		}
	}
	mutex_unlock(&ltr558->lock);

	return ret;
}

static DEVICE_ATTR(light_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   light_poll_delay_show, light_poll_delay_store);

static struct device_attribute dev_attr_light_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
       light_enable_show, light_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	&dev_attr_light_poll_delay.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};


static struct device_attribute dev_attr_proximity_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
       proximity_enable_show, proximity_enable_store);

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static void ltr558_work_func_proximity(struct work_struct *work)
{
	struct ltr558_data *ltr558 = container_of(work, struct ltr558_data,
						  work_proximity);
	int psval_lo, psval_hi, psdata;
	int	retry_count = 3;
    int data = 0;

	for(psval_lo =-1, psval_hi=-1; retry_count > 0 ; retry_count--){
		psval_lo = ltr558_i2c_read_reg(LTR558_PS_DATA_0);
		psval_hi = ltr558_i2c_read_reg(LTR558_PS_DATA_1);
		if ((psval_lo < 0) || (psval_hi < 0)){
			usleep(50000);
			continue;
		}
	}
	if(psval_lo < 0 || psval_hi < 0 ){
		goto out;
	}
	psdata = ((psval_hi & 7)* 256) + psval_lo;
    if (psdata < prox_threshold_lo){  /* far */
		data = 2;
		ltr558->ps_is_near_state = false;
		ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_0, 0);
		ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_1, 0);
		ltr558_i2c_write_reg(LTR558_PS_THRES_UP_0,  prox_threshold_hi & 0xFF);
		ltr558_i2c_write_reg(LTR558_PS_THRES_UP_1,  ((prox_threshold_hi >> 8) & 0x07));
		input_report_abs(ltr558->proximity_input_dev, ABS_DISTANCE, data);
		input_sync(ltr558->proximity_input_dev);
    }
    else if (psdata > prox_threshold_hi){ /* near */
		data = 0;
		ltr558->ps_is_near_state = true;

		ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_0, prox_threshold_lo & 0xFF);
		ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_1, ((prox_threshold_lo >> 8) & 0x07));
		ltr558_i2c_write_reg(LTR558_PS_THRES_UP_0,	0xFF);
		ltr558_i2c_write_reg(LTR558_PS_THRES_UP_1,	0x07);
		input_report_abs(ltr558->proximity_input_dev, ABS_DISTANCE, data);
		input_sync(ltr558->proximity_input_dev);
    }
	else{
		/* setup threshold for next interrupt detection */
		ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_0, prox_threshold_lo & 0x0ff);
		ltr558_i2c_write_reg(LTR558_PS_THRES_LOW_1, ((prox_threshold_lo >> 8) & 0x07));
		ltr558_i2c_write_reg(LTR558_PS_THRES_UP_0,  prox_threshold_hi & 0x0ff);
		ltr558_i2c_write_reg(LTR558_PS_THRES_UP_1,  ((prox_threshold_hi >> 8) & 0x07));
	}
	LTR558_DBGMSG(">>>%s : primitive prox_data psdata = %d mode=%d.\n", __func__, psdata, data);
	return;

out:
	pr_err("%s, error occured psval_lo=%d, psval_hi=%d.\n", __func__, psval_lo, psval_hi);
	return;
}

static void ltr558_work_func_light(struct work_struct *work)
{
	int als_ps_status;
	int interrupt, newdata;

	struct ltr558_data *ltr558 = container_of(work, struct ltr558_data,work_light);
	int alsval_ch0_lo, alsval_ch0_hi, alsval_ch0;
	int alsval_ch1_lo, alsval_ch1_hi, alsval_ch1;
	int ratio;
	int lux_val;
	int ch0_coeff,ch1_coeff;

	als_ps_status = ltr558_i2c_read_reg(LTR558_ALS_PS_STATUS);
	interrupt = als_ps_status & 10;
	newdata = als_ps_status & 5;
	if ((newdata == 4) | (newdata == 5)){
		alsval_ch1_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_0);
		alsval_ch1_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH1_1);
		alsval_ch1 = (alsval_ch1_hi * 256) + alsval_ch1_lo;

		alsval_ch0_lo = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_0);
		alsval_ch0_hi = ltr558_i2c_read_reg(LTR558_ALS_DATA_CH0_1);
		alsval_ch0 = (alsval_ch0_hi * 256) + alsval_ch0_lo;

		ratio = (alsval_ch1 * 1000)  / (alsval_ch0+alsval_ch1);   //*1000

		if (ratio > 850){
			ch0_coeff = 0;
			ch1_coeff = 0;
		}else if(ratio > 640){
			ch0_coeff = 16900;  //*10000
			ch1_coeff = 1690;
		}else if(ratio > 450){
			ch0_coeff = 37725;
			ch1_coeff = 13363;
		}else{
			ch0_coeff = 17743;
			ch1_coeff = -11059;
		}

		lux_val = (alsval_ch0*ch0_coeff - alsval_ch1*ch1_coeff)/100000; //0 to 75 in lux adjust 10000->100000
		LTR558_DBGMSG(">>>>>%s: lux = %d, ch0=%d, ch1=%d, ratio=%d \n", __func__, lux_val, alsval_ch0, alsval_ch1, ratio);
		input_report_abs(ltr558->light_input_dev, ABS_MISC,lux_val);
		input_sync(ltr558->light_input_dev);
	}

}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart ltr558_light_timer_func(struct hrtimer *timer)
{
	struct ltr558_data *ltr558 =
	    container_of(timer, struct ltr558_data, light_timer);
	queue_work(ltr558->wq, &ltr558->work_light);
	hrtimer_forward_now(&ltr558->light_timer, ltr558->light_poll_delay);
	return HRTIMER_RESTART;
}

/* assume this is ISR */
static irqreturn_t ltr558_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	struct ltr558_data *data = i2c_get_clientdata(client);

	LTR558_DBGMSG("==> ltr558_interrupt (timeout)\n");
	//Seems linux-3.0 trends to use threaded-interrupt, so we can call the work directly.
	wake_lock(&data->prx_wake_lock);
	ltr558_work_func_proximity(&data->work_proximity);
	wake_unlock(&data->prx_wake_lock);

	return IRQ_HANDLED;
}

static int ltr558_hardware_init(void)
{
	int ret = 0;
	int init_ps_gain;
	int init_als_gain;

	msleep(PON_DELAY);

	// full Gain1 at startup
	init_ps_gain = PS_RANGE1;
	ps_gainrange = init_ps_gain;

	// Full Range at startup
	init_als_gain = ALS_RANGE1_320;
	als_gainrange = init_als_gain;
	als_integration_time = ALS_INTEGRATION_TIME;
	/* Must enable INT before activate */
	ret = ltr558_i2c_write_reg(LTR558_INTERRUPT, DEFAULT_INT_MODE | ENABLE_PS_INTERRUPT);
	if (ret < 0) {
		LTR558_DBGMSG("ltr558_hardware_init fail\n");
		return ret;
	}
	ltr558_i2c_write_reg(LTR558_PS_LED, 0x7b);			/* 60K HZ 100% duty cycle 50mA */
	ltr558_i2c_write_reg(LTR558_PS_N_PULSES, 0x08);		/* 8 pulses */
	ltr558_i2c_write_reg(LTR558_PS_MEAS_RATE, 0x02);	/* 100ms integration, 200ms repreat */
	ltr558_i2c_write_reg(LTR558_INTERRUPT_PERSIST, 0x00);

	LTR558_DBGMSG("ltr558_hardware_init success\n");
	return ret;
}

static int ltr558_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct input_dev *input_dev;
	struct ltr558_data *ltr558;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	ltr558 = kzalloc(sizeof(struct ltr558_data), GFP_KERNEL);
	if (!ltr558) {
		pr_err("%s: failed to alloc memory for module data\n",
		       __func__);
		return -ENOMEM;
	}

	ltr558->pdata = client->dev.platform_data;
	ltr558->power_state = 0;
	ltr558->i2c_client = client;
	ltr558->ps_is_near_state = false;
	ltr588_client = ltr558->i2c_client;
	i2c_set_clientdata(client, ltr558);

	ret = ltr558_hardware_init();
	if(ret < 0)
		goto err_hardware_init;
	/* wake lock init */
	wake_lock_init(&ltr558->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	mutex_init(&ltr558->lock);

	/* allocate proximity input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		goto err_input_allocate_device_proximity;
	}
	ltr558->proximity_input_dev = input_dev;
	input_set_drvdata(input_dev, ltr558);
	input_dev->name = "proximity";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	LTR558_DBGMSG("registering proximity input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register input device\n", __func__);
		goto err_input_register_device_proximity;
	}

	ret = sysfs_create_group(&input_dev->dev.kobj,
				 &proximity_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_proximity;
	}
	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_light;
	}
	input_set_drvdata(input_dev, ltr558);
	input_dev->name = "light";
	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_MISC, 0, 1, 0, 0);

	LTR558_DBGMSG("registering light sensor input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}
	ltr558->light_input_dev = input_dev;
	ret = sysfs_create_group(&input_dev->dev.kobj, &light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	/* initialize timer for polling and workqueue for i2c reading */
	/* light hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&ltr558->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ltr558->light_poll_delay = ns_to_ktime(1000 * NSEC_PER_MSEC);
	ltr558->light_timer.function = ltr558_light_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	ltr558->wq = create_singlethread_workqueue("ltr558_wq");
	if (!ltr558->wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&ltr558->work_light, ltr558_work_func_light);

	/* this is the thread function we run on the work queue */
	INIT_WORK(&ltr558->work_proximity, ltr558_work_func_proximity);
#ifdef CONFIG_HAS_EARLYSUSPEND
	/* register suspend/resume function */
	ltr558->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	ltr558->early_suspend.suspend = ltr558_early_suspend;
	ltr558->early_suspend.resume = ltr558_late_resume;
	register_early_suspend(&ltr558->early_suspend);
#endif /* !CONFIG_HAS_EARLYSUSPEND */

	/* setup interrupt and wakeup */
	device_init_wakeup(&client->dev, true);
	/* Looks there will receive a fake interrupt after this request, careful */
	/* keep this as the last statement in this function to avoid interrupt by interrupt */
	ret = request_threaded_irq(ltr558->pdata->int_gpio, NULL, ltr558_interrupt, IRQ_TYPE_EDGE_FALLING,
			SENSOR_NAME, (void *)client);
	if (ret) {
		pr_err("%s Could not allocate irq(%d) !\n", __func__, ltr558->pdata->int_gpio);
		goto err_request_irq;
	}

	goto done;

	/* error, unwind it all */
err_request_irq:
	device_init_wakeup(&client->dev, false);
	destroy_workqueue(ltr558->wq);

err_create_workqueue:
	sysfs_remove_group(&ltr558->light_input_dev->dev.kobj,
		&light_attribute_group);

err_sysfs_create_group_light:
	input_unregister_device(ltr558->light_input_dev);

err_input_register_device_light:
err_input_allocate_device_light:
	sysfs_remove_group(&ltr558->proximity_input_dev->dev.kobj,
		&proximity_attribute_group);

err_sysfs_create_group_proximity:
	input_unregister_device(ltr558->proximity_input_dev);

err_input_register_device_proximity:
	input_free_device(input_dev);

err_input_allocate_device_proximity:
	mutex_destroy(&ltr558->lock);
	wake_lock_destroy(&ltr558->prx_wake_lock);

err_hardware_init:
	kfree(ltr558);

done:
	return ret;
}

static int ltr558_i2c_remove(struct i2c_client *client)
{
	struct ltr558_data *ltr558 = i2c_get_clientdata(client);

	free_irq(ltr558->pdata->int_gpio, (void *)client);
	device_init_wakeup(&client->dev, false);
	sysfs_remove_group(&ltr558->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(ltr558->light_input_dev);
	sysfs_remove_group(&ltr558->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(ltr558->proximity_input_dev);
	if (ltr558->power_state) { /* May have unreleased resource if this variable is corrupted */
		if (ltr558->power_state & PROXIMITY_ENABLED) {
			cancel_work_sync(&ltr558->work_proximity);
			ltr558_proximity_disable(ltr558);
		}
		if (ltr558->power_state & LIGHT_ENABLED) {
			hrtimer_cancel(&ltr558->light_timer);
			cancel_work_sync(&ltr558->work_light);
			ltr558_light_disable(ltr558);
		}
		ltr558->power_state = 0;
	}
	destroy_workqueue(ltr558->wq);
	mutex_destroy(&ltr558->lock);
	wake_lock_destroy(&ltr558->prx_wake_lock);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ltr558->early_suspend);
#endif /* !CONFIG_HAS_EARLYSUSPEND */
	kfree(ltr558);
	return 0;
}

static void ltr558_early_suspend(struct early_suspend *h)
{
	struct ltr558_data *ltr558 =
	    container_of(h, struct ltr558_data, early_suspend);

	LTR558_DBGMSG("ltr558_early_suspend.\n");

	if (ltr558->power_state & LIGHT_ENABLED)
		ltr558_light_disable(ltr558);

	if (ltr558->power_state & PROXIMITY_ENABLED) {
		enable_irq_wake(ltr558->pdata->int_gpio);
	}
}

static void ltr558_late_resume(struct early_suspend *h)
{
	/* Turn power back on if we were before suspend. */
	struct ltr558_data *ltr558 =
	    container_of(h, struct ltr558_data, early_suspend);

	LTR558_DBGMSG("ltr558_late_resume.\n");

	if (ltr558->power_state & PROXIMITY_ENABLED) {
		disable_irq_wake(ltr558->pdata->int_gpio);
	}
	if (ltr558->power_state & LIGHT_ENABLED){
		ltr558_light_enable(ltr558);
	}
}

static const struct i2c_device_id ltr558_device_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ltr558_device_id);

static struct i2c_driver ltr558_i2c_driver = {
	.driver = {
		   .name = SENSOR_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = ltr558_i2c_probe,
	.remove = ltr558_i2c_remove,
	.id_table = ltr558_device_id,
};

static int __init ltr558_init(void)
{
	return i2c_add_driver(&ltr558_i2c_driver);
}

static void __exit ltr558_exit(void)
{
	i2c_del_driver(&ltr558_i2c_driver);
}

module_init(ltr558_init);
module_exit(ltr558_exit);

MODULE_DESCRIPTION("ALP Sensor driver for ltr558");
MODULE_LICENSE("GPLv2");
