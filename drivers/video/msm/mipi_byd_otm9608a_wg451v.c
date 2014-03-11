#include <linux/module.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mdp4.h"
#include <mach/gpio.h>
#include <mach/pmic.h>
#include <linux/clk.h>
#include <mach/msm_iomap.h>
#include <mach/vreg.h>
#include <asm/io.h>
#include <linux/moduleparam.h>
#include <mach/rpc_pmapp.h>
#include <linux/wait.h>
#include <linux/mm.h>

static void otm9608a_backlight_set(int level);
static struct msm_panel_info pinfo;

static struct msm_panel_common_pdata *mipi_otm9608a_pdata;
static uint32 mddi_current_bl = 255;//added by zxd to ignore the same level.
static struct dsi_buf otm9608a_tx_buf;
static struct dsi_buf otm9608a_rx_buf;
#define SN3228B_MAX_BACKLIGHT     14
//[shao.wenqi@byd.com; modify; modify Backlig GPIO]
#define LCD_BL_GPIO  96
//[shao.wenqi@byd.com; end]
#define GP_CLK_M		1
#define GP_CLK_N		150
#define LCM_RESET_GPIO_PIN30 30  
DEFINE_SEMAPHORE(otm9608a_byd_sem);

struct otm9608a_state_type
{
       bool	disp_initialized;/* disp initialized */
       bool	display_on;	/* lcd is on */
       bool	disp_powered_up;/* lcd power and lcd bl power on */
	   //[shao.wenqi@byd.com; add; ]
	   	bool gp_clk_flag;
	   //[shao.wenqi@byd.com; end;]
		bool bk_init;

		
};

typedef struct otm9608a_state_type	otm9608a_state_t;
static otm9608a_state_t otm9608a_state= { 0 };


static char sleep_in[1] = {0x10};
static char display_off[1] = {0x28};
static struct dsi_cmd_desc otm9608a_sleep_in_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 20, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 150, sizeof(sleep_in), sleep_in}};

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
static char video11[10] = {0xC0,0x00,0x48,0x00,0x10,0x10,0x00,0x47,0x10,0x10};
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
static char video31[8] = {0xC5,0x96,0x57,0x00,0x57,0x33,0x33,0x34};
static char video32[2] = {0x00,0xA0};
static char video33[8] = {0xC5,0x96,0x57,0x00,0x57,0x33,0x33,0x34};
static char video34[2] = {0x00,0xB0};
static char video35[8] = {0xC5,0x04,0xAC,0x01,0x00,0x71,0xB1,0x83};
static char video36[2] = {0x00,0x00};
static char video37[2] = {0xD9,0x61};
static char video38[2] = {0x00,0x80};
static char video39[2] = {0xC6,0x64};
static char video40[2] = {0x00,0xB0};
static char video41[6] = {0xC6,0x03,0x10,0x00,0x1F,0x12};
static char video42[2] = {0x00,0xB7};
static char video43[2] = {0xB0,0x10};
static char video44[2] = {0x00,0xC0};
static char video45[2] = {0xB0,0x55};
static char video46[2] = {0x00,0xB1};
static char video47[2] = {0xB0,0x03};
static char video48[2] = {0x00,0x81};
static char video49[2] = {0xD6,0x00};
static char video50[2] = {0x00,0x00};
static char video51[17] = {0xE1,0x00,0x0D,0x13,0x0F,0x07,0x11,0x0B,0x0A,0x03,0x06,0x0B,0x08,0x0D,0x0E,0x09,0x01};
static char video52[2] = {0x00,0x00};
static char video53[17] = {0xE2,0x00,0x0F,0x15,0x0E,0x08,0x10,0x0B,0x0C,0x02,0x04,0x0B,0x04,0x0E,0x0D,0x08,0x00};
static char video54[2] = {0x00,0x80};	 
static char video55[11] = {0xCB,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video56[2] = {0x00,0x90};	 
static char video57[16] = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video58[2] = {0x00,0xA0};	 
static char video59[16] = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video60[2] = {0x00,0xB0};	 
static char video61[11] = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video62[2] = {0x00,0xC0};	 
static char video63[16] = {0xCB,0x00,0x00,0x00,0x04,0x00,0x00,0x04,0x04,0x00,0x00,0x04,0x04,0x04,0x00,0x00};
static char video64[2] = {0x00,0xD0};	 
static char video65[16] = {0xCB,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x04,0x00,0x00,0x04,0x04};
static char video66[2] = {0x00,0xE0};	 
static char video67[11] = {0xCB,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00};
static char video68[2] = {0x00,0xF0};	 
static char video69[11] = {0xCB,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static char video70[2] = {0x00,0x80};	 
static char video71[11] = {0xCC,0x00,0x00,0x00,0x02,0x00,0x00,0x0A,0x0E,0x00,0x00};
static char video72[2] = {0x00,0x90};	 
static char video73[16] = {0xCC,0x0C,0x10,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x09};
static char video74[2] = {0x00,0xA0};	 
static char video75[16] = {0xCC,0x0D,0x00,0x00,0x0B,0x0F,0x00,0x00,0x00,0x00,0x05,0x00,0x00,0x00,0x00,0x00};
static char video76[2] = {0x00,0xB0};	 
static char video77[11] = {0xCC,0x00,0x00,0x00,0x02,0x00,0x00,0x0A,0x0E,0x00,0x00};
static char video78[2] = {0x00,0xC0};	 
static char video79[16] = {0xCC,0x0C,0x10,0x06,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
static char video80[2] = {0x00,0xD0};	 
static char video81[16] = {0xCC,0x05,0x00,0x00,0x00,0x00,0x0F,0x0B,0x00,0x00,0x0D,0x09,0x01,0x00,0x00,0x00};
static char video82[2] = {0x00,0x80};
static char video83[13] = {0xCE,0x84,0x03,0x18,0x83,0x03,0x18,0x00,0x00,0x00,0x00,0x00,0x00};
static char video84[2] = {0x00,0x90};
static char video85[15] = {0xCE,0x33,0xBF,0x18,0x33,0xC0,0x18,0x10,0x0F,0x18,0x10,0x10,0x18,0x00,0x00};
static char video86[2] = {0x00,0xA0};
static char video87[15] = {0xCE,0x38,0x02,0x03,0xC1,0x00,0x18,0x00,0x38,0x01,0x03,0xC2,0x00,0x18,0x00};
static char video88[2] = {0x00,0xB0};
static char video89[15] = {0xCE,0x38,0x00,0x03,0xC3,0x00,0x18,0x00,0x30,0x00,0x03,0xC4,0x00,0x18,0x00};
static char video90[2] = {0x00,0xC0};
static char video91[15] = {0xCE,0x30,0x01,0x03,0xC5,0x00,0x18,0x00,0x30,0x02,0x03,0xC6,0x00,0x18,0x00};
static char video92[2] = {0x00,0xD0};
static char video93[15] = {0xCE,0x30,0x03,0x03,0xC7,0x00,0x18,0x00,0x30,0x04,0x03,0xC8,0x00,0x18,0x00};
static char video94[2] = {0x00,0x80};
static char video95[15] = {0xCF,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video96[2] = {0x00,0x90};
static char video97[15] = {0xCF,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video98[2] = {0x00,0xA0};
static char video99[15] = {0xCF,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video100[2] = {0x00,0xB0};
static char video101[15] = {0xCF,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00};
static char video102[2] = {0x00,0xC0};
static char video103[11] = {0xCF,0x01,0x01,0x20,0x20,0x00,0x00,0x02,0x04,0x00,0x00};
static char video104[2] = {0x00,0x00};
static char video105[3] = {0xD8,0xA7,0xA7};
static char video106[2] = {0x00,0x00};
static char video107[4] = {0xFF,0xFF,0xFF,0xFF};
static char video108[2] = {0x11,0x00};
static char video109[2] = {0x29,0x00};


//

#if 1
static struct dsi_cmd_desc otm9608a_video_display_on_cmds[] = {
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
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video53), video53},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video54), video54},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video55), video55},
	{DTYPE_GEN_LWRITE, 1, 0, 0, 0, sizeof(video56), video56},
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
	{DTYPE_GEN_LWRITE, 1, 0, 0, 120, sizeof(video109), video109},
};
#else
static struct dsi_cmd_desc otm9608a_video_display_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video0), video0},
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
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video19), video19},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video20), video20},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video21), video21},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video22), video22},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video23), video23},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video24), video24},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video25), video25},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video26), video26},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video27), video27},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video28), video28},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video29), video29},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video30), video30},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video31), video31},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video32), video32},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video33), video33},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video34), video34},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video35), video36},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video37), video37},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video38), video38},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video39), video39},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video40), video40},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video41), video41},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video42), video42},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video43), video43},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video44), video44},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video45), video45},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video46), video46},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video47), video47},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video48), video48},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video49), video49},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video50), video50},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video51), video51},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video52), video52},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video53), video53},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video54), video54},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video55), video55},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video56), video56},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video57), video57},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video58), video58},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video59), video59},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video60), video60},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video61), video61},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video62), video62},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video63), video63},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video64), video64},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video65), video65},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video66), video66},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video67), video67},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video68), video68},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video69), video69},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video70), video70},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video71), video71},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video72), video72},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video73), video73},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video74), video74},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video75), video75},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video76), video76},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video77), video77},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video78), video78},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video79), video79},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video80), video80},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video81), video81},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video82), video82},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video83), video83},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video84), video84},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video85), video85},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video86), video86},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video87), video87},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video88), video88},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video89), video89},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video90), video90},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video91), video91},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video92), video92},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video93), video93},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video94), video94},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video95), video95},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video96), video96},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video97), video97},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video98), video98},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video99), video99},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video100), video100},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video101), video101},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video102), video102},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video103), video103},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video104), video104},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video105), video105},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video106), video106},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video107), video107},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video108), video108},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video109), video109},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video110), video110},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 0, sizeof(video111), video111},
};

#endif

static struct mipi_dsi_phy_ctrl dsi_video_mode_phy_db = {
/* DSI_BIT_CLK at 500MHz, 2 lane, RGB888 */
		{0x03, 0x01, 0x01, 0x00},	/* regulator */
		/* timing   */
		{0x82, 0x31, 0x13, 0x0, 0x42, 0x4D, 0x18,
		0x35, 0x21, 0x03, 0x04},
		{0x7f, 0x00, 0x00, 0x00},	/* phy ctrl */
		{0xee, 0x02, 0x86, 0x00},	/* strength */
		/* pll control */
		{0x40, 0xf9, 0xb0, 0xda, 0x00, 0x50, 0x48, 0x63,
#if 1//defined(NOVATEK_TWO_LANE)
		0x30, 0x07, 0x03,
#else           /* default set to 1 lane */
		0x30, 0x07, 0x07,
#endif
		0x05, 0x14, 0x03, 0x0, 0x0, 0x54, 0x06, 0x10, 0x04, 0x0},
};

#define POWER_ON_NOT_BOOTLOGO 1
static int mipi_otm9608a_gpio_init(void)
{	
	printk(KERN_WARNING"lxj %s\n",__func__);
    if(unlikely(!otm9608a_state.disp_initialized)){
		int ret;	
		printk(KERN_INFO"%s called gaol\n",__func__);

		ret = gpio_request(LCM_RESET_GPIO_PIN30,
			"odmm_gpio_bkl_en");
		if (ret < 0)
			return ret;
		ret=gpio_tlmm_config(
	        GPIO_CFG(
	        LCM_RESET_GPIO_PIN30,
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
		otm9608a_state.disp_initialized = TRUE;

		//otm9608a_state.disp_powered_up = TRUE;  //remove first poweron don't reset lcd
    }
	return 0;
}

static int mipi_otm9608a_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk(KERN_WARNING"LCD %s is called\n",__func__);
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	
	//mipi_novatek_manufacture_id(mfd);
	down(&otm9608a_byd_sem);
	
	mipi_otm9608a_gpio_init();
	

	printk(KERN_WARNING"LCD otm9608a_state.disp_powered_up==%d\n",otm9608a_state.disp_powered_up);
	 //if(!otm9608a_state.disp_powered_up)
	 {
	 	//mipi_otm9608a_exitsleep();

		printk(KERN_WARNING"lxj reset and send cmd to LCD \n");
		gpio_direction_output(LCM_RESET_GPIO_PIN30, 1);	
		msleep(10);
		gpio_direction_output(LCM_RESET_GPIO_PIN30, 0);
		msleep(20);
		gpio_direction_output(LCM_RESET_GPIO_PIN30, 1);	
		msleep(130);

		mipi_set_tx_power_mode(1);			
		mipi_dsi_cmds_tx(&otm9608a_tx_buf, otm9608a_video_display_on_cmds,
		ARRAY_SIZE(otm9608a_video_display_on_cmds));
		mipi_set_tx_power_mode(0);
		
	 	//otm9608a_state.disp_powered_up = TRUE;
	 }	
	up(&otm9608a_byd_sem);

	return 0;
}

static int mipi_otm9608a_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk(KERN_WARNING"lxj %s\n",__func__);
	
	mfd = platform_get_drvdata(pdev);
	
	#if 1
	down(&otm9608a_byd_sem);
	
	//if(otm9608a_state.disp_powered_up)
	{
		//mipi_otm9608a_entersleep();
		   mipi_dsi_cmds_tx(&otm9608a_tx_buf, otm9608a_sleep_in_cmds,
			ARRAY_SIZE(otm9608a_sleep_in_cmds));

		//otm9608a_state.disp_powered_up = FALSE;
	}
	gpio_direction_output(LCM_RESET_GPIO_PIN30, 0);

	up(&otm9608a_byd_sem);
	#endif
	return 0;

}
//[shao.wenqi@byd.com; add]
#define ODMM_CLOSE_ORG_DESIGN 0
//[shao.wenqi@byd.com; end]
#if ODMM_CLOSE_ORG_DESIGN
static void otm9608a_backlight_set(int level)
{
printk(KERN_INFO"%s called %d gaol \n",__func__, level);


//for sn3228b backlight.
    int i;
    uint32 pulse_num = 0;
   // static uint32 current_level = SN3228B_MIN_BACKLIGHT;
    static uint32 level_list[SN3228B_MAX_BACKLIGHT + 1] = {0,14,13,12,11,10,9,8,7,6,5,4,3,2,1};//15 level
	printk(KERN_INFO"otm9608a_backlight_set %d gaol \n",level);        
	if(0 != level)
	{
	         pulse_num = level_list[level];

		for(i=0; i < pulse_num; i++)
	    {
	        gpio_set_value(LCD_BL_GPIO,0);         
	        udelay(1);
	        gpio_set_value(LCD_BL_GPIO,1);      
	        udelay(1);
	    }
	}
	else
	{
	    gpio_set_value(LCD_BL_GPIO,0);//backlight setting off
	    mdelay(5);
	    return;
	}
	            
//      printk(KERN_INFO "gpio %d pulse_num:%d, level:%d\n", LCD_BL_GPIO,pulse_num, level);

     return;

 int ret =-1;	
    	if(level) 
	    {
			if(otm9608a_state.disp_powered_up!= TRUE)
			{   
				printk(KERN_INFO "invalid backlight setting,cause lcd panel not ready,level is %d,zxd \n",level);
				//return;				
			}
			if(!otm9608a_state.gp_clk_flag)
			{
				ret = clk_enable(gp_clk);
                if(ret)
                    {
                        printk(KERN_INFO "gp_clk enable failed,ret= %d,zxd \n",ret);
                    }
				otm9608a_state.gp_clk_flag = TRUE;
			}
			
			/*gp clk = 32KHz, source ckl = 19.2Mhz ,now pwm need 200~30khz, 
			P_Div=4 N-M=149 M=1 2*D=2*N/MAX(level)*level ,(0~15)16 levels backlight. */		
			writel((1U << 16) | (~(270-level*18)& 0xffff),
				MSM_CLK_CTL_BASE + 0x58);
			writel((~(GP_CLK_N-GP_CLK_M)<< 16)|0xf58, MSM_CLK_CTL_BASE + 0x5c);
			
			ret=gpio_tlmm_config(GPIO_CFG
				(LCD_BL_GPIO,
				1,
				GPIO_CFG_OUTPUT, 		
				GPIO_CFG_PULL_UP,
				GPIO_CFG_16MA),
				GPIO_CFG_ENABLE);
		}
		else
		{
			ret = gpio_tlmm_config(GPIO_CFG
				(LCD_BL_GPIO,
				0,
				GPIO_CFG_OUTPUT,//LCD backlight EN OUTPUT
				GPIO_CFG_PULL_DOWN,
				GPIO_CFG_2MA),
				GPIO_CFG_ENABLE);
			gpio_set_value(LCD_BL_GPIO,0);
			writel(0x0, MSM_CLK_CTL_BASE + 0x5c);
			if(otm9608a_state.gp_clk_flag)
			{
				clk_disable(gp_clk);
				otm9608a_state.gp_clk_flag = FALSE;
			}
		}
              printk(KERN_INFO "otm9608a_backlight_set level is :%d,pwm \n",level);
		return;

}		
#endif

//add by zhangshaosong for PDM backlight
#define PERPH_WEB_BLOCK_ADDR (0xA9D00040)
#define PDM0_CTL_OFFSET (0x04)
#define SIZE_8B (0x08)
#define PDM_DUTY_MAXVAL BIT(16)
#define PDM_DUTY_REFVAL BIT(15)
#define LED_FULL		255
static void otm9608a_backlight_set(int level)
{
#if 0  //deleted by gaoguifeng temp for P0
	void __iomem *perph_base;
	int pdm_offset;
	u32 tcxo_pdm_ctl;
	u16 duty_val;
	int  duty_per;
	int id;
	 perph_base = ioremap(PERPH_WEB_BLOCK_ADDR, (0x08 - 1));
	 /* Pulse Density Modulation(PDM) ids start with 0 and
	  * every PDM register takes 4 bytes
	  */
	 id = 0;
	 pdm_offset = ((id) + 1) * 4;
	         
	 /* program tcxo_pdm_ctl register to enable pdm*/
	tcxo_pdm_ctl = readl_relaxed(perph_base);
	tcxo_pdm_ctl |= (1 << id);
	writel_relaxed(tcxo_pdm_ctl, perph_base);
	duty_per =100- (level * 100) / LED_FULL;
	duty_val = PDM_DUTY_REFVAL - ((PDM_DUTY_MAXVAL * duty_per) / 100);
	if (!duty_per)
		duty_val--;
	//printk(KERN_INFO "[zss]== pdm  duty_val :%d,\n",duty_val);
	writel_relaxed(duty_val, perph_base + pdm_offset);
#else
    int ret = 0;
	if(unlikely(!otm9608a_state.bk_init)){
			pmapp_disp_backlight_init();
			otm9608a_state.bk_init = TRUE;
	}


	ret = pmapp_disp_backlight_set_brightness(level);
	if (ret)
     	printk(KERN_INFO"can't set backlight level=%d\n",level);
#endif  //end 
}		
static void mipi_otm9608a_set_backlight(struct msm_fb_data_type *mfd)
{
     // [shao.wenqi@byd.com; delete; delete for org]
	 #if ODMM_CLOSE_ORG_DESIGN
     mfd->bl_level *= 10;
     printk(KERN_INFO"mipi_otm9608a_set_backlight mddi_current_bl %d mfd->bl_level %d \n",mddi_current_bl,mfd->bl_level);
     if(mddi_current_bl == mfd->bl_level)
        return;
     //pmapp_disp_backlight_set_brightness(mfd->bl_level);

     mddi_current_bl = mfd->bl_level;
	 #else
	 int level;
	level = mfd->bl_level;
	if(mddi_current_bl == level)
        	return;
     	//printk(KERN_INFO"==mddi_current_bl %d ==level %d ==\n",mddi_current_bl,level/16);
	otm9608a_backlight_set(level);
     	mddi_current_bl = level;
	return;
	 #endif
	 
}


static int __devinit mipi_otm9608a_lcd_probe(struct platform_device *pdev)
{
	printk(KERN_WARNING"lxj %s\n",__func__);
	if (pdev->id == 0) 
	{
		mipi_otm9608a_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_otm9608a_lcd_probe,
	.driver = {
		.name   = "mipi_otm9608a",
	},
};

static struct msm_fb_panel_data otm9608a_panel_data = {
	.on		= mipi_otm9608a_lcd_on,
	.off		= mipi_otm9608a_lcd_off,
	.set_backlight = mipi_otm9608a_set_backlight,
};

static struct platform_device this_device = {
	.name   = "mipi_otm9608a",
	.id	= 1,
	.dev	= {
		.platform_data = &otm9608a_panel_data,
	}
};

static int __init mipi_otm9608a_lcd_init(void)
{
	int ret;

	printk(KERN_WARNING"lxj %s\n",__func__);
	/* #suwg: if(!odmm_nt35516_lcd_id)
		return 0; */
	
	mipi_dsi_buf_alloc(&otm9608a_tx_buf, DSI_BUF_SIZE);			//mipi cmd buffer
	mipi_dsi_buf_alloc(&otm9608a_rx_buf, DSI_BUF_SIZE);
        ret = platform_driver_register(&this_driver);
	if (ret)
	{
	   printk(KERN_ERR"mipi_otm9608a_lcd_init platform_driver_register FAILED gaol \n");
          return ret;
	}

	pinfo.xres = 540;
	pinfo.yres = 960;
	pinfo.type = MIPI_CMD_PANEL;
	pinfo.pdest = DISPLAY_1;
	pinfo.wait_cycle = 0;
	pinfo.bpp = 24;
	pinfo.lcdc.h_back_porch = 50;//100;
	pinfo.lcdc.h_front_porch = 40;//100;
	pinfo.lcdc.h_pulse_width = 10;//8;
	pinfo.lcdc.v_back_porch = 8;//20;
	pinfo.lcdc.v_front_porch = 8;//20;
	pinfo.lcdc.v_pulse_width = 4;////1;

	pinfo.lcdc.border_clr = 0;	/* blk */
	pinfo.lcdc.underflow_clr = 0xff;	/* blue */
	pinfo.lcdc.hsync_skew = 0;
	pinfo.bl_max = 15;//16;
	pinfo.bl_min = 1;
	pinfo.fb_num = 2;

	pinfo.clk_rate = 499000000;
	pinfo.lcd.vsync_enable = FALSE;//TRUE;
	pinfo.lcd.hw_vsync_mode = FALSE;//TRUE;
	pinfo.lcd.refx100 = 5600;//7000;//6100; /* adjust refx100 to prevent tearing */

	pinfo.mipi.mode = DSI_CMD_MODE;
	pinfo.mipi.dst_format = DSI_CMD_DST_FORMAT_RGB888;
	pinfo.mipi.vc = 0;
	pinfo.mipi.rgb_swap = DSI_RGB_SWAP_BGR;//DSI_RGB_SWAP_RGB;
	pinfo.mipi.data_lane0 = TRUE;
	pinfo.mipi.data_lane1 = TRUE;
	pinfo.mipi.t_clk_post = 0x20;
	pinfo.mipi.t_clk_pre = 0x2F;
	pinfo.mipi.stream = 0; /* dma_p */
	pinfo.mipi.mdp_trigger = DSI_CMD_TRIGGER_NONE;//DSI_CMD_TRIGGER_SW_TE;//DSI_CMD_TRIGGER_SW_TE;
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
	
	otm9608a_panel_data.panel_info = pinfo;
	
	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

    pmapp_disp_backlight_init();

	return ret;
}
module_init(mipi_otm9608a_lcd_init);
