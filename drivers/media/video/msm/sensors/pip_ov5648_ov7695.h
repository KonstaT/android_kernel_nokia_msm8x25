/* Copyright (c) 2012, The Linux Foundation. All Rights Reserved.
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License version 2 and
 only version 2 as published by the Free Software Foundation.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 */



/* PIP ctrl definitions below*/
#define PIP_CRL_WRITE_SETTINGS      1
#define PIP_CRL_POWERUP             2
#define PIP_CRL_POWERDOWN           3
#define PIP_CTL_RESET_HW_PULSE      4
#define PIP_CTL_RESET_SW            5
#define PIP_CTL_STANDBY_SW          6
#define PIP_CTL_STANDBY_EXIT        7
#define PIP_CTL_STREAM_ON           8
#define PIP_CTL_STREAM_OFF          9


/* PIP working mode */
#define CAM_MODE_NORMAL             0
#define CAM_MODE_PIP                1

/* PIP settings type*/
#define PIP_REG_SETTINGS_SNAPSHOT   0
#define PIP_REG_SETTINGS_PREVIEW    1
#define PIP_REG_SETTINGS_60fps      2
#define PIP_REG_SETTINGS_90fps      3
#define PIP_REG_SETTINGS_ZSL        4
#define PIP_REG_SETTINGS_INIT       5

struct msm_camera_pip_ctl {
	int write_ctl;
	struct msm_camera_i2c_client *sensor_i2c_client;
};
