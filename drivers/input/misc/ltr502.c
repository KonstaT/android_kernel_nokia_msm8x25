/*  linux/driver/input/misc/ltr502.c
 *  Copyright (C) 2010 Samsung Electronics. All rights reserved.
 *  Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 and
 *  only version 2 as published by the Free Software Foundation.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
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
#include <linux/leds.h>
#include <linux/gpio.h>
#include <linux/wakelock.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/input/ltr502.h>
#include <linux/earlysuspend.h>
#include <linux/regulator/consumer.h>


/* Note about power vs enable/disable:
 *  The chip has two functions, proximity and ambient light sensing.
 *  There is no separate power enablement to the two functions (unlike
 *  the Capella CM3602/3623).
 *  This module implements two drivers: /dev/proximity and /dev/light.
 *  When either driver is enabled (via sysfs attributes), we give power
 *  to the chip.  When both are disabled, we remove power from the chip.
 *  In suspend, we remove power if light is disabled but not if proximity is
 *  enabled (proximity is allowed to wakeup from suspend).
 *
 *  There are no ioctls for either driver interfaces.  Output is via
 *  input device framework and control via sysfs attributes.
 */
#undef DEBUG_LTR502
#ifdef DEBUG_LTR502
#define ltr502_dbgmsg(str, args...) printk("%s: " str, __func__, ##args)
#else
#define ltr502_dbgmsg(str, args...)
#endif

#define SENSOR_NAME		"ltr502"

/* register addr */
#define REGS_CONFIG		0x0	/* Read  Only */
#define REGS_TIMING		0x1	/* Write Only */
#define REGS_DLS_CTL	0x2	/* Write Only */
#define REGS_INT_STATUS	0x3	/* Write Only */
#define REGS_DPS_CTL	0x4	/* Write Only */
#define REGS_DATA		0x5	/* Write Only */
#define REGS_WINDOW		0x8	/* Write Only */

/* sensor type */
#define LIGHT           0
#define PROXIMITY		1
#define ALL				2

/* sensor operation mode */
#define OPERATION_MASK	0x03	/*operation mask */
#define DLS_ACTIVE 		0x00	/*DLS active operation mode */
#define DPS_ACTIVE		0x01	/*DPS active operation mode */
#define ALL_ACTIVE		0x02	/*DLS and DPS active operation mode */
#define ALL_IDLE		0x03	/*Idle operation mode */

#define POWER_MASK		(0x03<<2)	/*mode mask */
#define POWER_UP		(0x00<<2)	/*Idle mode */
#define POWER_DOWN		(0x02<<2)	/*Idle mode */

/* sensor operation mode */
#define DPS_INT 		0x02	/*DPS INT */
#define DLS_INT			0x01	/*DLS INT */

static struct regulator *reg_ext_2v8;

static int lux_table[64] = {
	1, 1, 1, 2, 2, 2, 3, 4, 4, 5,
	6, 7, 9, 11, 13, 16, 19, 22, 27, 32,
	39, 46, 56, 67, 80, 96, 116, 139, 167, 200,
	240, 289, 346, 416, 499, 599, 720, 864, 1037, 1245,
	1495, 1795, 2154, 2586, 3105, 3728, 4475, 5372, 6449, 7743,
	9295, 11159, 13396, 16082, 19307, 23178, 27826, 33405, 40103, 48144,
	57797, 69386, 83298, 100000,
};

struct ltr502_data;

enum {
	LIGHT_ENABLED = BIT(0),
	PROXIMITY_ENABLED = BIT(1),
};

/* driver data */
struct ltr502_data {
	bool on;
	u8 power_state;
	struct ltr502_platform_data *pdata;
	struct i2c_client *i2c_client;
	struct mutex lock;
	struct workqueue_struct *wq;
	struct early_suspend early_suspend;

	struct input_dev *light_input_dev;
	struct work_struct work_light;
	struct hrtimer light_timer;
	ktime_t light_poll_delay;

	struct input_dev *proximity_input_dev;
	struct work_struct work_proximity;
	struct hrtimer proximity_timer;
	ktime_t proximity_poll_delay;
	struct wake_lock prx_wake_lock;
};

static void ltr502_early_suspend(struct early_suspend *h);
static void ltr502_late_resume(struct early_suspend *h);

int ltr502_i2c_write(struct ltr502_data *ltr502, u8 reg, u8 * val)
{
	int err = 0;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int retry = 10;
	struct i2c_client *client = ltr502->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		data[0] = reg;
		data[1] = *val;

		msg->addr = client->addr;
		msg->flags = 0;	/* write */
		msg->len = 2;
		msg->buf = data;

		err = i2c_transfer(client->adapter, msg, 1);

		if (err >= 0)
			return 0;

		usleep(10000);
	}
	return err;
}

int ltr502_i2c_read(struct ltr502_data *ltr502, u8 reg, u8 * data)
{
	int err = 0;
	struct i2c_msg msg[2];
	int retry = 10;
	struct i2c_client *client = ltr502->i2c_client;

	if ((client == NULL) || (!client->adapter))
		return -ENODEV;

	while (retry--) {
		msg[0].addr = client->addr;
		msg[0].flags = 0;	/* write */
		msg[0].len = 1;
		msg[0].buf = &reg;

		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;	/* read */
		msg[1].len = 1;
		msg[1].buf = data;

		err = i2c_transfer(client->adapter, msg, 2);

		if (err >= 0)
			return 0;

		usleep(10000);
	}
	return err;
}

static int ltr502_light_enable(struct ltr502_data *ltr502)
{
	u8 mode;
	int ret = 0;

	ltr502_dbgmsg("starting poll timer, delay %lldns\n",
		      ktime_to_ns(ltr502->light_poll_delay));

	if (!ltr502_i2c_read(ltr502, REGS_CONFIG, &mode)) {
		if( mode & POWER_UP ){
			ltr502_dbgmsg("LTR502 chip has been power up ");
		} else {
			mode &= ~(POWER_MASK);
			mode |= (POWER_UP);
			if (ltr502_i2c_write(ltr502, REGS_CONFIG, &mode)) {
				pr_err("fail to write REGS_CONFIG\n");
			}
		}
		if ((mode & OPERATION_MASK) == DPS_ACTIVE) {
			mode &= ~(OPERATION_MASK);
			mode |= (ALL_ACTIVE);
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");
				return ret;
			}
		} else if ((mode & OPERATION_MASK) == ALL_IDLE) {
			mode &= ~(OPERATION_MASK);
			mode |= (DLS_ACTIVE);
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");
				return ret;
			}
		} else {
			pr_err("light sensor has been enable\n");
			return -1;
		}
	} else {
		pr_err("fail to read REGS_CONFIG\n");
		return -1;
	}

	return ret;
}

static int ltr502_light_disable(struct ltr502_data *ltr502)
{
	u8 mode;
	int ret = 0;
	/* cancel p-sensor work queue to prevent error read */
	if (ltr502->power_state & PROXIMITY_ENABLED) {
		hrtimer_cancel(&ltr502->proximity_timer);
		cancel_work_sync(&ltr502->work_proximity);
	}
	if (!ltr502_i2c_read(ltr502, REGS_CONFIG, &mode)) {
		if ((mode & OPERATION_MASK) == DLS_ACTIVE) {
			mode &= ~(OPERATION_MASK);
			mode |= (ALL_IDLE);
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");
				return ret;
			}
		} else if ((mode & OPERATION_MASK) == ALL_ACTIVE) {
			mode &= ~(OPERATION_MASK);
			mode |= (DPS_ACTIVE);
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");
				return ret;
			}
		} else {
			pr_err("light sensor has been disable\n");
			return -1;
		}
	} else {
		pr_err("fail to read REGS_CONFIG\n");
		return -1;
	}
	/* reload p-sensor workqueue and delay 200ms before reading the register */
	if (ltr502->power_state & PROXIMITY_ENABLED) {
		usleep(200000);
		hrtimer_start(&ltr502->proximity_timer,
			      ltr502->proximity_poll_delay, HRTIMER_MODE_REL);
	}
	return ret;
}

static ssize_t light_poll_delay_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n", ktime_to_ns(ltr502->light_poll_delay));
}

static ssize_t light_poll_delay_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	ltr502_dbgmsg("new delay = %lldns, old delay = %lldns\n",
		      new_delay, ktime_to_ns(ltr502->light_poll_delay));
	mutex_lock(&ltr502->lock);
	if (new_delay != ktime_to_ns(ltr502->light_poll_delay)) {
		ltr502->light_poll_delay = ns_to_ktime(new_delay);
		if (ltr502->power_state & LIGHT_ENABLED) {
			hrtimer_cancel(&ltr502->light_timer);
			cancel_work_sync(&ltr502->work_light);
			hrtimer_start(&ltr502->light_timer,
				      ltr502->light_poll_delay,
				      HRTIMER_MODE_REL);
		}
	}
	mutex_unlock(&ltr502->lock);

	return size;
}

static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (ltr502->power_state & LIGHT_ENABLED) ? 1 : 0);
}

static ssize_t light_enable_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t size)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
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

	mutex_lock(&ltr502->lock);
	ltr502_dbgmsg("new_value = %d, old state = %d\n",
		      new_value, (ltr502->power_state & LIGHT_ENABLED) ? 1 : 0);
	if (new_value && !(ltr502->power_state & LIGHT_ENABLED)) {
		if (!ltr502_light_enable(ltr502)) {
			ret = size;
			ltr502->power_state |= LIGHT_ENABLED;
			hrtimer_start(&ltr502->light_timer,
				      ltr502->light_poll_delay,
				      HRTIMER_MODE_REL);
		}
	} else if (!new_value && (ltr502->power_state & LIGHT_ENABLED)) {
		if (!ltr502_light_disable(ltr502)) {
			ret = size;
			ltr502->power_state &= ~LIGHT_ENABLED;
			hrtimer_cancel(&ltr502->light_timer);
			cancel_work_sync(&ltr502->work_light);
		}
	}
	mutex_unlock(&ltr502->lock);

	return ret;
}

static int ltr502_proximity_enable(struct ltr502_data *ltr502)
{
	u8 mode;
	int ret = 0;

	if (!ltr502_i2c_read(ltr502, REGS_CONFIG, &mode)) {
		if( mode & POWER_UP ){
			ltr502_dbgmsg("LTR502 chip has been power up ");
		} else {
			mode &= ~(POWER_MASK);
			mode |= (POWER_UP);
			if (ltr502_i2c_write(ltr502, REGS_CONFIG, &mode)) {
				pr_err("fail to write REGS_CONFIG\n");
			}
		}
		if ((mode & OPERATION_MASK) == DLS_ACTIVE) {
			mode &= ~(OPERATION_MASK);
			mode |= (ALL_ACTIVE);
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			regulator_enable(reg_ext_2v8);    //vote to turn on LDO 2.8V
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");

			}
		} else if ((mode & OPERATION_MASK) == ALL_IDLE) {
			mode &= ~(OPERATION_MASK);
			mode |= (DPS_ACTIVE);
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			regulator_enable(reg_ext_2v8);    //vote to turn on LDO 2.8V
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");
				return ret;
			}
		} else {
			pr_err("proximity has been enable!\n");
			return -1;
		}
	} else {
		pr_err("fail to write REGS_CONFIG\n");
		return -1;
	}

	return ret;
}

static int ltr502_proximity_disable(struct ltr502_data *ltr502)
{
	u8 mode;
	int ret = 0;
	if (!ltr502_i2c_read(ltr502, REGS_CONFIG, &mode)) {
		if ((mode & OPERATION_MASK) == DPS_ACTIVE) {
			mode &= ~(OPERATION_MASK);
			mode |= (ALL_IDLE);
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			regulator_disable(reg_ext_2v8);   //vote to turn off LDO 2.8V
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");
				return ret;
			}
		} else if ((mode & OPERATION_MASK) == ALL_ACTIVE) {
			mode &= ~(OPERATION_MASK);
			mode |= DLS_ACTIVE;
			ret = ltr502_i2c_write(ltr502, REGS_CONFIG, &mode);
			regulator_disable(reg_ext_2v8);    //vote to turn off LDO 2.8V
			if (ret) {
				pr_err("fail to write REGS_CONFIG\n");
				return ret;
			}
		} else {
			pr_err("proximity has been disable!\n");
			return -1;
		}
	} else {
		pr_err("fail to read REGS_CONFIG\n");
		return -1;
	}

	return ret;
}

static ssize_t proximity_poll_delay_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
	return sprintf(buf, "%lld\n",
		       ktime_to_ns(ltr502->proximity_poll_delay));
}

static ssize_t proximity_poll_delay_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
	int64_t new_delay;
	int err;

	err = strict_strtoll(buf, 10, &new_delay);
	if (err < 0)
		return err;

	ltr502_dbgmsg("new delay = %lldns, old delay = %lldns\n",
		      new_delay, ktime_to_ns(ltr502->proximity_poll_delay));
	mutex_lock(&ltr502->lock);
	if (new_delay != ktime_to_ns(ltr502->proximity_poll_delay)) {
		ltr502->proximity_poll_delay = ns_to_ktime(new_delay);
		if (ltr502->power_state & PROXIMITY_ENABLED) {
			hrtimer_cancel(&ltr502->proximity_timer);
			cancel_work_sync(&ltr502->work_proximity);
			hrtimer_start(&ltr502->proximity_timer,
				      ltr502->proximity_poll_delay,
				      HRTIMER_MODE_REL);
		}
	}
	mutex_unlock(&ltr502->lock);

	return size;
}

static ssize_t proximity_enable_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n",
		       (ltr502->power_state & PROXIMITY_ENABLED) ? 1 : 0);
}

static ssize_t proximity_enable_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t size)
{
	struct ltr502_data *ltr502 = dev_get_drvdata(dev);
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

	mutex_lock(&ltr502->lock);
	ltr502_dbgmsg("new_value = %d, old state = %d\n",
		      new_value,
		      (ltr502->power_state & PROXIMITY_ENABLED) ? 1 : 0);
	if (new_value && !(ltr502->power_state & PROXIMITY_ENABLED)) {
		if (!ltr502_proximity_enable(ltr502)) {
			ret = size;
			ltr502->power_state |= PROXIMITY_ENABLED;
			wake_lock(&ltr502->prx_wake_lock);
			hrtimer_start(&ltr502->proximity_timer,
				      ltr502->proximity_poll_delay,
				      HRTIMER_MODE_REL);
		}
	} else if (!new_value && (ltr502->power_state & PROXIMITY_ENABLED)) {
		if (!ltr502_proximity_disable(ltr502)) {
			ret = size;
			ltr502->power_state &= ~PROXIMITY_ENABLED;
			wake_unlock(&ltr502->prx_wake_lock);
			hrtimer_cancel(&ltr502->proximity_timer);
			cancel_work_sync(&ltr502->work_proximity);
		}
	}
	mutex_unlock(&ltr502->lock);

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

static DEVICE_ATTR(proximity_poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
		   proximity_poll_delay_show, proximity_poll_delay_store);

static struct device_attribute dev_attr_proximity_enable =
__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
       proximity_enable_show, proximity_enable_store);

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	&dev_attr_proximity_poll_delay.attr,

	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};

static void ltr502_work_func_proximity(struct work_struct *work)
{
	struct ltr502_data *ltr502 = container_of(work, struct ltr502_data,
						  work_proximity);
	u8 data;
	int rc;

	rc = ltr502_i2c_read(ltr502, REGS_DATA, &data);
	if (rc) {
		pr_err("Fail to read the data regs!\n");
		return;
	}
	/* 0 is close, 1 is far */
	ltr502_dbgmsg("ltr502: DPS data = %x\n", data);
	data = (data & 0x80) ? 0 : 1;
	input_report_abs(ltr502->proximity_input_dev, ABS_DISTANCE, data);
	input_sync(ltr502->proximity_input_dev);
}

static void ltr502_work_func_light(struct work_struct *work)
{
	struct ltr502_data *ltr502 = container_of(work, struct ltr502_data,
						  work_light);
	u8 data;
	u8 pdata;
	int rc;

	rc = ltr502_i2c_read(ltr502, REGS_DATA, &data);
	if (rc) {
		pr_err("Fail to read the data regs!\n");
		return;
	}
	pdata = (data & 0x80) ? 0 : 1;
	data &= 0x3f;
	ltr502_dbgmsg("ltr502: DLS data = %d,lux = %d\n", data,
		      lux_table[data]);
	if (lux_table[data] > 2)	//prevent noise data "1 lux"
	{
		input_report_abs(ltr502->light_input_dev, ABS_MISC,
				 lux_table[data] * 2);
		input_sync(ltr502->light_input_dev);
	}

}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart ltr502_light_timer_func(struct hrtimer *timer)
{
	struct ltr502_data *ltr502 =
	    container_of(timer, struct ltr502_data, light_timer);
	queue_work(ltr502->wq, &ltr502->work_light);
	hrtimer_forward_now(&ltr502->light_timer, ltr502->light_poll_delay);
	return HRTIMER_RESTART;
}

/* This function is for light sensor.  It operates every a few seconds.
 * It asks for work to be done on a thread because i2c needs a thread
 * context (slow and blocking) and then reschedules the timer to run again.
 */
static enum hrtimer_restart ltr502_pxy_timer_func(struct hrtimer *timer)
{
	struct ltr502_data *ltr502 = container_of(timer, struct ltr502_data,
						  proximity_timer);
	queue_work(ltr502->wq, &ltr502->work_proximity);
	hrtimer_forward_now(&ltr502->proximity_timer,
			    ltr502->proximity_poll_delay);
	return HRTIMER_RESTART;
}

static int ltr502_hardware_init(struct ltr502_data *ltr502)
{
	int rc = -EIO;
	u8 int_status, data;
	/* wait 200ms for hardware init, no need anymore */
	//usleep(200000);
	ltr502_dbgmsg("start\n");
	//write default value to resume the sensor
	data = 0x11;
	rc = ltr502_i2c_write(ltr502, REGS_TIMING, &data);
	if (rc) {
		pr_err
		    ("fail to write REGS_TIMING,try to write REGS_DPS_CTL..\n");
		data = 0x44;
		rc = ltr502_i2c_write(ltr502, REGS_DPS_CTL, &data);
		if (rc) {
			pr_err("fail to write REGS_DPS_CTL\n");
			return rc;
		}
	}
	//enter the idle
	data = 0x03;
	rc = ltr502_i2c_write(ltr502, REGS_CONFIG, &data);
	if (rc) {
		pr_err("fail to write REGS_CONFIG\n");
		return rc;
	}
	//DPS_CTL level 80mm
	data = 0x44;
	rc = ltr502_i2c_write(ltr502, REGS_DPS_CTL, &data);
	if (rc) {
		pr_err("fail to write REGS_DPS_CTL\n");
		return rc;
	}

	/*FIXME:DLS 0x0F Window 93.5% loss for 7x27A version 1.5 due to the high window loss */
	/*FIXME:DLS 0x08 Window 72.2% loss for 7x27A version PVT */
	data = 0x08;
	rc = ltr502_i2c_write(ltr502, REGS_WINDOW, &data);
	if (rc) {
		pr_err("fail to write REGS_WINDOW\n");
		return rc;
	}
	//read the status
	rc = ltr502_i2c_read(ltr502, REGS_INT_STATUS, &int_status);
	if (rc) {
		pr_err("fail to write REGS_INT_STATUS\n");
		return rc;
	}
	//read data 
	rc = ltr502_i2c_read(ltr502, REGS_DATA, &data);
	if (rc) {
		pr_err("fail to write REGS_INT_STATUS\n");
		return rc;
	}

	ltr502_dbgmsg("success\n");
	return rc;
}

static int ltr502_i2c_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	int ret = -ENODEV;
	struct input_dev *input_dev;
	struct ltr502_data *ltr502;
	struct ltr502_platform_data *pdata = client->dev.platform_data;

	if (!pdata) {
		pr_err("%s: missing pdata!\n", __func__);
		return ret;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c functionality check failed!\n", __func__);
		return ret;
	}

	ltr502 = kzalloc(sizeof(struct ltr502_data), GFP_KERNEL);
	if (!ltr502) {
		pr_err("%s: failed to alloc memory for module data\n",
		       __func__);
		return -ENOMEM;
	}

	/* ext_2v8 control */
	if (!reg_ext_2v8) {
		reg_ext_2v8 = regulator_get(NULL, "ext_2p85v");
		if (IS_ERR(reg_ext_2v8)) {
			pr_err("'%s' regulator not found, rc=%ld\n",
				"ext_2v8", IS_ERR(reg_ext_2v8));
				reg_ext_2v8 = NULL;
				goto err_hardware_init;
		}
	}

	ltr502->power_state = 0;
	ltr502->pdata = pdata;
	ltr502->i2c_client = client;
	i2c_set_clientdata(client, ltr502);

	if (ltr502_hardware_init(ltr502)) {
		pr_err("LTR502 hardware init failed!");
		goto err_hardware_init;
	}

	/* wake lock init */
	wake_lock_init(&ltr502->prx_wake_lock, WAKE_LOCK_SUSPEND,
		       "prx_wake_lock");
	mutex_init(&ltr502->lock);

	/* allocate proximity input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		goto err_input_allocate_device_proximity;
	}
	ltr502->proximity_input_dev = input_dev;
	input_set_drvdata(input_dev, ltr502);
	input_dev->name = "proximity";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	ltr502_dbgmsg("registering proximity input device\n");
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
	/* this is the thread function we run on the work queue */
	INIT_WORK(&ltr502->work_proximity, ltr502_work_func_proximity);

	/* proximity hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&ltr502->proximity_timer, CLOCK_MONOTONIC,
		     HRTIMER_MODE_REL);
	ltr502->proximity_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	ltr502->proximity_timer.function = ltr502_pxy_timer_func;

	/* light hrtimer settings.  we poll for light values using a timer. */
	hrtimer_init(&ltr502->light_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ltr502->light_poll_delay = ns_to_ktime(200 * NSEC_PER_MSEC);
	ltr502->light_timer.function = ltr502_light_timer_func;

	/* the timer just fires off a work queue request.  we need a thread
	   to read the i2c (can be slow and blocking). */
	ltr502->wq = create_singlethread_workqueue("ltr502_wq");
	if (!ltr502->wq) {
		ret = -ENOMEM;
		pr_err("%s: could not create workqueue\n", __func__);
		goto err_create_workqueue;
	}
	/* this is the thread function we run on the work queue */
	INIT_WORK(&ltr502->work_light, ltr502_work_func_light);

	/* allocate lightsensor-level input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		ret = -ENOMEM;
		goto err_input_allocate_device_light;
	}
	input_set_drvdata(input_dev, ltr502);
	input_dev->name = "light";
	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_MISC, 0, 1, 0, 0);

	ltr502_dbgmsg("registering light sensor input device\n");
	ret = input_register_device(input_dev);
	if (ret < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(input_dev);
		goto err_input_register_device_light;
	}
	ltr502->light_input_dev = input_dev;
	ret = sysfs_create_group(&input_dev->dev.kobj, &light_attribute_group);
	if (ret) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto err_sysfs_create_group_light;
	}

	ltr502->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	ltr502->early_suspend.suspend = ltr502_early_suspend;
	ltr502->early_suspend.resume = ltr502_late_resume;
	register_early_suspend(&ltr502->early_suspend);

	goto done;

	/* error, unwind it all */
err_sysfs_create_group_light:
	input_unregister_device(ltr502->light_input_dev);
err_input_register_device_light:
err_input_allocate_device_light:
	destroy_workqueue(ltr502->wq);
err_create_workqueue:
	sysfs_remove_group(&ltr502->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
err_sysfs_create_group_proximity:
	input_unregister_device(ltr502->proximity_input_dev);
err_input_register_device_proximity:
	input_free_device(input_dev);
err_input_allocate_device_proximity:
	mutex_destroy(&ltr502->lock);
	wake_lock_destroy(&ltr502->prx_wake_lock);
err_hardware_init:
	kfree(ltr502);
	regulator_put(reg_ext_2v8);
done:
	return ret;
}

static void ltr502_early_suspend(struct early_suspend *h)
{
	/* We disable power only if proximity is disabled.  If proximity
	   is enabled, we leave power on because proximity is allowed
	   to wake up device.  We remove power without changing
	   ltr502->power_state because we use that state in resume.
	 */
	u8 mode;
	struct ltr502_data *ltr502 =
	    container_of(h, struct ltr502_data, early_suspend);

	ltr502_dbgmsg("early suspend\n");

	if (ltr502->power_state & LIGHT_ENABLED)
		ltr502_light_disable(ltr502);

	if (!(ltr502->power_state & PROXIMITY_ENABLED)) {
		ltr502_dbgmsg("enter power down\n");
		if (!ltr502_i2c_read(ltr502, REGS_CONFIG, &mode)) {
			mode &= ~(POWER_MASK);
			mode |= (POWER_DOWN);
			if (ltr502_i2c_write(ltr502, REGS_CONFIG, &mode)) {
				pr_err("fail to write REGS_DPS_CTL\n");
			}
		} else {
			pr_err("fail to write REGS_CONFIG\n");
		}
	}
}

static void ltr502_late_resume(struct early_suspend *h)
{
	/* Turn power back on if we were before suspend. */
	u8 mode;
	struct ltr502_data *ltr502 =
	    container_of(h, struct ltr502_data, early_suspend);

	ltr502_dbgmsg("late resume\n");

	if (ltr502->power_state & LIGHT_ENABLED)
		ltr502_light_enable(ltr502);

	if (!(ltr502->power_state & PROXIMITY_ENABLED)) {
		ltr502_dbgmsg("exit power down\n");
		if (!ltr502_i2c_read(ltr502, REGS_CONFIG, &mode)) {
			mode &= ~(POWER_MASK);
			mode |= (POWER_UP);
			if (ltr502_i2c_write(ltr502, REGS_CONFIG, &mode)) {
				pr_err("fail to write REGS_CONFIG\n");
			}
		} else {
			pr_err("fail to write REGS_CONFIG\n");
		}
	}
}

static int ltr502_i2c_remove(struct i2c_client *client)
{
	struct ltr502_data *ltr502 = i2c_get_clientdata(client);
	sysfs_remove_group(&ltr502->light_input_dev->dev.kobj,
			   &light_attribute_group);
	input_unregister_device(ltr502->light_input_dev);
	sysfs_remove_group(&ltr502->proximity_input_dev->dev.kobj,
			   &proximity_attribute_group);
	input_unregister_device(ltr502->proximity_input_dev);
	if (ltr502->power_state) {
		if (ltr502->power_state & PROXIMITY_ENABLED) {
			hrtimer_cancel(&ltr502->proximity_timer);
			cancel_work_sync(&ltr502->work_proximity);
			ltr502_proximity_disable(ltr502);
		}
		if (ltr502->power_state & LIGHT_ENABLED) {
			hrtimer_cancel(&ltr502->proximity_timer);
			cancel_work_sync(&ltr502->work_proximity);
			ltr502_light_disable(ltr502);
		}
		ltr502->power_state = 0;
	}
	destroy_workqueue(ltr502->wq);
	mutex_destroy(&ltr502->lock);
	wake_lock_destroy(&ltr502->prx_wake_lock);
	unregister_early_suspend(&ltr502->early_suspend);
	kfree(ltr502);
	return 0;
}

static const struct i2c_device_id ltr502_device_id[] = {
	{SENSOR_NAME, 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, ltr502_device_id);

static struct i2c_driver ltr502_i2c_driver = {
	.driver = {
		   .name = SENSOR_NAME,
		   .owner = THIS_MODULE,
		   },
	.probe = ltr502_i2c_probe,
	.remove = ltr502_i2c_remove,
	.id_table = ltr502_device_id,
};

static int __init ltr502_init(void)
{
	return i2c_add_driver(&ltr502_i2c_driver);
}

static void __exit ltr502_exit(void)
{
	i2c_del_driver(&ltr502_i2c_driver);
}

module_init(ltr502_init);
module_exit(ltr502_exit);

MODULE_AUTHOR("mjchen@sta.samsung.com");
MODULE_DESCRIPTION("Optical Sensor driver for ltr502");
MODULE_LICENSE("GPL");
