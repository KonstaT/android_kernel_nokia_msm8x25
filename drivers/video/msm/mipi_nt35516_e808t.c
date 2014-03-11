#include <linux/module.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mdp4.h"
#include <linux/gpio.h>
//#include <mach/gpio.h>
//#include <mach/gpio-v1.h>
#include <mach/pmic.h>
#include <linux/clk.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <asm/io.h>
#include <linux/moduleparam.h>
#include <mach/rpc_pmapp.h>
#include <linux/wait.h>
#include <linux/mm.h>

#define SN3228B_MAX_BACKLIGHT     14

#define GP_CLK_M		  1
#define GP_CLK_N		  150
#define LCM_RESET_GPIO_PIN85	  85  
#define LCD_LOW_POWER_DETECT	  1
#define DSI_BIT_CLK_500M	  1

#define NT35510_SLEEP_OFF_DELAY 120
#define NT35510_DISPLAY_ON_DELAY 20

#define MIPI_SEND_DATA_TYPE DTYPE_GEN_LWRITE	//DTYPE_DCS_LWRITE

DEFINE_SEMAPHORE(nt35516_sem);


static void hx8357_backlight_set(int level);

struct hx8357_state_type
{
	bool disp_initialized;/* disp initialized */
	bool display_on;	/* lcd is on */
	bool disp_powered_up;/* lcd power and lcd bl power on */
	bool bk_init;
	//[shao.wenqi@byd.com; add; ]
	bool gp_clk_flag;
	//[shao.wenqi@byd.com; end;]
};

typedef struct hx8357_state_type hx8357_state_t;
static hx8357_state_t hx8357_state= { 0 };
static uint32 mddi_current_bl = 255;//added by zxd to ignore the same level.
static struct msm_panel_info pinfo;
static struct msm_panel_common_pdata *mipi_hx8357_pdata;
static struct dsi_buf hx8357_tx_buf;
static struct dsi_buf hx8357_rx_buf;


static char exit_sleep[1] = {0x11};
static char display_on[1] = {0x29};
static char write_ram[1] = {0x2c}; /* write ram */
static char sleep_in[1] = {0x10};
static char display_off[1] = {0x28};
static struct dsi_cmd_desc hx8357_sleep_in_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(sleep_in), sleep_in}};
#if 0
static char video0[2] = {0x00,0x00};
static char video1[4] = {0xFF,0x96,0x08,0x01};
static char video2[2] = {0x00,0x80};
static char video3[3] = {0xFF,0x96,0x08};
static char video4[2] = {0x00,0x00};
static char video5[2] = {0xA0,0x00};
static char video6[2] = {0x00,0x80};
static char video7[6] = {0xB3,0x00,0x00,0x20,0x00,0x00};
static char video8[2] = {0x00,0xC0};
static char video9[2] = {0xB3,0x09};
static char video10[2] = {0x00,0x80};
static char video11[10] = {0xC0,0x00,0x48,0x00,0x10,0x10,0x00,0x47,0x10,0x10,};
static char video12[2] = {0x00,0x92};
static char video13[5] = {0xC0,0x00,0x10,0x00,0x13};
static char video14[2] = {0x00,0xA2};
static char video15[4] = {0xC0,0x0C,0x05,0x02};
static char video16[2] = {0x00,0xB3};
static char video17[3] = {0xC0,0x00,0x50};
static char video18[2] = {0x00,0x81};
static char video19[2] = {0xC1,0x55};
static char video20[2] = {0x00,0x80};
static char video21[4] = {0xC4,0x00,0x84,0xFC};
static char video22[2] = {0x00,0xA0};
static char video23[3] = {0xB3,0x10,0x00};
static char video24[2] = {0x00,0xA0};
static char video25[2] = {0xC0,0x00};
static char video26[2] = {0x00,0xA0};
static char video27[9] = {0xC4,0x33,0x09,0x90,0x2B,0x33,0x09,0x90,0x54};
static char video28[2] = {0x00,0x80};
static char video29[5] = {0xC5,0x08,0x00,0xA0,0x11};
static char video30[2] = {0x00,0x90};
static char video31[8] = {0xC5,0xD0,0x57,0x00,0x57,0x33,0x33,0x34};
static char video32[2] = {0x00,0xA0};
static char video33[8] = {0xC5,0x96,0x57,0x00,0x57,0x33,0x33,0x34};
static char video34[2] = {0x00,0xB0};
static char video35[8] = {0xC5,0x04,0xAC,0x01,0x00,0x71,0xB1,0x83};
static char video36[2] = {0x00,0x00};
static char video37[2] = {0xD9,0x5F};
static char video38[2] = {0x00,0x80};
static char video39[2] = {0xC6,0x64};
static char video40[2] = {0x00,0xB0};
static char video41[6] = {0xC6,0x03,0x10,0x00,0x1F,0x12};
static char video42[2] = {0x00,0xE1};
static char video43[2] = {0xC0,0x9F};
static char video44[2] = {0x00,0xB7};
static char video45[2] = {0xB0,0x10};
static char video46[2] = {0x00,0xC0};
static char video47[2] = {0xB0,0x55};
static char video48[2] = {0x00,0xB1};
static char video49[2] = {0xB0,0x03};
static char video50[2] = {0x00,0x80};
static char video51[2] = {0xD6,0x00};
static char video52[2] = {0x00,0x00};
//static char video53[17] = {0xE1,0x01,0x0D,0x13,0x0F,0x07,0x11,0x0B,0x0A,0x03,0x06,0x0B,0x08,0x0D,0x0E,0x09,0x01};
//static char video54[17] = {0xE1,0x01,0x0D,0x13,0x0F,0x07,0x11,0x0B,0x0A,0x03,0x06,0x0B,0x08,0x0D,0x0E,0x09,0x01};
static char video56[17] = {0xe1,0x01,0x10,0x12,0x0f,0x08,0x2c,0x0b,0x0a,0x03,0x07,0x12,0x08,0x0d,0x0e,0x09,0x01};
static char video55[2] = {0x00,0x00};
static char video57[17] = {0xe2,0x01,0x10,0x12,0x0f,0x07,0x2c,0x0b,0x0a,0x03,0x07,0x12,0x08,0x0d,0x0e,0x09,0x01};
static char video58[2] = {0x00,0x80};
static char video59[11] = {0xcb,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video60[2] = {0x00,0x90};
static char video61[16] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video62[2] = {0x00,0xa0};
static char video63[16] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video64[2] = {0x00,0xb0};
static char video65[11] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video66[2] = {0x00,0xc0};
static char video67[16] = {0xcb,0x00,0x00,0x00,0x04,0x00,0x00,0x04,0x04,0x00,0x00,0x04,0x04,0x04,0x00,0x00};
static char video68[2] = {0x00,0xd0};
static char video69[16] = {0xcb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x00,0x00,0x04,0x04};
static char video70[2] = {0x00,0xe0};
static char video71[11] = {0xcb,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00};
static char video72[2] = {0x00,0xf0};
static char video73[11] = {0xcb,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
static char video74[2] = {0x00,0x80};
static char video75[11] = {0xcc,0x00,0x00,0x00,0x02,0x00,0x00,0x0a,0x0e,0x00,0x00};
static char video76[2] = {0x00,0x90};
static char video77[16] = {0xcc,0x0c,0x10,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x09};
static char video78[2] = {0x00,0xa0};
static char video79[16] = {0xcc,0x0d,0x00,0x00,0x0b,0x0f,0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00};
static char video80[2] = {0x00,0xb0};
static char video81[11] = {0xcc,0x00,0x00,0x00,0x02,0x00,0x00,0x0a,0x0e,0x00,0x00};
static char video82[2] = {0x00,0xc0};
static char video83[16] = {0xcc,0x0c,0x10,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video84[2] = {0x00,0xd0};
static char video85[16] = {0xcc,0x05,0x00,0x00,0x00,0x00,0x0f,0x0b,0x00,0x00,0x0d,0x09,0x01,0x00,0x00,0x00};
static char video86[2] = {0x00,0x80};
static char video87[13] = {0xce,0x84,0x03,0x18,0x83,0x03,0x18,0x00,0x00,0x00,0x00,0x00,0x00};
static char video88[2] = {0x00,0x90};
static char video89[15] = {0xce,0x33,0xbf,0x18,0x33,0xc0,0x18,0x10,0x0f,0x18,0x10,0x10,0x18,0x00,0x00};
static char video90[2] = {0x00,0xa0};
static char video91[15] = {0xce,0x38,0x02,0x03,0xc1,0x00,0x18,0x00,0x38,0x01,0x03,0xc2,0x00,0x18,0x00};
static char video92[2] = {0x00,0xb0};
static char video93[15] = {0xce,0x38,0x00,0x03,0xc3,0x00,0x18,0x00,0x30,0x00,0x03,0xc4,0x00,0x18,0x00};
static char video94[2] = {0x00,0xc0};
static char video95[15] = {0xce,0x30,0x01,0x03,0xc5,0x00,0x18,0x00,0x30,0x02,0x03,0xc6,0x00,0x18,0x00};
static char video96[2] = {0x00,0xd0};
static char video97[15] = {0xce,0x30,0x03,0x03,0xc7,0x00,0x18,0x00,0x30,0x04,0x03,0xc8,0x00,0x18,0x00};
static char video98[2] = {0x00,0x80};
static char video99[15] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video100[2] = {0x00,0x90};
static char video101[15] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video102[2] = {0x00,0xa0};
static char video103[15] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video104[2] = {0x00,0xb0};
static char video105[15] = {0xcf,0xf0,0x00,0x00,0x00,0x00,0x00,0x00,0xf0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video106[2] = {0x00,0xc0};
static char video107[11] = {0xcf,0x01,0x01,0x20,0x20,0x00,0x00,0x02,0x04,0x00,0x00};
static char video108[2] = {0x00,0x00};
static char video109[3] = {0xd8,0x97,0x97};
static char video110[2] = {0x51,0x88}; 	
static char video111[2] = {0x53,0x2c};
static char video112[2] = {0x55,0x01};
static char video113[2] = {0x00,0xb1};
static char video114[2] = {0xc6,0xff};  // 00最大，ff最小
static char video115[2] = {0x35,0x00}; // TE 功能打开
static char video116[2] = {0x00,0x00};
static char video117[4] = {0xff,0xff,0xff,0xff};
static char video118[1] = {0x11};
static char video119[1] = {0x29};
static char video120[1] = {0x2c};
#else
static char nt35510_cmd_00[1] = {0x01};
static char nt35510_cmd_01[5] = {
	0xFF, 0xAA, 0x55, 0x25, 
	0x01};
static char nt35510_cmd_02[9] = {
	0xF3, 0x00, 0x32, 0x00, 
	0x38, 0x31, 0x08, 0x11, 
	0x00};
static char nt35510_cmd_03[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x00};
static char nt35510_cmd_04[6] = {
	0xB0, 0x00, 0x05, 0x02, 
	0x05, 0x02};
static char nt35510_cmd_05[3] = {0xB1, 0xCC, 0x02};
static char nt35510_cmd_06[2] = {0xB3, 0x00};
static char nt35510_cmd_07[2] = {0xB6, 0x03};
static char nt35510_cmd_08[3] = {0xB7, 0x70, 0x70};
static char nt35510_cmd_09[5] = {
	0xB8, 0x00, 0x06, 0x06, 
	0x06};
static char nt35510_cmd_10[4] = {0xBC, 0x00, 0x00, 0x00};
static char nt35510_cmd_11[6] = {
	0xBD, 0x01, 0x84, 0x06, 
	0x50, 0x00};
static char nt35510_cmd_12[4] = {0xCC, 0x03, 0x01, 0x06};
static char nt35510_cmd_13[6] = {
	0xF0, 0x55, 0xAA, 0x52, 
	0x08, 0x01};
static char nt35510_cmd_14[4] = {0xB0, 0x05, 0x05, 0x05};
static char nt35510_cmd_15[4] = {0xB1, 0x05, 0x05, 0x05};
static char nt35510_cmd_16[4] = {0xB2, 0x03, 0x03, 0x03};
static char nt35510_cmd_17[5] = {0xB8, 0x25, 0x25, 0x25};
static char nt35510_cmd_18[4] = {0xB3, 0x0B, 0x0B, 0x0B};
static char nt35510_cmd_19[4] = {0xB9, 0x34, 0x34, 0x34};
static char nt35510_cmd_20[2] = {0xBF, 0x01};   
static char nt35510_cmd_21[4] = {0xB5, 0x08, 0x08, 0x08};
static char nt35510_cmd_22[4] = {0xBA, 0x24, 0x24, 0x24};
static char nt35510_cmd_23[4] = {0xB4, 0x2D, 0x2D, 0x2D};
static char nt35510_cmd_24[4] = {0xBC, 0x00, 0x68, 0x00};
static char nt35510_cmd_25[4] = {0xBD, 0x00, 0x7C, 0x00};
static char nt35510_cmd_26[3] = {0xBE, 0x00, 0x45};
static char nt35510_cmd_27[5] = {
	0xF0, 0x55, 0xaa, 0x52, 
	0x01};
static char nt35510_cmd_28[53] = {
	0xd1, 0x00, 0x37, 0x00, 
	0x61, 0x00, 0x92, 0x00, 
	0xB4, 0x00, 0xCF, 0x00, 
	0xF6, 0x01, 0x2f, 0x01, 
	0x7F, 0x01, 0x97, 0x01, 
	0xC0, 0x01, 0xE5, 0x02, 
	0x1F, 0x02, 0x50, 0x02, 
	0x52, 0x02, 0x87, 0x02, 
	0xBE, 0x02, 0xE2, 0x03, 
	0x0F, 0x03, 0x30, 0x03, 
	0x5C, 0x03, 0x77, 0x03, 
	0x94, 0x03, 0x9F, 0x03, 
	0xAC, 0x03, 0xBA, 0x03, 
	0xf1};
static char nt35510_cmd_29[53] = {
	0xd2, 0x00, 0x37, 0x00, 
	0x61, 0x00, 0x92, 0x00, 
	0xB4, 0x00, 0xCF, 0x00, 
	0xF6, 0x01, 0x2f, 0x01, 
	0x7F, 0x01, 0x97, 0x01, 
	0xC0, 0x01, 0xE5, 0x02, 
	0x1F, 0x02, 0x50, 0x02, 
	0x52, 0x02, 0x87, 0x02, 
	0xBE, 0x02, 0xE2, 0x03, 
	0x0F, 0x03, 0x30, 0x03, 
	0x5C, 0x03, 0x77, 0x03, 
	0x94, 0x03, 0x9F, 0x03, 
	0xAC, 0x03, 0xBA, 0x03, 
	0xf1};  
static char nt35510_cmd_30[53] = {
	0xd3, 0x00, 0x37, 0x00, 
	0x61, 0x00, 0x92, 0x00, 
	0xB4, 0x00, 0xCF, 0x00, 
	0xF6, 0x01, 0x2f, 0x01, 
	0x7F, 0x01, 0x97, 0x01, 
	0xC0, 0x01, 0xE5, 0x02, 
	0x1F, 0x02, 0x50, 0x02, 
	0x52, 0x02, 0x87, 0x02, 
	0xBE, 0x02, 0xE2, 0x03, 
	0x0F, 0x03, 0x30, 0x03, 
	0x5C, 0x03, 0x77, 0x03, 
	0x94, 0x03, 0x9F, 0x03, 
	0xAC, 0x03, 0xBA, 0x03, 
	0xf1};
static char nt35510_cmd_31[53] = {
	0xd4, 0x00, 0x37, 0x00, 
	0x61, 0x00, 0x92, 0x00, 
	0xB4, 0x00, 0xCF, 0x00, 
	0xF6, 0x01, 0x2f, 0x01, 
	0x7F, 0x01, 0x97, 0x01, 
	0xC0, 0x01, 0xE5, 0x02, 
	0x1F, 0x02, 0x50, 0x02, 
	0x52, 0x02, 0x87, 0x02, 
	0xBE, 0x02, 0xE2, 0x03, 
	0x0F, 0x03, 0x30, 0x03, 
	0x5C, 0x03, 0x77, 0x03, 
	0x94, 0x03, 0x9F, 0x03, 
	0xAC, 0x03, 0xBA, 0x03, 
	0xf1};
static char nt35510_cmd_32[53] = {
	0xd5, 0x00, 0x37, 0x00, 
	0x61, 0x00, 0x92, 0x00, 
	0xB4, 0x00, 0xCF, 0x00, 
	0xF6, 0x01, 0x2f, 0x01, 
	0x7F, 0x01, 0x97, 0x01, 
	0xC0, 0x01, 0xE5, 0x02, 
	0x1F, 0x02, 0x50, 0x02, 
	0x52, 0x02, 0x87, 0x02, 
	0xBE, 0x02, 0xE2, 0x03, 
	0x0F, 0x03, 0x30, 0x03, 
	0x5C, 0x03, 0x77, 0x03, 
	0x94, 0x03, 0x9F, 0x03, 
	0xAC, 0x03, 0xBA, 0x03, 
	0xf1};
static char nt35510_cmd_33[53] = {
	0xd6, 0x00, 0x37, 0x00, 
	0x61, 0x00, 0x92, 0x00, 
	0xB4, 0x00, 0xCF, 0x00,
	0xF6, 0x01, 0x2f, 0x01, 
	0x7F, 0x01, 0x97, 0x01, 
	0xC0, 0x01, 0xE5, 0x02, 
	0x1F, 0x02, 0x50, 0x02, 
	0x52, 0x02, 0x87, 0x02, 
	0xBE, 0x02, 0xE2, 0x03, 
	0x0F, 0x03, 0x30, 0x03, 
	0x5C, 0x03, 0x77, 0x03, 
	0x94, 0x03, 0x9F, 0x03, 
	0xAC, 0x03, 0xBA, 0x03, 
	0xf1};
static char nt35510_cmd_34[2] = {0x36, 0x02};
#endif
//

static struct dsi_cmd_desc hx8357_cmd_on_cmds[] = {
#if 0
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video0), video0},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video1), video1},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video2), video2},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video3), video3},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video4), video4},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video5), video5},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video6), video6},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video7), video7},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video8), video8},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video9), video9},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video10), video10},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video11), video11},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video12), video12},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video13), video13},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video14), video14},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video15), video15},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video16), video16},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video17), video17},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video18), video18},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video19), video19},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video20), video20},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video21), video21},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video22), video22},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video23), video23},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video24), video24},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video25), video25},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video26), video26},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video27), video27},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video28), video28},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video29), video29},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video30), video30},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video31), video31},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video32), video32},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video33), video33},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video34), video34},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video35), video36},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video37), video37},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video38), video38},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video39), video39},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video40), video40},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video41), video41},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video42), video42},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video43), video43},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video44), video44},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video45), video45},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video46), video46},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video47), video47},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video48), video48},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video49), video49},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video50), video50},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video51), video51},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video52), video52},
	//{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video53), video53},
	//{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video54), video54},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video56), video56},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video55), video55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video57), video57},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video58), video58},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video59), video59},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video60), video60},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video61), video61},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video62), video62},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video63), video63},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video64), video64},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video65), video65},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video66), video66},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video67), video67},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video68), video68},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video69), video69},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video70), video70},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video71), video71},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video72), video72},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video73), video73},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video74), video74},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video75), video75},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video76), video76},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video77), video77},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video78), video78},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video79), video79},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video80), video80},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video81), video81},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video82), video82},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video83), video83},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video84), video84},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video85), video85},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video86), video86},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video87), video87},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video88), video88},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video89), video89},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video90), video90},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video91), video91},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video92), video92},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video93), video93},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video94), video94},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video95), video95},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video96), video96},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video97), video97},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video98), video98},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video99), video99},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video100), video100},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video101), video101},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video102), video102},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video103), video103},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video104), video104},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video105), video105},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video106), video106},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video107), video107},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video108), video108},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video109), video109},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video110), video110},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video111), video111},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video112), video112},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video113), video113},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video114), video114},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video115), video115},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video116), video116},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video117), video117},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video118), video118},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 150, sizeof(video119), video119},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 50, sizeof(video120), video120},
#else
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 120, sizeof(nt35510_cmd_00), nt35510_cmd_00},
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
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_26), nt35510_cmd_26},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_27), nt35510_cmd_27},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_28), nt35510_cmd_28},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_29), nt35510_cmd_29},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_30), nt35510_cmd_30},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_31), nt35510_cmd_31},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_32), nt35510_cmd_32},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_33), nt35510_cmd_33},
	{MIPI_SEND_DATA_TYPE, 1, 0, 0, 0, sizeof(nt35510_cmd_34), nt35510_cmd_34},

	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_SLEEP_OFF_DELAY, sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 1, 0, 0, NT35510_DISPLAY_ON_DELAY, sizeof(display_on), display_on},

	{DTYPE_DCS_WRITE, 1, 0, 0, 20,	sizeof(write_ram), write_ram},
#endif
};

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
#if 0
#ifdef DSI_BIT_CLK_500M
#if 1
	/* DSI_BIT_CLK at 454MHz, 2 lane, RGB888 */

	{0x03, 0x01, 0x01, 0x00},/* regulator */
	/* timing   */
	{0x80, 0x30, 0x12, 0x00, 0x40, 0x4A, 0x16, 0x34,
		0x16, 0x03, 0x04},
	/* phy ctrl */
	{0x7f, 0x00, 0x00, 0x00},
	/* strength */
	{0xee, 0x02, 0x86, 0x00},
	/* pll control */
	{0x01, 0xc0, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0f, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
#endif

#else
      /* DSI_BIT_CLK at 800MHz, 2 lane, RGB888 */
		{0x03, 0x01, 0x01, 0x00},	/* regulator */
		/* timing   */
		{0xCF, 0x43, 0x21, 0x0, 0x60, 0x68, 0x26,
		0x47, 0x28, 0x03, 0x04},
		
		{0x7f, 0x00, 0x00, 0x00},	/* phy ctrl */
		{0xee, 0x02, 0x86, 0x00},	/* strength */
		/* pll control */
		{0x1, 0x8a, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
		0x0, 0x07, 0x03,
		0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
#endif
#else
	{0x03, 0x01, 0x01, 0x00},
	/* timing   */
	{0xb9, 0x8e, 0x1f, 0x00, 0x98, 0x9c, 0x22, 0x90,
	0x18, 0x03, 0x04},
	/* phy ctrl */
	{0x7f, 0x00, 0x00, 0x00},
	/* strength */
	{0xbb, 0x02, 0x06, 0x00},
	/* pll control */
	{0x01, 0xec, 0x31, 0xd2, 0x00, 0x40, 0x37, 0x62,
	0x01, 0x0f, 0x07,
	0x05, 0x14, 0x03, 0x0, 0x0, 0x0, 0x20, 0x0, 0x02, 0x0},
#endif
};

#define LCM_RESET_GPIO_PIN85 85
static void msm_lcm_reset(void)
{
	#if 0
	gpio_request(LCM_RESET_GPIO_PIN85,
			"odmm_gpio_reset_en");
	gpio_tlmm_config(GPIO_CFG(LCM_RESET_GPIO_PIN85, 0,
			GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_16MA),
			GPIO_CFG_ENABLE);
	#endif
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 1);  
	mdelay(30);  
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 0);  
	mdelay(30); 
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 1); 
	mdelay(120); 
	gpio_free(LCM_RESET_GPIO_PIN85);
}

#define POWER_ON_NOT_BOOTLOGO 1
static int mipi_hx8357_gpio_init(void)
{
//    if(unlikely(!hx8357_state.disp_initialized))
	{
		int ret;	
		printk(KERN_INFO"%s called gaol\n",__func__);
		return 0;
		ret = gpio_request(85,
			"odmm_gpio_reset_en");
		if (ret < 0)
			return ret;
		printk(KERN_INFO" GPIO request successed\n");
		ret=gpio_tlmm_config(
	        GPIO_CFG(
	        85,
	        0,
	        GPIO_CFG_OUTPUT, //LCDC DEN
	        GPIO_CFG_PULL_UP,//GPIO_CFG_PULL_UP,//GPIO_CFG_PULL_UP,
			GPIO_CFG_16MA),
			GPIO_CFG_ENABLE);

		if(ret)
		{
			printk(KERN_DEBUG"Failure to set DEN Signal init gaol\n");
			return -1;
		}
		hx8357_state.disp_initialized = TRUE;

		hx8357_state.disp_powered_up = TRUE;// first poweron don't reset lcd
    }
	return 0;
}

static int mipi_hx8357_lcd_on(struct platform_device *pdev)
{

	struct msm_fb_data_type *mfd;
	printk(KERN_INFO "mipi_hx8357_lcd_on \n");
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	printk(KERN_INFO "platform_get_drvdata success \n");
	if (mfd->key != MFD_KEY)
		return -EINVAL;
	printk(KERN_INFO "MFD_KEY  Okay\n");
	//mipi_novatek_manufacture_id(mfd);

	down(&nt35516_sem);
	gpio_free(LCM_RESET_GPIO_PIN85);
//	mipi_hx8357_gpio_init();
		
	//if(!hx8357_state.disp_powered_up)
	{
		//mipi_hx8357_exitsleep();
		printk(KERN_INFO "begain to reset LCM\n");
		msm_lcm_reset();
		msleep(10);
		printk(KERN_INFO "[==wxj==]begain to send init cmds\n");
		mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_cmd_on_cmds,
				ARRAY_SIZE(hx8357_cmd_on_cmds));
		//mipi_dsi_cmd_bta_sw_trigger();
		mipi_set_tx_power_mode(0);		
	 	//hx8357_state.disp_powered_up = TRUE;
	}	

	up(&nt35516_sem);

	return 0;
}

static int mipi_hx8357_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	mfd = platform_get_drvdata(pdev);
	
	down(&nt35516_sem);
	printk(KERN_INFO "mipi_hx8357_lcd_off \n");
	//if(hx8357_state.disp_powered_up)
	{
		//mipi_hx8357_entersleep();
		mipi_dsi_cmds_tx(&hx8357_tx_buf, hx8357_sleep_in_cmds,
				ARRAY_SIZE(hx8357_sleep_in_cmds));

		//hx8357_state.disp_powered_up = FALSE;
	}

	gpio_direction_output(85, 0);	//gpio_direction_output(85, 0);
	up(&nt35516_sem);

	return 0;
}

//add by zhangshaosong for PDM backlight
#define PERPH_WEB_BLOCK_ADDR (0xA9D00040)
#define PDM0_CTL_OFFSET (0x04)
#define SIZE_8B (0x08)
#define PDM_DUTY_MAXVAL BIT(16)
#define PDM_DUTY_REFVAL BIT(15)
#define LED_FULL		255
static void hx8357_backlight_set(int level)
{
	int ret = 0;
	if(unlikely(!hx8357_state.bk_init)){
		pmapp_disp_backlight_init();
		hx8357_state.bk_init = TRUE;
	}

	ret = pmapp_disp_backlight_set_brightness(level*230/255);
	if (ret)
		printk(KERN_INFO"can't set backlight level=%d\n",level);
}

static void mipi_hx8357_set_backlight(struct msm_fb_data_type *mfd)
{

	int level;

	level = mfd->bl_level;

     	printk(KERN_INFO"==mddi_current_bl %d ==level %d ==\n",mddi_current_bl,level);
	if(mddi_current_bl == level)
        	return;

	hx8357_backlight_set(level);
     	mddi_current_bl = level;

	return;	 
}


static int __devinit mipi_hx8357_lcd_probe(struct platform_device *pdev)
{
	pr_debug("[LCD]%s is called\n", __func__);
	printk(KERN_INFO"[==wxj==]%s been called \n",__func__);
	if (pdev->id == 0) 
	{
		mipi_hx8357_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);

	mipi_hx8357_gpio_init();

	msm_lcm_reset();

	return 0;
}


static void mipi_nt35516_shutdown(struct platform_device *pdev)
{
	hx8357_backlight_set(0);
	gpio_direction_output(LCM_RESET_GPIO_PIN85, 0);
	msleep(120);
}
static struct platform_driver this_driver = {
	.probe  = mipi_hx8357_lcd_probe,
	.shutdown = mipi_nt35516_shutdown,
	.driver = {
		.name   = "mipi_nt35516",
	},
};

static struct msm_fb_panel_data hx8357_panel_data = {
	.on		= mipi_hx8357_lcd_on,
	.off		= mipi_hx8357_lcd_off,
	.set_backlight = mipi_hx8357_set_backlight,
};

static struct platform_device this_device = {
	.name   = "mipi_nt35516",
	.id	= 2,
	.dev	= {
		.platform_data = &hx8357_panel_data,
	}
};

static int __init mipi_hx8357_lcd_init(void)
{
	int ret;

	//if(odmm_nt35516_lcd_id != ODMM_35516_NT35516_ID)
	//	return 0;

	mipi_dsi_buf_alloc(&hx8357_tx_buf, DSI_BUF_SIZE);//mipi cmd buffer
	mipi_dsi_buf_alloc(&hx8357_rx_buf, DSI_BUF_SIZE);
	gpio_free(LCM_RESET_GPIO_PIN85);
	msm_lcm_reset();
	ret = platform_driver_register(&this_driver);
//	msm_lcm_reset();
	if (ret)
	{
		printk(KERN_ERR"mipi_hx8357_lcd_init platform_driver_register FAILED gaol \n");
		return ret;
	}

	pinfo.xres = 480;
	pinfo.yres = 800;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	#if 1
	pinfo.lcdc.h_back_porch = 29;
	pinfo.lcdc.h_front_porch = 7;
	pinfo.lcdc.h_pulse_width = 3;//8;
	pinfo.lcdc.v_back_porch = 12;
	pinfo.lcdc.v_front_porch = 16;
	pinfo.lcdc.v_pulse_width = 5;////1;
	#else
	pinfo.lcdc.h_back_porch = 100;
	pinfo.lcdc.h_front_porch = 100;
	pinfo.lcdc.h_pulse_width = 8;
	pinfo.lcdc.v_back_porch = 20;
	pinfo.lcdc.v_front_porch = 20;
	pinfo.lcdc.v_pulse_width = 1;
	#endif

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 255;//16;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;
	
	pinfo.clk_rate = 454000000;

	pinfo.lcd.vsync_enable = TRUE;
	pinfo.lcd.hw_vsync_mode = TRUE;
	pinfo.lcd.refx100 = 7200; //6200/* adjust refx100 to prevent tearing */

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
	pinfo.mipi.dsi_phy_db = &dsi_video_mode_phy_db;//&dsi_cmd_mode_phy_db;
	pinfo.mipi.tx_eot_append = 0x01;
	pinfo.mipi.rx_eot_ignore = 0x0;
	pinfo.mipi.dlane_swap = 0x01;
	
	hx8357_panel_data.panel_info = pinfo;
	
	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);
//	msm_lcm_reset();
	pmapp_disp_backlight_init();
    
	return ret;
}
module_init(mipi_hx8357_lcd_init);

