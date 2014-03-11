#ifndef __LINUX_FT6306_TS_H__
#define __LINUX_FT6306_TS_H__


#define SCREEN_MAX_X	479//480
#define SCREEN_MAX_Y	799//800

#define SCREEN_ICON_MAX_Y	100//800 hard icon

#define PRESS_MAX	255

#define MAX_TOUCH_POINTS 2
#define FT_MAX_ID         0x0f
#define FT_TOUCH_ID_POS			5
#define FT_TOUCH_STEP	      6

#define CFG_POINT_READ_BUF  (3 + 6 * (MAX_TOUCH_POINTS))

#define FT6306_NAME	"ft6306_touch"//"ft6306_touch" 

struct ft6306_touch_platform_data{
	u16	intr;		/* irq number	*/
	u32	maxx;
	u32	maxy;
	u32	use_virtual_keys:1;
	int (*wakeup)(void);
	int (*init)(int on_off);
	char *name;
	u16   reset;
	u16   irq;	
};

enum FT6306_ts_regs {
	FT6306_REG_THGROUP				= 0x80,     /* touch threshold, related to sensitivity */
	FT6306_REG_THPEAK				= 0x81,
	FT6306_REG_THCAL				= 0x82,
	FT6306_REG_THWATER				= 0x83,
	FT6306_REG_THTEMP				= 0x84,
	FT6306_REG_THDIFF				= 0x85,				
	FT6306_REG_CTRL					= 0x86,
	FT6306_REG_TIMEENTERMONITOR		= 0x87,
	FT6306_REG_PERIODACTIVE			= 0x88,      /* report rate */
	FT6306_REG_PERIODMONITOR		= 0x89,
	FT6306_REG_HEIGHT_B				= 0x8a,
	FT6306_REG_MAX_FRAME			= 0x8b,
	FT6306_REG_DIST_MOVE			= 0x8c,
	FT6306_REG_DIST_POINT			= 0x8d,
	FT6306_REG_FEG_FRAME			= 0x8e,
	FT6306_REG_SINGLE_CLICK_OFFSET	= 0x8f,
	FT6306_REG_DOUBLE_CLICK_TIME_MIN	= 0x90,
	FT6306_REG_SINGLE_CLICK_TIME		= 0x91,
	FT6306_REG_LEFT_RIGHT_OFFSET		= 0x92,
	FT6306_REG_UP_DOWN_OFFSET		= 0x93,
	FT6306_REG_DISTANCE_LEFT_RIGHT	= 0x94,
	FT6306_REG_DISTANCE_UP_DOWN		= 0x95,
	FT6306_REG_ZOOM_DIS_SQR			= 0x96,
	FT6306_REG_RADIAN_VALUE			= 0x97,
	FT6306_REG_MAX_X_HIGH			= 0x98,
	FT6306_REG_MAX_X_LOW			= 0x99,
	FT6306_REG_MAX_Y_HIGH			= 0x9a,
	FT6306_REG_MAX_Y_LOW			= 0x9b,
	FT6306_REG_K_X_HIGH				= 0x9c,
	FT6306_REG_K_X_LOW				= 0x9d,
	FT6306_REG_K_Y_HIGH				= 0x9e,
	FT6306_REG_K_Y_LOW				= 0x9f,
	FT6306_REG_AUTO_CLB_MODE		= 0xa0,
	FT6306_REG_LIB_VERSION_H 		= 0xa1,
	FT6306_REG_LIB_VERSION_L 		= 0xa2,		
	FT6306_REG_CIPHER				= 0xa3,
	FT6306_REG_MODE					= 0xa4,
	FT6306_REG_PMODE				= 0xa5,	  /* Power Consume Mode		*/	
	FT6306_REG_FIRMID				= 0xa6,   /* Firmware version */
	FT6306_REG_STATE				= 0xa7,
	FT6306_REG_FT5201ID				= 0xa8,
	FT6306_REG_ERR					= 0xa9,
	FT6306_REG_CLB					= 0xaa,
};

//FT6306_REG_PMODE
#define PMODE_ACTIVE        0x00
#define PMODE_MONITOR       0x01
#define PMODE_STANDBY       0x02
#define PMODE_HIBERNATE     0x03


#ifndef ABS_MT_TOUCH_MAJOR
#define ABS_MT_TOUCH_MAJOR	0x30	/* touching ellipse */
#define ABS_MT_TOUCH_MINOR	0x31	/* (omit if circular) */
#define ABS_MT_WIDTH_MAJOR	0x32	/* approaching ellipse */
#define ABS_MT_WIDTH_MINOR	0x33	/* (omit if circular) */
#define ABS_MT_ORIENTATION	0x34	/* Ellipse orientation */
#define ABS_MT_POSITION_X	0x35	/* Center X ellipse position */
#define ABS_MT_POSITION_Y	0x36	/* Center Y ellipse position */
#define ABS_MT_TOOL_TYPE	0x37	/* Type of touching device */
#define ABS_MT_BLOB_ID		0x38	/* Group set of pkts as blob */
#endif /* ABS_MT_TOUCH_MAJOR */

#ifndef ABS_MT_TRACKING_ID
#define ABS_MT_TRACKING_ID	0x39 /* Unique ID of initiated contact */
#endif

#endif
