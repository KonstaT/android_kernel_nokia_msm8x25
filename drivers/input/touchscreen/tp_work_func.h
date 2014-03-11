struct virtual_key{
	const char	*menu;
	const char	*home;
	const char	*back;
	const char	*search;
};

struct tp_platform_data{
	struct kobj_attribute byd_virtual_keys_attr;
	const char	*input_dev_name;
	// [sun.yu5@byd.com, modify,WG703T2_C000133,support 2  touchscreen, define  struct virtual_key vk from 1 to 2]
	struct virtual_key	vk[2];
	// [sun.yu5@byd.com, end]  
	int lcd_x;
	int lcd_y;
	int tp_x;
	int tp_y;
	int irq;
	int wakeup;
};

struct tp_dev{
	struct i2c_client	*client;
	struct input_dev	*input_dev;
	struct tp_platform_data *pdata;
	struct work_struct  work;
	void (*work_func)(struct work_struct *work);
};

extern int tp_work_func_register(struct tp_dev *tp_device);
extern void tp_work_func_unregister(void);