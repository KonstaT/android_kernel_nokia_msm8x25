/* Copyright (c) 2013, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */


#include <mach/socinfo.h>
#include <linux/gpio.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_hx8392a.h"

static struct msm_panel_common_pdata *mipi_hx8392a_pdata;
static struct dsi_buf hx8392a_tx_buf;
static struct dsi_buf hx8392a_rx_buf;

static int mipi_hx8392a_bl_ctrl = 0;

/* common setting */
static char exit_sleep[2] = {0x11, 0x00};
static char display_on[2] = {0x29, 0x00};
static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

/* panel setting */
static char video0[] = {
	0xB9,0xFF,0x83,0x92,
};

static char video1[] = {
	0x35,0x00
};

//Set Power
static char video2[] = {
	0xB1,0x7C,0x00,0x44,
	0x14,0x00,0x11,0x11,
	0x24,0x2C,0x3F,0x3F,
	0x42,0x72,
};

//SET MPU/Command CYC
static char video3[] = {
	0xB4,0x00,0x00,0x05,
	0x00,0xA0,0x05,0x16,
	0x9D,0x30,0x03,0x16,
	0x00,0x03,0x03,0x00,
	0x1B,0x04,0x07,0x07,
	0x01,0x00,0x1A,0x77,
};

//VCOMDC
static char video4[] = {
	0xB6,0x70,
};

static char video5[] = {
	0xBA,0x13,0x83,
};

//0x08->Command Mode
static char video6[] = {0xC2,0x03,};

static char video7[] = {
	0xBF,0x05,0xE0,0x02,
	0x00,
};

static char video8[] = {0xC0,0x01,0x94,};

static char video9[] = {
	0xC6,0x45,0x02,0x10,
	0x04,
};

static char video10[] = {
	0xC7,0x00,0x40,
};

//SETPANEL
static char video11[] = {0xCC,0x08,};

//Set EQ function for LTPS
static char video12[] = {0xD4,0x0C,};

//Set LTPS control output
static char video13[] = {
	0xD5,0x00,0x08,0x08,
	0x00,0x44,0x55,0x66,
	0x77,0xCC,0xCC,0xCC,
	0xCC,0x00,0x77,0x66,
	0x55,0x44,0xCC,0xCC,
	0xCC,0xCC,
};

//SET RGB/Video CYC
static char video14[] = {
	0xD8,0x00,0x00,0x04,
	0x00,0xa0,0x04,0x16,
	0x9d,0x30,0x03,0x16,
	0x00,0x03,0x03,0x00,
	0x1B,0x04,0x07,0x07,
	0x01,0x00,0x1A,0x77,
};

//R Gamma
static char video15[] = {
	0xE0,0x03,0x1B,0x22,
	0x3B,0x3B,0x3F,0x2B,
	0x47,0x05,0x0B,0x0E,
	0x10,0x13,0x11,0x12,
	0x11,0x1A,0x03,0x1B,
	0x22,0x3B,0x3B,0x3F,
	0x2B,0x47,0x05,0x0B,
	0x0E,0x10,0x13,0x11,
	0x12,0x11,0x1A,
};

//G Gamma
static char video16[] = {
	0xE1,0x03,0x1B,0x22,
	0x3B,0x3B,0x3F,0x2B,
	0x47,0x05,0x0B,0x0E,
	0x10,0x13,0x11,0x12,
	0x11,0x1A,0x03,0x1B,
	0x22,0x3B,0x3B,0x3F,
	0x2B,0x47,0x05,0x0B,
	0x0E,0x10,0x13,0x11,
	0x12,0x11,0x1A,
};

//B Gamma
static char video17[] = {
	0xE2,0x03,0x1B,0x22,
	0x3B,0x3B,0x3F,0x2B,
	0x47,0x05,0x0B,0x0E,
	0x10,0x13,0x11,0x12,
	0x11,0x1A,0x03,0x1B,
	0x22,0x3B,0x3B,0x3F,
	0x2B,0x47,0x05,0x0B,
	0x0E,0x10,0x13,0x11,
	0x12,0x11,0x1A,
};

//Set display related register
static char video18[] = {
	0xB2,0x08,0xC8,0x05,
	0x0F,0x08,0x44,0x00,
	0xFF,0x05,0x0F,0x04,
	0x20,
};


/* hx8392a must use DCS commands */
static struct dsi_cmd_desc hx8392a_video_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video0), video0},
	{DTYPE_DCS_WRITE,  1, 0, 0, 150, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video1), video1},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video2), video2},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video3), video3},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video4), video4},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video5), video5},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video6), video6},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video7), video7},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video8), video8},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video9), video9},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video10), video10},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video11), video11},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video12), video12},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video13), video13},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video14), video14},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video15), video15},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video16), video16},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video17), video17},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video18), video18},
	{DTYPE_DCS_WRITE,  1, 0, 0, 120, sizeof(display_on), display_on},
};

static struct dsi_cmd_desc hx8392a_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 10, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static int mipi_hx8392a_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	pr_debug("mipi_hx8392a_lcd_on E\n");

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi  = &mfd->panel_info.mipi;

	if (!mfd->cont_splash_done) {
		mfd->cont_splash_done = 1;
		return 0;
	}

	if (mipi->mode == DSI_VIDEO_MODE) {
		mipi_dsi_cmds_tx(&hx8392a_tx_buf,
			hx8392a_video_display_on_cmds,
			ARRAY_SIZE(hx8392a_video_display_on_cmds));
	}

	pr_debug("mipi_hx8392a_lcd_on X\n");

	return 0;
}

static int mipi_hx8392a_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("mipi_hx8392a_lcd_off E\n");

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&hx8392a_tx_buf, hx8392a_display_off_cmds,
			ARRAY_SIZE(hx8392a_display_off_cmds));

	pr_debug("mipi_hx8392a_lcd_off X\n");
	return 0;
}

static ssize_t mipi_hx8392a_wta_bl_ctrl(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int err;

	err =  kstrtoint(buf, 0, &mipi_hx8392a_bl_ctrl);
	if (err)
		return ret;

	pr_info("%s: bl ctrl set to %d\n", __func__, mipi_hx8392a_bl_ctrl);

	return ret;
}

static DEVICE_ATTR(bl_ctrl, S_IWUSR, NULL, mipi_hx8392a_wta_bl_ctrl);

static struct attribute *mipi_hx8392a_fs_attrs[] = {
	&dev_attr_bl_ctrl.attr,
	NULL,
};

static struct attribute_group mipi_hx8392a_fs_attr_group = {
	.attrs = mipi_hx8392a_fs_attrs,
};

static int mipi_hx8392a_create_sysfs(struct platform_device *pdev)
{
	int rc;
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	if (!mfd) {
		pr_err("%s: mfd not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi) {
		pr_err("%s: mfd->fbi not found\n", __func__);
		return -ENODEV;
	}
	if (!mfd->fbi->dev) {
		pr_err("%s: mfd->fbi->dev not found\n", __func__);
		return -ENODEV;
	}
	rc = sysfs_create_group(&mfd->fbi->dev->kobj,
		&mipi_hx8392a_fs_attr_group);
	if (rc) {
		pr_err("%s: sysfs group creation failed, rc=%d\n",
			__func__, rc);
		return rc;
	}

	return 0;
}

static int __devinit mipi_hx8392a_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_hx8392a_pdata = pdev->dev.platform_data;
		if (mipi_hx8392a_pdata->bl_lock)
			spin_lock_init(&mipi_hx8392a_pdata->bl_spinlock);

                /* SKUD use PWM as backlight control method */
                if(machine_is_msm8625q_skud() || machine_is_msm8625q_evbd()) {
                        mipi_hx8392a_bl_ctrl = 1;
                }

		return 0;
	}

	pthisdev = msm_fb_add_device(pdev);
	mipi_hx8392a_create_sysfs(pthisdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_hx8392a_lcd_probe,
	.driver = {
		.name   = "mipi_hx8392a",
	},
};

static int old_bl_level;

static void mipi_hx8392a_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;
	unsigned long flags;
	bl_level = mfd->bl_level;

	if (mipi_hx8392a_pdata->bl_lock) {
		if (!mipi_hx8392a_bl_ctrl) {
			/* Level received is of range 1 to bl_max,
			   We need to convert the levels to 1
			   to 31 */
			bl_level = (2 * bl_level * 31 + mfd->panel_info.bl_max)
					/(2 * mfd->panel_info.bl_max);
			if (bl_level == old_bl_level)
				return;

			if (bl_level == 0)
				mipi_hx8392a_pdata->backlight(0, 1);

			if (old_bl_level == 0)
				mipi_hx8392a_pdata->backlight(50, 1);

			spin_lock_irqsave(&mipi_hx8392a_pdata->bl_spinlock,
						flags);
			mipi_hx8392a_pdata->backlight(bl_level, 0);
			spin_unlock_irqrestore(&mipi_hx8392a_pdata->bl_spinlock,
						flags);
			old_bl_level = bl_level;
		} else {
			mipi_hx8392a_pdata->backlight(bl_level, 1);
		}
	} else {
		mipi_hx8392a_pdata->backlight(bl_level, mipi_hx8392a_bl_ctrl);
	}
}

static struct msm_fb_panel_data hx8392a_panel_data = {
	.on	= mipi_hx8392a_lcd_on,
	.off = mipi_hx8392a_lcd_off,
	.set_backlight = mipi_hx8392a_set_backlight,
};

static int ch_used[3];

static int mipi_hx8392a_lcd_init(void)
{
	mipi_dsi_buf_alloc(&hx8392a_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&hx8392a_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_hx8392a_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_hx8392a_lcd_init();
	if (ret) {
		pr_err("mipi_hx8392a_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_hx8392a", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	hx8392a_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &hx8392a_panel_data,
				sizeof(hx8392a_panel_data));
	if (ret) {
		pr_debug("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_debug("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}
