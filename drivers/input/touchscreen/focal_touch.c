/*
*Copyright (C) 2010 BYD, Inc.
*/

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/slab.h>
#include <linux/workqueue.h>
#include "tp_work_func.h"
#include <mach/vreg.h>

#define TS_DEBUG 0 //[delete; shao.wenqi@byd.com; as to many log printed to kernel]
struct ts_data {
	int x;
	int y;
	u8 event_flag;
};
struct focal_ts {
	struct tp_dev	tpdev;
	int use_irq;
	struct early_suspend early_suspend;
	int enable;
	};
static struct i2c_client *this_client;
#define CFG_SUPPORT_AUTO_UPG 1
 unsigned char tp_ID = 0xff;
#define FTS_TX_MAX				40
#define FTS_RX_MAX				30
#define FTS_DEVICE_MODE_REG	0x00
#define FTS_TXNUM_REG			0x03
#define FTS_RXNUM_REG			0x04
#define FTS_RAW_READ_REG		0x01
#define FTS_RAW_BEGIN_REG		0x10
#define FTS_FACTORYMODE_VALUE		0x40
#define FTS_WORKMODE_VALUE		0x00
static u16 before_rawdata[FTS_TX_MAX][FTS_RX_MAX];

#if CFG_SUPPORT_AUTO_UPG

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

typedef struct _FTS_CTP_PROJECT_SETTING_T
{
    unsigned char uc_i2C_addr;             //I2C slave address (8 bit address)
    unsigned char uc_io_voltage;           //IO Voltage 0---3.3v;	1----1.8v
    unsigned char uc_panel_factory_id;     //TP panel factory ID
}FTS_CTP_PROJECT_SETTING_T;

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE              0x0

#define I2C_CTPM_ADDRESS       0x70
#define    FTS_PACKET_LENGTH        128

static unsigned char CTPM_FW[]=
{
	#include "ft_app.i"
};
#endif
static int focal_ts_read(struct i2c_client *client, u8 reg, u8 *buf, int num)
{
	struct i2c_msg xfer_msg[2];

	xfer_msg[0].addr = client->addr;
	xfer_msg[0].len = 1;
	xfer_msg[0].flags = 0;
	xfer_msg[0].buf = &reg;

	xfer_msg[1].addr = client->addr;
	xfer_msg[1].len = num;
	xfer_msg[1].flags = I2C_M_RD;
	xfer_msg[1].buf = buf;
	return i2c_transfer(client->adapter, xfer_msg, 2);
}

// [sun.yu5@byd.com, add,WG703T2,support 2 touchscreen,  distinguish by ID]
static int focal_ts_get_ID(struct i2c_client *client)
{
    int ret = 0;

	ret = focal_ts_read(client,0xa8, &tp_ID, 1); //only read two fingers' data
	if (ret<0)
	{
#if	TS_DEBUG	
		printk(KERN_ERR "%s: i2c_transfer failed\n", __func__);
#endif
	}
	printk(KERN_ERR " focal_ts_get_ID   tp_ID=%x\n",tp_ID);
	return ret;
}
// [sun.yu5@byd.com, end]

// [shao.wenqi@byd.com, add,WG703T2_C000109,support 5 fingurs touch,  modify from 21 to ts->tpdev.pdata->wakeup]
#define ODMM_FINGERS_NUMBER 5
#define ODMM_FINGERS_REGESTER_OFFSET 3
#define ODMM_ONE_FINGER_BYTE 6
// [shao.wenqi@byd.com, end]

static u8 focal_work_buf[sizeof(struct ts_data)*ODMM_FINGERS_NUMBER] = {0x00,};
static void  focal_work_func(struct work_struct *work)
{
	int ret;
	u8 data_buf[ODMM_FINGERS_NUMBER*ODMM_ONE_FINGER_BYTE+ODMM_FINGERS_REGESTER_OFFSET+1]= {0}; 
	u16 low_byte	= 0;
	u16 high_byte	= 0;
	u8 i = 0;
    int finger_num = 0;
	struct ts_data *ts_data_info;
	struct tp_dev *tpdev = container_of(work, struct tp_dev, work);
    
    memset(focal_work_buf, 0x00, sizeof(focal_work_buf));
	ts_data_info = (void*)focal_work_buf;
	ret = focal_ts_read(tpdev->client,0x00, data_buf, (ODMM_FINGERS_NUMBER*ODMM_ONE_FINGER_BYTE+ODMM_FINGERS_REGESTER_OFFSET)); //only read five fingers' data

	//printk(KERN_ERR "i2c_transfer   ret=%d\n",ret);// [delete; shao.wenqi@byd.com; as many log printed to kernel]
	if (ret<0)
	{
		printk(KERN_ERR "%s: i2c_transfer failed\n", __func__);
	}
/*han.jiajia@byd.com modify*/
finger_num = data_buf[2] & 0x07;
    if (finger_num > ODMM_FINGERS_NUMBER)
    {
        finger_num = ODMM_FINGERS_NUMBER;
    }
/*han.jiajia@byd.com end*/
	// [shao.wenqi@byd.com,modify,WG703T2_C000109,support 5 fingurs touch,  modify from 21 to ts->tpdev.pdata->wakeup]
	for(i = 0; i < finger_num; i++)// [shao.wenqi@byd.com,end]
	{
		/*get the X coordinate, 2 bytes*/
		high_byte = data_buf[3+6*i];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data_buf[4+6*i];
		ts_data_info[i].x = high_byte |low_byte;

		/*get the Y coordinate, 2 bytes*/
		high_byte = data_buf[5+6*i];
		high_byte <<= 8;
		high_byte &= 0x0f00;
		low_byte = data_buf[6+6*i];
		ts_data_info[i].y = high_byte |low_byte;

		/*point status, 0:down, 1:up ,2 :contact*/
		ts_data_info[i].event_flag = (data_buf[3+6*i] & 0xC0) >>6;
#if	TS_DEBUG
		printk(KERN_INFO"ts_data_info[%d].event_flag = %d\n", i, ts_data_info[i].event_flag);
#endif
		if (ts_data_info[i].event_flag==0 || ts_data_info[i].event_flag==2)
		{	
			ts_data_info[i].x = ts_data_info[i].x * tpdev->pdata->lcd_x / tpdev->pdata->tp_x;
			ts_data_info[i].y = ts_data_info[i].y * tpdev->pdata->lcd_y / tpdev->pdata->tp_y;
			#if	TS_DEBUG
			printk(KERN_INFO"ts_data_info[%d].x = %d , ts_data_info[%d].y = %d \n", i, ts_data_info[i].x, i, ts_data_info[i].y);
			#endif
#ifdef ABS_MT_TRACKING_ID
            input_report_abs(tpdev->input_dev, ABS_MT_TRACKING_ID, i);
#endif
			input_report_abs(tpdev->input_dev, ABS_MT_POSITION_X, ts_data_info[i].x);
			input_report_abs(tpdev->input_dev, ABS_MT_POSITION_Y, ts_data_info[i].y);
			input_report_abs(tpdev->input_dev, ABS_MT_TOUCH_MAJOR, 0xc8);
		}
		else
		{
			input_report_abs(tpdev->input_dev, ABS_MT_TOUCH_MAJOR, 0x0);
		}
/*han.jiajia@byd.com modify*/
	 if(finger_num > 0 )
        input_mt_sync(tpdev->input_dev);	
/*han.jiajia@byd.com end*/
	}

    input_report_key(tpdev->input_dev, BTN_TOUCH, finger_num > 0);

	input_sync(tpdev->input_dev);
    
	enable_irq(tpdev->client->irq);
	
}

#if 1
static void focal_ts_enable(struct focal_ts *ts)
{
	if(ts->use_irq)
	{
		enable_irq(ts->tpdev.client->irq);
	}
	ts->enable = 1;
}

static void focal_ts_disable(struct focal_ts *ts)
{
	if(ts->use_irq)
	{
		disable_irq(ts->tpdev.client->irq);
	}
	ts->enable = 0;
}
#endif
static int focal_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{

	struct focal_ts *ts = i2c_get_clientdata(client);	
	focal_ts_disable(ts);
#if  0 // Delete by su.wenguang@byd.com at 2012/12/11, Bug[EG806T_P000837].
	cancel_work_sync(&ts->tpdev.work);
#endif
//yang.chenglei@byd.com let tp enter sleep mode begin
	i2c_smbus_write_byte_data(client, 0xa5, 0x03);
//yang.chenglei@byd.com let tp enter sleep mode end





#if 0
	mdelay(10);
	if (ts->power) {
		 ts->power(TS_OFF);
	}
#endif
	return 0;
}

//static struct vreg *vreg_gp4;
static int focal_ts_resume(struct i2c_client *client)
{
	
	struct focal_ts *ts = i2c_get_clientdata(client);


	gpio_request(ts->tpdev.pdata->wakeup,"focal wake up pin");
	gpio_tlmm_config(GPIO_CFG(ts->tpdev.pdata->wakeup,
              0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
              GPIO_CFG_4MA),
             GPIO_CFG_ENABLE);

	printk(KERN_INFO "focal_ts_resume %d\n",ts->tpdev.pdata->wakeup);
	gpio_set_value(ts->tpdev.pdata->wakeup, 1);
	mdelay(30);
	gpio_set_value(ts->tpdev.pdata->wakeup, 0);
	mdelay(50);
	printk(KERN_INFO "focal_ts_resume %d\n",gpio_get_value(ts->tpdev.pdata->wakeup));
	gpio_set_value(ts->tpdev.pdata->wakeup, 1);
	gpio_free(ts->tpdev.pdata->wakeup);
	mdelay(20);
	printk(KERN_INFO "focal_ts_resume %d\n",gpio_get_value(ts->tpdev.pdata->wakeup));
	focal_ts_enable(ts);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void focal_ts_early_suspend(struct early_suspend *h)
{
	struct focal_ts *ts;
	ts = container_of(h, struct focal_ts, early_suspend);
	focal_ts_suspend(ts->tpdev.client, PMSG_SUSPEND);
	printk(KERN_INFO "focal_ts_early_suspend\n");

}

static void focal_ts_late_resume(struct early_suspend *h)
{
	struct focal_ts *ts;
	ts = container_of(h, struct focal_ts, early_suspend);
	focal_ts_resume(ts->tpdev.client);
	printk(KERN_INFO "focal_ts_late_resume\n");
}
#endif

 static void vreg4_init(void)
#if 1
 {

      int rc = 0 ;
	struct vreg* vreg_gp4 = vreg_get(NULL, "gp2");

	rc = vreg_set_level(vreg_gp4, 2900);
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
#if CFG_SUPPORT_AUTO_UPG

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
        printk("[FTS]i2c_read_interface error\n");
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
        printk("[FTS]i2c_write_interface error line = %d, ret = %d\n", __LINE__, ret);
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

static int ft5x0x_i2c_txdata(char *txdata, int length)
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
static int ft5x0x_write_reg(u8 addr, u8 para)
{
    u8 buf[3];
    int ret = -1;

    buf[0] = addr;
    buf[1] = para;
    ret = ft5x0x_i2c_txdata(buf, 2);
    if (ret < 0) {
        pr_err("write reg failed! %#x ret: %d", buf[0], ret);
        return -1;
    }
    
    return 0;
}

static int ft5x0x_read_reg(u8 addr, u8 *pdata)
{
	int ret;
	u8 buf[2];
	struct i2c_msg msgs[2];

    //
	buf[0] = addr;    //register address
	
	msgs[0].addr = this_client->addr;
	msgs[0].flags = 0;
	msgs[0].len = 1;
	msgs[0].buf = buf;
	msgs[1].addr = this_client->addr;
	msgs[1].flags = I2C_M_RD;
	msgs[1].len = 1;
	msgs[1].buf = buf;

	ret = i2c_transfer(this_client->adapter, msgs, 2);
	if (ret < 0)
		pr_err("msg %s i2c read error: %d\n", __func__, ret);

	*pdata = buf[0];
	return ret;
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



E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    FTS_BYTE reg_val[2] = {0};
    FTS_DWRD i = 0;

    FTS_DWRD  packet_number;
    FTS_DWRD  j = 0;
    FTS_DWRD  temp;
    FTS_DWRD  lenght;
    FTS_BYTE  packet_buf[FTS_PACKET_LENGTH + 6];
    FTS_BYTE  auc_i2c_write_buf[10];
    FTS_BYTE bt_ecc;
    int      i_ret;
    unsigned char au_delay_timings[11] = {30, 33, 36, 39, 42, 45, 27, 24,21,18,15};
	UPGR_START:
    /*********Step 1:Reset  CTPM *****/
    /*write 0xaa to register 0xfc*/
    ft5x0x_write_reg(0xfc,0xaa);
    delay_qt_ms(50);
     /*write 0x55 to register 0xfc*/
    ft5x0x_write_reg(0xfc,0x55);
    printk("[FTS] Step 1: Reset CTPM test\n");
   
    delay_qt_ms(au_delay_timings[j]);   

    /*********Step 2:Enter upgrade mode *****/
    auc_i2c_write_buf[0] = 0x55;
    auc_i2c_write_buf[1] = 0xaa;
    do
    {
        i ++;
        i_ret = ft5x0x_i2c_txdata(auc_i2c_write_buf, 2);
        delay_qt_ms(5);
    }while(i_ret <= 0 && i < 5 );

    /*********Step 3:check READ-ID***********************/        
    cmd_write(0x90,0x00,0x00,0x00,4);
    byte_read(reg_val,2);
    if (reg_val[0] == 0x79 && reg_val[1] == 0x7)
    {
        printk("[FTS] Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    }
    else
    {
	if (j < 10)
        {
             j ++;
             msleep(200);
             goto UPGR_START; 
        }
        else
        {
            return ERR_READID;
        }
    }

    cmd_write(0xcd,0x0,0x00,0x00,1);
    byte_read(reg_val,1);
    printk("[FTS] bootloader version = 0x%x\n", reg_val[0]);

     /*********Step 4:erase app and panel paramenter area ********************/
    cmd_write(0x61,0x00,0x00,0x00,1);  //erase app area
    delay_qt_ms(1500); 
    cmd_write(0x63,0x00,0x00,0x00,1);  //erase panel parameter area
    delay_qt_ms(100);
    printk("[FTS] Step 4: erase. \n");

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    printk("[FTS] Step 5: start upgrade. \n");
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
        delay_qt_ms(FTS_PACKET_LENGTH/6 + 1);
        if ((j * FTS_PACKET_LENGTH % 1024) == 0)
        {
              printk("[FTS] upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
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
        delay_qt_ms(20);
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
        delay_qt_ms(20);
    }

    /*********Step 6: read out checksum***********************/
    /*send the opration head*/
    cmd_write(0xcc,0x00,0x00,0x00,1);
    byte_read(reg_val,1);
    printk("[FTS] Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    //if(reg_val[0] != bt_ecc)
    //{
        //return ERR_ECC;
    //}

    /*********Step 7: reset the new FW***********************/
    cmd_write(0x07,0x00,0x00,0x00,1);
    if(reg_val[0] != bt_ecc)
    {
        return ERR_ECC;
    }
    msleep(300);  //make sure CTP startup normally
    
    return ERR_OK;
}

int fts_ctpm_auto_clb(void)
{
    unsigned char uc_temp;
    unsigned char i ;

    printk("[FTS] start auto CLB.\n");
    msleep(200);
    ft5x0x_write_reg(0, 0x40);  
    delay_qt_ms(100);   //make sure already enter factory mode
    ft5x0x_write_reg(2, 0x4);  //write command to start calibration
    delay_qt_ms(300);
    for(i=0;i<100;i++)
    {
        ft5x0x_read_reg(0,&uc_temp);
        if ( ((uc_temp&0x70)>>4) == 0x0)  //return to normal mode, calibration finish
        {
            break;
        }
        delay_qt_ms(200);
        printk("[FTS] waiting calibration %d\n",i);
        
    }
    printk("[FTS] calibration OK.\n");
    
    msleep(300);
    ft5x0x_write_reg(0, 0x40);  //goto factory mode
    delay_qt_ms(100);   //make sure already enter factory mode
    ft5x0x_write_reg(2, 0x5);  //store CLB result
    delay_qt_ms(300);
    ft5x0x_write_reg(0, 0x0); //return to normal mode 
    msleep(300);
    printk("[FTS] store CLB result OK.\n");
    return 0;
}

int fts_ctpm_fw_upgrade_with_i_file(void)
{
   FTS_BYTE*     pbt_buf = FTS_NULL;
   int i_ret;
    
    //=========FW upgrade========================*/
   pbt_buf = CTPM_FW;
   /*call the upgrade function*/
   i_ret =  fts_ctpm_fw_upgrade(pbt_buf,sizeof(CTPM_FW));
   if (i_ret != 0)
   {
       printk("[FTS] upgrade failed i_ret = %d.\n", i_ret);
       //error handling ...
       //TBD
   }
   else
   {
       printk("[FTS] upgrade successfully.\n");
       fts_ctpm_auto_clb();  //start auto CLB
   }

   return i_ret;
}

static unsigned char ft5x0x_read_fw_ver(void)
{
	unsigned char ver;
	ft5x0x_read_reg(0xa6, &ver);
	return(ver);
}

unsigned char fts_ctpm_get_i_file_ver(void)
{
    unsigned int ui_sz;
    ui_sz = sizeof(CTPM_FW);
    if (ui_sz > 2)
    {
        return CTPM_FW[ui_sz - 2];
    }
    else
    {
        //TBD, error handling?
        return 0xff; //default value
    }
}

int fts_ctpm_auto_upg(void)
{
    unsigned char uc_host_fm_ver;
    unsigned char uc_tp_fm_ver;
    int           i_ret;

    uc_tp_fm_ver = ft5x0x_read_fw_ver();
    uc_host_fm_ver = fts_ctpm_get_i_file_ver();
   // if ( uc_tp_fm_ver == 0xa6  ||   //the firmware in touch panel maybe corrupted
     //    uc_tp_fm_ver < uc_host_fm_ver //the firmware in host flash is new, need upgrade
      //  )
    {
        msleep(100);
        printk("[FTS] uc_tp_fm_ver = 0x%x, uc_host_fm_ver = 0x%x\n",
            uc_tp_fm_ver, uc_host_fm_ver);
        i_ret = fts_ctpm_fw_upgrade_with_i_file();    
        if (i_ret == 0)
        {
            msleep(300);
            uc_host_fm_ver = fts_ctpm_get_i_file_ver();
            printk("[FTS] upgrade to new version 0x%x\n", uc_host_fm_ver);
        }
        else
        {
            printk("[FTS] upgrade failed ret=%d.\n", i_ret);
        }
    }

    return 0;
}

#endif
static int ft_rawdata_scan(void)
{
	int err = 0, i = 0;
	u8 regvalue = 0x00;
	/*scan*/

	err = ft5x0x_read_reg(FTS_DEVICE_MODE_REG, &regvalue);
	if (err < 0)
		return err;
	else {
		regvalue |= 0x80;
		err = ft5x0x_write_reg(FTS_DEVICE_MODE_REG, regvalue);
		if (err < 0)
			return err;
		else {
			for(i=0; i<20; i++) {
				msleep(8);
				err = ft5x0x_read_reg(FTS_DEVICE_MODE_REG, 
							&regvalue);
				if (err < 0)
					return err;
				else {
					if (0 == (regvalue >> 7))
						break;
				}
			}
			if (i >= 20)
				return -EIO;
		}
	}
	return 0;
}
static int ft5x0x_read_rawdata(struct i2c_client *client, u16 rawdata[][FTS_RX_MAX],
			u8 tx, u8 rx)
{
	u8 i = 0, j =0, k = 0;
	int err = 0;
	u8 regaddr = 0x00;
	u16 dataval = 0x0000;
	u8 writebuf[2] = {0};
	u8 read_buffer[FTS_RX_MAX * 2];
	if (ft_rawdata_scan() < 0) {
		dev_err(&client->dev, "%s() - scan raw data failed!\n", __func__);
		return err;
	}
	/*get rawdata*/
	dev_dbg(&client->dev, "%s() - Reading raw data...\n", __func__);
	for(i=0; i<tx; i++) {
		memset(read_buffer, 0x00, (FTS_RX_MAX * 2));
		writebuf[0] = FTS_RAW_READ_REG;
		writebuf[1] = i;
		err = ft5x0x_i2c_txdata(writebuf, 2);
		if (err < 0)
			return err;
		/* Read the data for this row */
		regaddr = FTS_RAW_BEGIN_REG;
		//err = ft5x0x_i2c_Read(client, &regaddr, 1, read_buffer, rx*2);
	    err = focal_ts_read(this_client,regaddr,read_buffer,rx*2); 

		if (err < 0)
			return err;
		k = 0;
		for (j = 0; j < rx*2; j += 2) {
			dataval  = read_buffer[j];
			dataval  = (dataval << 8);
			dataval |= read_buffer[j+1];
			rawdata[i][k] = dataval;
			k++;
        	}
	}

	return 0;
}
static ssize_t ft5x0x_rawdata_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	ssize_t num_read_chars = 0;
	u8 rx = 0, tx = 0;
	u8 i = 0, j = 0;
	int err = 0;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	/*entry factory*/
	err = ft5x0x_write_reg( FTS_DEVICE_MODE_REG, FTS_FACTORYMODE_VALUE);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	}

	/*get rx and tx num*/
	err = ft5x0x_read_reg(FTS_TXNUM_REG, &tx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:get tx error!\n", __func__);
		goto RAW_ERROR;
	}
	err = ft5x0x_read_reg( FTS_RXNUM_REG, &rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
			"%s:get rx error!\n", __func__);
		goto RAW_ERROR;
	}
	num_read_chars += sprintf(&(buf[num_read_chars]), 
			"tp channel: %u * %u\n", tx, rx);

	/*get rawdata*/
	err = ft5x0x_read_rawdata(client, before_rawdata, tx, rx);
	if (err < 0) {
		num_read_chars = sprintf(buf,
				"%s:rawdata show error!\n", __func__);
		goto RAW_ERROR;
	} else {
		for (i=0; i<tx; i++) {
			for (j=0; j<rx; j++) {
				num_read_chars += sprintf(&(buf[num_read_chars]),
						"%u ", before_rawdata[i][j]);
			}
			buf[num_read_chars-1] = '\n';
		}
	}
RAW_ERROR:
	/*enter work mode*/
	err = ft5x0x_write_reg(FTS_DEVICE_MODE_REG, FTS_WORKMODE_VALUE);
	if (err < 0)
		dev_err(&client->dev,
			"%s:enter work error!\n", __func__);
	msleep(100);
	return num_read_chars;
}

static ssize_t ft5x0x_rawdata_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{

	return -EPERM;
}
static DEVICE_ATTR(diag, S_IRUGO|S_IWUSR, ft5x0x_rawdata_show, ft5x0x_rawdata_store);
static struct kobject *android_touch_kobj;

static int focal_ts_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int ret = 0;
	unsigned char ver=0xff;
	struct focal_ts *ts;
       vreg4_init();

	printk(KERN_INFO "probing for focal_ts device %s at $%02X...\n", client->name, client->addr);
	if (!i2c_check_functionality(client->adapter,I2C_FUNC_I2C)) 
	{
		dev_err(&client->dev, "I2C functionality not supported\n");
		return -EIO;
	}
	ts = kzalloc(sizeof(struct focal_ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;
	
	ts->tpdev.client = client;
	this_client = client;
	i2c_set_clientdata(client, ts);
	ts->tpdev.pdata = client->dev.platform_data;
	ts->tpdev.work_func = focal_work_func;
	ts->use_irq = 1;
	ts->enable = 1;
	/*config WAKE UP pin */
	ret = gpio_request(ts->tpdev.pdata->wakeup,"focal wake up pin");
	if (ret<0)
		printk(KERN_ERR"focal wakeup gpio request failed\n");
	ret = gpio_tlmm_config(GPIO_CFG(ts->tpdev.pdata->wakeup,
              0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN,
              GPIO_CFG_4MA),
             GPIO_CFG_ENABLE);
	

	gpio_set_value(ts->tpdev.pdata->wakeup, 1);
	mdelay(20);
	gpio_set_value(ts->tpdev.pdata->wakeup, 0);
	mdelay(50);
	gpio_set_value(ts->tpdev.pdata->wakeup, 1);

	gpio_free(ts->tpdev.pdata->wakeup);


// [sun.yu5@byd.com, add,WG703T2,support 2 touchscreen,  upgrade the firmware]
	mdelay(100);
       focal_ts_get_ID(client);

     #if CFG_SUPPORT_AUTO_UPG
     fts_ctpm_auto_upg();
     #endif 

    //focal_ts_enable(ts);
//[sun.yu5@byd.com]
	ret = tp_work_func_register(&ts->tpdev);
	if(ret)
	{
		printk("tp_work_func_register error\n");
		goto err_work_func_register_failed;
	} 
	dev_set_drvdata(&ts->tpdev.input_dev->dev, ts);
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = focal_ts_early_suspend;
	ts->early_suspend.resume = focal_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	ret = focal_ts_read(client,0xa6, &ver, 1); //only read two fingers' data
	if (ret<0)
	{
		printk(KERN_ERR "%s: i2c_transfer failed\n", __func__);
	}
	printk(KERN_ERR " ver=%x\n",ver);
		
	android_touch_kobj = kobject_create_and_add("android_touch", NULL);
	if (android_touch_kobj == NULL) {
		printk(KERN_ERR "%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	ret = sysfs_create_file(android_touch_kobj, &dev_attr_diag.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	return 0;
err_work_func_register_failed:
	return ret;
}

static int focal_ts_remove(struct i2c_client *client)
{
	struct focal_ts *ts = i2c_get_clientdata(client);
	
	if (ts->use_irq)
		free_irq(client->irq, ts);
	kfree(ts);
	return 0;
}


static const struct i2c_device_id focal_ts_id[] = {
	{ "focal_ts", 0 },
	{ }
};

static struct i2c_driver focal_ts_driver = {
	.driver = {
		.name = "focal_ts",
		.owner = THIS_MODULE,
	},
	.probe		= focal_ts_probe,
	.remove		= focal_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= focal_ts_suspend,
	.resume		= focal_ts_resume,
#endif
	.id_table	= focal_ts_id,
};

static int __devinit focal_ts_init(void)
{
	return i2c_add_driver(&focal_ts_driver);
}

static void __exit focal_ts_exit(void)
{
	 tp_work_func_unregister();
	 i2c_del_driver(&focal_ts_driver);
}

module_init(focal_ts_init);
module_exit(focal_ts_exit);



MODULE_DESCRIPTION(" Focal touchscreen controller driver");
MODULE_LICENSE("GPL");

