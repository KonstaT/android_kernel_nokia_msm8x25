/*
	$License:
	Copyright (C) 2011 InvenSense Corporation, All Rights Reserved.

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License version 2 as
	published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
	$
 */

/**
 *  @addtogroup COMPASSDL
 *
 *  @{
 *      @file   mmc328xms.c
 *      @brief  Magnetometer setup and handling methods for Memsic MMC328XMS
 *              compass.
 */

/* ------------------ */
/* - Include Files. - */
/* ------------------ */

#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "mpu-dev.h"

#include <log.h>
#include <linux/mpu.h>
#include "mlsl.h"
#include "mldl_cfg.h"
#undef MPL_LOG_TAG
#define MPL_LOG_TAG "MPL-compass"

/* --------------------- */
/* -    Variables.     - */
/* --------------------- */

static int reset_int = 1000;
static int read_count = 1;

#define MMC328XMS_REG_CTL0            (0x07)
#define MMC328XMS_REG_ST              (0x06)
#define MMC328XMS_REG_X_LSB           (0x00)

#define MMC328XMS_VAL_TM_READY        (0x01)
#define MMC328XMS_VAL_RM_MAG          (0x20)
#define MMC328XMS_VAL_RRM_MAG         (0x40)
#define MMC328XMS_CNTL_MODE_WAKE_UP   (0x01)


/*****************************************
    Compass Initialization Functions
*****************************************/

int mmc328xms_suspend(void *mlsl_handle,
		    struct ext_slave_descr *slave,
		    struct ext_slave_platform_data *pdata)
{
	int result = INV_SUCCESS;
	// after data transfer, MMC sleeps automatically

	return result;
}

int mmc328xms_resume(void *mlsl_handle,
		   struct ext_slave_descr *slave,
		   struct ext_slave_platform_data *pdata)
{

	int result;
	result = inv_serial_single_write(mlsl_handle,
                                     pdata->address,
                                     MMC328XMS_REG_CTL0,
                                     MMC328XMS_VAL_RM_MAG);
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}
	//instruct a measurement
	result = inv_serial_single_write(mlsl_handle,
			pdata->address,
			MMC328XMS_REG_CTL0,
			MMC328XMS_CNTL_MODE_WAKE_UP);
	msleep(10);

	read_count = 1;
	return INV_SUCCESS;
}

int mmc328xms_read(void *mlsl_handle,
		 struct ext_slave_descr *slave,
		 struct ext_slave_platform_data *pdata,
		 unsigned char *data)
{

	int result;
	unsigned char status = 0;
	int md_times = 0;
	if (read_count > 1000) {
		read_count = 1;
	}
	//printk("enter mmc328xms_read\n");
	//wait for the measurement is done
    do{
        result = inv_serial_read(mlsl_handle,
                                 pdata->address,
                                 MMC328XMS_REG_ST,
                                 1, &status);
	//printk("pass inv_serial_read MMC328XMS_REG_ST\n");
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}
		md_times++;
		if (md_times > 3) {
			printk("mmc328x tried 3 times, failed");
			return INV_ERROR_COMPASS_DATA_NOT_READY;
    		}
		if (md_times > 1)
			msleep(1);
    } while ((status & MMC328XMS_VAL_TM_READY) != MMC328XMS_VAL_TM_READY);
	//printk("mmc328xms_read pass do while wait\n");
	result =
	    inv_serial_read(mlsl_handle, pdata->address, MMC328XMS_REG_X_LSB,
			    6, (unsigned char *)data);
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}
//    printk("memsic raw data is %d, %d, %d,  \n",
//   		 data[0] | (data[1]<<8),data[2] | (data[3]<<8), data[4] | (data[5]<<8));
	//manipulate...
	{
        short tmp[3];
        unsigned char tmpdata[6];
        int ii;

        for (ii = 0; ii < 6; ii++)
            tmpdata[ii] = data[ii];

        for (ii = 0; ii < 3; ii++) {
            tmp[ii] = (short)((tmpdata[2 * ii + 1 ] << 8) + tmpdata[2 * ii]);
            tmp[ii] = tmp[ii] - 4096;
            tmp[ii] = tmp[ii] * 16;
        }

        for (ii = 0; ii < 3; ii++) {
            data[2 * ii] = (unsigned char)((tmp[ii] >> 8) & 0xFF);
            data[2 * ii + 1] = (unsigned char)((tmp[ii]) & 0xFF);
        }
	//printk("caculated memsic data is %d, %d, %d \n", tmp[0],tmp[1],tmp[2]);

    }

	read_count++;

    // reset the RM periodically
	if (read_count % reset_int == 0) {
		/* 1st magnetization */
		result = inv_serial_single_write(mlsl_handle,
					pdata->address,
					MMC328XMS_REG_CTL0,
					MMC328XMS_VAL_RM_MAG);
		if (result) {
			LOG_RESULT_LOCATION(result);
			return result;
		}
		// a minimum of 50ms wait should be given to MEMSIC device
		// to finish the preparation for 2nd magnetization action
		msleep(50);
		/* 2nd magnetization */
		result = inv_serial_single_write(mlsl_handle,
					pdata->address,
					MMC328XMS_REG_CTL0,
					MMC328XMS_VAL_RRM_MAG);
		if (result) {
			LOG_RESULT_LOCATION(result);
			return result;
		}
		// a minimum of 100ms wait should be given to MEMSIC device
		// to finish magnetization action before taking a measurement
		msleep(100);
	}

	//instruct a measurement
	result = inv_serial_single_write(mlsl_handle,
			pdata->address,
			MMC328XMS_REG_CTL0,
			MMC328XMS_CNTL_MODE_WAKE_UP);
	if (result) {
		LOG_RESULT_LOCATION(result);
		return result;
	}

	//printk("mmc328xms_read exit\n");
	return INV_SUCCESS;
}

struct ext_slave_descr mmc328xms_descr = {
	/*.init             = */ NULL,
	/*.exit             = */ NULL,
	/*.suspend          = */ mmc328xms_suspend,
	/*.resume           = */ mmc328xms_resume,
	/*.read             = */ mmc328xms_read,
	/*.config           = */ NULL,
	/*.get_config       = */ NULL,
	/*.name             = */ "mmc328xms",
	/*.type             = */ EXT_SLAVE_TYPE_COMPASS,
	/*.id               = */ COMPASS_ID_MMC328XMS,
	/*.reg              = */ 0x01,
	/*.len              = */ 6,
	/*.endian           = */ EXT_SLAVE_BIG_ENDIAN,
	/*.range            = */ {400, 0},
	/*.trigger          = */ NULL,
};

struct ext_slave_descr *mmc328xms_get_slave_descr(void)
{
	return &mmc328xms_descr;
}
/* -------------------------------------------------------------------------- */
struct mmc328xms_mod_private_data {
	struct i2c_client *client;
	struct ext_slave_platform_data *pdata;
};

static unsigned short normal_i2c[] = { I2C_CLIENT_END };

static int mmc328xms_mod_probe(struct i2c_client *client,
			   const struct i2c_device_id *devid)
{
	struct ext_slave_platform_data *pdata;
	struct mmc328xms_mod_private_data *private_data;
	int result = 0;

	dev_info(&client->adapter->dev, "%s: %s\n", __func__, devid->name);

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		result = -ENODEV;
		goto out_no_free;
	}

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->adapter->dev,
			"Missing platform data for slave %s\n", devid->name);
		result = -EFAULT;
		goto out_no_free;
	}

	private_data = kzalloc(sizeof(*private_data), GFP_KERNEL);
	if (!private_data) {
		result = -ENOMEM;
		goto out_no_free;
	}

	i2c_set_clientdata(client, private_data);
	private_data->client = client;
	private_data->pdata = pdata;

	result = inv_mpu_register_slave(THIS_MODULE, client, pdata,
					mmc328xms_get_slave_descr);
	if (result) {
		dev_err(&client->adapter->dev,
			"Slave registration failed: %s, %d\n",
			devid->name, result);
		goto out_free_memory;
	}

	return result;

out_free_memory:
	kfree(private_data);
out_no_free:
	dev_err(&client->adapter->dev, "%s failed %d\n", __func__, result);
	return result;

}

static int mmc328xms_mod_remove(struct i2c_client *client)
{
	struct mmc328xms_mod_private_data *private_data =
		i2c_get_clientdata(client);

	dev_dbg(&client->adapter->dev, "%s\n", __func__);

	inv_mpu_unregister_slave(client, private_data->pdata,
				mmc328xms_get_slave_descr);

	kfree(private_data);
	return 0;
}

static const struct i2c_device_id mmc328xms_mod_id[] = {
	{ "mmc328xms", COMPASS_ID_MMC328XMS },
	{}
};

MODULE_DEVICE_TABLE(i2c, mmc328xms_mod_id);

static struct i2c_driver mmc328xms_mod_driver = {
	.class = I2C_CLASS_HWMON,
	.probe = mmc328xms_mod_probe,
	.remove = mmc328xms_mod_remove,
	.id_table = mmc328xms_mod_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "mmc328xms_mod",
		   },
	.address_list = normal_i2c,
};

static int __init mmc328xms_mod_init(void)
{
	int res = i2c_add_driver(&mmc328xms_mod_driver);
	pr_info("%s: Probe name %s\n", __func__, "mmc328xms_mod");
	if (res)
		pr_err("%s failed\n", __func__);
	return res;
}

static void __exit mmc328xms_mod_exit(void)
{
	pr_info("%s\n", __func__);
	i2c_del_driver(&mmc328xms_mod_driver);
}

module_init(mmc328xms_mod_init);
module_exit(mmc328xms_mod_exit);

MODULE_AUTHOR("Invensense Corporation");
MODULE_DESCRIPTION("Driver to integrate MMC328XMS sensor with the MPU");
MODULE_LICENSE("GPL");
MODULE_ALIAS("mmc328xms_mod");

/**
 *  @}
 */
