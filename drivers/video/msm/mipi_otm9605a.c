/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#define DEBUG

#include <mach/socinfo.h>
#include <linux/gpio.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_otm9605a.h"

static struct msm_panel_common_pdata *mipi_otm9605a_pdata;
static struct dsi_buf otm9605a_tx_buf;
static struct dsi_buf otm9605a_rx_buf;

static char otm9605a_video0[2]={
        0x00,0x00
};

static char otm9605a_video1[4]={
        0xFF,0x96,0x05,0x01
};

static char otm9605a_video2[2]={
        0x00,0x80
};

static char otm9605a_video3[3]={
        0xFF,0x96,0x05
};

static char otm9605a_video4[2]={
        0x00,0xB1
};

static char otm9605a_video5[2]={
        0xC5,0xA9
};

static char otm9605a_video6[2]={
        0x21,0x00
};

static char otm9605a_video7[2]={
        0x00,0x92
};
static char otm9605a_video8[3]={
        0xFF,0x10,0x02
};

static char otm9605a_video9[2]={
        0x00,0x91
};

static char otm9605a_video10[2]={
        0xC5,0x76
};

static char otm9605a_video11[2]={
        0x00,0x00
};

static char otm9605a_video12[3]={
        0xD8,0x6F,0x6F
};

static char otm9605a_video13[2]={
        0x00,0x00
};

static char otm9605a_video14[2]={
        0xD9,0x24
};

static char otm9605a_video15[2]={
        0x00,0xA6
};

static char otm9605a_video16[2]={
        0xC1,0x01
};

static char otm9605a_video17[2]={
        0x00,0xB4
};

static char otm9605a_video18[2]={
        0xC0,0x50
};

static char otm9605a_video19[2]={
        0x00,0x80
};

static char otm9605a_video20[3]={
        0xC1,0x36,0x55
};

static char otm9605a_video21[2]={
        0x00,0x89
};

static char otm9605a_video22[2]={
        0xC0,0x01
};

static char otm9605a_video23[2]={
        0x00,0x80
};

static char otm9605a_video24[11]={
        0xCB,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00
};

static char otm9605a_video25[2]={
        0x00,0x90
};

static char otm9605a_video26[16]={
        0xCB,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00
};

static char otm9605a_video27[2]={
        0x00,0xA0
};

static char otm9605a_video28[16]={
        0xCB,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00
};

static char otm9605a_video29[2]={
        0x00,0xB0
};

static char otm9605a_video30[11]={
        0xCB,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00
};

static char otm9605a_video31[2]={
        0x00,0xC0
};

static char otm9605a_video32[16]={
        0xCB,0x00,0x00,0x04,
        0x04,0x04,0x04,0x00,
        0x00,0x04,0x04,0x04,
        0x04,0x00,0x00,0x00
};

static char otm9605a_video33[2]={
        0x00,0xD0
};

static char otm9605a_video34[16]={
        0xCB,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x04,0x04,0x04,0x04,
        0x00,0x00,0x04,0x04
};

static char otm9605a_video35[2]={
        0x00,0xE0
};
static char otm9605a_video36[11]={
        0xCB,0x04,0x04,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00
};

static char otm9605a_video37[2]={
        0x00,0xF0
};

static char otm9605a_video38[11]={
        0xCB,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF,0xFF,
        0xFF,0xFF,0xFF
};

static char otm9605a_video39[2]={
        0x00,0x80
};

static char otm9605a_video40[11]={
        0xCC,0x00,0x00,0x26,
        0x25,0x02,0x06,0x00,
        0x00,0x0A,0x0E
};

static char otm9605a_video41[2]={
        0x00,0x90
};

static char otm9605a_video42[16]={
        0xCC,0x0C,0x10,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x26,0x25,0x01
};

static char otm9605a_video43[2]={
        0x00,0xA0
};

static char otm9605a_video44[16]={
        0xCC,0x05,0x00,0x00,
        0x09,0x0D,0x0B,0x0F,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00
};

static char otm9605a_video45[2]={
        0x00,0xB0
};

static char otm9605a_video46[11]={
        0xCC,0x00,0x00,0x25,
        0x26,0x05,0x01,0x00,
        0x00,0x0F,0x0B
};

static char otm9605a_video47[2]={
        0x00,0xC0
};

static char otm9605a_video48[16]={
        0xCC,0x0D,0x09,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,
        0x00,0x25,0x26,0x06
};

static char otm9605a_video49[2]={
        0x00,0xD0
};

static char otm9605a_video50[16]={
        0xCC,0x02,0x00,0x00,
        0x10,0x0C,0x0E,0x0A,
        0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00
};

static char otm9605a_video51[2]={
        0x00,0x80
};

static char otm9605a_video52[13]={
        0xCE,0x87,0x03,0x28,
        0x86,0x03,0x28,0x00,
        0x0F,0x00,0x00,0x0F,
        0x00
};

static char otm9605a_video53[2]={
        0x00,0x90
};

static char otm9605a_video54[15]={
        0xCE,0x33,0xBE,0x28,
        0x33,0xBF,0x28,0xF0,
        0x00,0x00,0xF0,0x00,
        0x00,0x00,0x00
};

static char otm9605a_video55[2]={
        0x00,0xA0
};

static char otm9605a_video56[15]={
        0xCE,0x38,0x03,0x83,
        0xC0,0x8A,0x18,0x00,
        0x38,0x02,0x83,0xC1,
        0x89,0x18,0x00
};

static char otm9605a_video57[2]={
        0x00,0xB0
};

static char otm9605a_video58[15]={
        0xCE,0x38,0x01,0x83,
        0xC2,0x88,0x18,0x00,
        0x38,0x00,0x83,0xC3,
        0x87,0x18,0x00
};

static char otm9605a_video59[2]={
        0x00,0xC0
};

static char otm9605a_video60[15]={
        0xCE,0x30,0x00,0x83,
        0xC4,0x86,0x18,0x00,
        0x30,0x01,0x83,0xC5,
        0x85,0x18,0x00
};

static char otm9605a_video61[2]={
        0x00,0xD0
};

static char otm9605a_video62[15]={
        0xCE,0x30,0x02,0x83,
        0xC6,0x84,0x18,0x00,
        0x30,0x03,0x83,0xC7,
        0x83,0x18,0x00
};

static char otm9605a_video63[2]={
        0x00,0xC0
};

static char otm9605a_video64[11]={
        0xCF,0x01,0x01,0x20,
        0x20,0x00,0x00,0x01,
        0x01,0x00,0x00
};

static char otm9605a_video65[2]={
        0x00,0x00
};

static char otm9605a_video66[17]={
	0xE1,0x00,0x03,0x0A,
	0x10,0x07,0x1F,0x0D,
	0x0D,0x01,0x05,0x02,
	0x05,0x0D,0x24,0x20,
	0x03
};

static char otm9605a_video67[2]={
        0x00,0x00
};

static char otm9605a_video68[17]={
	0xE2,0x00,0x03,0x0A,
	0x10,0x07,0x1F,0x0D,
	0x0D,0x01,0x05,0x02,
	0x05,0x0D,0x24,0x20,
	0x03
};

static char otm9605a_video69[2]={
        0x00,0x80
};

static char otm9605a_video70[2]={
        0xD6,0x88
};

static char otm9605a_exit_sleep[2] = {
        0x11, 0x00,
};

static char otm9605a_display_on[2] = {
        0x29, 0x00,
};

static struct dsi_cmd_desc otm9605a_video_display_on_cmds[] = {
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video0), otm9605a_video0},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video1), otm9605a_video1},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video2), otm9605a_video2},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video3), otm9605a_video3},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video4), otm9605a_video4},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video5), otm9605a_video5},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video6), otm9605a_video6},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video7), otm9605a_video7},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video8), otm9605a_video8},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video9), otm9605a_video9},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video10), otm9605a_video10},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video11), otm9605a_video11},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video12), otm9605a_video12},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video13), otm9605a_video13},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video14), otm9605a_video14},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video15), otm9605a_video15},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video16), otm9605a_video16},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video17), otm9605a_video17},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video18), otm9605a_video18},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video19), otm9605a_video19},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video20), otm9605a_video20},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video21), otm9605a_video21},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video22), otm9605a_video22},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video23), otm9605a_video23},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video24), otm9605a_video24},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video25), otm9605a_video25},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video26), otm9605a_video26},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video27), otm9605a_video27},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video28), otm9605a_video28},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video29), otm9605a_video29},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video30), otm9605a_video30},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video31), otm9605a_video31},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video32), otm9605a_video32},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video33), otm9605a_video33},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video34), otm9605a_video34},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video35), otm9605a_video35},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video36), otm9605a_video36},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video37), otm9605a_video37},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video38), otm9605a_video38},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video39), otm9605a_video39},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video40), otm9605a_video40},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video41), otm9605a_video41},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video42), otm9605a_video42},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video43), otm9605a_video43},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video44), otm9605a_video44},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video45), otm9605a_video45},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video46), otm9605a_video46},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video47), otm9605a_video47},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video48), otm9605a_video48},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video49), otm9605a_video49},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video50), otm9605a_video50},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video51), otm9605a_video51},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video52), otm9605a_video52},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video53), otm9605a_video53},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video54), otm9605a_video54},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video55), otm9605a_video55},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video56), otm9605a_video56},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video57), otm9605a_video57},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video58), otm9605a_video58},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video59), otm9605a_video59},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video60), otm9605a_video60},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video61), otm9605a_video61},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video62), otm9605a_video62},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video63), otm9605a_video63},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video64), otm9605a_video64},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video65), otm9605a_video65},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video66), otm9605a_video66},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video67), otm9605a_video67},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video68), otm9605a_video68},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video69), otm9605a_video69},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(otm9605a_video70), otm9605a_video70},

        {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(otm9605a_exit_sleep), otm9605a_exit_sleep},
        {DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(otm9605a_display_on), otm9605a_display_on},
};

static char otm9605a_set_sleepin[2] = {0x28, 0x00};
static char otm9605a_set_disp_off[2] = {0x10, 0x00};

static struct dsi_cmd_desc otm9605a_video_display_off_cmds[] = {
        {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(otm9605a_set_sleepin), otm9605a_set_sleepin},
        {DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(otm9605a_set_disp_off), otm9605a_set_disp_off},
};

static int mipi_otm9605a_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	pr_debug("mipi_otm9605a_lcd_on E\n");

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
		mipi_dsi_cmds_tx(&otm9605a_tx_buf,
			otm9605a_video_display_on_cmds,
			ARRAY_SIZE(otm9605a_video_display_on_cmds));
	}

	pr_debug("mipi_otm9605a_lcd_on X\n");

	return 0;
}

static int mipi_otm9605a_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	pr_debug("mipi_otm9605a_lcd_off E\n");

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&otm9605a_tx_buf, otm9605a_video_display_off_cmds,
			ARRAY_SIZE(otm9605a_video_display_off_cmds));

	pr_debug("mipi_otm9605a_lcd_off X\n");
	return 0;
}

static int __devinit mipi_otm9605a_lcd_probe(struct platform_device *pdev)
{
	struct platform_device *pthisdev = NULL;
	pr_debug("%s\n", __func__);

	if (pdev->id == 0) {
		mipi_otm9605a_pdata = pdev->dev.platform_data;
		if (mipi_otm9605a_pdata->bl_lock)
			spin_lock_init(&mipi_otm9605a_pdata->bl_spinlock);

		return 0;
	}

	pthisdev = msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_otm9605a_lcd_probe,
	.driver = {
		.name   = "mipi_otm9605a",
	},
};

static void mipi_otm9605a_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level = mfd->bl_level;

	pr_debug("%s: bl_level = %d\n", __func__, bl_level);

	mipi_otm9605a_pdata->backlight(bl_level, 1);

	return;
}

static struct msm_fb_panel_data otm9605a_panel_data = {
	.on	= mipi_otm9605a_lcd_on,
	.off = mipi_otm9605a_lcd_off,
	.set_backlight = mipi_otm9605a_set_backlight,
};

static int ch_used[3];

static int mipi_otm9605a_lcd_init(void)
{
	mipi_dsi_buf_alloc(&otm9605a_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&otm9605a_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
int mipi_otm9605a_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_otm9605a_lcd_init();
	if (ret) {
		pr_err("mipi_otm9605a_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_otm9605a", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	otm9605a_panel_data.panel_info = *pinfo;
	ret = platform_device_add_data(pdev, &otm9605a_panel_data,
				sizeof(otm9605a_panel_data));
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
