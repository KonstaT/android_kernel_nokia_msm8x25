/* drivers/input/touchscreen/tp_work_func.c
 *
 * Copyright (C) 2011 BYD std.
 * Author: wang.zhen16@byd.com
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/input.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/workqueue.h>
#include "tp_work_func.h"

struct tp_dev *tpdev;
static struct workqueue_struct *tp_wq;
// [sun.yu5@byd.com, modify,WG703T2_C000133,support 2  touchscreen, define  struct virtual_key vk from 1 to 2]

static ssize_t byd_virtual_keys_show(struct kobject *kobj,
			       struct kobj_attribute *attr, char *buf)
{
		return sprintf(buf,
			__stringify(EV_KEY) ":" __stringify(KEY_MENU)  ":%s"
			":" __stringify(EV_KEY) ":" __stringify(KEY_SEARCH)   ":%s"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOMEPAGE)   ":%s"
			":" __stringify(EV_KEY) ":" __stringify(KEY_BACK)   ":%s"
			"\n",tpdev->pdata->vk[0].menu,tpdev->pdata->vk[0].search,tpdev->pdata->vk[0].home,tpdev->pdata->vk[0].back);
} 
// [sun.yu5@byd.com, end] 

irqreturn_t tp_irq_handler(int irq, void *dev_id)
{
	struct tp_dev *tpdev = dev_id; 
	disable_irq_nosync(tpdev->client->irq);
	queue_work(tp_wq, &tpdev->work);

	return IRQ_HANDLED;
}
#define MXT_MAX_NUM_TOUCHES 5
int tp_work_func_register(struct tp_dev *tp_device)
{
	int ret = -1;
	struct kobject *properties_kobj;
	tpdev = tp_device;
	tp_wq = create_singlethread_workqueue("tp_wq");	
	if (!tp_wq)
	{
		printk(KERN_ERR "Could not create work queue focal_wq: no memory");
		return -ENOMEM;
	}
	INIT_WORK(&tpdev->work, tpdev->work_func);
	tpdev->pdata->byd_virtual_keys_attr.show = &byd_virtual_keys_show;
	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		ret = sysfs_create_file(properties_kobj,
					&tpdev->pdata->byd_virtual_keys_attr.attr);
	if (!properties_kobj || ret)
		pr_err("failed to create board_properties\n");
	tpdev->input_dev = input_allocate_device();
	if (!tpdev->input_dev) 
	{
		printk(KERN_ERR "failed to allocate input device.\n");
		ret = -EBUSY;
		goto err_alloc_dev_failed;
	}
	tpdev->input_dev->name = tpdev->pdata->input_dev_name;
	tpdev->input_dev->phys = tpdev->pdata->input_dev_name;
	set_bit(EV_ABS, tpdev->input_dev->evbit);
	set_bit(EV_SYN, tpdev->input_dev->evbit);
	set_bit(EV_KEY, tpdev->input_dev->evbit);
	set_bit(KEY_HOME, tpdev->input_dev->keybit);
	set_bit(KEY_MENU, tpdev->input_dev->keybit);
	set_bit(KEY_BACK, tpdev->input_dev->keybit);
	set_bit(KEY_SEARCH, tpdev->input_dev->keybit);

    
    //zgx
     //set_bit(BTN_MISC, tpdev->input_dev->keybit);
        
    //__set_bit(INPUT_PROP_POINTER, tpdev->input_dev->propbit);
    //__set_bit(INPUT_PROP_SEMI_MT, tpdev->input_dev->propbit);
    //__set_bit(INPUT_PROP_BUTTONPAD, tpdev->input_dev->propbit);
    __set_bit(INPUT_PROP_DIRECT, tpdev->input_dev->propbit);

    __set_bit(ABS_MT_POSITION_X, tpdev->input_dev->absbit);
    __set_bit(ABS_MT_POSITION_Y, tpdev->input_dev->absbit);
       
    __set_bit(BTN_TOUCH, tpdev->input_dev->keybit);
       //zgx end


       /* Single touch */
   input_set_abs_params(tpdev->input_dev, ABS_X, 0, tpdev->pdata->lcd_x, 5, 0);
   input_set_abs_params(tpdev->input_dev, ABS_Y, 0, tpdev->pdata->lcd_y, 5, 0);

   /* Multitouch */
   #ifdef ABS_MT_TRACKING_ID
    input_set_abs_params(tpdev->input_dev, ABS_MT_TRACKING_ID, 0, MXT_MAX_NUM_TOUCHES,
			     0, 0);
   #endif
	input_set_abs_params(tpdev->input_dev, ABS_MT_POSITION_X, 0, tpdev->pdata->lcd_x, 0, 0);
	input_set_abs_params(tpdev->input_dev, ABS_MT_POSITION_Y, 0, tpdev->pdata->lcd_y, 0, 0);
	
	//input_set_abs_params(tpdev->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xF, 0, 0);
	if (tpdev->client->irq)
	{
		//gpio_request(tpdev->pdata->irq, tpdev->client->name);
		gpio_tlmm_config(GPIO_CFG(tpdev->pdata->irq, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	} 
	if (request_irq(tpdev->client->irq, tp_irq_handler,IRQF_TRIGGER_FALLING, tpdev->client->name, tpdev) >= 0)
	{
		printk("Received IRQ!\n");
		/* Delete by su.wenguang@byd.com at 2012/12/11.
		if (irq_set_irq_wake(tpdev->client->irq, 1) < 0)
			printk(KERN_ERR "failed to set IRQ wake\n");
		*/
	} 
	else 
	{
		printk("Failed to request IRQ!\n");
	}
	ret = input_register_device(tpdev->input_dev);
	if (ret) 
	{
		printk(KERN_ERR "tp_probe: Unable to register %s \
			input device\n", tpdev->input_dev->name);
		goto err_input_register_device_failed;
	} 
	else 
	{
		printk("tp input device registered\n");
	}
	return 0;
err_input_register_device_failed:
	input_free_device(tpdev->input_dev);
err_alloc_dev_failed:	
	return ret;
}

void tp_work_func_unregister(void)
{
	input_unregister_device(tpdev->input_dev);
	if (tp_wq)
		destroy_workqueue(tp_wq);
}
