/* Copyright (c) 2012-2013, The Linux Foundation. All Rights Reserved.
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

/*
 * this needs to be before <linux/kernel.h> is loaded,
 * and <linux/sched.h> loads <linux/kernel.h>
 */
#define DEBUG 1

#include <linux/slab.h>
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include <mach/msm_rpcrouter.h>
#include <mach/msm_battery.h>
#ifdef CONFIG_MSM_SM_EVENT
#include <linux/sm_event_log.h>
#include <linux/sm_event.h>
#endif

#define	BATTERY_RPC_PROG		0x30000089
#define	BATTERY_RPC_VER_5_1		0x00050001

#define	BATTERY_REGISTER_PROC		2
#define	BATTERY_DEREGISTER_CLIENT_PROC	5

#define	BATTERY_RPC_CB_PROG		(BATTERY_RPC_PROG | 0x01000000)

#define	BATTERY_CB_TYPE_PROC		1
#define	BATTERY_CB_ID_ALL		1
#define	BATTERY_CB_ID_LOW_V		2
#define	BATTERY_CB_ID_CHG_EVT		3

#define	CHG_RPC_PROG			0x3000001a
#define	CHG_RPC_VER_1_1			0x00010001

#define	CHG_GET_GENERAL_STATUS_PROC	9

#define	RPC_TIMEOUT			5000	/* 5 sec */
#define	INVALID_HANDLER			-1

#define	MSM_BATT_POLLING_TIME		(10 * HZ)

#define	BATTERY_HIGH			4200
#define	BATTERY_LOW			3500
#define BATTERY_ULTRA_LOW		3400

#define	TEMPERATURE_HOT			350
#define	TEMPERATURE_COLD		50

struct msm_battery_info {
	struct msm_rpc_endpoint *charger_endpoint;
	struct msm_rpc_client *battery_client;
	u32 charger_api_version;
	u32 battery_api_version;

	u32 voltage_max_design;
	u32 voltage_min_design;
	u32 battery_technology;
	u32 available_charger_src;
	u32 (*calculate_capacity)(u32 voltage);

	struct power_supply *msm_psy_ac;
	struct power_supply *msm_psy_usb;
	struct power_supply *msm_psy_battery;
	struct power_supply *msm_psy_unknown;

#ifdef CONFIG_BATTERY_EARLYSUSPEND
	struct early_suspend early_suspend;
	bool is_suspended;
	struct mutex suspend_lock;
#endif /* CONFIG_BATTERY_EARLYSUSPEND */

	struct workqueue_struct	*battery_queue;
	struct delayed_work battery_work;
	struct mutex update_mutex;
	struct wake_lock charger_cb_wake_lock;

	s32 charger_handler;
	s32 battery_handler;

	int fuel_gauge;
	int (*get_battery_mvolts) (void);
	int (*get_battery_temperature) (void);
	int (*is_battery_present) (void);
	int (*is_battery_temp_within_range) (void);
	int (*is_battery_id_valid) (void);
	int (*get_battery_status)(void);
	int (*get_batt_remaining_capacity) (void);

	u32 charger_status;
	u32 charger_hardware;
	u32 hide;
	u32 battery_status;
	u32 battery_voltage;
	u32 battery_capacity;
	s32 battery_temp;
	u32 is_charging;
	u32 is_charging_complete;
	u32 is_charging_failed;

	struct power_supply *current_psy;
	u32 current_charger_src;

	u32 psy_status;
	u32 psy_health;
};

static struct msm_battery_info msm_battery_info = {
	.battery_handler = INVALID_HANDLER,
	.charger_handler = INVALID_HANDLER,
	.charger_status = CHARGER_STATUS_NULL,
	.charger_hardware = CHARGER_TYPE_USB_PC,
	.hide = 0,
	.battery_status = BATTERY_STATUS_GOOD,
	.battery_voltage = 3700,
	.battery_capacity = 50,
	.battery_temp = 200,
	.is_charging = false,
	.is_charging_complete = false,
	.is_charging_failed = false,
	.psy_status = POWER_SUPPLY_STATUS_DISCHARGING,
	.psy_health = POWER_SUPPLY_HEALTH_GOOD,
};

static enum power_supply_property msm_charger_psy_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_charger_supplied_to[] = {
	"battery",
};

static int msm_charger_psy_get_property(struct power_supply *psy,
				  enum power_supply_property psp,
				  union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		switch(psy->type) {
		case POWER_SUPPLY_TYPE_MAINS:
			val->intval = msm_battery_info.current_charger_src & AC_CHG
				? 1 : 0;
			break;
		case POWER_SUPPLY_TYPE_USB:
			val->intval = msm_battery_info.current_charger_src & USB_CHG
				? 1 : 0;
			break;
		case POWER_SUPPLY_TYPE_UNKNOWN:
			val->intval = msm_battery_info.current_charger_src & UNKNOWN_CHG
				? 1 : 0;
			break;
		default:
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct power_supply msm_psy_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.supplied_to = msm_charger_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_charger_supplied_to),
	.properties = msm_charger_psy_properties,
	.num_properties = ARRAY_SIZE(msm_charger_psy_properties),
	.get_property = msm_charger_psy_get_property,
};

static struct power_supply msm_psy_unknown = {
	.name = "unknown",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.supplied_to = msm_charger_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_charger_supplied_to),
	.properties = msm_charger_psy_properties,
	.num_properties = ARRAY_SIZE(msm_charger_psy_properties),
	.get_property = msm_charger_psy_get_property,
};

static struct power_supply msm_psy_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_USB,
	.supplied_to = msm_charger_supplied_to,
	.num_supplicants = ARRAY_SIZE(msm_charger_supplied_to),
	.properties = msm_charger_psy_properties,
	.num_properties = ARRAY_SIZE(msm_charger_psy_properties),
	.get_property = msm_charger_psy_get_property,
};

static enum power_supply_property msm_battery_psy_properties[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TEMP
};

static int msm_battery_psy_get_property(struct power_supply *psy,
		enum power_supply_property psp,
		union power_supply_propval *val)
{
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = msm_battery_info.psy_status;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = msm_battery_info.psy_health;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = (msm_battery_info.battery_status !=
			       BATTERY_STATUS_NULL);
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = msm_battery_info.battery_technology;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = msm_battery_info.voltage_max_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = msm_battery_info.voltage_min_design;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = msm_battery_info.battery_voltage * 1000;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = msm_battery_info.battery_capacity;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = msm_battery_info.battery_temp;
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static struct power_supply msm_psy_battery = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties = msm_battery_psy_properties,
	.num_properties = ARRAY_SIZE(msm_battery_psy_properties),
	.get_property = msm_battery_psy_get_property,
};

#ifndef CONFIG_BATTERY_MSM_FAKE
struct rpc_reply_charger {
	struct	rpc_reply_hdr hdr;
	u32 more_data;

	u32 charger_status;
	u32 charger_hardware;
	u32 hide;
	u32 battery_status;
	u32 battery_voltage;
	u32 battery_capacity;
	s32 battery_temp;
	u32 is_charging;
	u32 is_charging_complete;
	u32 is_charging_failed;
};

static struct rpc_reply_charger reply_charger;

#define	be32_to_cpu_self(v)	(v = be32_to_cpu(v))

static int msm_battery_get_charger_status(void)
{
	int rc;
	struct rpc_request_charger {
		struct rpc_request_hdr hdr;
		u32 more_data;
	} request_charger;

	request_charger.more_data = cpu_to_be32(1);
	memset(&reply_charger, 0, sizeof(reply_charger));

	rc = msm_rpc_call_reply(msm_battery_info.charger_endpoint,
				CHG_GET_GENERAL_STATUS_PROC,
				&request_charger, sizeof(request_charger),
				&reply_charger, sizeof(reply_charger),
				msecs_to_jiffies(RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("BATT: ERROR: %s, charger rpc call %d, rc=%d\n",
		       __func__, CHG_GET_GENERAL_STATUS_PROC, rc);
		return rc;
	} else if (be32_to_cpu(reply_charger.more_data)) {
		be32_to_cpu_self(reply_charger.charger_status);
		be32_to_cpu_self(reply_charger.charger_hardware);
		be32_to_cpu_self(reply_charger.hide);
		be32_to_cpu_self(reply_charger.battery_status);
		be32_to_cpu_self(reply_charger.battery_voltage);
		be32_to_cpu_self(reply_charger.battery_capacity);
		be32_to_cpu_self(reply_charger.battery_temp);
		be32_to_cpu_self(reply_charger.is_charging);
		be32_to_cpu_self(reply_charger.is_charging_complete);
		be32_to_cpu_self(reply_charger.is_charging_failed);
	} else {
		pr_err("BATT: ERROR: %s, No data in charger rpc reply\n",
		       __func__);
		return -EIO;
	}

	return 0;
}

static void update_charger_type(u32 charger_hardware)
{
	switch(charger_hardware) {
	case CHARGER_TYPE_USB_PC:
		pr_debug("BATT: usb pc charger inserted\n");

		msm_battery_info.current_psy = &msm_psy_usb;
		msm_battery_info.current_charger_src = USB_CHG;
		break;
	case CHARGER_TYPE_USB_WALL:
		pr_debug("BATT: usb wall charger inserted\n");

		msm_battery_info.current_psy = &msm_psy_ac;
		msm_battery_info.current_charger_src = AC_CHG;
		break;
	case CHARGER_TYPE_USB_UNKNOWN:
		pr_debug("BATT: unknown charger inserted\n");

		msm_battery_info.current_psy = &msm_psy_unknown;
		msm_battery_info.current_charger_src = UNKNOWN_CHG;
		break;
	default:
		pr_debug("BATT: CAUTION: charger hardware\n");

		msm_battery_info.current_psy = &msm_psy_unknown;
		msm_battery_info.current_charger_src = UNKNOWN_CHG;
		break;
	}
}

void msm_battery_update_psy_status(void)
{
	u32 charger_status;
	u32 charger_hardware;
	u32 hide;
	u32 battery_status;
	u32 battery_voltage;
	u32 battery_voltage_real;
	u32 battery_capacity;
	s32 battery_temp;
	u32 is_charging;
	u32 is_charging_complete;
	u32 is_charging_failed;

	bool is_awake = true;
#ifdef CONFIG_MSM_SM_EVENT
	sm_msm_battery_data_t battery_data;
#endif
#ifdef CONFIG_BATTERY_EARLYSUSPEND
	is_awake = !msm_battery_info.is_suspended;
#endif
	pr_debug("BATT: msm_battery_update_psy_status");

	mutex_lock(&msm_battery_info.update_mutex);

	if (msm_battery_get_charger_status()) {
		goto done;
	}

	if (msm_battery_info.fuel_gauge)
	{
		charger_status		= reply_charger.charger_status;
		charger_hardware	= reply_charger.charger_hardware;
		hide			= reply_charger.hide;
		battery_status		= msm_battery_info.get_battery_status();
		battery_voltage		= msm_battery_info.get_battery_mvolts();
		battery_voltage_real	= -1; // leave it as an invalid value now if use gauge.
		battery_capacity	= msm_battery_info.get_batt_remaining_capacity();
		battery_temp		= msm_battery_info.get_battery_temperature();
		is_charging		= reply_charger.is_charging;
		is_charging_complete	= reply_charger.is_charging_complete;
		is_charging_failed	= reply_charger.is_charging_failed;
	}
	else
	{
		charger_status		= reply_charger.charger_status;
		charger_hardware	= reply_charger.charger_hardware;
		hide			= reply_charger.hide;
		battery_status		= reply_charger.battery_status;
		battery_voltage		= reply_charger.battery_voltage & 0xFFFF;
		battery_voltage_real	= (reply_charger.battery_voltage >> 16) & 0xFFFF;
		battery_capacity	= reply_charger.battery_capacity & 0x7F;
		battery_temp		= reply_charger.battery_temp * 10;
		is_charging		= reply_charger.is_charging;
		is_charging_complete	= reply_charger.is_charging_complete;
		is_charging_failed	= reply_charger.is_charging_failed;
	}
#ifdef CONFIG_MSM_SM_EVENT
		battery_data.charger_status = charger_status;
		battery_data.battery_voltage = battery_voltage;
		battery_data.battery_temp = battery_temp;
		sm_add_event (SM_POWER_EVENT|SM_POWER_EVENT_BATTERY_UPDATE, 0, 0, (void *)&battery_data, sizeof(battery_data));
#endif

	pr_debug("BATT: received, %d, %d, 0x%x; %d, %d, %d, %d; %d, %d, %d; %d, %d, %d\n",
		  charger_status, charger_hardware, hide,
		  battery_status, battery_voltage, battery_capacity, battery_temp,
		  is_charging, is_charging_complete, is_charging_failed,
		  reply_charger.battery_voltage >> 16,
		  (reply_charger.battery_capacity >> 7) & 0x1FFF,
		  reply_charger.battery_capacity >> 20);

	if (charger_status	== msm_battery_info.charger_status &&
	    charger_hardware	== msm_battery_info.charger_hardware &&
	    hide		== msm_battery_info.hide &&
	    battery_status	== msm_battery_info.battery_status &&
	    battery_voltage	== msm_battery_info.battery_voltage &&
	    battery_capacity	== msm_battery_info.battery_capacity &&
	    battery_temp	== msm_battery_info.battery_temp &&
	    is_charging		== msm_battery_info.is_charging &&
	    is_charging_complete== msm_battery_info.is_charging_complete &&
	    is_charging_failed	== msm_battery_info.is_charging_failed) {
		goto done;
	}



	if (msm_battery_info.charger_status != charger_status) {
		if (msm_battery_info.charger_status == CHARGER_STATUS_NULL) {
			pr_debug("BATT: start charging\n");
			update_charger_type(charger_hardware);
		} else if (charger_status == CHARGER_STATUS_NULL) {
			pr_debug("BATT: end charging\n");

			if (msm_battery_info.current_charger_src & USB_CHG) {
				pr_debug("BATT: usb pc charger removed\n");
			} else if (msm_battery_info.current_charger_src & AC_CHG) {
				pr_debug("BATT: usb wall charger removed\n");
			} else if (msm_battery_info.current_charger_src & UNKNOWN_CHG) {
				pr_debug("BATT: unknown wall charger removed\n");
			} else {
				pr_debug("BATT: CAUTION: charger invalid: %d\n",
					  msm_battery_info.current_charger_src);
			}

			msm_battery_info.current_psy = &msm_psy_battery;
			msm_battery_info.current_charger_src = 0;
		} else {
			pr_err("BATT: CAUTION: charger status change\n");
		}
	}

	if ((charger_status != CHARGER_STATUS_NULL) &&
		(charger_hardware != msm_battery_info.charger_hardware)) {
		pr_debug("BATT: charger type changed\n");
		update_charger_type(charger_hardware);
	}

	if (charger_status == CHARGER_STATUS_NULL) {
		msm_battery_info.psy_status = POWER_SUPPLY_STATUS_DISCHARGING;
	} else if (battery_status == BATTERY_STATUS_NULL) {
		msm_battery_info.psy_status = POWER_SUPPLY_STATUS_UNKNOWN;
	} else if (battery_capacity == 100) {
		msm_battery_info.psy_status = POWER_SUPPLY_STATUS_FULL;
	} else {
		msm_battery_info.psy_status = POWER_SUPPLY_STATUS_CHARGING;
	}

	if (msm_battery_info.battery_status != battery_status) {
		if (battery_status == BATTERY_STATUS_NULL) {
			pr_debug("BATT: battery removed\n");

			msm_battery_info.psy_health = POWER_SUPPLY_HEALTH_DEAD;
		} else if (battery_status == BATTERY_STATUS_OVER_TEMPERATURE) {
			if (battery_temp >= TEMPERATURE_HOT) {
				pr_debug("BATT: battery overheat\n");

				msm_battery_info.psy_health = POWER_SUPPLY_HEALTH_OVERHEAT;
			} else if (battery_temp <= TEMPERATURE_COLD) {
				pr_debug("BATT: battery cold\n");

				msm_battery_info.psy_health = POWER_SUPPLY_HEALTH_COLD;
			}
		} else if (battery_status == BATTERY_STATUS_GOOD) {
			pr_debug("BATT: battery good\n");

			msm_battery_info.psy_health = POWER_SUPPLY_HEALTH_GOOD;
		} else {
			pr_err("BATT: CAUTION: battery status invalid: %d\n",
				  battery_status);
		}
	}

	if(is_charging && battery_capacity <= 0
		&& battery_voltage_real < BATTERY_ULTRA_LOW) {
		// charging but input current < output current so that the battery would be exhausted
		// set psy_health to POWER_SUPPLY_HEALTH_EXHAUST which will result framework to shutdown
		msm_battery_info.psy_health = POWER_SUPPLY_HEALTH_EXHAUST;
	}

	msm_battery_info.charger_status		= charger_status;
	msm_battery_info.charger_hardware	= charger_hardware;
	msm_battery_info.hide			= hide;
	msm_battery_info.battery_status		= battery_status;
	msm_battery_info.battery_voltage	= battery_voltage;
	msm_battery_info.battery_capacity	= battery_capacity;
	msm_battery_info.battery_temp		= battery_temp;
	msm_battery_info.is_charging		= is_charging;
	msm_battery_info.is_charging_complete	= is_charging_complete;
	msm_battery_info.is_charging_failed	= is_charging_failed;

	if (msm_battery_info.current_psy) {
		power_supply_changed(msm_battery_info.current_psy);
	}

done:
	mutex_unlock(&msm_battery_info.update_mutex);

	if(is_awake) {
		queue_delayed_work(msm_battery_info.battery_queue,
				   &msm_battery_info.battery_work,
				   MSM_BATT_POLLING_TIME);
	}

	return;
}
EXPORT_SYMBOL(msm_battery_update_psy_status);

#ifdef CONFIG_MSM_SM_EVENT
uint32_t msm_batt_get_batt_voltage (void)
{
	return msm_battery_info.battery_voltage;
}
EXPORT_SYMBOL(msm_batt_get_batt_voltage);
#endif

#ifdef CONFIG_BATTERY_EARLYSUSPEND
void msm_battery_early_suspend(struct early_suspend *h)
{
	pr_debug("BATT: %s\n", __func__);

	mutex_lock(&msm_battery_info.suspend_lock);
	if(msm_battery_info.is_suspended) {
		mutex_unlock(&msm_battery_info.suspend_lock);
		pr_err("BATT: CAUTION: %s, is already suspended\n", __func__);
		return;
	}
	msm_battery_info.is_suspended = true;
	mutex_unlock(&msm_battery_info.suspend_lock);
	flush_workqueue(msm_battery_info.battery_queue);
}

void msm_battery_late_resume(struct early_suspend *h)
{
	pr_debug("BATT: %s\n", __func__);

	mutex_lock(&msm_battery_info.suspend_lock);
	if(!msm_battery_info.is_suspended) {
		mutex_unlock(&msm_battery_info.suspend_lock);
		pr_err("BATT: CAUTION: %s, is already resumed\n", __func__);
		return;
	}
	msm_battery_info.is_suspended = false;
	mutex_unlock(&msm_battery_info.suspend_lock);

	queue_delayed_work(msm_battery_info.battery_queue,
			   &msm_battery_info.battery_work,
			   0);
}
#endif

struct battery_client_registration_request {
	/* Voltage at which cb func should be called */
	u32 desired_battery_voltage;
	/* Direction when the cb func should be called */
	u32 voltage_direction;
	/* Registered callback to be called */
	u32 battery_cb_id;
	/* Call back data */
	u32 cb_data;
	u32 more_data;
	u32 battery_error;
};

struct battery_client_registration_reply {
	u32 handler;
};

static int msm_battery_register_arg_func(struct msm_rpc_client *battery_client,
					 void *buffer, void *data)
{
	struct battery_client_registration_request *battery_register_request =
		(struct battery_client_registration_request *)data;

	u32 *request = (u32 *)buffer;
	int size = 0;

	*request = cpu_to_be32(battery_register_request->desired_battery_voltage);
	size += sizeof(u32);
	request++;

	*request = cpu_to_be32(battery_register_request->voltage_direction);
	size += sizeof(u32);
	request++;

	*request = cpu_to_be32(battery_register_request->battery_cb_id);
	size += sizeof(u32);
	request++;

	*request = cpu_to_be32(battery_register_request->cb_data);
	size += sizeof(u32);
	request++;

	*request = cpu_to_be32(battery_register_request->more_data);
	size += sizeof(u32);
	request++;

	*request = cpu_to_be32(battery_register_request->battery_error);
	size += sizeof(u32);

	return size;
}

static int msm_battery_register_ret_func(struct msm_rpc_client *battery_client,
					 void *buffer, void *data)
{
	struct battery_client_registration_reply *data_ptr, *buffer_ptr;

	data_ptr = (struct battery_client_registration_reply *)data;
	buffer_ptr = (struct battery_client_registration_reply *)buffer;

	data_ptr->handler = be32_to_cpu(buffer_ptr->handler);
	return 0;
}

static int msm_battery_register(u32 desired_battery_voltage, u32 voltage_direction,
				u32 battery_cb_id, u32 cb_data, s32 *handle)
{
	struct battery_client_registration_request battery_register_request;
	struct battery_client_registration_reply battery_register_reply;
	void *request;
	void *reply;
	int rc;

	battery_register_request.desired_battery_voltage = desired_battery_voltage;
	battery_register_request.voltage_direction = voltage_direction;
	battery_register_request.battery_cb_id = battery_cb_id;
	battery_register_request.cb_data = cb_data;
	battery_register_request.more_data = 1;
	battery_register_request.battery_error = 0;
	request = &battery_register_request;

	reply = &battery_register_reply;

	rc = msm_rpc_client_req(msm_battery_info.battery_client,
				BATTERY_REGISTER_PROC,
				msm_battery_register_arg_func, request,
				msm_battery_register_ret_func, reply,
				msecs_to_jiffies(RPC_TIMEOUT));
	if (rc < 0) {
		pr_err("BATT: ERROR: %s, vbatt register, rc=%d\n", __func__, rc);
		return rc;
	}

	*handle = battery_register_reply.handler;

	return 0;
}

struct battery_client_deregister_request {
	u32 handler;
};

struct battery_client_deregister_reply {
	u32 battery_error;
};

static int msm_battery_deregister_arg_func(struct msm_rpc_client *battery_client,
					   void *buffer, void *data)
{
	struct battery_client_deregister_request *deregister_request =
		(struct  battery_client_deregister_request *)data;
	u32 *request = (u32 *)buffer;
	int size = 0;

	*request = cpu_to_be32(deregister_request->handler);
	size += sizeof(u32);

	return size;
}

static int msm_battery_deregister_ret_func(struct msm_rpc_client *battery_client,
					   void *buffer, void *data)
{
	struct battery_client_deregister_reply *data_ptr, *buffer_ptr;

	data_ptr = (struct battery_client_deregister_reply *)data;
	buffer_ptr = (struct battery_client_deregister_reply *)buffer;

	data_ptr->battery_error = be32_to_cpu(buffer_ptr->battery_error);

	return 0;
}

static int msm_battery_deregister(u32 handler)
{
	int rc;
	struct battery_client_deregister_request request;
	struct battery_client_deregister_reply reply;

	request.handler = handler;

	rc = msm_rpc_client_req(msm_battery_info.battery_client,
				BATTERY_DEREGISTER_CLIENT_PROC,
				msm_battery_deregister_arg_func, &request,
				msm_battery_deregister_ret_func, &reply,
				msecs_to_jiffies(RPC_TIMEOUT));

	if (rc < 0) {
		pr_err("BATT: ERROR: %s, vbatt deregister, rc=%d\n", __func__, rc);
		return rc;
	}

	if (reply.battery_error != BATTERY_DEREGISTRATION_SUCCESSFUL) {
		pr_err("BATT: ERROR: %s, vbatt %d deregistration, code=%d\n",
		       __func__, handler, reply.battery_error);
		return -EIO;
	}

	return 0;
}
#endif /* CONFIG_BATTERY_MSM_FAKE */

static int msm_battery_cleanup(void)
{
	int rc = 0;

	pr_debug("BATT: %s\n", __func__);

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_battery_info.charger_handler != INVALID_HANDLER) {
		rc = msm_battery_deregister(msm_battery_info.charger_handler);
		if (rc < 0)
			pr_err("BATT: ERROR %s, charger deregister, rc=%d\n",
			       __func__, rc);
	}
	msm_battery_info.charger_handler = INVALID_HANDLER;

	if (msm_battery_info.battery_handler != INVALID_HANDLER) {
		rc = msm_battery_deregister(msm_battery_info.battery_handler);
		if (rc < 0)
			pr_err("BATT: ERROR: %s, battery deregister, rc=%d\n",
			       __func__, rc);
	}
	msm_battery_info.battery_handler = INVALID_HANDLER;

	if(msm_battery_info.battery_queue)
		destroy_workqueue(msm_battery_info.battery_queue);

#ifdef CONFIG_BATTERY_EARLYSUSPEND
	if (msm_battery_info.early_suspend.suspend == msm_battery_early_suspend)
		unregister_early_suspend(&msm_battery_info.early_suspend);
#endif
#endif

	if (msm_battery_info.msm_psy_ac)
		power_supply_unregister(msm_battery_info.msm_psy_ac);
	if (msm_battery_info.msm_psy_usb)
		power_supply_unregister(msm_battery_info.msm_psy_usb);
	if (msm_battery_info.msm_psy_unknown)
		power_supply_unregister(msm_battery_info.msm_psy_unknown);
	if (msm_battery_info.msm_psy_battery)
		power_supply_unregister(msm_battery_info.msm_psy_battery);

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (msm_battery_info.charger_endpoint) {
		rc = msm_rpc_close(msm_battery_info.charger_endpoint);
		if (rc < 0) {
			pr_err("BATT: ERROR: %s, charger rpc close, rc=%d\n",
			       __func__, rc);
		}
	}

	if (msm_battery_info.battery_client)
		msm_rpc_unregister_client(msm_battery_info.battery_client);
#endif

	return rc;
}

static u32 msm_battery_capacity(u32 current_voltage)
{
	u32 low_voltage = msm_battery_info.voltage_min_design;
	u32 high_voltage = msm_battery_info.voltage_max_design;

	if (current_voltage <= low_voltage)
		return 0;
	else if (current_voltage >= high_voltage)
		return 100;
	else
		return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}

#ifndef CONFIG_BATTERY_MSM_FAKE
static int msm_battery_cb_func(struct msm_rpc_client *client,
			       void *buffer, int in_size)
{
	int rc = 0;
	struct rpc_request_hdr *request;
	u32 procedure;
	u32 accept_status;

	pr_debug("BATT: %s\n", __func__);

	request = (struct rpc_request_hdr *)buffer;
	procedure = be32_to_cpu(request->procedure);

	switch (procedure) {
	case BATTERY_CB_TYPE_PROC:
		accept_status = RPC_ACCEPTSTAT_SUCCESS;
		break;

	default:
		accept_status = RPC_ACCEPTSTAT_PROC_UNAVAIL;
		pr_err("BATT: ERROR: %s, procedure (%d) not supported\n",
		       __func__, procedure);
		break;
	}

	msm_rpc_start_accepted_reply(msm_battery_info.battery_client,
				     be32_to_cpu(request->xid), accept_status);

	rc = msm_rpc_send_accepted_reply(msm_battery_info.battery_client, 0);
	if (rc)
		pr_err("BATT: ERROR: %s, sending reply, rc=%d\n", __func__, rc);

	if (accept_status == RPC_ACCEPTSTAT_SUCCESS)
	{
		wake_lock_timeout(&msm_battery_info.charger_cb_wake_lock,
				  10 * HZ);
		msm_battery_update_psy_status();
	}

	return rc;
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

int msm_battery_fuel_register(struct msm_batt_gauge *batt)
{
	int rc = 0;
	if(!batt)
	{
		pr_err("BATT: ERROR: %s, null gauge pointer\n" ,__func__);
		return EIO;
	}

	msm_battery_info.fuel_gauge = 1;
	msm_battery_info.get_battery_mvolts = batt->get_battery_mvolts;
	msm_battery_info.get_battery_temperature = batt->get_battery_temperature;
	msm_battery_info.is_battery_present = batt->is_battery_present;
	msm_battery_info.is_battery_temp_within_range = batt->is_battery_temp_within_range;
	msm_battery_info.is_battery_id_valid = batt->is_battery_id_valid;
	msm_battery_info.get_battery_status = batt->get_battery_status;
	msm_battery_info.get_batt_remaining_capacity = batt->get_batt_remaining_capacity;

	pr_debug("BATT: %s\n", __func__);

	return rc;

}
EXPORT_SYMBOL(msm_battery_fuel_register);

void msm_battery_fuel_unregister(struct msm_batt_gauge* batt)
{
	if(!batt)
	{
		pr_err("BATT: ERROR: %s, null gauge pointer\n", __func__);
		return;
	}

	msm_battery_info.fuel_gauge = 0;
	msm_battery_info.get_battery_mvolts = NULL;
	msm_battery_info.get_battery_temperature = NULL;
	msm_battery_info.is_battery_present = NULL;
	msm_battery_info.is_battery_temp_within_range = NULL;
	msm_battery_info.is_battery_id_valid = NULL;
	msm_battery_info.get_battery_status = NULL;
	msm_battery_info.get_batt_remaining_capacity = NULL;

	pr_debug("BATT: %s\n", __func__);

	return;
}
EXPORT_SYMBOL(msm_battery_fuel_unregister);

#ifndef CONFIG_BATTERY_MSM_FAKE
static void msm_battery_worker(struct work_struct *work)
{
	msm_battery_update_psy_status();
}
#endif  /* CONFIG_BATTERY_MSM_FAKE */

static int __devinit msm_battery_probe(struct platform_device *pdev)
{
	int rc;
	struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

	if (pdev->id != -1) {
		dev_err(&pdev->dev,
			"BATT: ERROR: %s, msm chipsets only support one battery",
			__func__);
		return -EINVAL;
	}

	pr_debug("BATT: %s, enter\n", __func__);

	msm_battery_info.voltage_max_design = pdata->voltage_max_design;
	if (!msm_battery_info.voltage_max_design) {
		msm_battery_info.voltage_max_design = BATTERY_HIGH;
	}

	msm_battery_info.voltage_min_design = pdata->voltage_min_design;
	if (!msm_battery_info.voltage_min_design) {
		msm_battery_info.voltage_min_design = BATTERY_LOW;
	}

	msm_battery_info.battery_technology = pdata->batt_technology;
	if (!msm_battery_info.battery_technology) {
		msm_battery_info.battery_technology = POWER_SUPPLY_TECHNOLOGY_LION;
	}

#ifndef CONFIG_BATTERY_MSM_FAKE
	if (pdata->avail_chg_sources & AC_CHG) {
#else
	{
#endif
		rc = power_supply_register(&pdev->dev, &msm_psy_ac);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"BATT: ERROR: %s, register msm_psy_ac, "
				"rc = %d\n", __func__, rc);
			msm_battery_cleanup();
			return rc;
		}
		msm_battery_info.available_charger_src |= AC_CHG;
		msm_battery_info.msm_psy_ac = &msm_psy_ac;
	}

	if (pdata->avail_chg_sources & UNKNOWN_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_unknown);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"BATT: ERROR: %s, register msm_psy_unknown, "
				"rc = %d\n", __func__, rc);
			msm_battery_cleanup();
			return rc;
		}
		msm_battery_info.available_charger_src |= UNKNOWN_CHG;
		msm_battery_info.msm_psy_unknown = &msm_psy_unknown;
	}
	if (pdata->avail_chg_sources & USB_CHG) {
		rc = power_supply_register(&pdev->dev, &msm_psy_usb);
		if (rc < 0) {
			dev_err(&pdev->dev,
				"BATT: ERROR %s, register msm_psy_usb, "
				"rc = %d\n", __func__, rc);
			msm_battery_cleanup();
			return rc;
		}
		msm_battery_info.available_charger_src |= USB_CHG;
		msm_battery_info.msm_psy_usb = &msm_psy_usb;
	}

	if (!msm_battery_info.available_charger_src) {
		dev_err(&pdev->dev,
			"BATT: ERROR: %s, no power supply(ac or usb) "
			"is avilable\n", __func__);
		msm_battery_cleanup();
		return -ENODEV;
	}

	msm_battery_info.calculate_capacity = pdata->calculate_capacity;
	if (!msm_battery_info.calculate_capacity) {
		msm_battery_info.calculate_capacity = msm_battery_capacity;
	}

	rc = power_supply_register(&pdev->dev, &msm_psy_battery);
	if (rc < 0) {
		dev_err(&pdev->dev, "BATT: ERROR: %s, register battery "
			"rc=%d\n", __func__, rc);
		msm_battery_cleanup();
		return rc;
	}
	msm_battery_info.msm_psy_battery = &msm_psy_battery;

	msm_battery_info.current_psy = &msm_psy_battery;
	msm_battery_info.current_charger_src = 0;
	power_supply_changed(msm_battery_info.current_psy);

#ifndef CONFIG_BATTERY_MSM_FAKE
#ifdef CONFIG_BATTERY_EARLYSUSPEND
	msm_battery_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	msm_battery_info.early_suspend.suspend = msm_battery_early_suspend;
	msm_battery_info.early_suspend.resume = msm_battery_late_resume;
	msm_battery_info.is_suspended = false;
	mutex_init(&msm_battery_info.suspend_lock);
	register_early_suspend(&msm_battery_info.early_suspend);
#endif
	msm_battery_info.battery_queue = create_singlethread_workqueue(
			"battery_queue");
	if (!msm_battery_info.battery_queue) {
		dev_err(&pdev->dev, "BATT: ERROR: %s, create battey work queue\n",
			__func__);
		msm_battery_cleanup();
		return -ENOMEM;
	}
	INIT_DELAYED_WORK(&msm_battery_info.battery_work, msm_battery_worker);
	mutex_init(&msm_battery_info.update_mutex);

	rc = msm_battery_register(msm_battery_info.voltage_min_design,
				  BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				  BATTERY_CB_ID_LOW_V,
				  BATTERY_VOLTAGE_BELOW_THIS_LEVEL,
				  &msm_battery_info.battery_handler);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"BATT: ERROR: %s, battery register, rc = %d\n",
			__func__, rc);
		msm_battery_cleanup();
		return rc;
	}

	rc = msm_battery_register(msm_battery_info.voltage_min_design,
				  VBATT_CHG_EVENTS,
				  BATTERY_CB_ID_CHG_EVT,
				  VBATT_CHG_EVENTS,
				  &msm_battery_info.charger_handler);
	if (rc < 0) {
		dev_err(&pdev->dev,
			"BATT: ERROR: %s, charger register, rc = %d\n",
			__func__, rc);
		msm_battery_cleanup();
		return rc;
	}

	wake_lock_init(&msm_battery_info.charger_cb_wake_lock, WAKE_LOCK_SUSPEND,
		       "msm_charger_cb");

	queue_delayed_work(msm_battery_info.battery_queue,
			   &msm_battery_info.battery_work,
			   0);
#else
	msm_battery_info.current_psy = &msm_psy_ac;
	msm_battery_info.current_charger_src = AC_CHG;
	power_supply_changed(msm_battery_info.current_psy);
#endif /* CONFIG_BATTERY_MSM_FAKE */

	pr_debug("BATT: %s, exit\n", __func__);

	return 0;
}

static int __devexit msm_battery_remove(struct platform_device *pdev)
{
	int rc = 0;

	wake_lock_destroy(&msm_battery_info.charger_cb_wake_lock);

	rc = msm_battery_cleanup();
	if (rc < 0) {
		dev_err(&pdev->dev,
			"BATT: ERROR: %s, battery cleanup, rc=%d\n",
			__func__, rc);
		return rc;
	}

	return 0;
}

static struct platform_driver msm_batt_driver = {
	.probe  = msm_battery_probe,
	.remove = __devexit_p(msm_battery_remove),
	.driver = {
		   .name = "msm-battery",
		   .owner = THIS_MODULE,
		   },
};

static int __devinit msm_battery_init_rpc(void)
{
	int rc = 0;

#ifdef CONFIG_BATTERY_MSM_FAKE
	pr_debug("BATT: CAUTION: fake msm battery\n");
#else
	msm_battery_info.charger_endpoint =
		msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VER_1_1, 0);
	if (msm_battery_info.charger_endpoint == NULL) {
		pr_err("BATT: ERROR: %s, rpc charger no server\n", __func__);
		return -ENODEV;
	}
	if (IS_ERR(msm_battery_info.charger_endpoint)) {
		rc = PTR_ERR(msm_battery_info.charger_endpoint);
		pr_err("BATT: ERROR: %s, rpc charger not connect, rc=%d\n",
		       __func__, rc);
		msm_battery_info.charger_endpoint = NULL;
		return -ENODEV;
	}
	msm_battery_info.charger_api_version = CHG_RPC_VER_1_1;

	msm_battery_info.battery_client = msm_rpc_register_client(
			"battery", BATTERY_RPC_PROG, BATTERY_RPC_VER_5_1, 1,
			msm_battery_cb_func);
	if (msm_battery_info.battery_client == NULL) {
		pr_err("BATT: ERROR: %s, rpc battery no client\n", __func__);
		return -ENODEV;
	}
	if (IS_ERR(msm_battery_info.battery_client)) {
		rc = PTR_ERR(msm_battery_info.battery_client);
		pr_err("BATT: ERROR: %s, rpc battery not register, rc=%d\n",
		       __func__, rc);
		msm_battery_info.battery_client = NULL;
		return -ENODEV;
	}
	msm_battery_info.battery_api_version = BATTERY_RPC_VER_5_1;
#endif /* CONFIG_BATTERY_MSM_FAKE */

	pr_debug("BATT: %s, rpc version charger=0x%08x, battery=0x%08x\n",
		__func__, msm_battery_info.charger_api_version,
		msm_battery_info.battery_api_version);

	return 0;
}

static int __init msm_battery_init(void)
{
	int rc = 0;

	pr_debug("BATT: %s, enter\n", __func__);

	rc = msm_battery_init_rpc();
	if (rc < 0) {
		msm_battery_cleanup();
		return rc;
	}

	rc = platform_driver_register(&msm_batt_driver);
	if (rc < 0) {
		pr_err("BATT: ERROR: %s, platform_driver_register, rc=%d\n",
		       __func__, rc);
	}

	pr_debug("BATT: %s, exit\n", __func__);
	return 0;
}

static void __exit msm_battery_exit(void)
{
	platform_driver_unregister(&msm_batt_driver);
}

module_init(msm_battery_init);
module_exit(msm_battery_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("2.0");
MODULE_ALIAS("platform:msm_battery");
