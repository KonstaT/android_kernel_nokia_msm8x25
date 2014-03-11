/* 
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
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

#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/signal.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/stat.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/input/isl29028.h>


#define ISL29028_MODULE_NAME	"isl29028"
#define DRIVER_VERSION		"1.0.0"
#define ISL29028_I2C_ADDR   	0x44
#define ISL29028_INTR	        17

#define DEFAULT_PROX_INTERVAL  200//ms200-100 inprove performance
#define DEFAULT_ALS_INTERVAL   500//ms

#define CONFIG_ALS_H_RANGE
#ifdef  CONFIG_ALS_H_RANGE
#define CONFIG_VAL	(ALS_H_RANGE | SLP_200MS)//proximity sleep pulse 200ms
#else
#define CONFIG_VAL	0x00
#endif
#define INTERRUPT_VAL	0x20

//isl29028
#define ISL29028_CONFIG		0x01
#define ISL29028_INTCONFIG	0x02
//PROX
#define ISL29028_PROX_LT	0x03
#define ISL29028_PROX_HT	0x04
//ALS
#define ISL29028_ALSIR_TH1	0x05
#define ISL29028_ALSIR_TH2	0x06
#define ISL29028_ALSIR_TH3	0x07
//PROX
#define ISL29028_PROX_DATA	0x08
//ALS
#define ISL29028_ALSIR_DATA1	0x09
#define ISL29028_ALSIR_DATA2	0x0A

#define ISl29028_TEST1	 0x0E
#define ISl29028_TEST2   0x0F

#define ALS_H_RANGE  (1<<1)
#define ALS_EN       (1<<2)
#define PROX_DR      (1<<3)
#define PROX_EN      (1<<7)
#define PROX_SLP     (0x07<<4)
#define SLP_800MS    (0<<4)
#define SLP_400MS    (1<<4)
#define SLP_200MS    (2<<4)
#define SLP_100MS    (3<<4)
#define SLP_75MS     (4<<4)
#define SLP_50MS     (5<<4)
#define SLP_12DOT5MS (6<<4)

#define PROX_FLAG  (1<<7)
#define ALS_FLAG   (1<<3)

#define ID_TYPE_LIGHT   (1<<31)
#define ID_TYPE_PROX    (1<<30)

#define CURRENT_100MA (100 <<8)
#define CURRENT_200MA (200 <<8)
#define PROXIMITY_MAX 140
//#define THRESHOLD	50
#define STEP		15
#define ADJ_DBG
#define NUM  5

//#define ISL_DEBUG
#ifdef ISL_DEBUG
#define PR_DEB_IF(cond,fmt,args...) do \
	{if(cond) printk(KERN_DEBUG "-ISL29028-SENSOR-" fmt,##args);}while(0)

#define PR_DEB(fmt,args...) do \
	{printk(KERN_DEBUG "-ISL29028-SENSOR-" fmt,##args);}while(0)
#else
#define PR_DEB_IF(cond,...)
#define PR_DEB(...)
#endif

struct isl29028_data{
	struct i2c_client *client;
	struct input_dev *proximity_input_dev;
	struct input_dev *light_input_dev;
	struct delayed_work als_work;
	struct work_struct prox_work;
	int prox_interval;
	int als_interval;
	struct wake_lock prox_wake_lock;
	atomic_t als_working;
	atomic_t prox_working;
	atomic_t prox_locking;
	struct isl29028_platform_data *pdata;
	bool  ps_near;
};

static struct workqueue_struct *wq;
static struct mutex prox_lock;
static struct mutex als_lock;

static int prox_threshold_hi = 50;	//Default value
static int prox_threshold_lo = 30;

//static int threshold = THRESHOLD;

/* set 3 level light sensor level*/
#define ISL_LUX_LEVELS 3
static const u16 isl_lux_levels[ISL_LUX_LEVELS] = {
	2,
	259,
	525
};

static u16 pre_lux_level = -1;
static u16 pre_prox_cm   = -1;

static int isl29028_read_reg(struct i2c_client *client,
				u8 reg, u8 *val)
{
	*val = i2c_smbus_read_byte_data(client, reg);
	if (!val) {
			pr_err("%s:i2c read failed!\n", __func__);
			return -EIO;
	}
	return 0;
}

static int isl29028_write_reg(struct i2c_client *client,
				u8 reg, u8 val)
{
	int ret = 0;

	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (ret) {
			pr_err("%s:i2c write failed!\n", __func__);
			ret = -EIO;
	}
	return ret;
}
static int isl29028_prox_read(struct i2c_client *client,
				u8 *val)
{
	int ret;

	ret = isl29028_read_reg(client, ISL29028_PROX_DATA, val);
	return ret;
}
static int isl29028_als_read(struct i2c_client *client,
				u16 *val)
{
	u8 lb,hb;
	int ret = 0;

	ret |= isl29028_read_reg(client, ISL29028_ALSIR_DATA1, &lb);
	ret |= isl29028_read_reg(client, ISL29028_ALSIR_DATA2, &hb);
	*val = ((hb & 0x0F) << 8) | lb;
	return ret;
}
static int isl29028_enable_prox(struct i2c_client *client)
{
	u8 temp;
	int ret = 0;
	char buf[2] = {0};

	buf[0] = prox_threshold_hi & 0xff;
	buf[1] = prox_threshold_lo & 0xff;
	isl29028_write_reg(client,ISL29028_PROX_LT,buf[1]);
	isl29028_write_reg(client,ISL29028_PROX_HT,buf[0]);

	ret |= isl29028_read_reg(client, ISL29028_CONFIG, &temp);
	temp|= PROX_EN;
	ret |= isl29028_write_reg(client, ISL29028_CONFIG, temp);

	return ret;
}
static int isl29028_disable_prox(struct i2c_client *client)
{
	u8 temp;
	int ret = 0;

	ret |= isl29028_read_reg(client, ISL29028_CONFIG, &temp);
	temp&= ~PROX_EN;
	ret |= isl29028_write_reg(client, ISL29028_CONFIG, temp);

	return ret;
}
static int isl29028_enable_als(struct i2c_client *client)
{
	u8 temp;
	int ret = 0;

	ret |= isl29028_read_reg(client, ISL29028_CONFIG, &temp);
	temp|= ALS_EN;
	ret |= isl29028_write_reg(client, ISL29028_CONFIG, temp);

	return ret;
}
static int isl29028_disable_als(struct i2c_client *client)
{
	u8 temp;
	int ret = 0;

	ret |= isl29028_read_reg(client, ISL29028_CONFIG, &temp);
	temp&= ~ALS_EN;
	ret |= isl29028_write_reg(client, ISL29028_CONFIG, temp);

	return ret;
}

static void sysfs_enable_prox(int enable,struct isl29028_data *data)
{
	if(enable){
		if(!atomic_read(&data->prox_working)){
			atomic_set(&data->prox_working,1);
			/*lock system*/
			wake_lock(&data->prox_wake_lock);
			atomic_set(&data->prox_locking,1);
			PR_DEB("Enter SYSFS_EN_PROX and locked prox_locking=%d\n",
				atomic_read(&data->prox_locking));

			isl29028_enable_prox(data->client);
			pre_prox_cm = -1;
			/*delay for 20ms to makesure prox enable*/
//			queue_delayed_work(wq, &data->prox_work,20);
		}else{
			PR_DEB("Prox thread has running\n");
		}	
	}else{
		if(atomic_read(&data->prox_working)){
			atomic_set(&data->prox_working,0);
			/*unlock system*/
			wake_unlock(&data->prox_wake_lock);
			atomic_set(&data->prox_locking,0);
			PR_DEB("SYSFS_DIS_PROX and unlocked prox_locking=%d\n",
				atomic_read(&data->prox_locking));

			cancel_work_sync(&data->prox_work);
			isl29028_disable_prox(data->client);
		}else{
			PR_DEB("Prox thread has not running already\n");
		}	
	}
}
static void sysfs_enable_als(int enable,struct isl29028_data *data)
{
	if(enable){
		if(!atomic_read(&data->als_working)){
			PR_DEB("Enter SYSFS_EN_ALS\n");
			atomic_set(&data->als_working,1);
			isl29028_enable_als(data->client);
			pre_lux_level = -1;
			/*delay for 20ms to makesure als enable*/
			queue_delayed_work(wq, &data->als_work,20);
		}else{
			PR_DEB("Als thread has running\n");		
		}
	}else{
		if(atomic_read(&data->als_working)){
			PR_DEB("Enter SYSFS_DIS_ALS\n");
			atomic_set(&data->als_working,0);
			cancel_delayed_work(&data->als_work);
			isl29028_disable_als(data->client);
		}else{
			PR_DEB("Als thread has not running already\n");		
		}

	}
}

/* sysfs interface*/
static ssize_t light_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int state;
	struct isl29028_data *data = dev_get_drvdata(dev);
	mutex_lock(&als_lock);
	state = atomic_read(&data->als_working);
	mutex_unlock(&als_lock);
	PR_DEB("GET_ALS_STATE:state = %d\n",state);
	return sprintf(buf, "%d\n", state);
}

static ssize_t proximity_enable_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int state;
	struct isl29028_data *data = dev_get_drvdata(dev);
	mutex_lock(&prox_lock);
	state = atomic_read(&data->prox_working);
	mutex_unlock(&prox_lock);
	PR_DEB("GET_PROX_STATE:state = %d\n",state);
	return sprintf(buf, "%d\n", state);
}

static ssize_t light_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct isl29028_data *data = dev_get_drvdata(dev);
	int new_value;
	if (sysfs_streq(buf, "1"))
		new_value = 1;
	else if (sysfs_streq(buf, "0"))
		new_value = 0;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
	PR_DEB("SYSFS_EN_ALS %d\n", new_value);
	mutex_lock(&als_lock);
	sysfs_enable_als(new_value,data);
	mutex_unlock(&als_lock);
	return size;
}

static ssize_t proximity_enable_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	struct isl29028_data *data = dev_get_drvdata(dev);
	int new_value;
	u8 int_flag;
	if (sysfs_streq(buf, "1"))
		new_value = 1;
	else if (sysfs_streq(buf, "0"))
		new_value = 0;
	else {
		pr_err("%s: invalid value %d\n", __func__, *buf);
		return -EINVAL;
	}
	PR_DEB("SYSFS_EN_PROX %d\n", new_value);
	mutex_lock(&prox_lock);
	isl29028_read_reg(data->client,ISL29028_INTCONFIG,&int_flag);
	sysfs_enable_prox(new_value,data);
	mutex_unlock(&prox_lock);
	return size;
}

static struct device_attribute dev_attr_light_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	light_enable_show, light_enable_store);

static struct device_attribute dev_attr_proximity_enable =
	__ATTR(enable, S_IRUGO | S_IWUSR | S_IWGRP,
	proximity_enable_show, proximity_enable_store);

static struct attribute *light_sysfs_attrs[] = {
	&dev_attr_light_enable.attr,
	NULL
};

static struct attribute_group light_attribute_group = {
	.attrs = light_sysfs_attrs,
};

static struct attribute *proximity_sysfs_attrs[] = {
	&dev_attr_proximity_enable.attr,
	NULL
};

static struct attribute_group proximity_attribute_group = {
	.attrs = proximity_sysfs_attrs,
};
/* end of sysfs */

static int isl29028_init_client(struct i2c_client *client)
{
	int ret = 0;
	ret |= isl29028_write_reg(client, ISL29028_CONFIG, CONFIG_VAL);
	ret |= isl29028_write_reg(client, ISL29028_INTCONFIG, INTERRUPT_VAL);
	return ret;
}

static void prox_work_func(struct work_struct *work)
{
	int cm = 0;
	u8 val;
	struct isl29028_data *data = container_of(work,struct isl29028_data,prox_work);

retry:
	isl29028_prox_read(data->client, &val);
	if(val < prox_threshold_lo){  //far
		cm = 2;
		data->ps_near = false;
		isl29028_write_reg(data->client,ISL29028_PROX_LT,0);
		isl29028_write_reg(data->client,ISL29028_PROX_LT,prox_threshold_hi & 0xff);
		input_report_abs(data->proximity_input_dev, ABS_DISTANCE, cm);
		input_sync(data->proximity_input_dev);
	}else if (val > prox_threshold_hi){ //near
		cm = 0;
		data->ps_near = true;
		isl29028_write_reg(data->client,ISL29028_PROX_HT,0xff);
		isl29028_write_reg(data->client,ISL29028_PROX_LT,prox_threshold_lo & 0xff);
		input_report_abs(data->proximity_input_dev, ABS_DISTANCE, cm);
		input_sync(data->proximity_input_dev);
		msleep(500);
		goto retry;
	}
	else{
		isl29028_write_reg(data->client,ISL29028_PROX_HT,0xff);
		isl29028_write_reg(data->client,ISL29028_PROX_LT,0x00);
		msleep(500);
		goto retry;
	}

	PR_DEB("############prox_data cm = %d\n",cm);
	pre_prox_cm = cm;
	isl29028_enable_prox(data->client);
//	queue_work(wq, &data->prox_work);
}

/* this is ISR*/
static irqreturn_t isl29028_interrupt(int vec, void *info)
{
	struct i2c_client *client=(struct i2c_client *)info;
	struct isl29028_data *data = i2c_get_clientdata(client);
	u8 int_flag;

	wake_lock(&data->prox_wake_lock);
	prox_work_func(&data->prox_work);
	isl29028_read_reg(client,ISL29028_INTCONFIG,&int_flag);
	if(int_flag & PROX_FLAG){
		int_flag &=~(PROX_FLAG);
		int_flag &=~(ALS_FLAG);
		isl29028_write_reg(client,ISL29028_INTCONFIG,int_flag);
	}
	wake_unlock(&data->prox_wake_lock);

	return IRQ_HANDLED;
}

static int to_lux(struct i2c_client *client,u16 data)
{
	int res = 0;
	u8 temp;

	isl29028_read_reg(client, ISL29028_CONFIG, &temp);
	if(temp & ALS_H_RANGE)
		res = (data * 522)/100;		//adjust the gain from 1000 to 100
	else
		res = (data * 326)/1000;

	return res;
}
static void als_work_func(struct work_struct *work)
{
	u16 val,val2,val3;
	int level=0;
	struct isl29028_data *data = container_of(work,struct isl29028_data,als_work.work);

	isl29028_als_read(data->client, &val);
	msleep(500);
	isl29028_als_read(data->client, &val2);
	msleep(500);
	isl29028_als_read(data->client, &val3);
	val = (val + val2 + val3) / 3;
	val = to_lux(data->client,val);
	PR_DEB("@@@@@@@@@@@@@@@@als_data = %d\n",val);

	while (level < ISL_LUX_LEVELS && val > isl_lux_levels[level]){
		level++;
	}

	if (level >= ISL_LUX_LEVELS) {
		level = ISL_LUX_LEVELS -1;
	}

	PR_DEB("@@@@@@@@@@@@@@@@als_level = %d\n",level);
	input_report_abs(data->light_input_dev, ABS_MISC, (int)val);
	input_sync(data->light_input_dev);

	if (level != pre_lux_level) {
		pre_lux_level =level;
	}
	PR_DEB("als-msecs=%d\n",(int)msecs_to_jiffies(data->als_interval));
	if(atomic_read(&data->als_working))
		queue_delayed_work(wq, &data->als_work,msecs_to_jiffies(data->als_interval));
}

/* isl290828 reset*/
static int isl29028_reset(struct i2c_client *client)
{
	int ret=0;

	//step 1
	ret |= isl29028_write_reg(client, ISl29028_TEST2, 0x29);//0x0F
	//step 2
	ret |= isl29028_write_reg(client, ISl29028_TEST1, 0x00);//0x0E
	ret |= isl29028_write_reg(client, ISl29028_TEST2, 0x00);//0x0F
	ret |= isl29028_write_reg(client, ISL29028_CONFIG, 0x00);//0x01
	//step 3 wait ~1ms or more
	msleep(10);
	if(ret != 0)
		PR_DEB("isl29028_reset failure!\n");
	return ret;
}

static int __devinit isl29028_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	int err = -ENODEV;
	struct input_dev *input_dev;
	struct isl29028_data *data;
	PR_DEB("isl29028_probe\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		return err;
	}

	data = kzalloc(sizeof(struct isl29028_data), GFP_KERNEL);
	if (data == NULL) {
		err = -ENOMEM;
		goto exit;
	}

	data->pdata = client->dev.platform_data;
	data->client = client;
	data->ps_near = false;
	i2c_set_clientdata(client, data);

	dev_info(&client->dev, "support ver. %s enabled\n", DRIVER_VERSION);

	/* allocate proximity input_device */
	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		err = -ENOMEM;
		dev_err(&data->client->dev, "proximity input device allocate failed\n");
		goto exit0;
	}
	input_set_drvdata(input_dev, data);
	input_dev->name = "proximity";
	input_set_capability(input_dev, EV_ABS, ABS_DISTANCE);
	input_set_abs_params(input_dev, ABS_DISTANCE, 0, 1, 0, 0);

	PR_DEB("registering proximity input device\n");
	err = input_register_device(input_dev);
	if (err) {
		pr_err("%s: could not register input device\n", __func__);
		goto exit1;
	}
	data->proximity_input_dev = input_dev;
	/* create proximity sysfs interface */
	err = sysfs_create_group(&input_dev->dev.kobj, &proximity_attribute_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto exit2;
	}

	/* allocate light input_device */
	input_dev = input_allocate_device();
	if (input_dev == NULL) {
		err = -ENOMEM;
		dev_err(&data->client->dev, "light input device allocate failed\n");
		goto exit2;
	}
	input_set_drvdata(input_dev, data);
	input_dev->name = "light";
	input_set_capability(input_dev, EV_ABS, ABS_MISC);
	input_set_abs_params(input_dev, ABS_MISC, 0, 1, 0, 0);

	PR_DEB("registering light input device\n");
	err = input_register_device(input_dev);
	if (err) {
		pr_err("%s: could not register input device\n", __func__);
		goto exit3;
	}
	data->light_input_dev = input_dev;
	/* create proximity sysfs interface */
	err = sysfs_create_group(&input_dev->dev.kobj, &light_attribute_group);
	if (err) {
		pr_err("%s: could not create sysfs group\n", __func__);
		goto exit4;
	}

	wq = create_workqueue("isl29028_workqueue");
	if (wq == NULL) {
		PR_DEB("can't create a workqueue\n");
		err = -1;
		goto exit4;
	}
	INIT_WORK(&data->prox_work, prox_work_func);
	INIT_DELAYED_WORK(&data->als_work, als_work_func);

	/*isl29028 reset*/
	isl29028_reset(client);
	/*isl29028 init*/
	err = isl29028_init_client(client);
	if(err)
		goto exit5;

	wake_lock_init(&data->prox_wake_lock, WAKE_LOCK_SUSPEND, "prox_wake_lock");
	/* setup interrupt and wakeup */
	device_init_wakeup(&client->dev, true);
	err = request_threaded_irq(data->pdata->int_gpio, NULL, isl29028_interrupt, IRQ_TYPE_EDGE_FALLING, ISL29028_MODULE_NAME, (void *)client);
	if(err){
		pr_err("%s Could not allocate irq(%d) !\n", __func__, data->pdata->int_gpio);
		goto exit_irq;
	}

	/*init mutex for prox */
        mutex_init(&prox_lock);
	/*init mutex for als */
        mutex_init(&als_lock);

	/*set init state to not-working*/
	atomic_set(&data->als_working,0);
	atomic_set(&data->prox_working,0);
	/*set init state to not-locking*/
	atomic_set(&data->prox_locking,0);

	/*set default interval*/
	data->prox_interval = DEFAULT_PROX_INTERVAL;
	data->als_interval = DEFAULT_ALS_INTERVAL;

	dev_info(&client->dev, "sensor driver probe successful\n");
	goto exit;

exit_irq:
	device_init_wakeup(&client->dev, false);
	wake_lock_destroy(&data->prox_wake_lock);
exit5:
	destroy_workqueue(wq);
exit4:
	sysfs_remove_group(&data->light_input_dev->dev.kobj, &light_attribute_group);
exit3:
	input_unregister_device(data->light_input_dev);
	input_free_device(data->light_input_dev);
exit2:
	sysfs_remove_group(&data->proximity_input_dev->dev.kobj, &proximity_attribute_group);
exit1:
	input_unregister_device(data->proximity_input_dev);
	input_free_device(data->proximity_input_dev);
exit0:
	kfree(data);

exit:
	return err;
}

static int __devexit isl29028_remove(struct i2c_client *client)
{

	struct isl29028_data *data = i2c_get_clientdata(client);

	free_irq(data->pdata->int_gpio,(void *)client);
	device_init_wakeup(&client->dev,false);

	if(atomic_read(&data->prox_working)){
		isl29028_disable_prox(data->client);
		cancel_work_sync(&data->prox_work);
	}

	if(atomic_read(&data->als_working)){
		isl29028_disable_prox(data->client);
		cancel_delayed_work_sync(&data->als_work);
	}
	flush_workqueue(wq);
	destroy_workqueue(wq);
	wq = NULL;

        mutex_destroy(&prox_lock);
        mutex_destroy(&als_lock);

	input_unregister_device(data->proximity_input_dev);
	input_unregister_device(data->light_input_dev);
	input_free_device(data->proximity_input_dev);
	input_free_device(data->light_input_dev);

	wake_lock_destroy(&data->prox_wake_lock);
	kfree(data);

	return 0;
}
#ifdef CONFIG_PM
static int isl29028_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret = 0,prox_s,als_s;
	u8 status;
	struct isl29028_data *data = i2c_get_clientdata(client);

	prox_s = atomic_read(&data->prox_working);
	als_s = atomic_read(&data->als_working);
	isl29028_read_reg(client,ISL29028_CONFIG,&status);

	if(status & ALS_EN){
		isl29028_disable_als(data->client);
	}

	if(status & PROX_EN){
		enable_irq_wake(data->pdata->int_gpio);
	}

	PR_DEB("Enter %s:prox_state:%d,als_state:%d\n",
		__func__,prox_s,als_s);

	if(prox_s){
		cancel_work_sync(&data->prox_work);
		isl29028_disable_prox(data->client);	
	}
	if(als_s){
		cancel_delayed_work_sync(&data->als_work);
		isl29028_disable_prox(data->client);	
	}
	PR_DEB("Exit %s\n",__func__);
	return ret;
}

static int isl29028_resume(struct i2c_client *client)
{
	int ret = 0,prox_s,als_s;
	u8 status;
	struct isl29028_data *data = i2c_get_clientdata(client);
	prox_s = atomic_read(&data->prox_working);
	als_s = atomic_read(&data->als_working);

	isl29028_read_reg(client,ISL29028_CONFIG,&status);

	if(status & ALS_EN){
		isl29028_enable_als(data->client);
	}

	if(status & PROX_EN){
		disable_irq_wake(data->pdata->int_gpio);
	}

	PR_DEB("Enter %s:prox_state:%d,als_state:%d\n",
		__func__,prox_s,als_s);

	if(prox_s){
		isl29028_enable_prox(data->client);
//		queue_delayed_work(wq, &data->prox_work,20);
	}
	if(als_s){
		isl29028_enable_als(data->client);
		queue_delayed_work(wq, &data->als_work,20);
	}
	PR_DEB("Exit %s\n",__func__);
	return ret;
}
#else
#define isl29028_suspend	NULL
#define isl29028_resume		NULL
#endif

static const struct i2c_device_id isl29028_id[] = {
	{ "isl29028", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, isl29028_id);

static struct i2c_driver isl29028_driver = {
	.driver = {
		.name	= ISL29028_MODULE_NAME,
		.owner	= THIS_MODULE,
	},
	.suspend = isl29028_suspend,
	.resume  = isl29028_resume,
	.probe	 = isl29028_probe,
	.remove  = __devexit_p(isl29028_remove),
	.id_table= isl29028_id,
};

static int __init isl29028_init(void)
{
	return i2c_add_driver(&isl29028_driver);
}

static void __exit isl29028_exit(void)
{
	i2c_del_driver(&isl29028_driver);
}

MODULE_DESCRIPTION("ISL29028 ambient light and proximity sensor driver");
MODULE_LICENSE("GPLv2");
MODULE_VERSION(DRIVER_VERSION);

module_init(isl29028_init);
module_exit(isl29028_exit);
