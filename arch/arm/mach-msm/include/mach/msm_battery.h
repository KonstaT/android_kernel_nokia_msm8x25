
#ifndef __MSM_BATTERY_H__
#define __MSM_BATTERY_H__

#define AC_CHG	0x00000001
#define USB_CHG	0x00000002
#define UNKNOWN_CHG	0x00000004

struct msm_psy_batt_pdata {
	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 batt_technology;
	u32 avail_chg_sources;
	u32 (*calculate_capacity)(u32 voltage);
};

enum {
	BATTERY_REGISTRATION_SUCCESSFUL = 0,
	BATTERY_DEREGISTRATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_MODIFICATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_INTERROGATION_SUCCESSFUL = BATTERY_REGISTRATION_SUCCESSFUL,
	BATTERY_CLIENT_TABLE_FULL = 1,
	BATTERY_REG_PARAMS_WRONG = 2,
	BATTERY_DEREGISTRATION_FAILED = 4,
	BATTERY_MODIFICATION_FAILED = 8,
	BATTERY_INTERROGATION_FAILED = 16,
	/* Client's filter could not be set because perhaps it does not exist */
	BATTERY_SET_FILTER_FAILED         = 32,
	/* Client's could not be found for enabling or disabling the individual
	 * client */
	BATTERY_ENABLE_DISABLE_INDIVIDUAL_CLIENT_FAILED  = 64,
	BATTERY_LAST_ERROR = 128,
};

enum {
	BATTERY_VOLTAGE_UP = 0,
	BATTERY_VOLTAGE_DOWN,
	BATTERY_VOLTAGE_ABOVE_THIS_LEVEL,
	BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
	BATTERY_VOLTAGE_LEVEL,
	BATTERY_ALL_ACTIVITY,
	VBATT_CHG_EVENTS,
	BATTERY_VOLTAGE_UNKNOWN
};

enum chg_charger_status_type {
	CHARGER_STATUS_GOOD,
	CHARGER_STATUS_WEAK,
	CHARGER_STATUS_NULL
};

enum chg_charger_hardware_type {
	CHARGER_TYPE_USB_PC,
	CHARGER_TYPE_USB_WALL,
	CHARGER_TYPE_USB_UNKNOWN,
	CHARGER_TYPE_USB_CARKIT
};

enum chg_battery_status_type {
	BATTERY_STATUS_GOOD,
	BATTERY_STATUS_OVER_TEMPERATURE,
	BATTERY_STATUS_NULL
};

struct msm_batt_gauge {
	int (*get_battery_mvolts) (void);
	int (*get_battery_temperature) (void);
	int (*is_battery_present) (void);
	int (*is_battery_temp_within_range) (void);
	int (*is_battery_id_valid) (void);
	int (*get_battery_status)(void);
	int (*get_batt_remaining_capacity) (void);
};

#ifdef CONFIG_MSM_SM_EVENT
uint32_t msm_batt_get_batt_voltage (void);
#endif
int msm_batt_fuel_register(struct msm_batt_gauge* batt);
void msm_batt_fuel_unregister(struct msm_batt_gauge* batt);

#endif
