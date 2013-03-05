/*
 * FAN53555 Fairchild Digitally Programmable TinyBuck Regulator Driver.
 *
 * Supported Part Numbers:
 * FAN53555UC00X/01X/03X/04X/05X
 *
 * Copyright (c) 2012 Marvell Technology Ltd.
 * Copyright (c) 2013, The Linux Foundation. All rights reserved.
 * Yunfan Zhang <yfzhang@marvell.com>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/param.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/regmap.h>
#include <linux/syscore_ops.h>
#include <linux/regulator/fan53555.h>
#include <linux/delay.h>

/* Voltage setting */
#define FAN53555_VSEL0		0x00
#define FAN53555_VSEL1		0x01
/* Control register */
#define FAN53555_CONTROL	0x02
/* IC Type */
#define FAN53555_ID1		0x03
/* IC mask version */
#define FAN53555_ID2		0x04
/* Monitor register */
#define FAN53555_MONITOR	0x05

/* VSEL bit definitions */
#define VSEL_BUCK_EN	(1 << 7)
#define VSEL_MODE		(1 << 6)
#define VSEL_NSEL_MASK	0x3F
/* Chip ID and Verison */
#define DIE_ID		0x0F	/* ID1 */
#define DIE_REV		0x0F	/* ID2 */
/* Control bit definitions */
#define CTL_OUTPUT_DISCHG	(1 << 7)
#define CTL_SLEW_MASK		(0x7 << 4)
#define CTL_SLEW_SHIFT		4
#define CTL_RESET			(1 << 2)
#define FAN53555_ENABLE			BIT(7)

#define FAN53555_NVOLTAGES	64	/* Numbers of voltages */
#define FAN53555_DEF_VTG_UV	1100000	/* Default voltage */

/* IC Type */
enum {
	FAN53555_CHIP_ID_00 = 0,
	FAN53555_CHIP_ID_01,
	FAN53555_CHIP_ID_02,
	FAN53555_CHIP_ID_03,
	FAN53555_CHIP_ID_04,
	FAN53555_CHIP_ID_05,
	FAN53555_CHIP_ID_09 = 12,
};

struct fan53555_device_info {
	struct regmap *regmap;
	struct device *dev;
	struct regulator_desc desc;
	struct regulator_dev *rdev;
	struct regulator_init_data *regulator;
	/* IC Type and Rev */
	int chip_id;
	int chip_rev;
	/* Voltage setting register */
	unsigned int vol_reg;
	unsigned int sleep_reg;
	/* Voltage range and step(linear) */
	unsigned int vsel_min;
	unsigned int vsel_step;
	/* Voltage slew rate limiting */
	unsigned int slew_rate;
	unsigned int slew_delay;
	/* Sleep voltage cache */
	unsigned int sleep_vol_cache;
	int curr_voltage;
	unsigned int vsel_ctrl_val;
	int restart_config_done;
	struct syscore_ops fan53555_syscore;
	struct mutex restart_lock;
};

static struct fan53555_device_info *fan53555;

static void dump_registers(struct fan53555_device_info *di,
			unsigned int reg, const char *func)
{
#ifdef DEBUG
	unsigned int val = 0;

	regmap_read(di->regmap, reg, &val);
	dev_dbg(di->dev, "%s: FAN53555: Reg = %x, Val = %x\n", func, reg, val);
#endif
}

static void fan53555_slew_delay(struct fan53555_device_info *di,
					int prev_uV, int new_uV)
{
	u8 val;
	int delay;

	val = abs(prev_uV - new_uV) / fan53555->vsel_step;
	delay =  ((val * di->slew_delay) / 1000) + 1;

	dev_dbg(di->dev, "Slew Delay = %d\n", delay);

	udelay(delay);
}

static void fan53555_restart_config(void)
{
	int rc, set_val;

	mutex_lock(&fan53555->restart_lock);

	set_val = DIV_ROUND_UP(FAN53555_DEF_VTG_UV - fan53555->vsel_min,
			fan53555->vsel_step);
	rc = regmap_update_bits(fan53555->regmap, fan53555->vol_reg,
			VSEL_NSEL_MASK, (set_val & VSEL_NSEL_MASK));
	if (rc)
		dev_err(fan53555->dev, "Unable to set volatge rc(%d)", rc);
	else
		udelay(20);

	fan53555->restart_config_done = true;

	mutex_unlock(&fan53555->restart_lock);
}

static int fan53555_enable(struct regulator_dev *rdev)
{
	int ret;
	unsigned int temp = 0;
	struct fan53555_device_info *di = rdev_get_drvdata(rdev);

	temp = di->vsel_ctrl_val & ~FAN53555_ENABLE;
	temp |= FAN53555_ENABLE;

	ret = regmap_write(di->regmap, di->vol_reg, temp);
	if (ret) {
		dev_err(di->dev, "Unable to enable regualtor rc(%d)", ret);
		return ret;
	}

	di->vsel_ctrl_val = temp;
	dump_registers(di, di->vol_reg, __func__);

	return ret;
}

static int fan53555_disable(struct regulator_dev *rdev)
{
	int ret;
	unsigned int temp = 0;
	struct fan53555_device_info *di = rdev_get_drvdata(rdev);

	temp = di->vsel_ctrl_val & ~FAN53555_ENABLE;

	ret = regmap_write(di->regmap, di->vol_reg, temp);
	if (ret) {
		dev_err(di->dev, "Unable to disable regualtor rc(%d)", ret);
		return ret;
	}

	di->vsel_ctrl_val = temp;
	dump_registers(di, di->vol_reg, __func__);

	return ret;
}

static int fan53555_get_voltage(struct regulator_dev *rdev)
{
	unsigned int val;
	int ret;
	struct fan53555_device_info *di = rdev_get_drvdata(rdev);

	ret = regmap_read(di->regmap, di->vol_reg, &val);
	if (ret) {
		dev_err(di->dev, "Unable to get volatge rc(%d)", ret);
		return ret;
	}
	di->curr_voltage = ((val & VSEL_NSEL_MASK) *
			di->vsel_step) + di->vsel_min;

	dump_registers(di, di->vol_reg, __func__);

	return di->curr_voltage;
}

static int fan53555_set_voltage(struct regulator_dev *rdev,
		int min_uV, int max_uV, unsigned *selector)
{
	struct fan53555_device_info *di = rdev_get_drvdata(rdev);
	int ret, set_val, new_uV;
	unsigned int temp = 0;

	mutex_lock(&di->restart_lock);
	/*
	 * Do not allow any other voltage transitions after
	 * restart configuration is done.
	 */
	if (di->restart_config_done) {
		dev_err(di->dev, "Restart config done. Cannot set volatage\n");
		ret = -EINVAL;
		goto err_set_vtg;
	}

	set_val = DIV_ROUND_UP(min_uV - di->vsel_min, di->vsel_step);
	new_uV = (set_val * di->vsel_step) + di->vsel_min;

	if (new_uV > (max_uV + di->vsel_step)) {
		dev_err(di->dev, "Unable to set volatge (%d %d)\n",
							min_uV, max_uV);
		ret = -EINVAL;
		goto err_set_vtg;
	}

	temp = di->vsel_ctrl_val & ~VSEL_NSEL_MASK;
	temp |= (set_val & VSEL_NSEL_MASK);

	ret = regmap_write(di->regmap, di->vol_reg, temp);
	if (ret) {
		dev_err(di->dev, "Unable to set volatge (%d %d)\n",
							min_uV, max_uV);
		goto err_set_vtg;
	} else {
		fan53555_slew_delay(di, di->curr_voltage, new_uV);
		di->curr_voltage = new_uV;
		di->vsel_ctrl_val = temp;
	}

	dump_registers(di, di->vol_reg, __func__);

err_set_vtg:
	mutex_unlock(&di->restart_lock);
	return ret;
}

static int fan53555_set_mode(struct regulator_dev *rdev, unsigned int mode)
{
	struct fan53555_device_info *di = rdev_get_drvdata(rdev);

	switch (mode) {
	case REGULATOR_MODE_FAST:
		regmap_update_bits(di->regmap, di->vol_reg,
				VSEL_MODE, VSEL_MODE);
		break;
	case REGULATOR_MODE_NORMAL:
		regmap_update_bits(di->regmap, di->vol_reg, VSEL_MODE, 0);
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static unsigned int fan53555_get_mode(struct regulator_dev *rdev)
{
	struct fan53555_device_info *di = rdev_get_drvdata(rdev);
	unsigned int val;
	int ret = 0;

	ret = regmap_read(di->regmap, di->vol_reg, &val);
	if (ret < 0)
		return ret;
	if (val & VSEL_MODE)
		return REGULATOR_MODE_FAST;
	else
		return REGULATOR_MODE_NORMAL;
}

static struct regulator_ops fan53555_regulator_ops = {
	.set_voltage = fan53555_set_voltage,
	.get_voltage = fan53555_get_voltage,
	.enable = fan53555_enable,
	.disable = fan53555_disable,
	.set_mode = fan53555_set_mode,
	.get_mode = fan53555_get_mode,
};

static struct regulator_desc rdesc = {
	.name = "fan53555-reg",
	.owner = THIS_MODULE,
	.n_voltages = FAN53555_NVOLTAGES,
	.ops = &fan53555_regulator_ops,
	.type = REGULATOR_VOLTAGE,
};

/* For 00,01,03,05 options:
 * VOUT = 0.60V + NSELx * 10mV, from 0.60 to 1.23V.
 * For 04 option:
 * VOUT = 0.603V + NSELx * 12.826mV, from 0.603 to 1.411V.
 * */
static int fan53555_device_setup(struct fan53555_device_info *di,
				struct fan53555_platform_data *pdata)
{
	unsigned int reg, data, mask, val;
	int ret;

	/* Setup voltage control register */
	switch (pdata->sleep_vsel_id) {
	case FAN53555_VSEL_ID_0:
		di->sleep_reg = FAN53555_VSEL0;
		di->vol_reg = FAN53555_VSEL1;
		break;
	case FAN53555_VSEL_ID_1:
		di->sleep_reg = FAN53555_VSEL1;
		di->vol_reg = FAN53555_VSEL0;
		break;
	default:
		dev_err(di->dev, "Invalid VSEL ID!\n");
		return -EINVAL;
	}
	/* Init voltage range and step */
	switch (di->chip_id) {
	case FAN53555_CHIP_ID_00:
	case FAN53555_CHIP_ID_01:
	case FAN53555_CHIP_ID_03:
	case FAN53555_CHIP_ID_05:
		di->vsel_min = 600000;
		di->vsel_step = 10000;
		break;
	case FAN53555_CHIP_ID_04:
	case FAN53555_CHIP_ID_09:
		di->vsel_min = 603000;
		di->vsel_step = 12826;
		break;
	default:
		dev_err(di->dev,
			"Chip ID[%d]\n not supported!\n", di->chip_id);
		return -EINVAL;
	}

	/* get the current programmed voltage */
	ret = regmap_read(di->regmap, di->vol_reg, &val);
	if (ret) {
		dev_err(di->dev, "Unable to get volatge rc(%d)", ret);
		return ret;
	}
	di->vsel_ctrl_val = val;
	di->curr_voltage = ((val & VSEL_NSEL_MASK) * di->vsel_step)
						+ di->vsel_min;

	/* Init slew rate */
	if (pdata->slew_rate & 0x7) {
		di->slew_rate = pdata->slew_rate;
		di->slew_delay = (di->vsel_step >> 6) * (1 << di->slew_rate);
	} else {
		di->slew_rate = FAN53555_SLEW_RATE_64MV;
		di->slew_delay = 200;
	}

	reg = FAN53555_CONTROL;
	data = di->slew_rate << CTL_SLEW_SHIFT;
	mask = CTL_SLEW_MASK;

	return regmap_update_bits(di->regmap, reg, mask, data);
}

static struct regmap_config fan53555_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int __devinit fan53555_regulator_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct fan53555_device_info *di;
	struct fan53555_platform_data *pdata;
	unsigned int val;
	int ret;

	pdata = client->dev.platform_data;
	if (!pdata || !pdata->regulator) {
		dev_err(&client->dev, "Platform data not found!\n");
		return -ENODEV;
	}

	di = devm_kzalloc(&client->dev, sizeof(struct fan53555_device_info),
					GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "Failed to allocate device info data!\n");
		return -ENOMEM;
	}
	di->regmap = devm_regmap_init_i2c(client, &fan53555_regmap_config);
	if (IS_ERR(di->regmap)) {
		dev_err(&client->dev, "Failed to allocate regmap!\n");
		return PTR_ERR(di->regmap);
	}
	di->dev = &client->dev;
	di->regulator = pdata->regulator;
	i2c_set_clientdata(client, di);
	di->restart_config_done = false;
	/* Get chip ID */
	ret = regmap_read(di->regmap, FAN53555_ID1, &val);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to get chip ID!\n");
		return -ENODEV;
	}
	di->chip_id = val & DIE_ID;
	/* Get chip revision */
	ret = regmap_read(di->regmap, FAN53555_ID2, &val);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to get chip Rev!\n");
		return -ENODEV;
	}
	di->chip_rev = val & DIE_REV;
	dev_info(&client->dev, "FAN53555 Option[%d] Rev[%d] Detected!\n",
				di->chip_id, di->chip_rev);
	mutex_init(&di->restart_lock);
	/* Device init */
	ret = fan53555_device_setup(di, pdata);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to setup device!\n");
		return ret;
	}
	/* Register regulator */
	di->rdev = regulator_register(&rdesc, &client->dev,
					di->regulator, di, NULL);
	if (IS_ERR(di->rdev)) {
		dev_err(&client->dev, "Unable to register regulator rc(%ld)",
						PTR_ERR(di->regulator));
		return PTR_ERR(di->regulator);
	}

	fan53555 = di;
	/*
	 * Register for the syscore shutdown hook. This is to make sure
	 * that the buck voltage is set to default before restart.
	 */
	di->fan53555_syscore.shutdown = fan53555_restart_config;
	register_syscore_ops(&di->fan53555_syscore);

	dump_registers(di, FAN53555_VSEL0, __func__);
	dump_registers(di, FAN53555_VSEL1, __func__);
	dump_registers(di, FAN53555_CONTROL, __func__);
	dump_registers(di, FAN53555_ID1, __func__);
	dump_registers(di, FAN53555_ID2, __func__);
	dump_registers(di, FAN53555_MONITOR, __func__);

	return ret;

}

static int __devexit fan53555_regulator_remove(struct i2c_client *client)
{
	struct fan53555_device_info *di = i2c_get_clientdata(client);

	regulator_unregister(di->rdev);
	unregister_syscore_ops(&di->fan53555_syscore);
	mutex_destroy(&di->restart_lock);

	return 0;
}

static const struct i2c_device_id fan53555_id[] = {
	{"fan53555", -1},
	{ },
};

static struct i2c_driver fan53555_regulator_driver = {
	.driver = {
		.name = "fan53555-regulator",
	},
	.probe = fan53555_regulator_probe,
	.remove = __devexit_p(fan53555_regulator_remove),
	.id_table = fan53555_id,
};

static int __init fan53555_regulator_init(void)
{
	return i2c_add_driver(&fan53555_regulator_driver);
}
/* Revisit the code to decide the actual sequence */
fs_initcall(fan53555_regulator_init);

static void __exit fan53555_regulator_exit(void)
{
	i2c_del_driver(&fan53555_regulator_driver);
}
module_exit(fan53555_regulator_exit);


MODULE_AUTHOR("Yunfan Zhang <yfzhang@marvell.com>");
MODULE_DESCRIPTION("FAN53555 regulator driver");
MODULE_LICENSE("GPL v2");
