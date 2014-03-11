/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _SM_EVENT_LOG_H
#define _SM_EVENT_LOG_H

/*
 * wakeup reason
 * refer to struct of msm_pm_smem_t in pm2.c
 * export the struct to outside
 */
typedef struct {
	uint32_t sleep_time;
	uint32_t irq_mask;
	uint32_t resources_used;
	uint32_t reserved1;

	uint32_t wakeup_reason;
	uint32_t pending_irqs;
	uint32_t rpc_prog;
	uint32_t rpc_proc;
	char     smd_port_name[20];
	uint32_t reserved2;
}sm_msm_pm_smem_t;

typedef struct{
	uint32_t prog;
	uint32_t version;
	uint32_t type;
	uint32_t src_pid;
	uint32_t src_cid;
	uint32_t confirm_rx;
	uint32_t size;
	uint32_t dst_pid;
	uint32_t dst_cid;

}sm_msm_rpc_data_t;

typedef struct{
	uint32_t charger_status;
	uint32_t charger_type;
	uint32_t battery_status;
	uint32_t battery_level;
	uint32_t battery_voltage;
	uint32_t battery_temp;
}sm_msm_battery_data_t;


typedef struct {
	uint32_t func_addr;
	uint32_t irq_num;
} sm_msm_irq_data_t;

typedef struct {
	uint32_t battery_voltage;//mV
	uint32_t rtime_sec;
	uint32_t rtime_nanosec;
} sm_periodical_status_data_t;

typedef union {
	sm_msm_pm_smem_t wakeup_reason;
	char wakelock_name[1];
	char device_name[1];
	char clk_name[1];
	sm_msm_rpc_data_t sm_msm_rpc;
	sm_msm_battery_data_t battery_info;
	sm_periodical_status_data_t periodical_status;
	sm_msm_irq_data_t irq_data;
}sm_event_data_t;

typedef struct {
	/*
	 * event_seq: event sequence number
	 */
	uint32_t event_seq;

	/*
	 * event_id: major_event|minor_event
	 * major_event: 2 high bytes in event_id
	 * minor_event: 2 low bytes in event_id
	 */
	uint32_t event_id;

	uint32_t param1;
	uint32_t param2;

	uint32_t sec_time;
	uint32_t nanosec_time;

	sm_event_data_t data;
	uint32_t data_len;
	atomic_t done;
}sm_event_item_t;

typedef int32_t (*event_notify)(sm_event_item_t *ev);
typedef enum {
	SM_STATE_NONE,
	SM_STATE_RUNNING,
	SM_STATE_EARLYSUSPEND,
	SM_STATE_SUSPEND,
	SM_STATE_RESUME,
	SM_STATE_LATERESUME,
	SM_STATE_INVALID,

}sm_state;

struct traceirq_entry {
	unsigned long		ip;
	unsigned int		flags;
	unsigned long		caller;
	unsigned int		cpuid;
	unsigned long long 	cycles;
	unsigned int		reserved[2];
};
#define TRACK_BUF_SIZE	(PAGE_SIZE << 2)
extern struct traceirq_entry *g_track_irq_buf;
#ifdef CONFIG_SMP
extern atomic_t g_track_index;
#else
extern int g_track_index;
#endif


#define VERSION_NUMBER 0x0
#define HIWORD(x) (((x)&0xffff)<<16)
#define LOWORD(x) ((x)&0xffff)

/*
 * major event, maximum 16 events
 */
#define SM_POWER_EVENT			HIWORD(0x1)
#define SM_DEVICE_EVENT			HIWORD(0x2)
#define SM_CLOCK_EVENT			HIWORD(0x4)
#define SM_WAKELOCK_EVENT		HIWORD(0x8)
#define SM_RPCROUTER_EVENT		HIWORD(0x10)
#define SM_IRQ_EVENT			HIWORD(0x20)
#define SM_STATUS_EVENT			HIWORD(0x40)
#define SM_STATE_MACHINE_EVENT		HIWORD(0x80)
#define SM_IRQ_ONOFF_EVENT		HIWORD(0x100)

/*
 * minor event of SM_POWER_EVENT, maximum 16 events
 */
#define SM_POWER_EVENT_SLEEP		LOWORD(0x1)
#define SM_POWER_EVENT_EARLY_SUSPEND	LOWORD(0x2)
#define SM_POWER_EVENT_SUSPEND		LOWORD(0x4)
#define SM_POWER_EVENT_RESUME		LOWORD(0x20)
#define SM_POWER_EVENT_LATE_RESUME	LOWORD(0x40)
#define SM_POWER_EVENT_BATTERY_UPDATE	LOWORD(0x80)

/*
 * minor event of SM_DEVICE_EVENT, maximum 16 events
 */
#define SM_DEVICE_EVENT_SUSPEND		LOWORD(0x1)
#define SM_DEVICE_EVENT_RESUME		LOWORD(0x2)

/*
 * minor event of SM_CLOCK_EVENT, maximum 16 events
 */
#define SM_CLK_EVENT_SET_ENABLE		LOWORD(0x1)
#define SM_CLK_EVENT_SET_DISABLE	LOWORD(0x2)

/*
 * minor event of SM_WAKELOCK_EVENT
 */
#define WAKELOCK_EVENT_ON		LOWORD(0x1)
#define WAKELOCK_EVENT_OFF		LOWORD(0x2)

/*
 * minor event of SM_RPCROUTER_EVENT
 */
#define RPCROUTER_WRITE_CALL		LOWORD(0x1)
#define RPCROUTER_WRITE_REPLY		LOWORD(0x2)
#define RPCROUTER_READ_CALL		LOWORD(0x4)
#define RPCROUTER_READ_REPLY		LOWORD(0x8)

/*
 * minor event of SM_IRQ_EVENT
 */
#define IRQ_EVENT_ENTER			LOWORD(0x1)
#define IRQ_EVENT_LEAVE			LOWORD(0x2)

/*
 * minor event of SM_STATUS_EVENT
 */
#define STATUS_PERIODICAL_SYNC		LOWORD(0x1)

/*
 * param1
 */
#define SM_EVENT_START			0x1
#define SM_EVENT_END			0x2

/*
 * wakeup reason from MP
 */
#define DEM_WAKEUP_REASON_NONE       0x00000000
#define DEM_WAKEUP_REASON_SMD        0x00000001
#define DEM_WAKEUP_REASON_INT        0x00000002
#define DEM_WAKEUP_REASON_GPIO       0x00000004
#define DEM_WAKEUP_REASON_TIMER      0x00000008
#define DEM_WAKEUP_REASON_ALARM      0x00000010
#define DEM_WAKEUP_REASON_RESET      0x00000020
#define DEM_WAKEUP_REASON_OTHER      0x00000040
#define DEM_WAKEUP_REASON_REMOTE     0x00000080

/*
 * event option for event driver
 */
#define GET_EVENT_ENABLE_REINDEX	0x00000001
#define GET_EVENT_NO_WAIT		0x00000010
#define GET_EVENT_WAIT_WAKE_ONE		0x00000020
#define GET_EVENT_WAIT_WAKE_ALL		0x00000040

int32_t sm_log_event_register (void);
int32_t sm_log_event_unregister (void);
#endif
