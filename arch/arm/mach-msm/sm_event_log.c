/*
 * Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include "timer.h"

#include <linux/jiffies.h>
#include <linux/sm_event.h>
#include <linux/sm_event_log.h>
#include <mach/msm_battery.h>

#include "smd_rpc_sym.h"
#include "smd_rpcrouter.h"
#include <asm/io.h>
#include <linux/dma-mapping.h>
#include <mach/msm_iomap.h>
#include <asm/hardware/cache-l2x0.h>
#include "cache-ops.h"
#include <mach/oem_rapi_client.h>

#define CLEAN_ADDR	(L2X0_CLEAN_LINE_PA + MSM_L2CC_BASE)
#define outer_cache_one_line(start) \
		writel_relaxed((start), CLEAN_ADDR)

#define cache_clean(vaddr)					\
	do {							\
		unsigned long paddr;				\
		cache_clean_nosync_oneline(vaddr, 32);		\
		paddr = (unsigned long)__virt_to_phys(vaddr);	\
		outer_cache_one_line(paddr);			\
	} while (0)
/*
 * SM_MAXIMUM_EVENT should be (2^n)
 */
#define SM_MAXIMUM_EVENT (1<<10)
#define SM_MAXIMUM_EVENT_MASK (SM_MAXIMUM_EVENT - 1)

struct sm_event_notify_t
{
	struct list_head link;
	event_notify notify;
};

struct sm_events_t
{
	sm_event_item_t *event_pool;
	atomic_t read_pos;
	atomic_t write_pos;
	uint32_t event_mask;
	wait_queue_head_t   wait_wakeone_q;
	wait_queue_head_t   wait_wakeall_q;
	uint32_t wait_flag;
};

typedef int32_t (* sm_event_callback)(uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len);
struct sm_event_filter
{
	sm_state state_start;
	sm_state state_end;
	uint32_t event_filter;
	sm_event_callback callback;
};

static uint32_t sm_current_state = SM_STATE_NONE;
sm_periodical_status_data_t sm_periodcal_status;

/*
 * rtc time in AP may not ready, wait about 10 seconds
 */
static uint32_t sm_periodical_ktime_sec_report = 10;

static struct sm_events_t sm_events;

static atomic_t sm_status_report_event;

static struct sm_event_filter sm_event_pre_filter[] =
{
	{SM_STATE_EARLYSUSPEND, SM_STATE_LATERESUME, SM_WAKELOCK_EVENT | 0xffff, NULL},
	{SM_STATE_SUSPEND, SM_STATE_RESUME, SM_CLOCK_EVENT | 0xffff, NULL},
};

static inline void sm_get_ktime (uint32_t *sec, uint32_t *nanosec);
static int32_t sm_add_log_event (uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len);
static int32_t sm_get_log_event_and_data (sm_event_item_t *ev, uint32_t *start, uint32_t *count, uint32_t flag);
static void sm_set_log_system_state(int want_state);
static int32_t sm_sprint_log_info (char *buf, int32_t buf_sz, sm_event_item_t *ev, uint32_t count);

sm_event_ops event_log_ops =
{
	.sm_add_event		= sm_add_log_event,
	.sm_get_event_and_data	= sm_get_log_event_and_data,
	.sm_set_system_state	= sm_set_log_system_state,
	.sm_sprint_info		= sm_sprint_log_info,
};

static inline int sm_get_event_state(void)
{
	return sm_current_state;
}

static size_t sm_sprint_battery_status (sm_msm_battery_data_t *battery_info, char *buf, int32_t buf_sz)
{
	size_t len = 0;

	if (battery_info->charger_status == CHARGER_STATUS_GOOD)
		len += snprintf(buf + len, buf_sz - len, "charger good, ");
	else if (battery_info->charger_status == CHARGER_STATUS_WEAK)
		len += snprintf(buf + len, buf_sz - len, "charger weak, ");
	else if (battery_info->charger_status == CHARGER_STATUS_NULL)
		len += snprintf(buf + len, buf_sz - len, "none charger, ");

	len += snprintf(buf + len, buf_sz - len, "battery voltage = %u mV, ",
		battery_info->battery_voltage);

	len += snprintf(buf + len, buf_sz - len, "battery temp = %u",
		battery_info->battery_temp);

	return len;
}

static inline size_t sm_sprint_string_name (char *string_name, char *buf, int32_t buf_sz)
{
	return snprintf(buf, buf_sz, "%s ", string_name);
}

static size_t sm_sprint_wakeup_reason (sm_msm_pm_smem_t *reason, char *buf, int32_t buf_sz)
{
	sm_msm_pm_smem_t *wakeup_reason;
	ssize_t len = 0;

	wakeup_reason = reason;
	len += snprintf(buf + len, buf_sz - len, ";Wakeup reason ");

	switch(wakeup_reason->wakeup_reason) {
		case DEM_WAKEUP_REASON_NONE:
			len += snprintf(buf + len, buf_sz - len, "<none>");
		break;
		case DEM_WAKEUP_REASON_SMD:
			len += snprintf(buf + len, buf_sz - len, "<smd = %s>", wakeup_reason->smd_port_name);
		break;
		case DEM_WAKEUP_REASON_INT:
			len += snprintf(buf + len, buf_sz - len, "<interrupt, pending = 0x%x>", wakeup_reason->pending_irqs);
		break;
		case DEM_WAKEUP_REASON_GPIO:
			len += snprintf(buf + len, buf_sz - len, "<gpio = 0x%x>", wakeup_reason->reserved2);
		break;
		case DEM_WAKEUP_REASON_TIMER:
			len += snprintf(buf + len, buf_sz - len, "<timer>");
		break;
		case DEM_WAKEUP_REASON_ALARM:
			len += snprintf(buf + len, buf_sz - len, "<alarm>");
		break;
		case DEM_WAKEUP_REASON_RESET:
			len += snprintf(buf + len, buf_sz - len, "<reset>");
		break;
		case DEM_WAKEUP_REASON_OTHER:
			len += snprintf(buf + len, buf_sz - len, "<other>");
		break;
		case DEM_WAKEUP_REASON_REMOTE:
			len += snprintf(buf + len, buf_sz - len, "<remote>");
		break;
		default:
			len += snprintf(buf + len, buf_sz - len, "<unknow>");
	}

	return len;
}

static char *rpc_type_to_str(int i)
{
	switch (i) {
	case RPCROUTER_CTRL_CMD_DATA:
		return "data    ";
	case RPCROUTER_CTRL_CMD_HELLO:
		return "hello   ";
	case RPCROUTER_CTRL_CMD_BYE:
		return "bye     ";
	case RPCROUTER_CTRL_CMD_NEW_SERVER:
		return "new_srvr";
	case RPCROUTER_CTRL_CMD_REMOVE_SERVER:
		return "rmv_srvr";
	case RPCROUTER_CTRL_CMD_REMOVE_CLIENT:
		return "rmv_clnt";
	case RPCROUTER_CTRL_CMD_RESUME_TX:
		return "resum_tx";
	case RPCROUTER_CTRL_CMD_EXIT:
		return "cmd_exit";
	default:
		return "invalid";
	}
}

static inline size_t sm_sprint_one_info (char *buf, int32_t buf_sz, sm_event_item_t *ev_item)
{
	ssize_t len = 0;
	sm_msm_rpc_data_t *rpc_data;
	sm_periodical_status_data_t *status_data;
	sm_msm_irq_data_t *irq_data;

	len += snprintf(buf + len, buf_sz - len, "[%5u.%06u]  Event %d: ",
		ev_item->sec_time, ev_item->nanosec_time/1000, ev_item->event_seq);
	switch(ev_item->event_id) {
		case (SM_POWER_EVENT | SM_POWER_EVENT_EARLY_SUSPEND):
			len += snprintf(buf + len, buf_sz - len, "EARLY SUSPEND %s",
				ev_item->param1 == SM_EVENT_START?"start":"success");
		break;
		case (SM_POWER_EVENT | SM_POWER_EVENT_SUSPEND):
			len += snprintf(buf + len, buf_sz - len, "SUSPEND %s",
				ev_item->param1 == SM_EVENT_START?"start":"success");
		break;
		case (SM_POWER_EVENT | SM_POWER_EVENT_RESUME):
			len += snprintf(buf + len, buf_sz - len, "RESUME %s",
				ev_item->param1 == SM_EVENT_START?"start":"success");
			if (ev_item->param1 == SM_EVENT_START) {
				len += snprintf(buf + len, buf_sz - len, ", slept %d msec ", ev_item->param2);
				if (ev_item->data_len > 0)
					len += sm_sprint_wakeup_reason ((sm_msm_pm_smem_t *)&(ev_item->data.wakeup_reason), buf + len, buf_sz - len);
			}
		break;
		case (SM_POWER_EVENT | SM_POWER_EVENT_LATE_RESUME):
			len += snprintf(buf + len, buf_sz - len, "LATE RESUME %s", ev_item->param1 == SM_EVENT_START?"start":"success");
		break;
		case (SM_POWER_EVENT | SM_POWER_EVENT_BATTERY_UPDATE):
			len += snprintf(buf + len, buf_sz - len, "BATTERY UPDATE, ");

			if (ev_item->data_len > 0)
				len += sm_sprint_battery_status ((sm_msm_battery_data_t *)&(ev_item->data.battery_info), buf + len, buf_sz - len);
		break;
		case (SM_WAKELOCK_EVENT | WAKELOCK_EVENT_ON):
			len += snprintf(buf + len, buf_sz - len, "WAKE LOCK ");

			if (ev_item->data_len > 0)
				len += sm_sprint_string_name (ev_item->data.wakelock_name, buf + len, buf_sz - len);
			len += snprintf(buf + len, buf_sz - len, " on");
		break;
		case (SM_WAKELOCK_EVENT | WAKELOCK_EVENT_OFF):
			len += snprintf(buf + len, buf_sz - len, "WAKE LOCK ");

			if (ev_item->data_len > 0)
				len += sm_sprint_string_name (ev_item->data.wakelock_name, buf + len, buf_sz - len);
			len += snprintf(buf + len, buf_sz - len, " off");
		break;
		case (SM_DEVICE_EVENT | SM_DEVICE_EVENT_SUSPEND):
			len += snprintf(buf + len, buf_sz - len, "DEVICE SUSPEND %dus, ", ev_item->param1);
			if (ev_item->data_len > 0)
				len += sm_sprint_string_name (ev_item->data.device_name, buf + len, buf_sz - len);
		break;
		case (SM_DEVICE_EVENT | SM_DEVICE_EVENT_RESUME):
			len += snprintf(buf + len, buf_sz - len, "DEVICE RESUME %dus, ", ev_item->param1);
			if (ev_item->data_len > 0)
				len += sm_sprint_string_name (ev_item->data.device_name, buf + len, buf_sz - len);
		break;
		case (SM_CLOCK_EVENT | SM_CLK_EVENT_SET_ENABLE):
			len += snprintf(buf + len, buf_sz - len, "CLK, ");
			if (ev_item->data_len > 0)
				len += sm_sprint_string_name (ev_item->data.clk_name, buf + len, buf_sz - len);
			len += snprintf(buf + len, buf_sz - len, " is still enable");
		break;
		case (SM_RPCROUTER_EVENT | RPCROUTER_WRITE_CALL):
			rpc_data = (sm_msm_rpc_data_t *)&(ev_item->data);
			len += snprintf(buf + len, buf_sz - len, "RPC  Call AP--->MP, cid(0x%08x--->0x%08x), 0x%08x:0x%08x(%s), type=%s, %d bytes",
				rpc_data->src_cid, rpc_data->dst_cid,
				be32_to_cpu(rpc_data->version), be32_to_cpu(rpc_data->prog),smd_rpc_get_sym(be32_to_cpu(rpc_data->prog)),
				rpc_type_to_str(rpc_data->type),ev_item->param1);
		break;
		case (SM_RPCROUTER_EVENT | RPCROUTER_WRITE_REPLY):
			rpc_data = (sm_msm_rpc_data_t *)&(ev_item->data);
			len += snprintf(buf + len, buf_sz - len, "RPC reply AP--->MP, cid(0x%08x--->0x%08x), type=%s, %d bytes",
				rpc_data->src_cid, rpc_data->dst_cid,
				rpc_type_to_str(rpc_data->type),  ev_item->param1);
		break;
		case (SM_RPCROUTER_EVENT | RPCROUTER_READ_CALL):
			rpc_data = (sm_msm_rpc_data_t *)&(ev_item->data);
			len += snprintf(buf + len, buf_sz - len, "RPC  Call AP<---MP, cid(0x%08x<---0x%08x), 0x%08x:0x%08x(%s), type=%s, %d bytes",
				rpc_data->dst_cid, rpc_data->src_cid,
				be32_to_cpu(rpc_data->version), be32_to_cpu(rpc_data->prog),smd_rpc_get_sym(be32_to_cpu(rpc_data->prog)),
				rpc_type_to_str(rpc_data->type),ev_item->param1);
		break;
		case (SM_RPCROUTER_EVENT | RPCROUTER_READ_REPLY):
			rpc_data = (sm_msm_rpc_data_t *)&(ev_item->data);
			len += snprintf(buf + len, buf_sz - len, "RPC reply AP<---MP, cid(0x%08x<---0x%08x), type=%s, %d bytes",
				rpc_data->dst_cid, rpc_data->src_cid,
				rpc_type_to_str(rpc_data->type),  ev_item->param1);
		break;
		case (SM_IRQ_EVENT | IRQ_EVENT_ENTER):
			irq_data = (sm_msm_irq_data_t*)&(ev_item->data);
			len += snprintf(buf + len, buf_sz - len, "entering irq, irq num = 0x%x,  func = %p",
			irq_data->irq_num, (void*)irq_data->func_addr);
			break;
		case (SM_STATUS_EVENT | STATUS_PERIODICAL_SYNC):
			status_data = (sm_periodical_status_data_t*)&(ev_item->data);
			len += snprintf(buf + len, buf_sz - len, "PERIODICAL REPORT, rtc time: %u.%9u, battery = %d mV, status = %d",
				status_data->rtime_sec, status_data->rtime_nanosec,
				status_data->battery_voltage,
				ev_item->param1);
			break;
		default:
			len += snprintf(buf + len, buf_sz - len, "unknow event: id = %x, param = 0x%x", ev_item->event_id, ev_item->param1);
		break;
	}

	return len;
}

static void print_event_info (sm_event_item_t *ev)
{
#ifdef __SM_DEBUG
	static char print_buf[4096];
	sm_sprint_info (print_buf, sizeof(print_buf), ev, 1);
	printk ("%s", print_buf);
#endif
}

static inline int sm_is_event_index_valid (uint32_t index)
{
	uint32_t range_min;

	if(atomic_read(&sm_events.read_pos) >= SM_MAXIMUM_EVENT)
		range_min = atomic_read(&sm_events.read_pos) - SM_MAXIMUM_EVENT;
	else
		range_min = 0;

	if ((index > range_min) && (index <= atomic_read(&sm_events.read_pos))) {
		return 1;
	}

	return 0;
}

/*
 * return valid event count, return < 0 for fail
 * if start index not in range of the current saved index, return error
 */
static int32_t sm_get_event (sm_event_item_t *event, uint32_t *start, uint32_t *count)
{
	uint32_t buf_start, buf_count, buf_index;
	sm_event_item_t *ev;

	if (*count < 1)
		return -EINVAL;

	if (!sm_is_event_index_valid (*start)) {
		return -EINVAL;
	}

	buf_start = *start;
	buf_count = *count;

	if ((buf_start + buf_count) > (atomic_read(&sm_events.read_pos) + 1))
		buf_count = atomic_read(&sm_events.read_pos) + 1 - buf_start;

	buf_index = 0;
	ev = event;
	while (buf_index++ < buf_count) {
		memcpy (ev, &sm_events.event_pool[buf_start&SM_MAXIMUM_EVENT_MASK], sizeof(sm_event_item_t));
		//the buffer is flushed
		if (ev->event_seq != buf_start) {
			buf_start++;
			continue;
		}

		buf_start++;
		ev++;
	}

	if (ev == event)
		return -EAGAIN;

	*count = buf_count;
	return buf_count;
}

static void sm_notify_event (sm_event_item_t *ev)
{
	if (sm_events.wait_flag & GET_EVENT_WAIT_WAKE_ALL) {
		sm_events.wait_flag &= ~GET_EVENT_WAIT_WAKE_ALL;
		wake_up_all (&sm_events.wait_wakeall_q);
	}

	if (sm_events.wait_flag & GET_EVENT_WAIT_WAKE_ONE) {
		sm_events.wait_flag &= ~(GET_EVENT_WAIT_WAKE_ONE);
		wake_up (&sm_events.wait_wakeone_q);
	}
}

static void fixup_event (sm_event_item_t *ev)
{
	if (ev->sec_time >= sm_periodical_ktime_sec_report) {
		sm_periodical_ktime_sec_report = ev->sec_time + 30*60;
		atomic_set(&sm_status_report_event, 1);
	}
}

static inline int32_t sm_event_pre_handle (uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len)
{
	int32_t i;
	struct sm_event_filter *filter;

	for ( i = 0; i < sizeof(sm_event_pre_filter)/sizeof(sm_event_pre_filter[0]); i++) {
		filter = &sm_event_pre_filter[i];

		if ((filter->event_filter & event_id) == event_id) {
			if ((sm_current_state >= filter->state_start) && (sm_current_state <= filter->state_end)) {
				if (filter->callback)
					filter->callback (event_id, param1, param2, data, data_len);
				return 0;
			}
			else
				return -1;
		}
	}

	return 0;
}

static void sm_update_read_pos (sm_event_item_t *ev)
{
	uint32_t start, end, i, ev_index;
	sm_event_item_t *ev_item;

	start = atomic_read (&sm_events.read_pos);
	end = atomic_read (&sm_events.write_pos);

	for (i = start + 1; i <= end; i++) {
		ev_item = &(sm_events.event_pool[i&SM_MAXIMUM_EVENT_MASK]);
		ev_index = ev_item->event_seq;

		if (atomic_read(&ev_item->done) != ev_index)
			return;
	}

	if (end == atomic_read(&sm_events.write_pos)) {
		atomic_set (&sm_events.read_pos, end);
		sm_notify_event (ev);
	}
}

static inline void sm_get_ktime (uint32_t *sec, uint32_t *nanosec)
{
	int this_cpu;
	unsigned long flags;
	unsigned long long t;

	raw_local_irq_save(flags);
	this_cpu = smp_processor_id();
	t = cpu_clock(this_cpu);
	raw_local_irq_restore(flags);

	*nanosec = do_div(t, 1000000000);
	*sec= (unsigned long)t;
}

static void sm_report_periodical_status (void)
{
	struct timespec ts;

	getnstimeofday(&ts);
	sm_periodcal_status.rtime_sec = ts.tv_sec;
	sm_periodcal_status.rtime_nanosec = ts.tv_nsec;
	sm_periodcal_status.battery_voltage = msm_batt_get_batt_voltage();

	sm_add_log_event(SM_STATUS_EVENT | STATUS_PERIODICAL_SYNC, sm_current_state,
		0, (void *)&sm_periodcal_status, sizeof(sm_periodical_status_data_t));
}

static __always_inline void log_irq_info(unsigned long ip,  unsigned int flags, unsigned long caller, unsigned long r0)
{
	unsigned int irq_idx;

#ifdef CONFIG_SMP
	irq_idx = atomic_inc_return(&g_track_index);
	irq_idx = irq_idx & (TRACK_BUF_SIZE - 1);
#else
	g_track_index++;
	irq_idx = g_track_index & (TRACK_BUF_SIZE - 1);
#endif
	g_track_irq_buf[irq_idx].ip = ip;
	g_track_irq_buf[irq_idx].flags = flags;
	g_track_irq_buf[irq_idx].caller = caller;
#ifdef CONFIG_SMP
	g_track_irq_buf[irq_idx].cpuid = smp_processor_id();
#endif
	g_track_irq_buf[irq_idx].reserved[0] = current->pid;
	g_track_irq_buf[irq_idx].reserved[1] = r0;

	g_track_irq_buf[irq_idx].cycles = jiffies;

/*
	cache_clean((unsigned long)(g_track_irq_buf + g_track_index));
	cache_clean((unsigned long)(&g_track_index));
*/
}

static int32_t sm_add_log_event(uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len)
{
	sm_event_item_t *ev;
	sm_event_data_t *ev_data;
	uint32_t cur_index;
	int32_t rc;

	/* for performace reason, irqs on/off occurs more frequently */
	if (likely(event_id & SM_IRQ_ONOFF_EVENT)) {
		/* no need to check if data is NULL since it
		 * is called by track_hardirqs_on/off
		 */
		log_irq_info(param1, param2, ((unsigned long*)data)[0], ((unsigned long*)data)[1]);
		return 0;
	}

	rc = sm_event_pre_handle (event_id, param1, param2, data, data_len);
	if (rc ) {
		return rc;
	}

	cur_index = atomic_inc_return (&sm_events.write_pos);
	ev = &(sm_events.event_pool[cur_index&SM_MAXIMUM_EVENT_MASK]);

	ev_data = &(ev->data);
	ev->event_seq = cur_index;
	barrier();
	ev->event_id = event_id;
	ev->param1 = param1;
	ev->param2 = param2;
	ev->data_len = data_len;

	if ((data_len > 0) && (data_len <= sizeof(sm_event_data_t))) {
		memcpy (ev_data, data, data_len);
	} else if (data_len > sizeof(sm_event_data_t)) {
		printk ("%s: Warning: the data buffer is too small to store event\n", __func__);
		printk ("event_seq = 0x%x, event_id = 0x%x, ev_param = 0x%x, param2 = 0x%x, data len = %d\n",
			ev->event_seq, ev->event_id, ev->param1, ev->param2, data_len);
		ev->data_len = 0;
		((char *)ev_data)[0] = 0;
	}

	sm_get_ktime (&ev->sec_time, &ev->nanosec_time);
	fixup_event (ev);

	barrier();
	atomic_set(&ev->done, cur_index);

	sm_update_read_pos (ev);
	print_event_info(ev);

	if (atomic_cmpxchg(&sm_status_report_event, 1, 0) == 1)
		sm_report_periodical_status ();
	return 0;
}

static unsigned long get_phys(unsigned long virtp)
{
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	struct mm_struct *mm = &init_mm;

	/* built-in case */
	if (virtp >= PAGE_OFFSET)
		return (__pa(virtp));

	/* kernel module case */
	pgd = pgd_offset(mm, virtp);
	if (!(pgd_none(*pgd) || pgd_bad(*pgd))) {
		/* XXX: need to check if pud is valid */
		pud = pud_offset(pgd, virtp);
		pmd = pmd_offset(pud, virtp);
		if (!(pmd_none(*pmd) || pmd_bad(*pmd))) {
			pte = pte_offset_kernel(pmd, virtp);
			if (pte_present(*pte)) {
				return __pa(page_address(pte_page(*pte)) +
					(virtp & ~PAGE_MASK));
			}
		}
	}

	return 0;
}

static int32_t sm_log_event_init (void)
{
#ifdef CONFIG_MSM_AMSS_ENHANCE_DEBUG
	nzi_buf_item_type input;
#endif

	sm_events.event_pool = (sm_event_item_t *)kmalloc(sizeof(sm_event_item_t)*SM_MAXIMUM_EVENT, GFP_KERNEL);
	if (sm_events.event_pool == NULL) {
		printk ("%s: ERROR, can't malloc %d size of data\n",
			__func__, sizeof(sm_event_item_t)*SM_MAXIMUM_EVENT);
		return -ENOMEM;
	}
#ifdef CONFIG_MSM_AMSS_ENHANCE_DEBUG
	input.extension.len = 1;
	input.extension.data[0] = (uint32_t)get_phys((unsigned long)&sm_events.write_pos);
	input.address = (uint32_t)__virt_to_phys((unsigned long)sm_events.event_pool);
	input.size = sizeof(sm_event_item_t) * SM_MAXIMUM_EVENT;
	strncpy(input.file_name, "smevent",
			NZI_ITEM_FILE_NAME_LENGTH);
	input.file_name[NZI_ITEM_FILE_NAME_LENGTH - 1] = 0;
	send_modem_logaddr(&input);

	input.extension.len = 1;
	input.extension.data[0] = (uint32_t)__virt_to_phys((unsigned long)&g_track_index);
	input.address = (uint32_t)__virt_to_phys((unsigned long)g_track_irq_buf);
	input.size = sizeof(struct traceirq_entry) * TRACK_BUF_SIZE;
	strncpy(input.file_name, "irqx",
			NZI_ITEM_FILE_NAME_LENGTH);
	input.file_name[NZI_ITEM_FILE_NAME_LENGTH - 1] = 0;
	send_modem_logaddr(&input);
#endif

	init_waitqueue_head(&(sm_events.wait_wakeone_q));
	init_waitqueue_head(&(sm_events.wait_wakeall_q));
	sm_events.wait_flag = 0;
	atomic_set(&sm_events.read_pos, 0);
	atomic_set(&sm_events.write_pos, 0);
	sm_set_log_system_state (SM_STATE_RUNNING);
	return 0;
}

static void sm_log_event_cleanup (void)
{
	kfree (sm_events.event_pool);
	sm_events.event_pool = NULL;
}

int32_t sm_sprint_log_info (char *buf, int32_t buf_sz, sm_event_item_t *ev, uint32_t count)
{
	uint32_t i;
	ssize_t len = 0;

	for (i = 0; i < count; i++)
	{
		len += sm_sprint_one_info (buf + len, buf_sz - len, ev + i);
		len += snprintf(buf + len, buf_sz - len, "\n");
	}

	return len;
}

static int32_t sm_get_log_event_and_data (sm_event_item_t *ev, uint32_t *start, uint32_t *count, uint32_t flag)
{
	int32_t rc = 0;
	uint32_t new_start = 0, new_count = 0;

	if (ev == NULL) {
		printk ("%s: can't store event to NULL address\n", __func__);
		return -EINVAL;
	}

retry:
	new_start = *start;
	new_count = *count;

	if (flag & GET_EVENT_ENABLE_REINDEX) {
		new_start = *start;

		if ((new_start + SM_MAXIMUM_EVENT <= atomic_read(&sm_events.read_pos)))
			new_start = atomic_read(&sm_events.read_pos) - SM_MAXIMUM_EVENT + 1;

		if (new_start == 0)
			new_start  = 1;
	}

	if (new_start > atomic_read(&sm_events.read_pos)) {
		if (flag & GET_EVENT_WAIT_WAKE_ONE) {
			sm_events.wait_flag |= GET_EVENT_WAIT_WAKE_ONE;
			rc = wait_event_interruptible (sm_events.wait_wakeone_q,
				new_start <= atomic_read(&sm_events.read_pos));

			if (rc )
				return rc;
		} else if (flag & GET_EVENT_WAIT_WAKE_ALL) {
			sm_events.wait_flag |= GET_EVENT_WAIT_WAKE_ALL;
			rc = wait_event_interruptible (sm_events.wait_wakeall_q,
				new_start <= atomic_read(&sm_events.read_pos));
			if (rc )
				return rc;
		} else {
			return -ERANGE;
		}
	}

	if((rc = sm_get_event (ev, &new_start, &new_count)) > 0) {
		*start = new_start;
		*count = new_count;
	}

	if (rc == -EAGAIN)
		goto retry;

	return rc;
}

static void sm_set_log_system_state(int want_state)
{
	if(want_state > SM_STATE_NONE && want_state < SM_STATE_INVALID)
		sm_current_state = want_state;
}

int32_t sm_log_event_register (void)
{
	int32_t rc;

	rc = sm_log_event_init ();
	if (rc)
		return rc;
	rc = sm_event_register (&event_log_ops, sm_events.event_pool);
	if (rc) {
		sm_log_event_cleanup();
		return rc;
	}

	printk ("%s\n", __func__);
	return 0;
}

int32_t sm_log_event_unregister (void)
{
	int32_t rc;

	rc = sm_event_unregister (&event_log_ops);

	if (rc)
		return rc;

	sm_log_event_cleanup ();
	return 0;
}

static int __init sm_log_event_module_init(void)
{
	return sm_log_event_register ();
}

static void __exit sm_log_event_module_exit(void)
{
	sm_log_event_unregister ();
}

module_init(sm_log_event_module_init);
module_exit(sm_log_event_module_exit);
MODULE_DESCRIPTION("MSM system event monitor driver");
MODULE_LICENSE("GPL v2");
