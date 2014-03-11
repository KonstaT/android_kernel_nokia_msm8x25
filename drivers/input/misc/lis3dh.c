/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <asm/div64.h>

/* Registers defines */
#define	G_MAX		16000

#define SENSITIVITY_2G		1	/*	mg/LSB */
#define SENSITIVITY_4G		2	/*	mg/LSB */
#define SENSITIVITY_8G		4	/*	mg/LSB */
#define SENSITIVITY_16G		12	/*	mg/LSB */

/* Accelerometer Sensor Operating Mode */
#define LIS3DH_ACC_ENABLE	0x01
#define LIS3DH_ACC_DISABLE	0x00
#define	HIGH_RESOLUTION		0x08	/*	for control reg 4 */
#define	AXISDATA_REG		0x28	/*	Begin from OUT_X_L */
#define WHOAMI_LIS3DH_ACC	0x33	/*	Expected content for WAI */

/* CONTROL REGISTERS */
#define WHO_AM_I		0x0F	/*	WhoAmI register		*/
#define	TEMP_CFG_REG		0x1F	/*	temper sens control reg	*/

/* ctrl 1: ODR3 ODR2 ODR ODR0 LPen Zenable Yenable Zenable */
#define	CTRL_REG1		0x20	/*	control reg 1		*/
#define	CTRL_REG2		0x21	/*	control reg 2		*/
#define	CTRL_REG3		0x22	/*	control reg 3		*/
#define	CTRL_REG4		0x23	/*	control reg 4		*/
#define	CTRL_REG5		0x24	/*	control reg 5		*/
#define	CTRL_REG6		0x25	/*	control reg 6		*/
#define STATUS_REG		0x27
#define OUT_X_L			0x28
#define OUT_X			0x29
#define OUT_Y_L			0x2A
#define OUT_Y			0x2B
#define OUT_Z_L			0x2C
#define OUT_Z			0x2D
#define	FIFO_CTRL_REG		0x2E	/*	FiFo control reg	*/
#define	FIFO_SRC_REG		0x2F	/*	FiFo src reg	*/
#define	INT_CFG1		0x30	/*	interrupt 1 config	*/
#define	INT_SRC1		0x31	/*	interrupt 1 source	*/
#define	INT_THS1		0x32	/*	interrupt 1 threshold	*/
#define	INT_DUR1		0x33	/*	interrupt 1 duration	*/
#define	TT_CFG			0x38	/*	tap config		*/
#define	TT_SRC			0x39	/*	tap source		*/
#define	TT_THS			0x3A	/*	tap threshold		*/
#define	TT_LIM			0x3B	/*	tap time limit		*/
#define	TT_TLAT			0x3C	/*	tap time latency	*/
#define	TT_TW			0x3D	/*	tap time window		*/
/* end CONTROL REGISTRES	*/

#define ENABLE_HIGH_RESOLUTION	1
#define LIS3DH_ACC_PM_OFF		0x00
#define LIS3DH_ACC_ENABLE_ALL_AXES	0x07

#define FIFO_EMPTY			0x20
#define FSS_MASK			0x1F
#define PMODE_MASK			0x08
#define ODR_MASK			0XF0

#define ODR1		0x10  /* 1Hz output data rate */
#define ODR10		0x20  /* 10Hz output data rate */
#define ODR25		0x30  /* 25Hz output data rate */
#define ODR50		0x40  /* 50Hz output data rate */
#define ODR100		0x50  /* 100Hz output data rate */
#define ODR200		0x60  /* 200Hz output data rate */
#define ODR400		0x70  /* 400Hz output data rate */
#define ODR1250		0x90  /* 1250Hz output data rate */

#define	IA			0x40
#define	ZH			0x20
#define	ZL			0x10
#define	YH			0x08
#define	YL			0x04
#define	XH			0x02
#define	XL			0x01

/* CTRL REG BITS*/
#define	CTRL_REG3_I1_AOI1	0x40
#define	CTRL_REG6_I2_TAPEN	0x80
#define	CTRL_REG6_HLACTIVE	0x02
/* MASK */
#define NO_MASK			0xFF
#define INT1_DURATION_MASK	0x7F
#define	INT1_THRESHOLD_MASK	0x7F
#define TAP_CFG_MASK		0x3F
#define	TAP_THS_MASK		0x7F
#define	TAP_TLIM_MASK		0x7F
#define	TAP_TLAT_MASK		NO_MASK
#define	TAP_TW_MASK		NO_MASK

/* TAP_SOURCE_REG BIT */
#define	DTAP			0x20
#define	STAP			0x10
#define	SIGNTAP			0x08
#define	ZTAP			0x04
#define	YTAP			0x02
#define	XTAZ			0x01
#define	FUZZ			0
#define	FLAT			0
#define	I2C_RETRY_DELAY		5
#define	I2C_RETRIES		5
#define	I2C_AUTO_INCREMENT	0x80

/* RESUME STATE INDICES */
#define	RES_CTRL_REG1		0
#define	RES_CTRL_REG2		1
#define	RES_CTRL_REG3		2
#define	RES_CTRL_REG4		3
#define	RES_CTRL_REG5		4
#define	RES_CTRL_REG6		5
#define	RES_INT_CFG1		6
#define	RES_INT_THS1		7
#define	RES_INT_DUR1		8
#define	RES_TT_CFG		9
#define	RES_TT_THS		10
#define	RES_TT_LIM		11
#define	RES_TT_TLAT		12
#define	RES_TT_TW		13
#define	RES_TEMP_CFG_REG	14
#define	RES_REFERENCE_REG	15
#define	RES_FIFO_CTRL_REG	16
#define	RESUME_ENTRIES		17
/* end RESUME STATE INDICES */

#define BYPASS_MODE	0x00
#define FIFO_MODE	0x40
#define AC		(1 << 7) /* register auto-increment bit */
#define MAX_ENTRY	1
#define MAX_DELAY	(MAX_ENTRY * 9523809LL)
#define READ_REPEAT_SHIFT	3
#define READ_REPEAT			(1 << READ_REPEAT_SHIFT)

//#define ACC_DEBUG

#ifdef ACC_DEBUG
#define acc_debug(fmt,arg...) \
        printk(fmt,##arg)
#else
#define acc_debug(fmt,arg...) 
#endif

/* default register setting for device init */
static const char default_ctrl_regs[] = {
	0x77,	/* 400HZ, PM-normal, xyz enable */
	0x00,	/* normal mode */
	0x00,	/* No interrupt eanble */
	0x88,	/* Block data updata, LSB, Full scale 2G, High resolution output */
	0x00,	/* fifo disable */
};

static const struct odr_delay {
	u8 odr; /* odr reg setting */
	s64 delay_ns; /* odr in ns */
} odr_delay_table[] = {
	{  ODR400,    2500000LL << READ_REPEAT_SHIFT }, /* 400Hz */
	{  ODR100,   10000000LL << READ_REPEAT_SHIFT }, /* 100Hz */
	{   ODR50,   20000000LL << READ_REPEAT_SHIFT }, /*  50Hz */
	{   ODR10,  100000000LL << READ_REPEAT_SHIFT }, /*  10Hz */
	{    ODR1, 1000000000LL << READ_REPEAT_SHIFT }, /*   1Hz */
};

/* LIS3DH acceleration data */
struct lis3dh_t {
	s16 x;
	s16 y;
	s16 z;
};

struct lis3dh_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct mutex lock;
	struct workqueue_struct *lis3dh_wq;
	struct work_struct work;
	struct hrtimer timer;
	bool enable;
	bool drop_next_event;
	bool interruptible;	/* interrupt or polling? */
	int entries;		/* number of fifo entries */
	u8 ctrl_regs[5];	/* saving register settings */
	u32 time_to_read;	/* time needed to read one entry */
	ktime_t polling_delay;	/* polling time for timer */
	struct completion data_ready;
	bool timer_enable;      /* prevent workqueue and timer risk*/
	bool work_enable;
};

static void set_polling_delay(struct lis3dh_data *lis3dh_data, int res)
{
	s64 delay_ns;
	delay_ns = lis3dh_data->entries + 1 - res;
	if (delay_ns < 0)
		delay_ns = 0;
	delay_ns = delay_ns * lis3dh_data->time_to_read;
	lis3dh_data->polling_delay = ns_to_ktime(delay_ns);
}

/* data readout */
static int lis3dh_read_acc_values(struct i2c_client *client,
				struct lis3dh_t *data, int total_read)
{
	int err;
	s8 reg = OUT_X_L | AC; /* read from OUT_X_L to OUT_Z by auto-inc */
	s8 acc_data[6];
	acc_debug("enter funtion %s\n",__func__);
	err = i2c_smbus_read_i2c_block_data(client, reg,sizeof(acc_data), acc_data);
	if (err != sizeof(acc_data)) {
		pr_err("%s : failed to read 5 bytes for getting x/y/z\n", __func__);
		return -EIO;
	}

	data->x = ((acc_data[1] << 8) + acc_data[0]) >> 4;
	data->y = ((acc_data[3] << 8) + acc_data[1]) >> 4;
	data->z = ((acc_data[5] << 8) + acc_data[4]) >> 4;
	
	/*for test*/
	acc_debug("ACC sensor report rel x0 %d\n",acc_data[0]);
	acc_debug("ACC sensor report rel x1 %d\n",acc_data[1]);
	acc_debug("ACC sensor report rel y0 %d\n",acc_data[2]);	
	acc_debug("ACC sensor report rel y1 %d\n",acc_data[3]);
	acc_debug("ACC sensor report rel z0 %d\n",acc_data[4]);
	acc_debug("ACC sensor report rel z1 %d\n",acc_data[5]);
	acc_debug("ACC sensor data x = %d8 , y = %8d , z = %8d \n", data->x, data->y, data->z);
	
	return 0;
}

static int lis3dh_report_acc_values(struct lis3dh_data *lis3dh_data)
{
	int res;
	struct lis3dh_t data;

	res = lis3dh_read_acc_values(lis3dh_data->client, &data,6);
	if (res < 0)
		return res;
		
	input_report_rel(lis3dh_data->input_dev, REL_RX, -data.y);
	input_report_rel(lis3dh_data->input_dev, REL_RY, -data.x);
	input_report_rel(lis3dh_data->input_dev, REL_RZ, -data.z);
	input_sync(lis3dh_data->input_dev);
	return res;
}

static enum hrtimer_restart lis3dh_timer_func(struct hrtimer *timer)
{
	struct lis3dh_data *lis3dh_data = container_of(timer, struct lis3dh_data, timer);
	if(likely(lis3dh_data->timer_enable))
		queue_work(lis3dh_data->lis3dh_wq, &lis3dh_data->work);
	return HRTIMER_NORESTART;
}

static void lis3dh_work_func(struct work_struct *work)
{
	int res;
	struct lis3dh_data *lis3dh_data = container_of(work, struct lis3dh_data, work);

	acc_debug("enter funtion %s\n",__func__);
	/* read the status register */
	res =  i2c_smbus_read_byte_data(lis3dh_data->client, STATUS_REG);
	acc_debug("ACC sensor src status(0x27) is  %d\n",res);
	if(res & 0x80)	{
		res = lis3dh_report_acc_values(lis3dh_data);
		if (res < 0)
			return;
	}
	if(likely(lis3dh_data->work_enable))
		hrtimer_start(&lis3dh_data->timer, lis3dh_data->polling_delay, HRTIMER_MODE_REL);
}

static irqreturn_t lis3dh_interrupt_thread(int irq, void *lis3dh_data_p)
{
	int res;
	struct lis3dh_data *lis3dh_data = lis3dh_data_p;
	acc_debug("enter funtion %s\n",__func__);
	res = lis3dh_report_acc_values(lis3dh_data);
	if (res < 0)
		pr_err("%s: failed to report acc values\n", __func__);
	return IRQ_HANDLED;
}

static ssize_t lis3dh_show_enable(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct lis3dh_data *lis3dh_data  = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", lis3dh_data->enable);
}

static ssize_t lis3dh_set_enable(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	int err = 0;
	struct lis3dh_data *lis3dh_data  = dev_get_drvdata(dev);
	bool new_enable;
	acc_debug("enter funtion %s\n",__func__);

	if (sysfs_streq(buf, "1"))
		new_enable = true;
	else if (sysfs_streq(buf, "0"))
		new_enable = false;
	else {
		pr_debug("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}

	if (new_enable == lis3dh_data->enable)
		return size;

	mutex_lock(&lis3dh_data->lock);
	if (new_enable) {
		/* turning on */
		err = i2c_smbus_write_i2c_block_data(lis3dh_data->client,
			CTRL_REG1 | AC, sizeof(lis3dh_data->ctrl_regs),
						lis3dh_data->ctrl_regs);
		if (err < 0) {
			err = -EIO;
			goto unlock;
		}

		lis3dh_data->timer_enable = 1;
		lis3dh_data->work_enable = 1;

		if (lis3dh_data->interruptible)
			enable_irq(lis3dh_data->client->irq);
		else {
			set_polling_delay(lis3dh_data, 0);
			hrtimer_start(&lis3dh_data->timer, lis3dh_data->polling_delay, HRTIMER_MODE_REL);
		}
	} else {
		lis3dh_data->timer_enable = 0;
		lis3dh_data->work_enable = 0;

		if (lis3dh_data->interruptible)
			disable_irq(lis3dh_data->client->irq);
		else {
			hrtimer_cancel(&lis3dh_data->timer);
			cancel_work_sync(&lis3dh_data->work);
		}
		/* turning off */
		err = i2c_smbus_write_byte_data(lis3dh_data->client,
						CTRL_REG1, LIS3DH_ACC_DISABLE);
		if (err < 0)
			goto unlock;
	}
	lis3dh_data->enable = new_enable;

unlock:
	mutex_unlock(&lis3dh_data->lock);

	return err ? err : size;
}

static ssize_t lis3dh_show_delay(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct lis3dh_data *lis3dh_data  = dev_get_drvdata(dev);
	u64 delay;

	delay = lis3dh_data->time_to_read * lis3dh_data->entries;
	delay = ktime_to_ns(ns_to_ktime(delay));

	return sprintf(buf, "%lld\n", delay);
}

static ssize_t lis3dh_set_delay(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	struct lis3dh_data *lis3dh_data  = dev_get_drvdata(dev);
	int odr_value = ODR100;
	int res = 0;
	int i;
	u64 delay_ns;
	u8 ctrl;

	res = strict_strtoll(buf, 10, &delay_ns);
	if (res < 0)
		return res;

	mutex_lock(&lis3dh_data->lock);
	if (!lis3dh_data->interruptible)
		hrtimer_cancel(&lis3dh_data->timer);
	else
		disable_irq(lis3dh_data->client->irq);

	/* round to the nearest supported ODR that is equal or above than
	 * the requested value
	 */
	for (i = 0; i < ARRAY_SIZE(odr_delay_table); i++) {
		if (delay_ns < odr_delay_table[i].delay_ns)
			break;
	}
	if (i > 0)
		i--;

	odr_value = odr_delay_table[i].odr;
	delay_ns = odr_delay_table[i].delay_ns;
	lis3dh_data->time_to_read = delay_ns;
	lis3dh_data->entries = 1;

	if (delay_ns >= odr_delay_table[3].delay_ns) {
		if (delay_ns >= MAX_DELAY) {
			lis3dh_data->entries = MAX_ENTRY;
			delay_ns = MAX_DELAY;
		} else {
			do_div(delay_ns, odr_delay_table[3].delay_ns);
			lis3dh_data->entries = delay_ns;
		}
		lis3dh_data->time_to_read = odr_delay_table[3].delay_ns;
	}

	if (odr_value != (lis3dh_data->ctrl_regs[0] & ODR_MASK)) {
		ctrl = (lis3dh_data->ctrl_regs[0] & ~ODR_MASK);
		ctrl |= odr_value;
		lis3dh_data->ctrl_regs[0] = ctrl;
		res = i2c_smbus_write_byte_data(lis3dh_data->client, CTRL_REG1, ctrl);
	}
	/* we see a noise in the first sample or two after we
	 * change rates.  this delay helps eliminate that noise.
	 */
	//msleep((u32)delay_ns * 2 / NSEC_PER_MSEC);

	if (!lis3dh_data->interruptible) {
		delay_ns = lis3dh_data->entries * lis3dh_data->time_to_read;
		lis3dh_data->polling_delay = ns_to_ktime(delay_ns);
		if (lis3dh_data->enable)
			hrtimer_start(&lis3dh_data->timer,
				lis3dh_data->polling_delay, HRTIMER_MODE_REL);
	} else
		enable_irq(lis3dh_data->client->irq);

	mutex_unlock(&lis3dh_data->lock);
	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
			lis3dh_show_enable, lis3dh_set_enable);
static DEVICE_ATTR(poll_delay, S_IRUGO | S_IWUSR | S_IWGRP,
			lis3dh_show_delay, lis3dh_set_delay);

static int lis3dh_probe(struct i2c_client *client,
			       const struct i2c_device_id *devid)
{
	int ret;
	int err = 0;
	struct lis3dh_data *data;
	struct input_dev *input_dev;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL) {
		dev_err(&client->dev, "failed to allocate memory for module data\n");
		err = -ENOMEM;
		goto exit;
	}

	data->client = client;

	/* read chip id */
	ret = i2c_smbus_read_byte_data(client, WHO_AM_I);
	if (ret != WHOAMI_LIS3DH_ACC) {
		if (ret < 0) {
			pr_err("%s: i2c for reading chip id failed\n", __func__);
			err = ret;
		} else {
			pr_err("%s : Device identification failed\n", __func__);
			err = -ENODEV;
		}
		goto err_read_reg;
	}

	mutex_init(&data->lock);

	/* allocate acc input_device */
	input_dev = input_allocate_device();
	if (!input_dev) {
		pr_err("%s: could not allocate input device\n", __func__);
		err = -ENOMEM;
		goto err_input_allocate_device;
	}

	data->input_dev = input_dev;
	input_set_drvdata(input_dev, data);
	input_dev->name = "acc";
	/* X */
	input_set_capability(input_dev, EV_REL, REL_RX);
	input_set_abs_params(input_dev, REL_RX, -32768, 32767, 0, 0);
	/* Y */
	input_set_capability(input_dev, EV_REL, REL_RY);
	input_set_abs_params(input_dev, REL_RY, -32768, 32767, 0, 0);
	/* Z */
	input_set_capability(input_dev, EV_REL, REL_RZ);
	input_set_abs_params(input_dev, REL_RZ, -32768, 32767, 0, 0);

	err = input_register_device(input_dev);
	if (err < 0) {
		pr_err("%s: could not register input device\n", __func__);
		input_free_device(data->input_dev);
		goto err_input_register_device;
	}

	memcpy(&data->ctrl_regs, &default_ctrl_regs, sizeof(default_ctrl_regs));

	if (data->client->irq >= 0) { /* interrupt */
		data->interruptible = true;
		err = request_threaded_irq(data->client->irq, NULL,
			lis3dh_interrupt_thread, IRQF_TRIGGER_HIGH | IRQF_ONESHOT,
				"lis3dh", data);
		if (err < 0) {
			pr_err("%s: can't allocate irq.\n", __func__);
			goto err_request_irq;
		}
		disable_irq(data->client->irq);
	} else { /* polling */
		u64 delay_ns;
		data->ctrl_regs[2] = 0x00; /* disable interrupt */
		/* hrtimer settings.  we poll for acc values using a timer. */
		acc_debug("enter polling mode %s\n",__func__);
		hrtimer_init(&data->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		data->polling_delay = ns_to_ktime(200 * NSEC_PER_MSEC); //this data need to carefully modified
		data->time_to_read = 10000000LL;
		delay_ns = ktime_to_ns(data->polling_delay);
		do_div(delay_ns, data->time_to_read);
		data->entries = delay_ns;
		data->timer.function = lis3dh_timer_func;

		/* the timer just fires off a work queue request.
		   We need a thread to read i2c (can be slow and blocking). */
		data->lis3dh_wq = create_singlethread_workqueue("lis3dh_wq");
		if (!data->lis3dh_wq) {
			err = -ENOMEM;
			pr_err("%s: could not create workqueue\n", __func__);
			goto err_create_workqueue;
		}
		/* this is the thread function we run on the work queue */
		INIT_WORK(&data->work, lis3dh_work_func);
	}

	if (device_create_file(&input_dev->dev, &dev_attr_enable) < 0) {
		pr_err("Failed to create device file(%s)!\n", dev_attr_enable.attr.name);
		goto err_device_create_file;
	}

	if (device_create_file(&input_dev->dev,	&dev_attr_poll_delay) < 0) {
		pr_err("Failed to create device file(%s)!\n",
				dev_attr_poll_delay.attr.name);
		goto err_device_create_file2;
	}

	i2c_set_clientdata(client, data);
	dev_set_drvdata(&input_dev->dev, data);

	return 0;

err_device_create_file2:
	device_remove_file(&input_dev->dev, &dev_attr_enable);
err_device_create_file:
	if (data->interruptible) {
		enable_irq(data->client->irq);
		free_irq(data->client->irq, data);
	} else
		destroy_workqueue(data->lis3dh_wq);
	input_unregister_device(data->input_dev);
err_create_workqueue:
err_request_irq:
err_input_register_device:
err_input_allocate_device:
	mutex_destroy(&data->lock);
err_read_reg:
	kfree(data);
exit:
	return err;
}

static int lis3dh_remove(struct i2c_client *client)
{
	int err = 0;
	struct lis3dh_data *lis3dh_data = i2c_get_clientdata(client);

	device_remove_file(&lis3dh_data->input_dev->dev, &dev_attr_enable);
	device_remove_file(&lis3dh_data->input_dev->dev, &dev_attr_poll_delay);

	if (lis3dh_data->enable)
		err = i2c_smbus_write_byte_data(lis3dh_data->client,
					CTRL_REG1, LIS3DH_ACC_DISABLE);
	if (lis3dh_data->interruptible) {
		if (!lis3dh_data->enable) /* no disable_irq before free_irq */
			enable_irq(lis3dh_data->client->irq);
		free_irq(lis3dh_data->client->irq, lis3dh_data);
	} else {
		hrtimer_cancel(&lis3dh_data->timer);
		cancel_work_sync(&lis3dh_data->work);
		destroy_workqueue(lis3dh_data->lis3dh_wq);
	}

	input_unregister_device(lis3dh_data->input_dev);
	mutex_destroy(&lis3dh_data->lock);
	kfree(lis3dh_data);

	return err;
}

static int lis3dh_suspend(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct lis3dh_data *lis3dh_data = i2c_get_clientdata(client);

	if (lis3dh_data->enable) {
		mutex_lock(&lis3dh_data->lock);
		if (!lis3dh_data->interruptible) {
			hrtimer_cancel(&lis3dh_data->timer);
			cancel_work_sync(&lis3dh_data->work);
		}
		err = i2c_smbus_write_byte_data(lis3dh_data->client,
						CTRL_REG1, LIS3DH_ACC_DISABLE);
		mutex_unlock(&lis3dh_data->lock);
	}

	return err;
}

static int lis3dh_resume(struct device *dev)
{
	int err = 0;
	struct i2c_client *client = to_i2c_client(dev);
	struct lis3dh_data *lis3dh_data = i2c_get_clientdata(client);

	if (lis3dh_data->enable) {
		mutex_lock(&lis3dh_data->lock);
		if (!lis3dh_data->interruptible)
			hrtimer_start(&lis3dh_data->timer,
				lis3dh_data->polling_delay, HRTIMER_MODE_REL);
		err = i2c_smbus_write_i2c_block_data(client,
				CTRL_REG1 | AC, sizeof(lis3dh_data->ctrl_regs),
							lis3dh_data->ctrl_regs);
		mutex_unlock(&lis3dh_data->lock);
	}

	return err;
}

static const struct dev_pm_ops lis3dh_pm_ops = {
	.suspend = lis3dh_suspend,
	.resume = lis3dh_resume
};

static const struct i2c_device_id lis3dh_id[] = {
	{ "lis3dh", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lis3dh_id);

static struct i2c_driver lis3dh_driver = {
	.probe = lis3dh_probe,
	.remove = __devexit_p(lis3dh_remove),
	.id_table = lis3dh_id,
	.driver = {
		.pm = &lis3dh_pm_ops,
		.owner = THIS_MODULE,
		.name = "lis3dh"
	},
};

static int __init lis3dh_init(void)
{
	return i2c_add_driver(&lis3dh_driver);
}

static void __exit lis3dh_exit(void)
{
	i2c_del_driver(&lis3dh_driver);
}

module_init(lis3dh_init);
module_exit(lis3dh_exit);

MODULE_DESCRIPTION("lis3dh digital accelerometer sensor driver");
MODULE_LICENSE("GPL v2");
