/* 
 * drivers/input/touchscreen/ft6306_touch.c
 *
 * FocalTech ft6306 TouchScreen driver. 
 *
 * Copyright (c) 2010  Focal tech Ltd.
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
 *	VERSION		DATE			AUTHOR			NOTE
 *	1.0		2010-01-05		WenFS		only support mulititouch
  *	2.0		2012-03-22		David.Wang	add factory mode 
 * 
 */


#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/ft6306_touch.h>
#include <linux/earlysuspend.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/gpio.h>

//#include <mach/irqs.h>
#include <linux/kernel.h>
#include <linux/semaphore.h>
#include <linux/mutex.h>
#include <linux/syscalls.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>

#include <linux/firmware.h>
#include <linux/platform_device.h>

#include <linux/input/mt.h>
#include <linux/module.h>
#include <linux/regulator/consumer.h>
#include <mach/vreg.h>

#define __FT6306_FT__
static struct i2c_client *this_client;
static struct ft6306_touch_platform_data *ft6306_touch_pdata;
unsigned char fts_ctpm_get_upg_ver(void);
static u8 ft6306_tp_type_id; //add for save tp id by liukai

#define CONFIG_FT6306_MULTITOUCH  1
#define    FTS_PACKET_LENGTH        128
#define CFG_SUPPORT_TOUCH_KEY  0
#define  N_patch  0

#define CFG_SUPPORT_AUTO_UPG 1

#if CFG_SUPPORT_AUTO_UPG
static unsigned char CTPM_FW_0X51[]=
{
	//#include "BYD_LAVA_ver0x12_app.i"
};

static unsigned char CTPM_FW_0X53[]=
{
""
	//#include "BYD_LAVA_ver0x12_app.i"
};
static unsigned char CTPM_FW_0X79[]=
{
	""
	//#include "BYD_LAVA_ver0x12_app.i"
};

#endif

#define DBUG(x) //x
#define DBUG2(x) //x
#define DBUG3(x) //x //about reset suspend resume
#define DBUG4(x) //x //about update fw

struct ts_event {
    u16 au16_x[MAX_TOUCH_POINTS];              //x coordinate
    u16 au16_y[MAX_TOUCH_POINTS];              //y coordinate
    u8  au8_touch_event[MAX_TOUCH_POINTS];     //touch event:  0 -- down; 1-- contact; 2 -- contact
    u8  au8_finger_id[MAX_TOUCH_POINTS];       //touch ID
	u16	pressure;
    u8  touch_point;
};

struct ft6306_touch_data {
	struct input_dev	*input_dev;
	struct ts_event		event;
	struct work_struct 	pen_event_work;
	struct workqueue_struct *ts_workqueue;
	struct early_suspend	early_suspend;
	struct mutex 		device_mode_mutex;   /* Ensures that only one function can specify the Device Mode at a time. */
};
//register address
#define FT6306_REG_FW_VER 0xA6

static void ft6306_chip_reset(void)
{

	/* FT5316 wants 40mSec minimum after reset to get organized */
	gpio_set_value(ft6306_touch_pdata->reset, 1);
	msleep(10);
	gpio_set_value(ft6306_touch_pdata->reset, 0);
	msleep(10);
	gpio_set_value(ft6306_touch_pdata->reset, 1);
	msleep(10);

}
/*
*ft6x06_i2c_Read-read data and write data by i2c
*@client: handle of i2c
*@writebuf: Data that will be written to the slave
*@writelen: How many bytes to write
*@readbuf: Where to store data read from slave
*@readlen: How many bytes to read
*
*Returns negative errno, else the number of messages executed
*
*
*/
int ft6306_i2c_Read(struct i2c_client *client, char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&client->dev, "f%s: i2c read error.\n",
				__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}


static int ft6306_i2c_rxdata(char *rxdata, int length)
{
	int ret;

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= rxdata,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= length,
			.buf	= rxdata,
		},
	};
   //  pr_err("touch msg %s i2c read error: %d\n", __func__, this_client->addr);
	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("touch msg %s i2c read error: %d\n", __func__, ret);
	
	return ret;
}

static int ft6306_i2c_txdata(char *txdata, int length)
{
	int ret;

	struct i2c_msg msg[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= length,
			.buf	= txdata,
		},
	};

	//msleep(1);
	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		pr_err("%s i2c write error: %d\n", __func__, ret);

	return ret;
}

static int ft6306_write_reg(u8 addr, u8 para)
{
	u8 buf[3];
	int ret = -1;

	buf[0] = addr;
	buf[1] = para;
	ret = ft6306_i2c_txdata(buf, 2);
	if (ret < 0) {
		pr_err("write reg failed! %#x ret: %d", buf[0], ret);
		return -1;
	}
    
	return 0;
}

static int ft6306_read_reg(u8 addr, u8 *pdata)
{
	int ret;
	u8 buf[2] = {0};

	struct i2c_msg msgs[] = {
		{
			.addr	= this_client->addr,
			.flags	= 0,
			.len	= 1,
			.buf	= buf,
		},
		{
			.addr	= this_client->addr,
			.flags	= I2C_M_RD,
			.len	= 1,
			.buf	= buf,
		},
	};
	buf[0] = addr;

	msleep(50);
	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	*pdata = buf[0];
	return ret;
  
}
/*get the tp's id*/
static void ft6306_tp_id(void)
{
	if(ft6306_read_reg(0xa8, &ft6306_tp_type_id) < 0)
	{
		pr_err("%s ERROR: could not read register\n", __FUNCTION__);
	}
	else 
	{
		if (ft6306_tp_type_id == 0xA8)
		{
			printk("[%s]: Read TP MODULE ID error!\n", __func__);
		}
		else
		{
			printk("[%s]: TP MODULE ID is 0x%02X !\n", __func__, ft6306_tp_type_id);
		}
	}
}

/*read the version of firmware*/
static unsigned char ft6306_read_fw_ver(void)
{
	unsigned char ver;
	ft6306_read_reg(FT6306_REG_FIRMID, &ver);
	return(ver);
}

typedef enum
{
	ERR_OK,
	ERR_MODE,
	ERR_READID,
	ERR_ERASE,
	ERR_STATUS,
	ERR_ECC,
	ERR_DL_ERASE_FAIL,
	ERR_DL_PROGRAM_FAIL,
	ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

typedef unsigned char         FTS_BYTE;     //8 bit
typedef unsigned short        FTS_WORD;    //16 bit
typedef unsigned int          FTS_DWRD;    //16 bit
typedef unsigned char         FTS_BOOL;    //8 bit

#define FTS_NULL			0x0
#define FTS_TRUE			0x01
#define FTS_FALSE			0x0

#define I2C_CTPM_ADDRESS	0xFF //no reference!
#if N_patch
//Switch to option menu definitions
#define STO_OPT_KEY_Y 900
#define STO_BACK_KEY_SAMPLING 30
#define STO_TOUCH_ON_BOTTOM_SENT 0xff
#define STO_TOUCH_RELEASE_TIMEOUT 200
#define STO_REPLAY_EVENT_ON_BOTTOM 0x00
typedef enum
{
    STO_EVENT_ON_BOTTOM,
    STO_EVENT_BACK_KEY
} E_STO_REPLAY_EVENT;
static u8 sto_back_key_sampling_counter;
static struct ts_event sto_saved_event;
static struct timer_list sto_timer;
static void ft6306_replay_sto_event(E_STO_REPLAY_EVENT event);
static void ft6306_sto_timer_callback (unsigned long data);
static FTS_BOOL ft6306_handle_swipe_to_option(
        const struct ft6306_touch_data *data, const struct ts_event *event);
#endif
void delay_qt_ms(unsigned long  w_ms)
{
	unsigned long i;
	unsigned long j;

	for (i = 0; i < w_ms; i++)
	{
		for (j = 0; j < 1000; j++)
		{
			udelay(1);
		}
	}
}


/*
[function]: 
    callback: read data from ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[out]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
FTS_BOOL i2c_read_interface(FTS_BYTE bt_ctpm_addr, FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
	int ret;
    
	ret=i2c_master_recv(this_client, pbt_buf, dw_lenth);

	if(ret<=0)
	{
		DBUG(printk("[TSP]i2c_read_interface error\n");)
		return FTS_FALSE;
	}
  
	return FTS_TRUE;
}

/*
[function]: 
    callback: write data to ctpm by i2c interface,implemented by special user;
[parameters]:
    bt_ctpm_addr[in]    :the address of the ctpm;
    pbt_buf[in]        :data buffer;
    dw_lenth[in]        :the length of the data buffer;
[return]:
    FTS_TRUE     :success;
    FTS_FALSE    :fail;
*/
FTS_BOOL i2c_write_interface(FTS_BYTE bt_ctpm_addr, FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
	int ret;
	ret=i2c_master_send(this_client, pbt_buf, dw_lenth);
	if(ret<=0)
	{
		DBUG(printk("[TSP]i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);)
		return FTS_FALSE;
	}

	return FTS_TRUE;
}

/*
[function]: 
    send a command to ctpm.
[parameters]:
    btcmd[in]        :command code;
    btPara1[in]    :parameter 1;    
    btPara2[in]    :parameter 2;    
    btPara3[in]    :parameter 3;    
    num[in]        :the valid input parameter numbers, if only command code needed and no parameters followed,then the num is 1;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL cmd_write(FTS_BYTE btcmd,FTS_BYTE btPara1,FTS_BYTE btPara2,FTS_BYTE btPara3,FTS_BYTE num)
{
	FTS_BYTE write_cmd[4] = {0};

	write_cmd[0] = btcmd;
	write_cmd[1] = btPara1;
	write_cmd[2] = btPara2;
	write_cmd[3] = btPara3;
	return i2c_write_interface(I2C_CTPM_ADDRESS, write_cmd, num);
}

/*
[function]: 
    write data to ctpm , the destination address is 0.
[parameters]:
    pbt_buf[in]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_write(FTS_BYTE* pbt_buf, FTS_DWRD dw_len)
{
	return i2c_write_interface(I2C_CTPM_ADDRESS, pbt_buf, dw_len);
}

/*
[function]: 
    read out data from ctpm,the destination address is 0.
[parameters]:
    pbt_buf[out]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_read(FTS_BYTE* pbt_buf, FTS_BYTE bt_len)
{
	return i2c_read_interface(I2C_CTPM_ADDRESS, pbt_buf, bt_len);
}


/*
[function]: 
    burn the FW to ctpm.
[parameters]:(ref. SPEC)
    pbt_buf[in]    :point to Head+FW ;
    dw_lenth[in]:the length of the FW + 6(the Head length);    
    bt_ecc[in]    :the ECC of the FW
[return]:
    ERR_OK        :no error;
    ERR_MODE    :fail to switch to UPDATE mode;
    ERR_READID    :read id fail;
    ERR_ERASE    :erase chip fail;
    ERR_STATUS    :status error;
    ERR_ECC        :ecc error.
*/
/* update firmware partition*/

//#define    FTS_PACKET_LENGTH        128

/*firmware update interface*/
E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    FTS_BYTE reg_val[2] = {0};
    FTS_DWRD i = 0;

    FTS_DWRD  packet_number;
    FTS_DWRD  j;
    FTS_DWRD  temp;
    FTS_DWRD  lenght;
    FTS_BYTE  packet_buf[FTS_PACKET_LENGTH + 6];
    FTS_BYTE  auc_i2c_write_buf[10];
    FTS_BYTE bt_ecc;
    int      i_ret;
    #define FTS_UPGRADE_LOOP	3

    /*********Step 1:Reset  CTPM *****/
    /*write 0xaa to register 0xfc*/
    for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
    ft6306_write_reg(0xbc,0xaa);
    msleep(50);
     /*write 0x55 to register 0xfc*/
    ft6306_write_reg(0xbc,0x55);
    printk("[TSP] Step 1: Reset CTPM test, bin-length=%d\n",dw_lenth);
   
    msleep(30);  


    /*********Step 2:Enter upgrade mode *****/
    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do
    {
        i ++;
        i_ret = ft6306_i2c_txdata(auc_i2c_write_buf, 2);
        msleep(5);
    }while(i_ret <= 0 && i < 5 );

    /*********Step 3:check READ-ID***********************/        
    cmd_write(0x90,0x00,0x00,0x00,4);
    byte_read(reg_val,2);
    if (reg_val[0] == 0x79 && reg_val[1] == 0x8)
    {
        printk("[TSP] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
	break;
    }
    else
    {
    	printk("%s: ERR_READID, ID1 = 0x%x,ID2 = 0x%x\n", __func__,reg_val[0],reg_val[1]);
        //return ERR_READID;
        //i_is_new_protocol = 1;
    }
    }

    if (i > FTS_UPGRADE_LOOP)
	return -EIO;

     /*********Step 4:erase app*******************************/
    cmd_write(0x61,0x00,0x00,0x00,1);
   
    msleep(1500);
    cmd_write(0x63,0x00,0x00,0x00,1);
    msleep(100);

    printk("[TSP] Step 4: erase.\n");

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    printk("[TSP] Step 5: start upgrade.\n");
    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(lenght>>8);
        packet_buf[5] = (FTS_BYTE)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++)
        {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }
        
        byte_write(&packet_buf[0],FTS_PACKET_LENGTH + 6);
        msleep(FTS_PACKET_LENGTH/6 + 1);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0)
        {
              printk("[TSP] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;

        for (i=0;i<temp;i++)
        {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }

        byte_write(&packet_buf[0],temp+6);    
        msleep(20);
    }

    //send the last six byte
    for (i = 0; i<6; i++)
    {
        temp = 0x6ffa + i;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        temp =1;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i]; 
        bt_ecc ^= packet_buf[6];

        byte_write(&packet_buf[0],7);  
        msleep(20);
    }

    /*********Step 6: read out checksum***********************/
    /*send the opration head*/
    cmd_write(0xcc,0x00,0x00,0x00,1);
    byte_read(reg_val,1);
    printk("[TSP] Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc)
    {
    	printk("%s: ERR_ECC\n", __func__);
        return ERR_ECC;
    }

    /*********Step 7: reset the new FW***********************/
    cmd_write(0x07,0x00,0x00,0x00,1);
    msleep(300);

    return ERR_OK;
}

/*  touch panel calibration  */
int fts_ctpm_auto_clb(void)
{
	unsigned char uc_temp;
	unsigned char i ;

	printk("[FTS] start auto CLB.\n");
	msleep(200);
	ft6306_write_reg(0, 0x40);  
	delay_qt_ms(100);   //make sure already enter factory mode
	ft6306_write_reg(2, 0x4);  //write command to start calibration
	delay_qt_ms(300);
	for(i=0;i<100;i++)
	{
		ft6306_read_reg(0, &uc_temp);
		if ( ((uc_temp&0x70)>>4) == 0x0)  //return to normal mode, calibration finish
		{
			break;
		}
		delay_qt_ms(200);
		printk("[FTS] waiting calibration %d\n",i);
        
	}
	printk("[FTS] calibration OK.\n");
    
	msleep(300);
	ft6306_write_reg(0, 0x40);  //goto factory mode
	delay_qt_ms(100);   //make sure already enter factory mode
	ft6306_write_reg(2, 0x5);  //store CLB result
	delay_qt_ms(300);
	ft6306_write_reg(0, 0x0); //return to normal mode 
	msleep(300);
	printk("[FTS] store CLB result OK.\n");
	return 0;
}


/***********************************************
	read the version of current firmware
***********************************************/
static ssize_t ft6306_tpfwver_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ft6306_touch_data *data = NULL;
	u8	   fwver = 0;
	ssize_t num_read_chars = 0;

	DBUG4(printk("[%s]: Enter!\n", __func__);)
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	data = (struct ft6306_touch_data *) i2c_get_clientdata( client );
	
	mutex_lock(&data->device_mode_mutex);
	if(ft6306_read_reg(FT6306_REG_FW_VER, &fwver) < 0)
		num_read_chars = snprintf(buf, PAGE_SIZE, "get tp fw version fail!\n");
	else
		num_read_chars = snprintf(buf, PAGE_SIZE, "0x%02X\n", fwver);
	mutex_unlock(&data->device_mode_mutex);
	return num_read_chars;
}

static ssize_t ft6306_tpfwver_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	DBUG(printk("[%s]: Enter!\n", __func__);)
	/* place holder for future use */
	return -EPERM;
}
/***********************************
  set or read the report rate 
************************************/
static ssize_t ft6306_tprwreg_show(struct device *dev,	struct device_attribute *attr, char *buf)
{
	DBUG(printk("[%s]: Enter!\n", __func__);)
	/* place holder for future use */
	return -EPERM;
}

static ssize_t ft6306_tprwreg_store(struct device *dev,	struct device_attribute *attr, const char *buf, size_t count)
{
	struct ft6306_touch_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	//int retval;
	u16 wmreg=0;
	u8 regaddr=0x88, regvalue=0xff;
	u8 valbuf[5];

	data = (struct ft6306_touch_data *) i2c_get_clientdata( client );
	
	DBUG(printk("[%s]: Enter!\n", __func__);)
	
	memset(valbuf, 0, sizeof(valbuf));
	mutex_lock(&data->device_mode_mutex);
	
	num_read_chars = count - 1;
	if(num_read_chars!=2)
	{
		if(num_read_chars!=4)
		{
			pr_info("please input 2 or 4 character\n");
			goto error_return;
		}
	}
	
	memcpy(valbuf, buf, num_read_chars);
	#if 0
	retval = strict_strtoul(valbuf, 16, &wmreg);
	if (0 != retval)
    	{
        	pr_err("%s() - ERROR: Could not convert the given input to a number. The given input was: \"%s\"\n", __FUNCTION__, buf);
        	goto error_return;
    	}
	#endif
	DBUG4(printk("[%s]:valbuf=%s wmreg=%x\n", __func__, valbuf, wmreg);)
	
	if(2 == num_read_chars)
	{
		/*read the register at regaddr, report rate*/
		regaddr = wmreg;
		if(ft6306_read_reg(regaddr, &regvalue) < 0)
			pr_err("Could not read the register(0x%02x)\n", regaddr);
		else
			pr_info("the register(0x%02x) is 0x%02x\n", regaddr, regvalue);
	}
	else
	{
		regaddr = wmreg>>8;
		regvalue = wmreg;
		if(ft6306_write_reg(regaddr, regvalue) < 0)
			pr_err("Could not write the register(0x%02x)\n", regaddr);
		else
			pr_err("Write 0x%02x into register(0x%02x) successful\n", regvalue, regaddr);
	}
error_return:
	mutex_unlock(&data->device_mode_mutex);

	return count;
}

/*get the size of firmware to update */
static int ft6306_GetFirmwareSize(char * firmware_name)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize = 0; 
	char filepath[128];
	memset(filepath, 0, sizeof(filepath));
	DBUG(printk("[%s]: Enter!\n", __func__);)
	
	sprintf(filepath, "%s", firmware_name);///mnt/sdcard
	
	pr_info("filepath=%s\n", filepath);
	if(NULL == pfile){
		pfile = filp_open(filepath, O_RDONLY, 0);
	}
	if(IS_ERR(pfile)){
		pr_err("error occured while opening file %s.\n", filepath);
		return -1;
	}
	inode=pfile->f_dentry->d_inode; 
	magic=inode->i_sb->s_magic;
	fsize=inode->i_size; 
	filp_close(pfile, NULL);
	DBUG4(printk("[%s]: fsize = %d!\n", __func__, fsize);)
	return fsize;
}
/*read the new firmware*/
static int ft6306_ReadFirmware(char * firmware_name, unsigned char * firmware_buf)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize; 
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	
	DBUG(printk("[%s]: Enter!\n", __func__);)
	
	sprintf(filepath, "%s", firmware_name);///mnt/sdcard/
	//pr_info("filepath=%s\n", filepath);
	if(NULL == pfile){
		pfile = filp_open(filepath, O_RDONLY, 0);
	}
	if(IS_ERR(pfile)){
		pr_err("error occured while opening file %s.\n", filepath);
		printk("[%s]: Can not open the app!\n", __func__);
		return -1;
	}
	inode=pfile->f_dentry->d_inode; 
	magic=inode->i_sb->s_magic;
	fsize=inode->i_size; 
	DBUG4(printk("[%s]: fsize = %d!\n", __func__, fsize);)
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;

	vfs_read(pfile, firmware_buf, fsize, &pos);

	filp_close(pfile, NULL);
	set_fs(old_fs);
	return 0;
}

/*the interface to update firmware with app.bin*/
int fts_ctpm_fw_upgrade_with_app_file(char * firmware_name)
{
	FTS_BYTE*     pbt_buf = FTS_NULL;
	int i_ret; 
	u8 fwver,file_id;
	u8 buf[128];
	u8 reg_val[2] = {0};
	u8 auc_i2c_write_buf[10] = {0};
	u32 i = 0;

	DBUG(printk("[%s]: Enter!\n", __func__);)
	int fwsize = ft6306_GetFirmwareSize(firmware_name);
	if(fwsize <= 0)
	{
		pr_err("%s ERROR:Get firmware size failed\n", __FUNCTION__);
		return -1;
	}
	//=========FW upgrade========================*/
	pbt_buf = (unsigned char *) kmalloc(fwsize+1,GFP_ATOMIC);
	if(ft6306_ReadFirmware(firmware_name, pbt_buf))
	{
		pr_err("%s() - ERROR: request_firmware failed\n", __FUNCTION__);
		kfree(pbt_buf);
		return -1;
	}

          file_id=pbt_buf[fwsize-1];
          printk("[TSP] firmware_file_id=0x%x\n",file_id);
     
         /*********Step 1:Reset  CTPM *****/
         /*write 0xaa to register 0xfc*/
         for (i = 0; i < FTS_UPGRADE_LOOP; i++) {
         ft6306_write_reg(0xbc,0xaa);
         msleep(50);
          /*write 0x55 to register 0xfc*/
         ft6306_write_reg(0xbc,0x55);
         printk("[TSP] Step 1: Reset CTPM test, bin-length=%d\n",fwsize);
        
         msleep(30);  
     
         /*********Step 2:Enter upgrade mode *****/
         auc_i2c_write_buf[0] = 0x55;
         auc_i2c_write_buf[1] = 0xaa;
         do
         {
             i ++;
             i_ret = ft6306_i2c_txdata(auc_i2c_write_buf, 2);
             msleep(5);
         }while(i_ret <= 0 && i < 5 );
     
         /*********Step 3:check READ-ID***********************/        
         cmd_write(0x90,0x00,0x00,0x00,4);
         byte_read(reg_val,2);
         if (reg_val[0] == 0x79 && reg_val[1] == 0x8)
         {
             printk("[TSP] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
     	break;
         }
         else
         {
         	printk("%s: ERR_READID, ID1 = 0x%x,ID2 = 0x%x\n", __func__,reg_val[0],reg_val[1]);
         }
         }
     
         if (i > FTS_UPGRADE_LOOP)
     	return -EIO;
    /*********Step 4:read panel id in flash***********************/        
	/*set read start address */

    	buf[0] = 0x3;
    	buf[1] = 0x0;
    	buf[2] = 0x78;
    	buf[3] = 0x0;

        ft6306_i2c_Read(this_client, buf, 4, buf, 128);
        printk("[FTS]panel_factory_id = 0x%x\n",buf[4]);
	if(buf[4]==file_id)	{
		
        printk("[TSP] panel_factory_id match with firmware_fie_id\n");
   	/*call the upgrade function*/
   	i_ret =  fts_ctpm_fw_upgrade(pbt_buf, fwsize);
   	if (i_ret != 0)
   	{
		pr_err("%s() - ERROR:[FTS] upgrade failed i_ret = %d.\n",__FUNCTION__,  i_ret);
		//error handling ...
		//TBD
   	}
	else
   	{
		pr_info("[FTS] upgrade successfully.\n");
		if(ft6306_read_reg(FT6306_REG_FW_VER, &fwver) >= 0)
			pr_info("the new fw ver is 0x%02x\n", fwver);
		fts_ctpm_auto_clb();  //start auto CLB
	}
		}
	
	else{
	/********* reset the new FW***********************/
	 printk("[TSP] panel_factory_id and firmware_fie_id do not match\n");
         cmd_write(0x07,0x00,0x00,0x00,1);
         msleep(300);
		}
	kfree(pbt_buf);
	return i_ret;
}

static ssize_t ft6306_fwupgradeapp_show(struct device *dev,	struct device_attribute *attr, char *buf)
{
	DBUG(printk("[%s]: Enter!\n", __func__);)
	/* place holder for future use */
	return -EPERM;
}

static ssize_t ft6306_fwupgradeapp_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, 
					size_t count)
{
	struct ft6306_touch_data *data = NULL;
	DBUG(printk("[%s]: Enter!\n", __func__);)
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	char fwname[128];

	data = (struct ft6306_touch_data *) i2c_get_clientdata( client );
	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	DBUG4(printk("[%s]: fwname is %s\n", __func__, fwname);)
	fwname[count-1] = '\0';

	mutex_lock(&data->device_mode_mutex);

	disable_irq(client->irq);
	num_read_chars = fts_ctpm_fw_upgrade_with_app_file(fwname);
	if(num_read_chars < 0)
		goto error_return;

	enable_irq(client->irq);

error_return:
	mutex_unlock(&data->device_mode_mutex);

	return count;
}


#if CFG_SUPPORT_AUTO_UPG
static int ft6306_fwupgrade_with_ifile(void)
{
   FTS_BYTE*     pbt_buf = FTS_NULL;
   FTS_DWRD dw_lenth = 0;
   int i_ret;
    
	//=========FW upgrade========================*/
	if (ft6306_tp_type_id == 0x51)
	{
		pbt_buf = CTPM_FW_0X51;
		dw_lenth =sizeof(CTPM_FW_0X51);
	}
	else if (ft6306_tp_type_id == 0x53)
	{
		pbt_buf = CTPM_FW_0X53;
		dw_lenth =sizeof(CTPM_FW_0X51);
	}
	else if (ft6306_tp_type_id == 0x79)
	{
		pbt_buf = CTPM_FW_0X79;
		dw_lenth =sizeof(CTPM_FW_0X79);
	}
	else
	{
		printk(KERN_ERR"ft6306_fwupgrade_ifile(),not support this tp.\n");
		return 1;
	}
	
	/*call the upgrade function*/
	i_ret =  fts_ctpm_fw_upgrade(pbt_buf,sizeof(dw_lenth));
	if (i_ret != 0)
	{
		printk(KERN_ERR"ft6306_fwupgrade_ifile() = %d.\n", i_ret);
		//error handling ...
		//TBD
	}
	else
	{
		printk(KERN_ERR"ft6306_fwupgrade_ifile()upgrade successfully.\n");
		fts_ctpm_auto_clb();  //start auto CLB
	}

	return i_ret;
}

unsigned char ft6306_get_ifile_ver(void)
{
	FTS_BYTE*     pbt_buf = FTS_NULL;
	FTS_DWRD dw_lenth = 0;
	
	if (ft6306_tp_type_id == 0x51)
	{
		pbt_buf = CTPM_FW_0X51;
		dw_lenth =sizeof(CTPM_FW_0X51);
	}
	else if (ft6306_tp_type_id == 0x53)
	{
		pbt_buf = CTPM_FW_0X53;
		dw_lenth =sizeof(CTPM_FW_0X51);
	}
	else if (ft6306_tp_type_id == 0x79)
	{
		pbt_buf = CTPM_FW_0X79;
		dw_lenth =sizeof(CTPM_FW_0X79);
	}
	else
	{
		printk(KERN_ERR"ft6306_get_ifile_ver(),not support this tp.\n");
		return 0xff;
	}	
	
    if (dw_lenth > 2)
    {
        return pbt_buf[dw_lenth - 2];
    }
    else
    {
        //TBD, error handling?
        return 0xff; //default value
    }
	
}

int ft6306_fwupgrate_ifile(void)
{
    unsigned char old_ver;
    unsigned char new_ver;
    int           i_ret;

    old_ver = ft6306_read_fw_ver();
    new_ver = ft6306_get_ifile_ver();

	printk(KERN_ERR"ft6306_fwupgrate_ifile():old_ver = 0x%x, new_ver = 0x%x\n",
		old_ver, 
		new_ver);
	
	if (new_ver == 0xFF)
	{
		return 0;
	}
	
	//if (old_ver < new_ver)  
	{
		msleep(100);

		i_ret = ft6306_fwupgrade_with_ifile();   

		if (i_ret == 0)
		{
			msleep(300);
			new_ver = ft6306_read_fw_ver();
			printk(KERN_ERR"ft6306_fwupgrate_ifilenew_ver = 0x%x\n", new_ver);
			return 1;
		}
		else
		{
			printk(KERN_ERR"[FTS] upgrade failed ret=%d.\n", i_ret);
		}
	}

    return 0;
}

static ssize_t ft6306_fwupgradeifile_show(struct device *dev,	struct device_attribute *attr, char *buf)
{
	DBUG(printk("[%s]: Enter!\n", __func__);)
	int  irt = 0;
	struct ft6306_touch_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	data = (struct ft6306_touch_data *) i2c_get_clientdata( client );	

	disable_irq(client->irq);
	irt = ft6306_fwupgrate_ifile();
	enable_irq(client->irq);		
	return -EPERM;
}

static ssize_t ft6306_fwupgradeifile_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, 
					size_t count)
{
	int  irt = 0;
	struct ft6306_touch_data *data = NULL;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	data = (struct ft6306_touch_data *) i2c_get_clientdata( client );	

	disable_irq(client->irq);
	irt = ft6306_fwupgrate_ifile();
	enable_irq(client->irq);
	return count;		
}

#endif

/*************************
release touch panel	
**************************/
static void ft6306_touch_release(void)
{
	struct ft6306_touch_data *data = i2c_get_clientdata(this_client);
	/*Disable PRESSUE,modify by Daivd.Wang*/
	//input_report_abs(data->input_dev, ABS_MT_PRESSURE, 0);
	
	input_report_key(data->input_dev, BTN_TOUCH, 0); 
	input_mt_sync(data->input_dev);
	input_sync(data->input_dev);

}
/*read the data transfered by i2c*/
static int ft6306_read_data(void)
{
	struct ft6306_touch_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	u8 buf[CFG_POINT_READ_BUF] = {0};
	int ret = -1;
	int i;
	
	ret = ft6306_i2c_rxdata(buf, CFG_POINT_READ_BUF);
    if (ret < 0) {
		printk("%s read_data i2c_rxdata failed: %d\n", __func__, ret);
		return ret;
	}
	memset(event, 0, sizeof(struct ts_event));
	event->touch_point = buf[2] & 0x07; 

    if (event->touch_point > MAX_TOUCH_POINTS)
    {
        event->touch_point = MAX_TOUCH_POINTS;
    }

    for (i = 0; i < event->touch_point; i++)
    {
        event->au16_x[i] = (s16)(buf[3 + 6*i] & 0x0F)<<8 | (s16)buf[4 + 6*i];
        event->au16_y[i] = (s16)(buf[5 + 6*i] & 0x0F)<<8 | (s16)buf[6 + 6*i];
        event->au8_touch_event[i] = buf[0x3 + 6*i] >> 6;
        event->au8_finger_id[i] = (buf[5 + 6*i])>>4;
    }

    event->pressure = 200;

    return 0;
}
#if N_patch
static void ft6306_replay_sto_event(E_STO_REPLAY_EVENT event)
{
    int y;
    struct ft6306_touch_data *data = i2c_get_clientdata(this_client);

    DBUG(printk("ft6306_replay_sto_event event:%d\n", event););
    switch (event)
    {
        case STO_EVENT_ON_BOTTOM:
            y = SCREEN_MAX_Y;
            break;
        case STO_EVENT_BACK_KEY:
        default:
            y = STO_OPT_KEY_Y;
            break;
    }

    input_report_abs(data->input_dev,
            ABS_MT_POSITION_X,
            sto_saved_event.au16_x[0]);

    input_report_abs(data->input_dev,
            ABS_MT_POSITION_Y,
            y);

    input_report_abs(data->input_dev,
            ABS_MT_WIDTH_MAJOR,
            1);
    input_report_abs(data->input_dev,
            ABS_MT_TRACKING_ID,
            sto_saved_event.au8_finger_id[0]);
	/*Disable PRESSUE,modify by Daivd.Wang*/
	#if 0
    input_report_abs(data->input_dev,
            ABS_MT_PRESSURE,
            sto_saved_event.pressure);
	#endif
    input_report_key(data->input_dev, BTN_TOUCH, 1);

    input_mt_sync(data->input_dev);
    input_sync(data->input_dev);
}

static void ft6306_sto_timer_callback (unsigned long data)
{
    DBUG(printk("ft6306_sto_timer_callback\n"););
    sto_back_key_sampling_counter = 0;
    ft6306_replay_sto_event(STO_EVENT_BACK_KEY);
    ft6306_touch_release();
}

static FTS_BOOL ft6306_handle_swipe_to_option(
        const struct ft6306_touch_data *data, const struct ts_event *event)
{
    DBUG(printk("ft6306_handle_swipe_to_option touch_point:%d y:%d counter:%d event:%d\n",
                event->touch_point, event->au16_y[0],
                sto_back_key_sampling_counter, event->au8_touch_event[0]);)

    //Release
    if (event->touch_point == 0)
    {
        if (sto_back_key_sampling_counter &&
                sto_back_key_sampling_counter < STO_BACK_KEY_SAMPLING)
        {
            //HACK: When swipe from back key area into screen, a touch release (sometimes)
            //will be sent.
            mod_timer(&sto_timer, jiffies + msecs_to_jiffies(STO_TOUCH_RELEASE_TIMEOUT));
            return FTS_TRUE;
        }
        sto_back_key_sampling_counter = 0;
        return FTS_FALSE;
    }

    //Ignore multi-touch
    if (event->touch_point > 1)
    {
        sto_back_key_sampling_counter = 0;
        return FTS_FALSE;
    }

    //release
    if (event->au8_touch_event[0] == 1)
    {
        if (sto_back_key_sampling_counter &&
                sto_back_key_sampling_counter < STO_BACK_KEY_SAMPLING)
        {
            ft6306_replay_sto_event(STO_EVENT_BACK_KEY);
        }
        sto_back_key_sampling_counter = 0;
        return FTS_FALSE;
    }

    //Touch on backkey area
    if (event->au16_y[0] == STO_OPT_KEY_Y) {
        //Buffer it and skip.
        if (sto_back_key_sampling_counter < STO_BACK_KEY_SAMPLING)
        {
            memcpy(&sto_saved_event, event, sizeof(struct ts_event));
            sto_back_key_sampling_counter++;
            return FTS_TRUE;
        } else {
            return FTS_FALSE;
        }
    }

    //Below contions happens when touch on screen
    if (sto_back_key_sampling_counter == 0 ||
            sto_back_key_sampling_counter > STO_BACK_KEY_SAMPLING)
    {
        return FTS_FALSE;
    }

    //Need to replay touch on bottom
    if (sto_back_key_sampling_counter > 0)
    {
        ft6306_replay_sto_event(STO_EVENT_ON_BOTTOM);
        sto_back_key_sampling_counter = STO_TOUCH_ON_BOTTOM_SENT;
    }

    return FTS_FALSE;
}
#endif
static void ft6306_report_value(void)
{
	struct ft6306_touch_data *data = i2c_get_clientdata(this_client);
	struct ts_event *event = &data->event;
	int i;
#if N_patch
        if(ft6306_handle_swipe_to_option(data, event)) {
            return;
        }
#endif
	for (i  = 0; i < event->touch_point; i++)
	{
		if ((event->au16_x[i] < SCREEN_MAX_X)
			&& (event->au16_y[i] < SCREEN_MAX_Y +400))
		{
			/* LCD view area. */
			input_report_abs(data->input_dev,
				ABS_MT_POSITION_X,
				event->au16_x[i]);
			
			input_report_abs(data->input_dev,
				ABS_MT_POSITION_Y,
				event->au16_y[i]);
			
			input_report_abs(data->input_dev,
				ABS_MT_WIDTH_MAJOR,
				1);
			input_report_abs(data->input_dev,
				ABS_MT_TRACKING_ID,
				event->au8_finger_id[i]);
			/*Disable PRESSUE,modify by Daivd.Wang*/	
			#if 0	
			if ((event->au8_touch_event[i]== 0)
				|| (event->au8_touch_event[i] == 2))
			{
				input_report_abs(data->input_dev,
					ABS_MT_PRESSURE,
					event->pressure);
			}
			else
			{
				input_report_abs(data->input_dev,
					ABS_MT_PRESSURE,
					0);
			}
			#endif
			input_report_key(data->input_dev, BTN_TOUCH, 1); 
			DBUG(printk("id[%d] = %d, x[%d] = %d, y[%d] = %d \n", i, event->au8_finger_id[i], i, event->au16_x[i], i, event->au16_y[i]);)
		}
		else //maybe the touch key area
		{
#if CFG_SUPPORT_TOUCH_KEY
			if (event->au16_y[i] >= SCREEN_VIRTUAL_Y)
			{
				ft5x06_touch_key_process(data->input_dev,
					event->au16_x[i],
					event->au16_y[i],
					event->au8_touch_event[i]);
			}
#endif
		}	    
		input_mt_sync(data->input_dev);
	}
	
	input_sync(data->input_dev);

	if (event->touch_point == 0)
		ft6306_touch_release();
}

static void ft6306_touch_pen_irq_work(struct work_struct *work)
{
	//int ret = -1;
	
	if (ft6306_read_data()== 0)	
	{	
		ft6306_report_value();
	}
	else
	{
		printk("data package read error\n");
	}
 	
 	//enable_irq(this_client->irq);
}

static irqreturn_t ft6306_touch_interrupt(int irq, void *dev_id)
{
	struct ft6306_touch_data *ft6306_touch = dev_id;
	DBUG(printk("[%s]: Enter!\n",__func__);)
#if N_patch		
         if(timer_pending(&sto_timer))
         {
             del_timer( &sto_timer);
         }
#endif		 
	if (!work_pending(&ft6306_touch->pen_event_work)) {
		queue_work(ft6306_touch->ts_workqueue, &ft6306_touch->pen_event_work);
	}

	return IRQ_HANDLED;
}
#ifdef CONFIG_HAS_EARLYSUSPEND

static void ft6306_touch_suspend(struct early_suspend *handler)
{
	struct ft6306_touch_data *ts;
	struct ts_event *event;
	ts =  container_of(handler, struct ft6306_touch_data, early_suspend);
	event = &ts->event;
	printk("[%s]: Enter!\n", __func__);
	disable_irq(this_client->irq);
	cancel_work_sync(&ts->pen_event_work);
	flush_workqueue(ts->ts_workqueue);
	/*added by liukai if find the touch point when suspend release it*/
	if( event->touch_point != 0) 
	{
		printk("[%s]:Find the touchpoint when ft6306 suspend!\n", __func__);
		event->touch_point = 0;
		ft6306_touch_release();
	}
	/*add end*/
	/* ==switch to deep sleep mode ==*/ 
   	ft6306_write_reg(FT6306_REG_PMODE, PMODE_HIBERNATE);
	printk("[%s]: Exit!\n", __func__);
}

static void ft6306_touch_resume(struct early_suspend *handler)
{
	printk("[%s]: Enter!\n", __func__);
		/*reset touch*/
	ft6306_chip_reset();
	msleep(10);
	enable_irq(this_client->irq);
	printk("[%s]: exist!\n", __func__);
}
#endif  //CONFIG_HAS_EARLYSUSPEND

/* sysfs */
/*read the version of firmware*/
static DEVICE_ATTR(fwversion, S_IRUGO|S_IWUSR, ft6306_tpfwver_show, ft6306_tpfwver_store);
/*read and set the report rate*/
static DEVICE_ATTR(fwrate, S_IRUGO|S_IWUSR, ft6306_tprwreg_show, ft6306_tprwreg_store);
/*upgrade the tp firmware with app.bin*/ 
static DEVICE_ATTR(fwupdate, S_IRUGO|S_IWUSR|S_IWGRP, ft6306_fwupgradeapp_show, ft6306_fwupgradeapp_store);

#if CFG_SUPPORT_AUTO_UPG
static DEVICE_ATTR(fwupdateifile, S_IRUGO|S_IWUSR, ft6306_fwupgradeifile_show, ft6306_fwupgradeifile_store);
#endif
static struct attribute *ft6306_attributes[] = {
	&dev_attr_fwversion.attr,
	&dev_attr_fwrate.attr,
	&dev_attr_fwupdate.attr,
#if CFG_SUPPORT_AUTO_UPG	
	&dev_attr_fwupdateifile.attr,
#endif	
	NULL
};

static struct attribute_group ft6306_attribute_group = {
	.attrs = ft6306_attributes
};

#ifdef __FT6306_FT__


/*------------------------------------------------------------------------
*For FT6306 factory Test
-------------------------------------------------------------------------*/
	
//#define FTS_PACKET_LENGTH        128
#define FTS_SETTING_BUF_LEN        128
	
#define FTS_TX_MAX				40
#define FTS_RX_MAX				40
#define FTS_DEVICE_MODE_REG	0x00
#define FTS_TXNUM_REG			0x03
#define FTS_RXNUM_REG			0x04
#define FTS_RAW_READ_REG		0x01
#define FTS_RAW_BEGIN_REG		0x10
#define FTS_VOLTAGE_REG		0x05
	
#define FTS_FACTORYMODE_VALUE		0x40
#define FTS_WORKMODE_VALUE		0x00

/*open & short param*/
#define VERYSMALL_TX_RX	1
#define SMALL_TX_RX			2
#define NORMAL_TX_RX		0
#define RAWDATA_BEYOND_VALUE		10000
#define DIFFERDATA_ABS_OPEN		10
#define DIFFERDATA_ABS_ABNORMAL	100
#define RAWDATA_SMALL_VALUE		5500 /*cross short*/
static u16 g_min_rawdata = 7000;
static u16 g_max_rawdata = 9500;
static u16 g_min_diffdata = 50;
static u16 g_max_diffdata = 550;
static u8 g_voltage_level = 2;	/*default*/

int ft6306_ft_i2c_Read(char *writebuf,
		    int writelen, char *readbuf, int readlen)
{
	int ret;

	if (writelen > 0) {
		struct i2c_msg msgs[] = {
			{
			 .addr = this_client->addr,
			 .flags = 0,
			 .len = writelen,
			 .buf = writebuf,
			 },
			{
			 .addr = this_client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(this_client->adapter, msgs, 2);
		if (ret < 0)
			dev_err(&this_client->dev, "f%s: i2c read error.\n",__func__);
	} else {
		struct i2c_msg msgs[] = {
			{
			 .addr = this_client->addr,
			 .flags = I2C_M_RD,
			 .len = readlen,
			 .buf = readbuf,
			 },
		};
		ret = i2c_transfer(this_client->adapter, msgs, 1);
		if (ret < 0)
			dev_err(&this_client->dev, "%s:i2c read error.\n", __func__);
	}
	return ret;
}
/*write data by i2c*/
int ft6306_ft_i2c_Write(char *writebuf, int writelen)
{
	int ret;

	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = writelen,
		 .buf = writebuf,
		 },
	};

	ret = i2c_transfer(this_client->adapter, msg, 1);
	if (ret < 0)
		dev_err(&this_client->dev, "%s i2c write error.\n", __func__);

	return ret;
}

int ft6306_ft_write_reg(u8 regaddr, u8 regvalue)
{
	unsigned char buf[2] = {0};
	buf[0] = regaddr;
	buf[1] = regvalue;

	return ft6306_ft_i2c_Write(buf, sizeof(buf));
}


int ft6306_ft_read_reg(u8 regaddr, u8 *regvalue)
{
	return ft6306_ft_i2c_Read(&regaddr, 1, regvalue, 1);
}
static ssize_t ft6306_vendor_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u8 id_type = 0;
	u8 fwver = 0x0;
	/*fw version*/
	if(ft6306_read_reg(FT6306_REG_FW_VER, &fwver) < 0)
	{
		num_read_chars = sprintf(buf, "%s","get tp fw version fail!\n");
	}
	else
	{
		num_read_chars = sprintf(buf, "FW Version:0x%02X\n", fwver);
	}

	/*chip id*/
	if(ft6306_read_reg(0xa8, &id_type) < 0)
	{
		num_read_chars += sprintf(&buf[num_read_chars], "%s","get chip id fail!\n");;
	}
	else 
	{
		num_read_chars += sprintf(&buf[num_read_chars], "Chip ID = 0x%02X\n", id_type);	
	}	

	return num_read_chars;
}

static ssize_t ft6306_vendor_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}

static ssize_t ft6306_ft_tprwreg_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	/*place holder for future use*/
	return -EPERM;
}

static ssize_t ft6306_ft_tprwreg_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 regaddr = 0xff, regvalue = 0xff;
	u8 valbuf[5] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	//mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 2) {
		if (num_read_chars != 4) {
			pr_info("please input 2 or 4 character\n");
			goto error_return;
		}
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&this_client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}

	if (2 == num_read_chars) {
		/*read register*/
		regaddr = wmreg;
		if (ft6306_ft_read_reg(regaddr, &regvalue) < 0)
			dev_err(&this_client->dev, "Could not read the register(0x%02x)\n",
						regaddr);
		else
			pr_info("the register(0x%02x) is 0x%02x\n",
					regaddr, regvalue);
	} else {
		regaddr = wmreg >> 8;
		regvalue = wmreg;
		if (ft6306_ft_write_reg(regaddr, regvalue) < 0)
			dev_err(&this_client->dev, "Could not write the register(0x%02x)\n",
							regaddr);
		else
			dev_err(&this_client->dev, "Write 0x%02x into register(0x%02x) successful\n",
							regvalue, regaddr);
	}

error_return:
	//mutex_unlock(&g_device_mutex);

	return count;
}

#if 0
static int ft6306_ft_read_rawdata(u16 rawdata[][FTS_RX_MAX],
			u8 tx, u8 rx)
{
	u8 i = 0, j = 0, k = 0;
	int err = 0;
	u8 regvalue = 0x00;
	u8 regaddr = 0x00;
	u16 dataval = 0x0000;
	u8 writebuf[2] = {0};
	u8 read_buffer[FTS_RX_MAX * 2];
	/*scan*/
	err = ft6306_ft_read_reg(FTS_DEVICE_MODE_REG, &regvalue);
	if (err < 0) {
		return err;
	} else {
		regvalue |= 0x80;
		err = ft6306_ft_write_reg(FTS_DEVICE_MODE_REG, regvalue);
		if (err < 0) {
			return err;
		} else {
			for(i=0; i<20; i++)
			{
				msleep(8);
				err = ft6306_ft_read_reg(FTS_DEVICE_MODE_REG, 
							&regvalue);
				if (err < 0) {
					return err;
				} else {
					if (0 == (regvalue >> 7))
						break;
				}
			}
		}
	}

	/*get rawdata*/
	dev_dbg(&this_client->dev, "%s() - Reading raw data...\n", __func__);
	for(i=0; i<tx; i++)
	{
		memset(read_buffer, 0x00, (FTS_RX_MAX * 2));
		writebuf[0] = FTS_RAW_READ_REG;
		writebuf[1] = i;
		err = ft6306_ft_i2c_Write(writebuf, 2);
		if (err < 0) {
			return err;
		}
		/* Read the data for this row */
		regaddr = FTS_RAW_BEGIN_REG;
		err = ft6306_ft_i2c_Read( &regaddr, 1, read_buffer, rx*2);
		if (err < 0) {
			return err;
		}
		k = 0;
		for (j = 0; j < rx*2; j += 2)
        	{
			dataval  = read_buffer[j];
			dataval  = (dataval << 8);
			dataval |= read_buffer[j+1];
			rawdata[i][k] = dataval;
			k++;
        	}
	}

	return 0;
}
/*raw data show*/

static ssize_t ft6306_ft_rawdata_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u16 before_rawdata[FTS_TX_MAX][FTS_RX_MAX];
	u8 rx = 0, tx = 0;
	u8 i = 0, j = 0;
	int err = 0;
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	//mutex_lock(&g_device_mutex);
	/*entry factory*/
	err = ft6306_ft_write_reg(FTS_DEVICE_MODE_REG, FTS_FACTORYMODE_VALUE);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	}

	/*get rx and tx num*/
	err = ft6306_ft_read_reg(FTS_TXNUM_REG, &tx);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:get tx error!\n", __func__);
		goto RAW_ERROR;
	}
	err = ft6306_ft_read_reg(FTS_RXNUM_REG, &rx);
	if (err < 0)
	{
		num_read_chars = sprintf(buf,
			"%s:get rx error!\n", __func__);
		goto RAW_ERROR;
	}
	num_read_chars += sprintf(&(buf[num_read_chars]),"FT6306 tp channel: %u * %u\n", tx, rx);
	num_read_chars += sprintf(&(buf[num_read_chars]),"Reference data:\n");

	/*get rawdata*/
	err = ft6306_ft_read_rawdata(before_rawdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	} else {
		for (i=0; i<tx; i++) {
			for (j=0; j<rx; j++) {
				num_read_chars += sprintf(&(buf[num_read_chars]),
						"%5d"/*"%u "*/, before_rawdata[i][j]);
			}
			//buf[num_read_chars-1] = '\n';
			num_read_chars += sprintf(&(buf[num_read_chars]), "\n");
		}
	}
RAW_ERROR:
	/*enter work mode*/
	err = ft6306_ft_write_reg(FTS_DEVICE_MODE_REG, FTS_WORKMODE_VALUE);
	if (err < 0)
		dev_err(&this_client->dev,"%s:enter work error!\n", __func__);
	msleep(100);
	//mutex_unlock(&g_device_mutex);
	return num_read_chars;
}
static ssize_t ft6306_ft_rawdata_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}
/*diff data show. default voltage level is 2.*/
static ssize_t ft6306_ft_diffdata_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	u16 before_rawdata[FTS_TX_MAX][FTS_RX_MAX];
	u16 after_rawdata[FTS_TX_MAX][FTS_RX_MAX];
	u8 orig_vol = 0x00;
	u8 rx = 0, tx = 0;
	u8 i = 0, j = 0;
	int err = 0;
	u8 regvalue = 0x00;
	u16 tmpdata = 0x0000;
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	//mutex_lock(&g_device_mutex);
	/*entry factory*/
	err = ft6306_ft_write_reg(FTS_DEVICE_MODE_REG, FTS_FACTORYMODE_VALUE);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	}

	/*get rx and tx num*/
	err = ft6306_ft_read_reg(FTS_TXNUM_REG, &tx);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:get tx error!\n", __func__);
		goto RAW_ERROR;
	}
	err = ft6306_ft_read_reg(FTS_RXNUM_REG, &rx);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:get rx error!\n", __func__);
		goto RAW_ERROR;
	}
	//num_read_chars += sprintf(&(buf[num_read_chars]), "tp channel: %u * %u\n", tx, rx);
	num_read_chars += sprintf(&(buf[num_read_chars]),"Delta data:\n");
	/*get rawdata*/
	err = ft6306_ft_read_rawdata(before_rawdata, tx, rx);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:diffdata show error!\n", __func__);
		goto RAW_ERROR;
	} 

	/*get original voltage and change it to get new frame rawdata*/
	err = ft6306_ft_read_reg(FTS_VOLTAGE_REG, &regvalue);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:get original voltage error!\n", __func__);
		goto RAW_ERROR;
	} 
	else 
	{
		orig_vol = regvalue;
	}
	
	if (orig_vol <= 1)
	{
    		regvalue = orig_vol + 2;
	}
	else 
	{			
		if(orig_vol >= 4)
		{
			regvalue = 1;
		}
		else
		{
			regvalue = orig_vol - 2;
		}
    }
	if (regvalue > 7)
		regvalue = 7;
	if (regvalue <= 0)
		regvalue = 0;
	#if 0	
	num_read_chars += sprintf(&(buf[num_read_chars]),
		"original voltage: %u changed voltage:%u\n",
		orig_vol, regvalue);
	#endif
	err = ft6306_ft_write_reg(FTS_VOLTAGE_REG, regvalue);
	if (err < 0) 
	{
		num_read_chars = sprintf(buf,"%s:set original voltage error!\n", __func__);
		goto RAW_ERROR;
	}
	
	/*get rawdata*/
	for (i=0; i<3; i++)
		err = ft6306_ft_read_rawdata(after_rawdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,"%s:diffdata show error!\n", __func__);
		goto RETURN_ORIG_VOLTAGE;
	} else {
		for (i=0; i<tx; i++) {
			for (j=0; j<rx; j++) {
				if (after_rawdata[i][j] > before_rawdata[i][j])
					tmpdata = after_rawdata[i][j] - before_rawdata[i][j];
				else
					tmpdata = before_rawdata[i][j] - after_rawdata[i][j];
				num_read_chars += sprintf(&(buf[num_read_chars]),
						"%5d", tmpdata);
			}
			num_read_chars += sprintf(&(buf[num_read_chars]), "\n");
			//buf[num_read_chars-1] = '\n';
		}
	}
	
RETURN_ORIG_VOLTAGE:
	err = ft6306_ft_write_reg(FTS_VOLTAGE_REG, orig_vol);
	if (err < 0)
		dev_err(&this_client->dev,
			"%s:return original voltage error!\n", __func__);
RAW_ERROR:
	/*enter work mode*/
	err = ft6306_ft_write_reg(FTS_DEVICE_MODE_REG, FTS_WORKMODE_VALUE);
	if (err < 0)
		dev_err(&this_client->dev,"%s:enter work error!\n", __func__);
	msleep(100);
	//mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}
static ssize_t ft6306_ft_diffdata_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}

static ssize_t ft6306_ft_diag_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars1 = 0;
	ssize_t num_read_chars2 = 0;

	num_read_chars1 = ft6306_ft_rawdata_show(dev,attr,buf);
	num_read_chars2 = ft6306_ft_diffdata_show(dev,attr,(buf+num_read_chars1));
	return (num_read_chars1+ num_read_chars2);
}
static ssize_t ft6306_ft_diag_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	/*place holder for future use*/
	return -EPERM;
}

#endif


/*open short show*/
static ssize_t ft6306_ft_setrawrange_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;

	//mutex_lock(&g_device_mutex);
	num_read_chars = sprintf(buf,
				"min rawdata:%d max rawdata:%d\n",
				g_min_rawdata, g_max_rawdata);
	
	//mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft6306_ft_setrawrange_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 valbuf[12] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	//mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 8) {
		dev_err(&this_client->dev, "%s:please input 1 character\n",
				__func__);
		goto error_return;
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&this_client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}
	g_min_rawdata = wmreg >> 16;
	g_max_rawdata = wmreg;
error_return:
	//mutex_unlock(&g_device_mutex);

	return count;
}
static ssize_t ft6306_ft_setdiffrange_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	//mutex_lock(&g_device_mutex);
	num_read_chars = sprintf(buf,
				"min diffdata:%d max diffdata:%d\n",
				g_min_diffdata, g_max_diffdata);
	//mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft6306_ft_setdiffrange_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 valbuf[12] = {0};

	memset(valbuf, 0, sizeof(valbuf));
//	mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 8) {
		dev_err(&this_client->dev, "%s:please input 1 character\n",
				__func__);
		goto error_return;
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&this_client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}
	g_min_diffdata = wmreg >> 16;
	g_max_diffdata = wmreg;
error_return:
	//mutex_unlock(&g_device_mutex);

	return count;

}

static ssize_t ft6306_ft_setvoltagelevel_show(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	ssize_t num_read_chars = 0;
	//mutex_lock(&g_device_mutex);
	num_read_chars = sprintf(buf, "voltage level:%d\n", g_voltage_level);
	//mutex_unlock(&g_device_mutex);
	
	return num_read_chars;
}

static ssize_t ft6306_ft_setvoltagelevel_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
//	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	ssize_t num_read_chars = 0;
	int retval;
	long unsigned int wmreg = 0;
	u8 valbuf[5] = {0};

	memset(valbuf, 0, sizeof(valbuf));
	//mutex_lock(&g_device_mutex);
	num_read_chars = count - 1;

	if (num_read_chars != 1) {
		dev_err(&this_client->dev, "%s:please input 1 character\n",
				__func__);
		goto error_return;
	}

	memcpy(valbuf, buf, num_read_chars);
	retval = strict_strtoul(valbuf, 16, &wmreg);

	if (0 != retval) {
		dev_err(&this_client->dev, "%s() - ERROR: Could not convert the "\
						"given input to a number." \
						"The given input was: \"%s\"\n",
						__func__, buf);
		goto error_return;
	}
	g_voltage_level = wmreg;
	
error_return:
	//mutex_unlock(&g_device_mutex);

	return count;
}

/*------------------------------------------------------------------------
*get the fw version
*example:cat version
-------------------------------------------------------------------------*/
//static DEVICE_ATTR(version, (S_IWUSR|S_IRUGO), ft6306_version_show,
//			ft6306_version_store);


/*------------------------------------------------------------------------
*get the id version
*example:cat id
-------------------------------------------------------------------------*/
static DEVICE_ATTR(vendor, (S_IWUSR|S_IRUGO), ft6306_vendor_show,
			ft6306_vendor_store);

/*------------------------------------------------------------------------
*read and write register
*read example: echo 88 > rwreg ---read register 0x88
*write example:echo 8807 > rwreg ---write 0x07 into register 0x88
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
-------------------------------------------------------------------------*/
static DEVICE_ATTR(rwreg, S_IRUGO | S_IWUSR, ft6306_ft_tprwreg_show,
			ft6306_ft_tprwreg_store);

/*------------------------------------------------------------------------
*show a frame rawdata
*example:example:cat rawdata
-------------------------------------------------------------------------*/
//static DEVICE_ATTR(rawdata, S_IRUGO | S_IWUSR, ft6306_ft_rawdata_show,
//			ft6306_ft_rawdata_store);

/*------------------------------------------------------------------------
*show a frame diffdata
*example:cat diffdata
-------------------------------------------------------------------------*/
//static DEVICE_ATTR(diffdata, S_IRUGO | S_IWUSR, ft6306_ft_diffdata_show,
//			ft6306_ft_diffdata_store);


/*------------------------------------------------------------------------
*show a frame diffdata
*example:cat diag
-------------------------------------------------------------------------*/
//static DEVICE_ATTR(diag, S_IRUGO | S_IWUSR, ft6306_ft_diag_show,
		//	ft6306_ft_diag_store);


/*------------------------------------------------------------------------
-------------------------------------------------------------------------*/


/*------------------------------------------------------------------------
*set range of rawdata
*example:echo 60009999 > setraw(range is 6000-9999)
*		cat setraw
-------------------------------------------------------------------------*/
static DEVICE_ATTR(setraw, S_IRUGO | S_IWUSR, ft6306_ft_setrawrange_show,
			ft6306_ft_setrawrange_store);
/*------------------------------------------------------------------------
*set range of diffdata
*example:echo 00501000 > setdiffdata(range is 50-1000)
*		cat setdiffdata
-------------------------------------------------------------------------*/

static DEVICE_ATTR(setdiffdata, S_IRUGO | S_IWUSR, ft6306_ft_setdiffrange_show,
			ft6306_ft_setdiffrange_store);
/*------------------------------------------------------------------------
*set range of diffdata
*example:echo 2 > setvol
*		cat setvol
-------------------------------------------------------------------------*/
static DEVICE_ATTR(setvol, S_IRUGO | S_IWUSR,
			ft6306_ft_setvoltagelevel_show,
			ft6306_ft_setvoltagelevel_store);




static struct kobject *android_touch_ftobj;
static int ft6306_touch_sysfs_init(void)
{
	int ret;
	android_touch_ftobj = kobject_create_and_add("android_touch", NULL);
	if (android_touch_ftobj == NULL)
	{
		printk(KERN_ERR "TOUCH_ERR: subsystem_register failed\n");
		ret = -ENOMEM;
		return ret;
	}
#if 0	
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_version.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file version failed\n");
		return ret;
	}
#endif
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_vendor.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file chip vendor failed\n");
		return ret;
	}
#if 0	
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_rawdata.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file rawdata failed\n");
		return ret;
	}

	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_diffdata.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file diffdata failed\n");
		return ret;
	}
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_diag.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file diag failed\n");
		return ret;
	}
#endif	
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_setraw.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file setraw failed\n");
		return ret;
	}
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_setdiffdata.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file setdiffdata failed\n");
		return ret;
	}
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_setvol.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file setvol failed\n");
		return ret;
	}
	
	ret = sysfs_create_file(android_touch_ftobj, &dev_attr_rwreg.attr);
	if (ret) 
	{
		printk(KERN_ERR "TOUCH_ERR: create_file rwreg failed\n");
		return ret;
	}	
	return 0;
}

static void ft6306_touch_sysfs_deinit(void)
{
	//sysfs_remove_file(android_touch_ftobj, &dev_attr_fwversion.attr);
	sysfs_remove_file(android_touch_ftobj, &dev_attr_vendor.attr);
	//sysfs_remove_file(android_touch_ftobj, &dev_attr_rawdata.attr);
	//sysfs_remove_file(android_touch_ftobj, &dev_attr_diffdata.attr);
	//sysfs_remove_file(android_touch_ftobj, &dev_attr_diag.attr);
	sysfs_remove_file(android_touch_ftobj, &dev_attr_setraw.attr);
	sysfs_remove_file(android_touch_ftobj, &dev_attr_setdiffdata.attr);
	sysfs_remove_file(android_touch_ftobj, &dev_attr_setvol.attr);
	sysfs_remove_file(android_touch_ftobj, &dev_attr_rwreg.attr);
	kobject_del(android_touch_ftobj);
}

/*-----------------------------------------------------------------------------------
* tp factory test end
-----------------------------------------------------------------------------------*/
#endif
/***********************************************************************************************
	add by liukai for virtualkeys 
***********************************************************************************************/

static ssize_t ft6306_virtual_keys_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
//for customer test system
#if 0
	if (ft6306_tp_type_id == 0x51)
	{
		return sprintf(buf, 
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":124:1000:50:50"
	":" __stringify(EV_KEY) ":" __stringify(KEY_HOMEPAGE) ":286:1000:50:50"
	":" __stringify(EV_KEY) ":" __stringify(KEY_MENU) ":398:1000:50:50"
	"\n");
	}
		else if(ft6306_tp_type_id == 0x59)
	{
		return sprintf(buf, 
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":240:900:0:20"
	
	"\n");	
	}
	else
	{
		return sprintf(buf, "TP matching failed! type: %02x\n", ft6306_tp_type_id);
	}
	#endif
		return sprintf(buf, 
	__stringify(EV_KEY) ":" __stringify(KEY_BACK) ":240:900:50:50"
	
	"\n");	
  
}

static struct kobj_attribute ft6306_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.ft6306",
		.mode = S_IRUGO,
	},
	.show = &ft6306_virtual_keys_show,
};

static struct attribute *ft6306_properties_attrs[] = {
	&ft6306_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group ft6306_properties_attr_group = {
	.attrs = ft6306_properties_attrs
};

extern struct kobject * intel_get_properties(void);


static int ft6306_virtual_keys_init(void) 
{
	
	int ret;
	struct kobject *properties_kobj;
	properties_kobj = kobject_create_and_add("board_properties", NULL);
	
	if (!properties_kobj)
	{
		printk(KERN_ERR "failed to create board_properties\n");
		return  -ENOMEM;
	}
		ret = sysfs_create_group(properties_kobj,&ft6306_properties_attr_group);
	if (ret )
	{
		printk(KERN_ERR "failed to create virtualkeys.ft6306\n");
		return ret;
	}
	return 0;
}
#if 0
 static void vreg4_init(void)
 {

      int rc = 0 ;
	struct vreg* vreg_gp4 = vreg_get(NULL, "gp2");

	rc = vreg_set_level(vreg_gp4, 2850);
	if (rc) {
		printk(KERN_ERR "%s: vreg set level failed (%d)\n",
			   __func__, rc);
		return;
	}
		#if 1	
	rc= vreg_enable(vreg_gp4);
	if (rc) {
		printk(KERN_ERR "%s: vreg enable failed (%d)\n",
			 __func__, rc);
		return;
	}
		mdelay(10);
        #endif
      }
#endif
static int ft6306_touch_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct ft6306_touch_data *ft6306_touch;
	struct input_dev *input_dev;
	int err = 0;
	unsigned char uc_reg_value; 
	//vreg4_init();

	printk( "ft6306_touch_probe--enter\n");
	ft6306_touch_pdata = client->dev.platform_data;
	if (ft6306_touch_pdata == NULL) 
	{
		dev_err(&client->dev, "%s: platform data is null\n", __func__);
		goto exit_platform_data_null;
	}
	
	/*init RESET pin*/
	err = gpio_request(ft6306_touch_pdata->reset,"ft6306 reset pin");
	if (err<0)
		printk(KERN_ERR"ft6306 wakeup gpio request failed\n");
	err = gpio_tlmm_config(GPIO_CFG(ft6306_touch_pdata->reset,
              0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
              GPIO_CFG_4MA),
             GPIO_CFG_ENABLE);
	
	ft6306_chip_reset();

/* Check if the I2C bus supports BYTE transfer */
	err = i2c_check_functionality(client->adapter,I2C_FUNC_SMBUS_BYTE);
	if (!err) 
	{
		dev_err(&client->dev,
			    "%s adapter not supported\n",
				dev_driver_string(&client->adapter->dev));
		goto exit_check_functionality_failed;
	}
		
	ft6306_touch = kzalloc(sizeof(*ft6306_touch), GFP_KERNEL);
	if (!ft6306_touch)	
	{
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	this_client = client;
	i2c_set_clientdata(client, ft6306_touch);
	mutex_init(&ft6306_touch->device_mode_mutex);
	
	INIT_WORK(&ft6306_touch->pen_event_work, ft6306_touch_pen_irq_work);

	ft6306_touch->ts_workqueue = create_singlethread_workqueue(dev_name(&client->dev));
	if (!ft6306_touch->ts_workqueue) 
	{
		err = -ESRCH;
		goto exit_create_singlethread;
	}
	/*init INTERRUPT pin*/	
	if (ft6306_touch_pdata->irq)
	{
		//gpio_request(tpdev->pdata->irq, tpdev->client->name);
		gpio_tlmm_config(GPIO_CFG(ft6306_touch_pdata->irq, 0, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	} 
	if (request_irq(client->irq, ft6306_touch_interrupt,IRQF_TRIGGER_FALLING, FT6306_NAME, ft6306_touch) >= 0)
	{
		printk("Received IRQ!\n");
		/* Delete by su.wenguang@byd.com at 2012/12/11.
		if (irq_set_irq_wake(tpdev->client->irq, 1) < 0)
			printk(KERN_ERR "failed to set IRQ wake\n");
		*/
	} 

	input_dev = input_allocate_device();
	if (!input_dev) 
	{
		err = -ENOMEM;
		dev_err(&client->dev, "failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	ft6306_touch->input_dev = input_dev;
	set_bit(ABS_MT_POSITION_X, input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, input_dev->absbit);
	/*Disable PRESSUE,modify by Daivd.Wang*/
	//set_bit(ABS_MT_PRESSURE, input_dev->absbit);
	
	set_bit(INPUT_PROP_DIRECT, input_dev->propbit);

	input_set_abs_params(input_dev,
			ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	/*Disable PRESSUE,modify by Daivd.Wang*/
	#if 0
	input_set_abs_params(input_dev,
			ABS_MT_PRESSURE, 0, PRESS_MAX, 0, 0);
	#endif
	
	input_set_abs_params(input_dev,
			ABS_MT_WIDTH_MAJOR, 0, 200, 0, 0);
	input_set_abs_params(input_dev,
			ABS_MT_TRACKING_ID, 0, 5, 0, 0);

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);	
	__set_bit(EV_KEY, input_dev->evbit);
	__set_bit(KEY_HOME, input_dev->keybit);
	__set_bit(KEY_MENU, input_dev->keybit);	

	msleep(150);
	/*get the tp module id*/
	ft6306_tp_id();
	
	//ft6306_fwupgrate_ifile();

	/*tp's device name*/
	input_dev->name	= "ft6306";		//dev_name(&client->dev)
	input_dev->phys = "ft6306";
	
	/* Initialize virtualkeys */
	ft6306_virtual_keys_init();

	input_set_capability(input_dev, EV_KEY, KEY_PROG1);

	/*register the input device*/
	err = input_register_device(input_dev);
	if (err) 
	{
		dev_err(&client->dev,
		"ft6306_touch_probe: failed to register input device: %s\n",
		dev_name(&client->dev));
		goto exit_input_register_device_failed;
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	//ft6306_touch->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ft6306_touch->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	ft6306_touch->early_suspend.suspend = ft6306_touch_suspend;
	ft6306_touch->early_suspend.resume	= ft6306_touch_resume;
	register_early_suspend(&ft6306_touch->early_suspend);
#endif

	uc_reg_value = ft6306_read_fw_ver();
	printk("[%s]: Firmware version is 0x%x\n", __func__, uc_reg_value);
	
	ft6306_read_reg(FT6306_REG_PERIODACTIVE, &uc_reg_value);
	printk("[%s]: report rate is %dHz.\n", __func__, uc_reg_value * 10);

	if(uc_reg_value != 0xb) 
	{
		ft6306_write_reg(FT6306_REG_PERIODACTIVE, 0xb);
		ft6306_read_reg(FT6306_REG_PERIODACTIVE, &uc_reg_value);
		printk("[%s]: set report rate to %dHz.\n", __func__, uc_reg_value * 10);
	}
	
	ft6306_read_reg(FT6306_REG_THGROUP, &uc_reg_value);
	printk("[%s]: touch threshold is %d.\n", __func__, uc_reg_value * 4);
	
	

	//create sysfs
	err = sysfs_create_group(&client->dev.kobj, &ft6306_attribute_group);
	if (0 != err)
	{
		dev_err(&client->dev, "%s() - ERROR: sysfs_create_group() failed: %d\n", __FUNCTION__, err);
		sysfs_remove_group(&client->dev.kobj, &ft6306_attribute_group);
	}
	else
	{
		printk("[%s] - sysfs_create_group() succeeded.\n", __func__);
	}
#ifdef __FT6306_FT__	
	ft6306_touch_sysfs_init();
#endif
#if N_patch
  //Switch to option vars init.                                                                          
    sto_back_key_sampling_counter = 0;                                                                     
    setup_timer(&sto_timer, ft6306_sto_timer_callback, 0);    
#endif
     DBUG(printk("[%s]: End of probe !\n", __func__);)
		printk( "ft6306_touch_probe--exit\n");
	return 0;

exit_input_register_device_failed:
	input_free_device(input_dev);
exit_input_dev_alloc_failed:
	kfree(ft6306_touch);
//exit_irq_request_failed:
//	cancel_work_sync(&ft6306_touch->pen_event_work);
//	destroy_workqueue(ft6306_touch->ts_workqueue);
//	free_irq(client->irq, ft6306_touch);

exit_platform_data_null:
exit_create_singlethread:
	DBUG(printk("[%s]: singlethread error !\n", __func__);)
	i2c_set_clientdata(client, NULL);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

/***********************************************************************************************
remove touch
***********************************************************************************************/
static int __devexit ft6306_touch_remove(struct i2c_client *client)
{
	DBUG(printk("[%s]: Enter!\n", __func__);)
	struct ft6306_touch_data *ft6306_touch = i2c_get_clientdata(client);
#ifdef __FT6306_FT__		
	ft6306_touch_sysfs_deinit();
#endif	
	unregister_early_suspend(&ft6306_touch->early_suspend);
	free_irq(client->irq, ft6306_touch);
	input_unregister_device(ft6306_touch->input_dev);
	kfree(ft6306_touch);
	cancel_work_sync(&ft6306_touch->pen_event_work);
	destroy_workqueue(ft6306_touch->ts_workqueue);
	i2c_set_clientdata(client, NULL);
	return 0;
}

static const struct i2c_device_id ft6306_touch_id[] = {
	{ FT6306_NAME, 0 },
	{ },
};

MODULE_DEVICE_TABLE(i2c, ft6306_touch_id);
/***********************************************************************************************
touch driver
***********************************************************************************************/
static struct i2c_driver ft6306_touch_driver = {
	.probe		= ft6306_touch_probe,
	.remove		= __devexit_p(ft6306_touch_remove),
	.id_table	= ft6306_touch_id,
	.driver	= {
		.name	= FT6306_NAME,
		.owner	= THIS_MODULE,
	},
};

/***********************************************************************************************
initialize the touch
***********************************************************************************************/
static int __init ft6306_touch_init(void)
{
	int ret;
	printk("[%s]: Enter!\n", __func__);
	ret = i2c_add_driver(&ft6306_touch_driver);
	DBUG(printk("ret=%d\n",ret);)
	return ret;
}

/***********************************************************************************************
exit the touch
***********************************************************************************************/
static void __exit ft6306_touch_exit(void)
{
	printk("[%s]: Enter!\n", __func__);
	i2c_del_driver(&ft6306_touch_driver);
}

module_init(ft6306_touch_init);
module_exit(ft6306_touch_exit);

MODULE_AUTHOR("<wenfs@Focaltech-systems.com>");
MODULE_DESCRIPTION("FocalTech ft6306 TouchScreen driver");
MODULE_LICENSE("GPL");
