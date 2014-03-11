
#include <linux/gpio.h>
#include <linux/delay.h>
#include <media/v4l2-subdev.h>
#include <linux/ioctl.h>
#include <linux/proc_fs.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/glanceview.h>
#include <mach/rpc_pmapp.h>

static struct  glanceview_ops * glanceview_devs[GV_DEVICE_ID_COUNT]={NULL};
static struct  gv_state gvstate_data;
static int gv_oneled_state;

void glanceview_oneled_state_ctrl(bool flag)
{
	int ret = 0;
	if(flag){
		ret = pmapp_disp_backlight_set_brightness(0);
		if (ret)
			printk(KERN_ERR"[GLANCEVIEW]%s()can't set backlight level=0\n", __func__);
		gpio_direction_output(121, 1);
		gv_oneled_state = 1;
		printk("[GLANCEVIEW]turn on onled gv_oneled_state = %d, then trun off Backlight!\n", gv_oneled_state);
	}else{
		gpio_direction_output(121, 0);
		gv_oneled_state = 0;
		printk("[GLANCEVIEW]turn off onled gv_oneled_state = %d\n", gv_oneled_state);
	}
}

int glanceview_register(enum  glanceview_index index,
    struct glanceview_ops *new_gv_dev,struct device * data)
{
	int ret = 0;
	if (index < gv_device_id_lcd || index >= gv_device_id_count)
		return -EINVAL;

	glanceview_devs[index] = new_gv_dev;
    glanceview_devs[index]->data=data;
    glanceview_devs[index]->state=0;
	return ret;
}
EXPORT_SYMBOL(glanceview_register);

enum  gvop_val glanceview_filter(enum  glanceview_index index,enum glanceview_op op)
{   
    enum  gvop_val ret=gv_bypass;
    static int old_state=0;
    static enum glanceview_op old_op=gv_enable;
    
    if(glanceview_devs[index]==NULL)
    {
	    return ret;
    }
    
    switch (index)
    {
        case gv_device_id_lcd:
        if(glanceview_devs[index]->state==1)
        {
            if(old_state==0 && old_op==gv_disable)
            {
                printk("sdl attempt enter in glanceview mode from lcd off\n");
                old_state=glanceview_devs[index]->state;
                old_op=op;
                glanceview_devs[index]->state=0;
                break;
            }
            //if(0==glanceview_devs[index]->gv_glance_mode(glanceview_devs[index]->data,op))
            {
                ret=gv_filtered;
            }
		 }	
         old_state=glanceview_devs[index]->state;
		 old_op=op;
        break;
        case gv_device_id_gsensor:
        if(glanceview_devs[index]->state==1)
        {
            if(0==glanceview_devs[index]->gv_glance_mode(glanceview_devs[index]->data,op))
            ret=gv_filtered;
        }
        break;
        default:
        break;
    }
	return ret;
}
EXPORT_SYMBOL(glanceview_filter);

bool glanceview_suspend_statues(void)
{
	bool retn = 0;
	if(glanceview_filter(gv_device_id_lcd,gv_disable)==gv_bypass)
	{
	
		retn = 0;
	}
	else
	{
		retn =1;
	}
	printk(KERN_DEBUG"glanceview_suspend_statues() %d\n",retn);
	return retn;
}

bool glanceview_resume_statues(void)
{
	bool retn = 0;
	if(glanceview_filter(gv_device_id_lcd,gv_enable)==gv_bypass)
	{
	
		retn = 0;
	}
	else
	{
		retn =1;
	}
	printk(KERN_DEBUG"glanceview_resume_statues() %d\n", retn);
	return retn;
}

	

static int glanceview_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int glanceview_close(struct inode *inode, struct file *filp)
{
    return 0;
}

extern bool suspend_get_state(void);

static ssize_t glanceview_write(struct file *file,
				      const char __user *user_buf,size_t count, loff_t *ppos)
{
	char messages[8]={0};
    unsigned int data=0;
#if 0	
	int i = 0;
	while(1)
	{
		if (suspend_get_state())
		{
			printk("glanceview_write() %d\n",i);
			break;
		}
		msleep(2);
		i ++;
		if (i > 500)
		{
			printk("glanceview_write() %d\n",i);
			return -EFAULT;
		}
	}
#endif	
	if (copy_from_user(messages, user_buf, 2))
		return -EFAULT;
    sscanf(messages,"%2x\n",&data);   
    memcpy(&gvstate_data, &data,sizeof(gvstate_data));
    if(gvstate_data.gv_lcd_setflag==1)
    {
        if(glanceview_devs[gv_device_id_lcd]!=NULL)
        {
            glanceview_devs[gv_device_id_lcd]->state=gvstate_data.gv_lcd_curmod;
            printk("sdl %s gv_lcd_curmod=%d\n",__func__,glanceview_devs[gv_device_id_lcd]->state);
        }
    }

    
    if(gvstate_data.gv_2taps_setflag==1)
    {
        if(glanceview_devs[gv_device_id_gsensor]!=NULL)    
        {
            glanceview_devs[gv_device_id_gsensor]->state=gvstate_data.gv_2taps_curmod;
            printk("sdl %s gv_2taps_curmod=%d\n",__func__,glanceview_devs[gv_device_id_gsensor]->state);
        }
        
    }
    return count;
}
static ssize_t glanceview_read(struct file *file, char __user *user_buf,
				     size_t count, loff_t *ppos)
{
    int len=0;
    char  buf[10]={0};
    int data=0;
    memcpy(&data,&gvstate_data,sizeof(data));
	sprintf(buf, "%x \n",data);
	len=strlen(buf);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}
static const struct file_operations glanceview_fops = {
	.owner = THIS_MODULE,
	.read = glanceview_read,
    .write = glanceview_write,
	.open = glanceview_open,	/* open */
	.release = glanceview_close,	/* release */
};

static struct miscdevice glanceview_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "glanceview",
    .fops  = &glanceview_fops,
};

static int oneled_open(struct inode *inode, struct file *filp)
{
    return 0;
}

static int oneled_close(struct inode *inode, struct file *filp)
{
    return 0;
}

static ssize_t oneled_write(struct file *file,
				      const char __user *user_buf,size_t count, loff_t *ppos)
{
	char messages[8]={0};
	unsigned int data=0;
	if (copy_from_user(messages, user_buf, 1))
		return -EFAULT;
    sscanf(messages,"%d\n",&data);
    memcpy(&gv_oneled_state, &data,sizeof(gv_oneled_state));

	if(gv_oneled_state == 1){
		glanceview_oneled_state_ctrl(1);
	}else if(gv_oneled_state == 0){
		glanceview_oneled_state_ctrl(0);
	}   
    return count;
}
static ssize_t oneled_read(struct file *file, char __user *user_buf,
				     size_t count, loff_t *ppos)
{
    int len=0;
    char  buf[10]={0};
    int data=0;
    memcpy(&data,&gv_oneled_state,sizeof(data));
	sprintf(buf, "%x \n",data);
	len=strlen(buf);
	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static const struct file_operations oneled_fops = {
	.owner = THIS_MODULE,
	.read = oneled_read,
       .write = oneled_write,
	.open = oneled_open,
	.release = oneled_close,
};

static struct miscdevice oneled_miscdev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "gv_oneled",
    .fops  = &oneled_fops,
};
static int glanceview_probe(struct platform_device *pdev)
{
    int result = 0;
    memset((void *)&gvstate_data,0,sizeof(gvstate_data));
    printk("sdl --- glanceview_probe\n");
    result = misc_register(&glanceview_miscdev);
    result = misc_register(&oneled_miscdev);
    if (result) {
    	return result;
    }
    return 0;
}

static int glanceview_remove(struct platform_device *pdev)
{
	misc_deregister(&glanceview_miscdev);

	return 0;
}

static struct platform_driver glanceview_driver = {
    .driver = {
        .name	= "glanceview",
    },
    .probe		= glanceview_probe,
    .remove		= glanceview_remove,
};


static int __init glanceview_init(void)
{

	return platform_driver_register(&glanceview_driver);
}

static void __exit glanceview_exit(void)
{
	platform_driver_unregister(&glanceview_driver);
}

module_init(glanceview_init);
module_exit(glanceview_exit);

MODULE_LICENSE("GPL");

