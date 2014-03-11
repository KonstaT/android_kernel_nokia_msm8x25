#ifndef GLANCEVIEW_H
#define GLANCEVIEW_H
#define GV_DEVICE_ID_COUNT 2

struct gv_state
{
	unsigned  gv_2taps_setflag:1;
	unsigned  gv_2taps_curmod:1;
	unsigned  gv_lcd_setflag:1;
	unsigned  gv_lcd_curmod:1;
	unsigned  gv_reserve:4;
};




enum  glanceview_index {
		gv_device_id_lcd=0,
		gv_device_id_gsensor,
	    gv_device_id_count
};

enum  glanceview_op{
      gv_disable=0,
      gv_enable,
      gv_gvmode,
};
struct glanceview_ops {
	int	(*gv_glance_mode)(struct device *,enum  glanceview_op);
	struct device * data;
	char state;
};


enum  gvop_val{
      gv_bypass=0,
      gv_filtered,

};

int glanceview_register(enum  glanceview_index index,
    struct glanceview_ops *new_gv_dev,struct device * data);
enum  gvop_val glanceview_filter(enum  glanceview_index index,enum glanceview_op op);
    

#endif// GLANCEVIEW_H


