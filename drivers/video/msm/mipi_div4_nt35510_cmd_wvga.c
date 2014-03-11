#include <linux/module.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mdp4.h"
#include <linux/gpio.h>
#include <mach/pmic.h>
#include <linux/clk.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <asm/io.h>
#include <linux/moduleparam.h>
#include <mach/rpc_pmapp.h>
#include <linux/wait.h>
#include <linux/mm.h>

enum
{
	NT35510_STATUS_LCD_SLEEP,
	NT35510_STATUS_LCD_ON,
	NT35510_STATUS_LCD_GLANSCREEN_ON,
	NT35510_STATUS_LCD_GLANSCREEN_OFF,
};
#define SN3228B_MAX_BACKLIGHT     14

#define GP_CLK_M		  1
#define GP_CLK_N		  150
#define LCM_RESET_GPIO_PIN85	  85  
//pjn add for l1ED control
#define LCM_1LED_CTRL_GPIO_PIN121  121
#define LCD_LOW_POWER_DETECT	  1
#define DSI_BIT_CLK_500M	  1
#define LCM_ID_GPIO_PIN120	  120  

#define MIPI_SEND_DATA_TYPE DTYPE_GEN_LWRITE	//DTYPE_DCS_LWRITE	//

DEFINE_SEMAPHORE(nt35510_sem);

static void nt35510_backlight_set(int level);

static void mipi_nt35510_gs_set_backlight(int level);

struct nt35510_state_type
{
	bool disp_initialized;/* disp initialized */
	bool display_on;	/* lcd is on */
	uint8 disp_powered_up;/* lcd power and lcd bl power on */
	bool bk_init;
	bool gp_clk_flag;

};

typedef struct nt35510_state_type nt35510_state_t;
static nt35510_state_t nt35510_state= { 0 };
static uint32 mddi_current_bl = 255;//added by zxd to ignore the same level.
static struct msm_panel_info pinfo;
static struct msm_panel_common_pdata *mipi_nt35510_pdata;
static struct dsi_buf nt35510_tx_buf;
static struct dsi_buf nt35510_rx_buf;

//pjn add for low power glance screen,it specifies lcd state switch 
int glancescreen_flag = 0;

//add for panel module vender detect
extern int ODMM_PANEL_ID;
extern char panel_detect_desc[16];

//extern void lcd_lowpower_mode(void);
extern bool glanceview_suspend_statues(void);
extern bool glanceview_resume_statues(void);
extern void mipi_dsi_te_ctrl(struct msm_fb_data_type *mfd, bool flag);
extern void glanceview_oneled_state_ctrl(bool flag);

static char exit_sleep[1] = {0x11};
static char display_on[1] = {0x29};
static char write_ram[1] = {0x2c}; /* write ram */
static char enter_sleep[2] = {0x10, 0x00};
static char display_off[2] = {0x28, 0x00};
//static char STBYB_in[2] = {0x4F, 0x01};

#if 0
static char idle_off[2] = {0x38, 0x00};
static struct dsi_cmd_desc nt35510_ilde_mode_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(idle_off), idle_off},
};
#else
static char idle_on[2] = {0x39, 0x00};
static struct dsi_cmd_desc nt35510_ilde_mode_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(idle_on), idle_on},
};
#endif

#if 0	//use platform tear ctrl code
static char set_tear_on[2] = {0x35, 0x00};
static struct dsi_cmd_desc dsi_tear_on_cmd = {
	DTYPE_DCS_WRITE1, 1, 0, 0, 0, sizeof(set_tear_on), set_tear_on};


static char set_tear_off[2] = {0x34, 0x00};
static struct dsi_cmd_desc dsi_tear_off_cmd = {
	DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(set_tear_off), set_tear_off};
#endif

static struct dsi_cmd_desc nt35510_sleep_in_cmds[] = {
	//{DTYPE_DCS_WRITE, 1, 0, 0, 0, sizeof(set_tear_off), set_tear_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep},
//	{DTYPE_DCS_WRITE, 1, 0, 0, 5,	sizeof(write_ram), write_ram},
//	{DTYPE_DCS_WRITE1, 1, 0, 0, 5, sizeof(STBYB_in), STBYB_in},
};

#if 0
//static char nt35510_cmd_00[1] = {0x01};
static char nt35510_cmd_01[5] = {
	0xFF, 0xAA, 0x55, 0x25, 
	0x01};
static char nt35510_cmd_02[9] = {
	0xF3, 0x00, 0x32, 0x00, 
	0x38, 0x31, 0x08, 0x11, 
	0x00};
//page 1
static char nt35510_cmd_03[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x01};
static char nt35510_cmd_04[4] = {0xB0, 0x05, 0x05, 0x05};
static char nt35510_cmd_05[4] = {0xB6, 0x44, 0x44, 0x44};
static char nt35510_cmd_06[4] = {0xB1, 0x05, 0x05, 0x05};
static char nt35510_cmd_07[4] = {0xB7, 0x34, 0x34, 0x34};
static char nt35510_cmd_08[4] = {0xB2, 0x03, 0x03, 0x03};
static char nt35510_cmd_09[4] = {0xB3, 0x0B, 0x0B, 0x0B};
static char nt35510_cmd_10[4] = {0xB8, 0x25, 0x25, 0x25};

static char nt35510_cmd_11[4] = {0xB9, 0x34, 0x34, 0x34};
static char nt35510_cmd_12[2] = {0xBF, 0x01};   
static char nt35510_cmd_13[4] = {0xB4, 0x2D, 0x2D, 0x2D};
static char nt35510_cmd_14[4] = {0xB5, 0x08, 0x08, 0x08};
static char nt35510_cmd_15[4] = {0xBA, 0x24, 0x24, 0x24};

static char nt35510_cmd_16[4] = {0xBC, 0x00, 0xc8, 0x00};//{0xBC, 0x00, 0x68, 0x00};
static char nt35510_cmd_17[4] = {0xBD, 0x00, 0xc8, 0x00};//{0xBD, 0x00, 0x7C, 0x00};
static char nt35510_cmd_18[3] = {0xBE, 0x00, 0x78};//{0xBE, 0x00, 0x45};
//gamma 2.2
static char nt35510_cmd_19[53] = {
	0xd1, 0x00, 0x37, 0x00, 
	0xa3, 0x00, 0xc8, 0x00, 
	0xe4, 0x00, 0xf8, 0x01, 
	0x0c, 0x01, 0x35, 0x01, 
	0x61, 0x01, 0x84, 0x01, 
	0xb8, 0x01, 0xe0, 0x02, 
	0x22, 0x02, 0x57, 0x02, 
	0x59, 0x02, 0x87, 0x02, 
	0xBa, 0x02, 0xd9, 0x03, 
	0x03, 0x03, 0x20, 0x03, 
	0x49, 0x03, 0x62, 0x03, 
	0x87, 0x03, 0x9e, 0x03, 
	0xbc, 0x03, 0xea, 0x03, 
	0xf1};
static char nt35510_cmd_20[53] = {
	0xd2, 0x00, 0x37, 0x00, 
	0xa3, 0x00, 0xc8, 0x00, 
	0xe4, 0x00, 0xf8, 0x01, 
	0x0c, 0x01, 0x35, 0x01, 
	0x61, 0x01, 0x84, 0x01, 
	0xb8, 0x01, 0xe0, 0x02, 
	0x22, 0x02, 0x57, 0x02, 
	0x59, 0x02, 0x87, 0x02, 
	0xBa, 0x02, 0xd9, 0x03, 
	0x03, 0x03, 0x20, 0x03, 
	0x49, 0x03, 0x62, 0x03, 
	0x87, 0x03, 0x9e, 0x03, 
	0xbc, 0x03, 0xea, 0x03, 
	0xf1};
static char nt35510_cmd_21[53] = {
	0xd3, 0x00, 0x37, 0x00, 
	0xa3, 0x00, 0xc8, 0x00, 
	0xe4, 0x00, 0xf8, 0x01, 
	0x0c, 0x01, 0x35, 0x01, 
	0x61, 0x01, 0x84, 0x01, 
	0xb8, 0x01, 0xe0, 0x02, 
	0x22, 0x02, 0x57, 0x02, 
	0x59, 0x02, 0x87, 0x02, 
	0xBa, 0x02, 0xd9, 0x03, 
	0x03, 0x03, 0x20, 0x03, 
	0x49, 0x03, 0x62, 0x03, 
	0x87, 0x03, 0x9e, 0x03, 
	0xbc, 0x03, 0xea, 0x03, 
	0xf1};
static char nt35510_cmd_22[53] = {
	0xd4, 0x00, 0x37, 0x00, 
	0xa3, 0x00, 0xc8, 0x00, 
	0xe4, 0x00, 0xf8, 0x01, 
	0x0c, 0x01, 0x35, 0x01, 
	0x61, 0x01, 0x84, 0x01, 
	0xb8, 0x01, 0xe0, 0x02, 
	0x22, 0x02, 0x57, 0x02, 
	0x59, 0x02, 0x87, 0x02, 
	0xBa, 0x02, 0xd9, 0x03, 
	0x03, 0x03, 0x20, 0x03, 
	0x49, 0x03, 0x62, 0x03, 
	0x87, 0x03, 0x9e, 0x03, 
	0xbc, 0x03, 0xea, 0x03, 
	0xf1};
static char nt35510_cmd_23[53] = {
	0xd5, 0x00, 0x37, 0x00, 
	0xa3, 0x00, 0xc8, 0x00, 
	0xe4, 0x00, 0xf8, 0x01, 
	0x0c, 0x01, 0x35, 0x01, 
	0x61, 0x01, 0x84, 0x01, 
	0xb8, 0x01, 0xe0, 0x02, 
	0x22, 0x02, 0x57, 0x02, 
	0x59, 0x02, 0x87, 0x02, 
	0xBa, 0x02, 0xd9, 0x03, 
	0x03, 0x03, 0x20, 0x03, 
	0x49, 0x03, 0x62, 0x03, 
	0x87, 0x03, 0x9e, 0x03, 
	0xbc, 0x03, 0xea, 0x03, 
	0xf1};
static char nt35510_cmd_24[53] = {
	0xd6, 0x00, 0x37, 0x00, 
	0xa3, 0x00, 0xc8, 0x00, 
	0xe4, 0x00, 0xf8, 0x01, 
	0x0c, 0x01, 0x35, 0x01, 
	0x61, 0x01, 0x84, 0x01, 
	0xb8, 0x01, 0xe0, 0x02, 
	0x22, 0x02, 0x57, 0x02, 
	0x59, 0x02, 0x87, 0x02, 
	0xBa, 0x02, 0xd9, 0x03, 
	0x03, 0x03, 0x20, 0x03, 
	0x49, 0x03, 0x62, 0x03, 
	0x87, 0x03, 0x9e, 0x03, 
	0xbc, 0x03, 0xea, 0x03, 
	0xf1};
//page 0	
static char nt35510_cmd_25[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x00};
/*
 static char nt35510_cmd_26[6] = {
	0xB0, 0x00, 0x05, 0x02, 
	0x05, 0x02};*/
static char nt35510_cmd_27[3] = {0xB1, 0xEC, 0x00};//{0xB1, 0xCC, 0x00};
static char nt35510_cmd_28[2] = {0xB3, 0x00};
static char nt35510_cmd_29[2] = {0xB6, 0x03};
static char nt35510_cmd_30[3] = {0xB7, 0x00, 0x00};
static char nt35510_cmd_31[5] = {
	0xB8, 0x00, 0x06, 0x06, 
	0x06};
static char nt35510_cmd_32[2] = {0xBa, 0x01};
static char nt35510_cmd_33[4] = {0xBC, 0x00, 0x00, 0x00};
static char nt35510_cmd_34[6] = {
	0xBD, 0x01, 0x78, 0x06, 
	0x50, 0x00};
static char nt35510_cmd_35[4] = {0xCC, 0x03, 0x01, 0x06};


static char nt35510_cmd_36[2] = {0x36, 0x00};
static char nt35510_cmd_37[2] = {0x3a, 0x77};
//pjn add vivid color
static char nt35510_cmd_38[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x00};
static char nt35510_cmd_39[2] = {0xb4, 0x10};
static char nt35510_cmd_40[5] = {
	0xFF, 0xAA, 0x55, 0x25, 
	0x01};
//light	
/*
static char nt35510_cmd_41[12] = {
	0xF9, 0x14, 0x00, 0x05, 
	0x09, 0x0d, 0x11, 0x14,
	0x18, 0x1c, 0x20, 0x24};*/
//medium

static char nt35510_cmd_41[12] = {
	0xF9, 0x14, 0x00, 0x0a, 
	0x11, 0x17, 0x1d, 0x24,
	0x2a, 0x31, 0x37, 0x3d};
//high
/*
static char nt35510_cmd_41[12] = {
	0xF9, 0x14, 0x00, 0x0d, 
	0x1a, 0x26, 0x33, 0x40,
	0x4d, 0x5a, 0x66, 0x73};*/

static struct dsi_cmd_desc nt35510_cmd_on_cmds[] = {
//	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 120, sizeof(nt35510_cmd_00), nt35510_cmd_00},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_01), nt35510_cmd_01},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_02), nt35510_cmd_02},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_03), nt35510_cmd_03},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_04), nt35510_cmd_04},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_05), nt35510_cmd_05},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_06), nt35510_cmd_06},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_07), nt35510_cmd_07},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_08), nt35510_cmd_08},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_09), nt35510_cmd_09},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_10), nt35510_cmd_10},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_11), nt35510_cmd_11},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_12), nt35510_cmd_12},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_13), nt35510_cmd_13},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_14), nt35510_cmd_14},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_15), nt35510_cmd_15},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_16), nt35510_cmd_16},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_17), nt35510_cmd_17},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_18), nt35510_cmd_18},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_19), nt35510_cmd_19},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_20), nt35510_cmd_20},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_21), nt35510_cmd_21},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_22), nt35510_cmd_22},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_23), nt35510_cmd_23},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_24), nt35510_cmd_24},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_25), nt35510_cmd_25},
	//{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_26), nt35510_cmd_26},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_27), nt35510_cmd_27},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_28), nt35510_cmd_28},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_29), nt35510_cmd_29},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_30), nt35510_cmd_30},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_31), nt35510_cmd_31},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_32), nt35510_cmd_32},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_33), nt35510_cmd_33},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_34), nt35510_cmd_34},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_35), nt35510_cmd_35},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_36), nt35510_cmd_36},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_37), nt35510_cmd_37},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_38), nt35510_cmd_38},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_39), nt35510_cmd_39},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_40), nt35510_cmd_40},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_41), nt35510_cmd_41},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
        {DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(set_tear_on), set_tear_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_on), display_on},

	{DTYPE_DCS_WRITE, 1, 0, 0, 5,	sizeof(write_ram), write_ram},
};
#endif

//pjn add low power glance screen
#if 1
//static char nt35510_cmd_00[1] = {0x01};
static char nt35510_cmd_01[5] = {
	0xFF, 0xAA, 0x55, 0x25, 
	0x01};
static char nt35510_cmd_02[9] = {
	0xF3, 0x00, 0x32, 0x00, 
	0x38, 0x31, 0x08, 0x11, 
	0x00};
//page 1
static char nt35510_cmd_03[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x01};
#if 0
static char nt35510_cmd_04[4] = {0xB0, 0x05, 0x0C, 0x05};
static char nt35510_cmd_05[4] = {0xB6, 0x44, 0x14, 0x44};
static char nt35510_cmd_06[4] = {0xB1, 0x05, 0x0C, 0x05};
static char nt35510_cmd_07[4] = {0xB7, 0x34, 0x14, 0x34};
static char nt35510_cmd_08[4] = {0xB2, 0x03, 0x03, 0x03};
static char nt35510_cmd_09[4] = {0xB3, 0x0B, 0x08, 0x0B};
static char nt35510_cmd_10[4] = {0xB8, 0x25, 0x25, 0x25};

static char nt35510_cmd_11[4] = {0xB9, 0x34, 0x14, 0x34};
static char nt35510_cmd_12[2] = {0xBF, 0x01};   
static char nt35510_cmd_13[4] = {0xB4, 0x2D, 0x2D, 0x2D};
static char nt35510_cmd_14[4] = {0xB5, 0x08, 0x08, 0x08};
static char nt35510_cmd_15[4] = {0xBA, 0x24, 0x24, 0x24};

static char nt35510_cmd_16[4] = {0xBC, 0x00, 0x80, 0x00};//{0xBC, 0x00, 0x68, 0x00};
static char nt35510_cmd_17[4] = {0xBD, 0x00, 0x80, 0x00};//{0xBD, 0x00, 0x7C, 0x00};
static char nt35510_cmd_18[3] = {0xBE, 0x00, 0x78};//{0xBE, 0x00, 0x45};

static char nt35510_cmd_04[4] = {0xB0, 0x05, 0x05, 0x05};
static char nt35510_cmd_05[4] = {0xB6, 0x44, 0x44, 0x44};
static char nt35510_cmd_06[4] = {0xB1, 0x05, 0x05, 0x05};
static char nt35510_cmd_07[4] = {0xB7, 0x34, 0x34, 0x34};
static char nt35510_cmd_08[4] = {0xB2, 0x03, 0x03, 0x03};
static char nt35510_cmd_09[4] = {0xB3, 0x0B, 0x0B, 0x0B};
static char nt35510_cmd_10[4] = {0xB8, 0x25, 0x25, 0x25};

static char nt35510_cmd_11[4] = {0xB9, 0x34, 0x34, 0x34};
static char nt35510_cmd_12[2] = {0xBF, 0x01};   
static char nt35510_cmd_13[4] = {0xB4, 0x2D, 0x2D, 0x2D};
static char nt35510_cmd_14[4] = {0xB5, 0x08, 0x08, 0x08};
static char nt35510_cmd_15[4] = {0xBA, 0x24, 0x24, 0x24};

static char nt35510_cmd_16[4] = {0xBC, 0x00, 0xc8, 0x00};//{0xBC, 0x00, 0x68, 0x00};
static char nt35510_cmd_17[4] = {0xBD, 0x00, 0xc8, 0x00};//{0xBD, 0x00, 0x7C, 0x00};
static char nt35510_cmd_18[3] = {0xBE, 0x00, 0x78};//{0xBE, 0x00, 0x45};
#else
static char nt35510_cmd_04[4] = {0xB0, 0x05, 0x0C, 0x05};
static char nt35510_cmd_05[4] = {0xB6, 0x44, 0x34, 0x44};
static char nt35510_cmd_06[4] = {0xB1, 0x05, 0x0C, 0x05};
static char nt35510_cmd_07[4] = {0xB7, 0x34, 0x24, 0x34};
static char nt35510_cmd_08[4] = {0xB2, 0x03, 0x00, 0x03};
static char nt35510_cmd_09[4] = {0xB3, 0x0B, 0x08, 0x0B};
static char nt35510_cmd_10[4] = {0xB8, 0x25, 0x25, 0x25};

static char nt35510_cmd_11[4] = {0xB9, 0x34, 0x14, 0x34};
static char nt35510_cmd_12[2] = {0xBF, 0x01};   
static char nt35510_cmd_13[4] = {0xB4, 0x2D, 0x2D, 0x2D};
static char nt35510_cmd_14[4] = {0xB5, 0x08, 0x08, 0x08};
static char nt35510_cmd_15[4] = {0xBA, 0x24, 0x14, 0x24};

static char nt35510_cmd_16[4] = {0xBC, 0x00, 0x60, 0x00};//{0xBC, 0x00, 0x68, 0x00};
static char nt35510_cmd_17[4] = {0xBD, 0x00, 0x60, 0x00};//{0xBD, 0x00, 0x7C, 0x00};
static char nt35510_cmd_18[3] = {0xBE, 0x00, 0x52};//{0xBE, 0x00, 0x45};
#endif
//gamma 2.2
static char nt35510_cmd_19[53] = {
	0xD1, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x33, 0x00, 
	0x4b, 0x00, 0x5e, 0x00, 
	0x7f, 0x00, 0x9f, 0x00, 
	0xCb, 0x00, 0xee, 0x01, 
	0x2a, 0x01, 0x5f, 0x01, 
	0xAf, 0x01, 0xea, 0x01, 
	0xEc, 0x02, 0x27, 0x02, 
	0x68, 0x02, 0x92, 0x02, 
	0xc8, 0x02, 0xe8, 0x03, 
	0x25, 0x03, 0x4a, 0x03, 
	0x85, 0x03, 0x95, 0x03, 
	0xa3, 0x03, 0xD0, 0x03, 
	0xFF};
static char nt35510_cmd_20[53] = {
	0xD2, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x33, 0x00, 
	0x4b, 0x00, 0x5e, 0x00, 
	0x7f, 0x00, 0x9f, 0x00, 
	0xCb, 0x00, 0xee, 0x01, 
	0x2a, 0x01, 0x5f, 0x01, 
	0xAf, 0x01, 0xea, 0x01, 
	0xEc, 0x02, 0x27, 0x02, 
	0x68, 0x02, 0x92, 0x02, 
	0xc8, 0x02, 0xe8, 0x03, 
	0x25, 0x03, 0x4a, 0x03, 
	0x85, 0x03, 0x95, 0x03, 
	0xa3, 0x03, 0xD0, 0x03, 
	0xFF};
static char nt35510_cmd_21[53] = {
	0xD3, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x33, 0x00, 
	0x4b, 0x00, 0x5e, 0x00, 
	0x7f, 0x00, 0x9f, 0x00, 
	0xCb, 0x00, 0xee, 0x01, 
	0x2a, 0x01, 0x5f, 0x01, 
	0xAf, 0x01, 0xea, 0x01, 
	0xEc, 0x02, 0x27, 0x02, 
	0x68, 0x02, 0x92, 0x02, 
	0xc8, 0x02, 0xe8, 0x03, 
	0x25, 0x03, 0x4a, 0x03, 
	0x85, 0x03, 0x95, 0x03, 
	0xa3, 0x03, 0xD0, 0x03, 
	0xFF};
static char nt35510_cmd_22[53] = {
	0xD4, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x33, 0x00, 
	0x4b, 0x00, 0x5e, 0x00, 
	0x7f, 0x00, 0x9f, 0x00, 
	0xCb, 0x00, 0xee, 0x01, 
	0x2a, 0x01, 0x5f, 0x01, 
	0xAf, 0x01, 0xea, 0x01, 
	0xEc, 0x02, 0x27, 0x02, 
	0x68, 0x02, 0x92, 0x02, 
	0xc8, 0x02, 0xe8, 0x03, 
	0x25, 0x03, 0x4a, 0x03, 
	0x85, 0x03, 0x95, 0x03, 
	0xa3, 0x03, 0xD0, 0x03, 
	0xFF};
static char nt35510_cmd_23[53] = {
	0xD5, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x33, 0x00, 
	0x4b, 0x00, 0x5e, 0x00, 
	0x7f, 0x00, 0x9f, 0x00, 
	0xCb, 0x00, 0xee, 0x01, 
	0x2a, 0x01, 0x5f, 0x01, 
	0xAf, 0x01, 0xea, 0x01, 
	0xEc, 0x02, 0x27, 0x02, 
	0x68, 0x02, 0x92, 0x02, 
	0xc8, 0x02, 0xe8, 0x03, 
	0x25, 0x03, 0x4a, 0x03, 
	0x85, 0x03, 0x95, 0x03, 
	0xa3, 0x03, 0xD0, 0x03, 
	0xFF};
static char nt35510_cmd_24[53] = {
	0xD6, 0x00, 0x00, 0x00, 
	0x15, 0x00, 0x33, 0x00, 
	0x4b, 0x00, 0x5e, 0x00, 
	0x7f, 0x00, 0x9f, 0x00, 
	0xCb, 0x00, 0xee, 0x01, 
	0x2a, 0x01, 0x5f, 0x01, 
	0xAf, 0x01, 0xea, 0x01, 
	0xEc, 0x02, 0x27, 0x02, 
	0x68, 0x02, 0x92, 0x02, 
	0xc8, 0x02, 0xe8, 0x03, 
	0x25, 0x03, 0x4a, 0x03, 
	0x85, 0x03, 0x95, 0x03, 
	0xa3, 0x03, 0xD0, 0x03, 
	0xFF};
//page 0	
static char nt35510_cmd_25[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x00};
/*
 static char nt35510_cmd_26[6] = {
	0xB0, 0x00, 0x05, 0x02, 
	0x05, 0x02};*/
static char nt35510_cmd_27[3] = {0xB1, 0xEC, 0x00};//{0xB1, 0xCC, 0x00};
static char nt35510_cmd_28[2] = {0xB3, 0x00};
static char nt35510_cmd_29[2] = {0xB6, 0x03};
static char nt35510_cmd_30[3] = {0xB7, 0x00, 0x00};
static char nt35510_cmd_31[5] = {
	0xB8, 0x00, 0x06, 0x06, 
	0x06};
static char nt35510_cmd_32[2] = {0xBa, 0x01};
static char nt35510_cmd_33[4] = {0xBC, 0x00, 0x00, 0x00};
static char nt35510_cmd_34[6] = {
	0xBE, 0x01, 0x84, 0x1C, 
	0x1C, 0x01};
static char nt35510_cmd_35[4] = {0xCC, 0x03, 0x01, 0x06};


static char nt35510_cmd_36[2] = {0x36, 0x00};
static char nt35510_cmd_37[2] = {0x3a, 0x77};
//pjn add vivid color
static char nt35510_cmd_38[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x00};
static char nt35510_cmd_39[2] = {0xb4, 0x10};
static char nt35510_cmd_40[5] = {
	0xFF, 0xAA, 0x55, 0x25, 
	0x01};
//light	
/*
static char nt35510_cmd_41[12] = {
	0xF9, 0x14, 0x00, 0x05, 
	0x09, 0x0d, 0x11, 0x14,
	0x18, 0x1c, 0x20, 0x24};*/
//medium

static char nt35510_cmd_41[12] = {
	0xF9, 0x14, 0x00, 0x0a, 
	0x11, 0x17, 0x1d, 0x24,
	0x2a, 0x31, 0x37, 0x3d};
//high
/*
static char nt35510_cmd_41[12] = {
	0xF9, 0x14, 0x00, 0x0d, 
	0x1a, 0x26, 0x33, 0x40,
	0x4d, 0x5a, 0x66, 0x73};*/

static struct dsi_cmd_desc nt35510_cmd_on_cmds[] = {
//	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 120, sizeof(nt35510_cmd_00), nt35510_cmd_00},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_01), nt35510_cmd_01},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_02), nt35510_cmd_02},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_03), nt35510_cmd_03},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_04), nt35510_cmd_04},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_05), nt35510_cmd_05},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_06), nt35510_cmd_06},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_07), nt35510_cmd_07},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_08), nt35510_cmd_08},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_09), nt35510_cmd_09},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_10), nt35510_cmd_10},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_11), nt35510_cmd_11},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_12), nt35510_cmd_12},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_13), nt35510_cmd_13},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_14), nt35510_cmd_14},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_15), nt35510_cmd_15},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_16), nt35510_cmd_16},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_17), nt35510_cmd_17},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_18), nt35510_cmd_18},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_19), nt35510_cmd_19},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_20), nt35510_cmd_20},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_21), nt35510_cmd_21},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_22), nt35510_cmd_22},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_23), nt35510_cmd_23},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_24), nt35510_cmd_24},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_25), nt35510_cmd_25},
	//{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_26), nt35510_cmd_26},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_27), nt35510_cmd_27},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_28), nt35510_cmd_28},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_29), nt35510_cmd_29},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_30), nt35510_cmd_30},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_31), nt35510_cmd_31},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_32), nt35510_cmd_32},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_33), nt35510_cmd_33},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_34), nt35510_cmd_34},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_35), nt35510_cmd_35},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_36), nt35510_cmd_36},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_37), nt35510_cmd_37},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_38), nt35510_cmd_38},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_39), nt35510_cmd_39},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_40), nt35510_cmd_40},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_41), nt35510_cmd_41},

	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc nt35510_cmd_on_cmds_1[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 5, sizeof(display_on), display_on},
	//{DTYPE_DCS_WRITE, 1, 0, 0, 1, sizeof(set_tear_on), set_tear_on},
	{DTYPE_DCS_WRITE, 1, 0, 0, 5,	sizeof(write_ram), write_ram},
};
#endif

//pjn end
static struct mipi_dsi_phy_ctrl dsi_cmd_mode_phy_db = {
#if 0
	/* DSI_BIT_CLK at 350MHz, 2 lane, RGB888 */
	{0x03, 0x01, 0x01, 0x00},
	/* timing   */
	/*{0x69, 0x2B, 0x0D, 0x00, 0x37, 0x3C, 0x12, 0x2E,
	0x10, 0x03, 0x04},*/
	{0x69, 0x2B, 0x0D, 0x00, 0x37, 0x3C, 0x1f, 0x35,
	0x10, 0x07, 0x04}, //pjn modify
	/* phy ctrl */
	{0x7f, 0x00, 0x00, 0x00},
	/* strength */
	{0xbb, 0x02, 0x06, 0x00},
	/* pll control */
	{0x01, 0x59, 0x31, 0xD2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0F, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
#else 
	/* DSI_BIT_CLK at 460MHz, 2 lane, RGB888 */
	{0x03, 0x01, 0x01, 0x00},
	/* timing   */
	/*{0x69, 0x2B, 0x0D, 0x00, 0x37, 0x3C, 0x12, 0x2E,
	0x10, 0x03, 0x04},*/
	{0x82, 0x31, 0x12, 0x00, 0x41, 0x4A, 0x16, 0x35,
	0x16, 0x03, 0x04}, 
	/* phy ctrl */
	{0x7f, 0x00, 0x00, 0x00},
	/* strength */
	{0xbb, 0x02, 0x06, 0x00},
	/* pll control */
	{0x01, 0xC6, 0x31, 0xD2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0F, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
#endif
};

//pjn add read 0x0a register
#if 0
static char nt35510_cmd_page1[6] = {
                   0xF0, 0x55, 0xAA, 0x52, 
                   0x08, 0x01};

static struct dsi_cmd_desc nt35510_change_page[] = {
         {MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_page1), nt35510_cmd_page1},
};

static u32 mipi_nt35510_read_reg(struct msm_fb_data_type *mfd, u16 reg, int page)
{
         u32 data;
         int len = 4;
         
         struct dsi_cmd_desc cmd_read_reg = {
                   DTYPE_GEN_READ2, 1, 0, 1, 0, /* cmd 0x24 */
                            sizeof(reg), (char *) &reg};

         mipi_dsi_buf_init(&nt35510_tx_buf);
         mipi_dsi_buf_init(&nt35510_rx_buf);

	if(page)
       		  /* mutex had been acquired at mipi_dsi_on */
       		  mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_change_page, ARRAY_SIZE(nt35510_change_page));

         len = mipi_dsi_cmds_rx(mfd, &nt35510_tx_buf, &nt35510_rx_buf,
                                   &cmd_read_reg, len);

         data = *(u32 *)nt35510_rx_buf.data;

         printk("[==wxj==]%s: reg=0x%x.data=0x%08x\n", __func__, reg, data);

         return data;
}
#endif
//end

static int msm_lcm_send_init_code(void)
{
	int ret = 0;
	my_prepare_enable_clocks();
	msleep(80);
	ret = mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_on_cmds,
				             ARRAY_SIZE(nt35510_cmd_on_cmds));
	my_unprepare_disable_clocks();
	msleep(16);

	my_prepare_enable_clocks();
	msleep(80);
	ret = mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_on_cmds_1,
				             ARRAY_SIZE(nt35510_cmd_on_cmds_1));
	msleep(16);
	my_unprepare_disable_clocks();
	return ret;
}

static void msm_lcm_reset(void)
{	
	printk(KERN_INFO "[DISPLAY] begain to reset panel\n");
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 1);  
	msleep(1);  
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 0);  
	msleep(3); 
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 1); 
	msleep(30); 
}

#define POWER_ON_NOT_BOOTLOGO 1
static int mipi_nt35510_gpio_init(void)
{
	int ret;	
	{
		printk(KERN_INFO"[DISPLAY] %s called\n",__func__);

		ret = gpio_request(LCM_RESET_GPIO_PIN85,	"odmm_gpio_reset_en");
		if(ret < 0)
			printk(KERN_ERR" LCM_RESET_GPIO_PIN85 request failed\n");
		
		ret=gpio_tlmm_config(
	        GPIO_CFG(
	        LCM_RESET_GPIO_PIN85,
	        0,
	        GPIO_CFG_OUTPUT, //LCDC DEN
	        GPIO_CFG_PULL_UP,
			GPIO_CFG_4MA),
			GPIO_CFG_ENABLE);

		if(ret)
		{
			printk(KERN_DEBUG"Failure to set LCM_RESET_GPIO_PIN85 config\n");
			return -1;
		}
   	 }
		
	ret = gpio_request(LCM_1LED_CTRL_GPIO_PIN121,"lcm_gpio_1led_en");
	if (ret < 0){
		printk(KERN_INFO" pjn=== GPIO121 request failed\n");
		return ret;
	}

	ret = gpio_tlmm_config(
			GPIO_CFG(
	        121,
	        0,
	        GPIO_CFG_OUTPUT, //LCDC DEN
	        GPIO_CFG_PULL_UP,//GPIO_CFG_PULL_UP,//GPIO_CFG_PULL_UP,
			GPIO_CFG_4MA),
			GPIO_CFG_ENABLE);
	if(ret)
	{
		printk(KERN_DEBUG"Failure to set LCM_1LED_CTRL_GPIO_PIN121 config\n");
		return -1;
	}

	gpio_request(LCM_ID_GPIO_PIN120, "odmm_panel_id");
	gpio_tlmm_config(GPIO_CFG(LCM_ID_GPIO_PIN120, 0,
			GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

	gpio_export(LCM_RESET_GPIO_PIN85, 0);
	gpio_export(LCM_1LED_CTRL_GPIO_PIN121, 0);
	gpio_export(LCM_ID_GPIO_PIN120, 0);

	gpio_direction_output(LCM_1LED_CTRL_GPIO_PIN121, 0); 
	
	return 0;
}

static int mipi_nt35510_lcd_on(struct platform_device *pdev)
{

	struct msm_fb_data_type *mfd;
	
	MSM_DRM_DEBUG("start. \n");
	
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
	{
		printk("%s [DISPLAY:mfd] \n", __func__);
		return -ENODEV;
	}
	if (!mfd->cont_splash_done)
	{
		mfd->cont_splash_done = 1;
		printk(" %s [DISPLAY:cont_splash_done] \n", __func__);
		return 0;
	}

	pr_debug("platform_get_drvdata success \n");
	if (mfd->key != MFD_KEY)
	{
		printk(" %s [DISPLAY:MFD_KEY] \n", __func__);
		return -EINVAL;
	}
	MSM_DRM_DEBUG("MFD_KEY  Okay\n");

	down(&nt35510_sem);
	if (glanceview_resume_statues())
	{
		printk("%s(glance 3)=%d skip init\n",__func__,nt35510_state.disp_powered_up);
		nt35510_state.disp_powered_up = NT35510_STATUS_LCD_GLANSCREEN_ON;
	}
	else
	{
		if(nt35510_state.disp_powered_up == NT35510_STATUS_LCD_SLEEP)
		{
			printk("%s(0)=%d deep sleep to normal\n",__func__,nt35510_state.disp_powered_up);

			msm_lcm_reset();
			//mipi_set_tx_power_mode(1);
#if 0
			mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_on_cmds,
				             ARRAY_SIZE(nt35510_cmd_on_cmds));
			mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_cmd_on_cmds_1,
				             ARRAY_SIZE(nt35510_cmd_on_cmds_1));
#endif							 
             msm_lcm_send_init_code();
			//mipi_set_tx_power_mode(0);
		}
		
		else if(nt35510_state.disp_powered_up == NT35510_STATUS_LCD_GLANSCREEN_OFF)
		{		
			printk("%s(3)=%d idle mode to normal\n",__func__,nt35510_state.disp_powered_up);
			//nt35510_backlight_set(0);
			//gpio_direction_output(LCM_1LED_CTRL_GPIO_PIN121, 0); 
			glanceview_oneled_state_ctrl(0);
			//pjn add lcd reset
			msm_lcm_reset();

			msm_lcm_send_init_code();
		}
		else
		{
			printk("%s(error)=%d\n",__func__,nt35510_state.disp_powered_up);
			
			//gpio_direction_output(LCM_1LED_CTRL_GPIO_PIN121, 0); 
			glanceview_oneled_state_ctrl(0);
			
			msm_lcm_reset();
			//mipi_set_tx_power_mode(1);
			msm_lcm_send_init_code();

			//mipi_set_tx_power_mode(0);			
		}
		nt35510_state.disp_powered_up = NT35510_STATUS_LCD_ON;

		mipi_dsi_te_ctrl(mfd, 1);
	}
    
	up(&nt35510_sem);

	MSM_DRM_DEBUG("end.\n");
	return 0;
}

//pjn add for glance scrren
int mipi_nt35510_glance_screen(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	int gpio121_val = 2;
	mfd = platform_get_drvdata(pdev);
	
    MSM_DRM_DEBUG("start.\n");
	if (!mfd)
	{
		printk("%s mfd == NULL\n",__func__);
		return -ENODEV;	
	}
		
	if (glancescreen_flag) {
		gpio121_val = gpio_get_value_cansleep(LCM_1LED_CTRL_GPIO_PIN121);
		if((!gpio121_val) && glancescreen_suspend_flag){
			gpio_set_value_cansleep(LCM_1LED_CTRL_GPIO_PIN121, 1); 
		}
		return 0;
	}
	
	down(&nt35510_sem);
	nt35510_backlight_set(0);
	msm_lcm_reset();
	//mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_lp_cmd_on_cmds,
		   //ARRAY_SIZE(nt35510_lp_cmd_on_cmds));
	glancescreen_flag = 1;
	nt35510_state.disp_powered_up = NT35510_STATUS_LCD_GLANSCREEN_OFF;
	up(&nt35510_sem);
	
	MSM_DRM_DEBUG(" end.\n");
	return 0;
}

static int mipi_nt35510_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
    //int gpio121_val = 2;
	
	mfd = platform_get_drvdata(pdev);

	MSM_DRM_DEBUG(" start.\n");
	
	down(&nt35510_sem);

	mipi_dsi_te_ctrl(mfd, 0);

	if (glanceview_suspend_statues())
	{
	 	if (nt35510_state.disp_powered_up == NT35510_STATUS_LCD_GLANSCREEN_ON)
	 	{
	 		printk("%s(glance 2)=%d \n", __func__,nt35510_state.disp_powered_up);
			
			if (glancescreen_flag == 1)
			{
				//move oneled control owner to OS, it will be much more better
				//msleep(2);
				//gpio_set_value_cansleep(LCM_1LED_CTRL_GPIO_PIN121, 1);
				glancescreen_flag = 2;
				printk("%s(glance backlight)=%d \n", __func__,nt35510_state.disp_powered_up);
			}
	 	}
		else
		{
			printk("%s(glance other)=%d,\n", __func__,nt35510_state.disp_powered_up);
			mipi_nt35510_gs_set_backlight(0);
	    
			mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_ilde_mode_on_cmds,
				             ARRAY_SIZE(nt35510_ilde_mode_on_cmds));
		
			glancescreen_flag = 1;
		}
		nt35510_state.disp_powered_up = NT35510_STATUS_LCD_GLANSCREEN_OFF;
		
	}
	else
	{
		if(nt35510_state.disp_powered_up == NT35510_STATUS_LCD_ON )
		{
			printk("%s(1)=%d\n",__func__,nt35510_state.disp_powered_up);
			
			//mipi_set_tx_power_mode(1);
			
			mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_sleep_in_cmds,
					ARRAY_SIZE(nt35510_sleep_in_cmds));

			//mipi_set_tx_power_mode(0);
			gpio_direction_output(LCM_RESET_GPIO_PIN85, 0);		
		}
		
		else if(nt35510_state.disp_powered_up == NT35510_STATUS_LCD_GLANSCREEN_ON)
		{
			printk("%s(2)=%d\n",__func__,nt35510_state.disp_powered_up);
			//gpio_direction_output(LCM_1LED_CTRL_GPIO_PIN121, 0); 
			glanceview_oneled_state_ctrl(0);
			
			//mipi_set_tx_power_mode(1);

			mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_sleep_in_cmds,
					ARRAY_SIZE(nt35510_sleep_in_cmds));

			//mipi_set_tx_power_mode(0);
			gpio_direction_output(LCM_RESET_GPIO_PIN85, 0);	
		}
		else
		{
			printk("%s(error)=%d\n",__func__,nt35510_state.disp_powered_up);
			//gpio_direction_output(LCM_1LED_CTRL_GPIO_PIN121, 0); 
			glanceview_oneled_state_ctrl(0);
			
			//mipi_set_tx_power_mode(1);
			
			mipi_dsi_cmds_tx(&nt35510_tx_buf, nt35510_sleep_in_cmds,
					ARRAY_SIZE(nt35510_sleep_in_cmds));

			//mipi_set_tx_power_mode(0);
			gpio_direction_output(LCM_RESET_GPIO_PIN85, 0);		
			
		}
		nt35510_state.disp_powered_up = NT35510_STATUS_LCD_SLEEP;
	}
	
	up(&nt35510_sem);
	MSM_DRM_DEBUG(" end.\n");
	return 0;
}

#define PERPH_WEB_BLOCK_ADDR (0xA9D00040)
#define PDM0_CTL_OFFSET (0x04)
#define SIZE_8B (0x08)
#define PDM_DUTY_MAXVAL BIT(16)
#define PDM_DUTY_REFVAL BIT(15)
#define LED_FULL		255

static void nt35510_backlight_set(int level)
{
	int ret = 0;
	if(unlikely(!nt35510_state.bk_init)){
		pmapp_disp_backlight_init();
		nt35510_state.bk_init = TRUE;
	}

	if(level > 0)
		gpio_direction_output(LCM_1LED_CTRL_GPIO_PIN121, 0);
	
	ret = pmapp_disp_backlight_set_brightness(level*255/255);
	if (ret)
		printk(KERN_ERR" can't set backlight level=%d\n",level);
}

static void mipi_nt35510_set_backlight(struct msm_fb_data_type *mfd)
{

	int level;

	level = mfd->bl_level;

     	printk(KERN_INFO"[DISPLAY] current_bl=%d; target_bl=%d\n", mddi_current_bl, level);
//	if(mddi_current_bl == level)
//        	return;

	nt35510_backlight_set(level);
     	mddi_current_bl = level;

	return;	 
}

static void mipi_nt35510_gs_set_backlight(int level)
{
	printk("mipi_nt35510_gs_set_backlight()=%d\n",level);
	nt35510_backlight_set(level);	 
}


static int __devinit mipi_nt35510_lcd_probe(struct platform_device *pdev)
{
	printk(KERN_INFO "[DISPLAY] %s is called\n", __func__);
	if (pdev->id == 0) 
	{
		mipi_nt35510_pdata = pdev->dev.platform_data;
		return 0;
	}

	nt35510_state.disp_initialized = TRUE;
	nt35510_state.disp_powered_up = NT35510_STATUS_LCD_ON;// first poweron don't reset lcd
	
	msm_fb_add_device(pdev);

	mipi_nt35510_gpio_init();

	return 0;
}


static void mipi_nt35510_shutdown(struct platform_device *pdev)
{
	printk(KERN_INFO "[DISPLAY] %s is called\n", __func__);
	nt35510_backlight_set(0);
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 0);
	gpio_free(LCM_RESET_GPIO_PIN85);
	msleep(120);
}

static struct platform_driver this_driver = {
	.probe  = mipi_nt35510_lcd_probe,
	.shutdown = mipi_nt35510_shutdown,
	.driver = {
		.name   = "mipi_nt35510",
	},
};

static struct msm_fb_panel_data nt35510_panel_data = {
	.on		= mipi_nt35510_lcd_on,
	.off		= mipi_nt35510_lcd_off,
	.glance_screen = mipi_nt35510_glance_screen,
	.set_backlight = mipi_nt35510_set_backlight,
};

static struct platform_device this_device = {
	.name   = "mipi_nt35510",
	.id	= 2,
	.dev	= {
		.platform_data = &nt35510_panel_data,
	}
};

static int __init mipi_div4_nt35510_lcd_init(void)
{
	int ret;

	#if 1
	gpio_request(LCM_ID_GPIO_PIN120, "odmm_panel_id");
	gpio_tlmm_config(GPIO_CFG(LCM_ID_GPIO_PIN120, 0,
			GPIO_CFG_INPUT,
			GPIO_CFG_NO_PULL,
			GPIO_CFG_2MA),
			GPIO_CFG_ENABLE);

	ODMM_PANEL_ID = gpio_get_value_cansleep(LCM_ID_GPIO_PIN120);  
	if(ODMM_PANEL_ID == 0){
		strncpy(panel_detect_desc, "N1 DIV4 CMD", 16);
		printk(KERN_INFO "[DISPLAY]Detect Device Used DIV4 Panel as ODMM_PANEL_ID = %d\n", ODMM_PANEL_ID);
	}else{
		strncpy(panel_detect_desc, "N1 TRUST CMD", 16);
		printk(KERN_INFO "[DISPLAY]Detect Device Used TRUST Panel as ODMM_PANEL_ID = %d\n", ODMM_PANEL_ID);
	}
	gpio_free(LCM_ID_GPIO_PIN120);
      #endif

	//if(odmm_nt35510_lcd_id != ODMM_35516_nt35510_ID)
	//	return 0;

	mipi_dsi_buf_alloc(&nt35510_tx_buf, DSI_BUF_SIZE);//mipi cmd buffer
	mipi_dsi_buf_alloc(&nt35510_rx_buf, DSI_BUF_SIZE);

	ret = platform_driver_register(&this_driver);
	if (ret)
	{
		printk(KERN_ERR"[DISPLAY] %s platform_driver_register FAILED\n",  __func__);
		return ret;
	}

	pinfo.xres = 480;
	pinfo.yres = 800;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;

	pinfo.lcdc.h_back_porch = 29;
	pinfo.lcdc.h_front_porch = 7;
	pinfo.lcdc.h_pulse_width = 3;//8;
	pinfo.lcdc.v_back_porch = 12;
	pinfo.lcdc.v_front_porch = 16;
	pinfo.lcdc.v_pulse_width = 5;////1;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;//16;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	
	pinfo.clk_rate = 350000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 6200; //6000; //6200/* adjust refx100 to prevent tearing */
	pinfo.lcd.v_back_porch = 12;
	pinfo.lcd.v_front_porch = 16;
	pinfo.lcd.v_pulse_width = 5;////1;

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_BGR;//DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x20;
	pinfo.mipi.t_clk_pre = 0x2F;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_SW_TE;//DSI_CMD_TRIGGER_SW_TE;
	pinfo.mipi.dma_trigger = DSI_CMD_TRIGGER_SW;
	pinfo.mipi.te_sel = 1; /* TE from vsync gpio */
	pinfo.mipi.interleave_max = 1;
	pinfo.mipi.insert_dcs_cmd = TRUE;
	pinfo.mipi.wr_mem_continue = 0x3c;
	pinfo.mipi.wr_mem_start = 0x2c;
	pinfo.mipi.dsi_phy_db = &dsi_cmd_mode_phy_db;
	pinfo.mipi.tx_eot_append = 0x01;
	pinfo.mipi.rx_eot_ignore = 0x0;
	pinfo.mipi.dlane_swap = 0x01;
	
	nt35510_panel_data.panel_info = pinfo;
	
	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	pmapp_disp_backlight_init();
    
	return ret;
}
struct dsi_buf *  get_lcd_mipi_tx_buf(void)
{
	return &nt35510_tx_buf;
}
module_init(mipi_div4_nt35510_lcd_init);
