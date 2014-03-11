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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/sm_event.h>
#include <linux/sm_event_log.h>

static sm_event_ops *event_ops = NULL;
unsigned int event_mask;
static atomic_t event_use_count;
static int32_t enable_flag = 0;

static sm_event_item_t *sm_event_buffer;
EXPORT_SYMBOL(event_mask);

int32_t sm_event_register (sm_event_ops *ops, sm_event_item_t *event_pool)
{
	if (event_ops == NULL) {
		event_ops = ops;
		sm_event_buffer = event_pool;
		atomic_set (&event_use_count, 0);
		smp_mb();
		enable_flag = 1;
		return 0;
	}

	return -EBUSY;
}
EXPORT_SYMBOL(sm_event_register);

int32_t sm_event_unregister (sm_event_ops *ops)
{
	if (event_ops == ops) {
		int32_t retry;

		enable_flag = 0;
		barrier();

		retry = 3;

		while (retry-- > 0) {
			if (!atomic_read (&event_use_count)) {
				event_ops = NULL;
				sm_event_buffer = NULL;
				return 0;
			}
			schedule();
		}

		return -EBUSY;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(sm_event_unregister);

int32_t sm_sprint_info (char *buf, int32_t buf_sz, sm_event_item_t *ev, uint32_t count)
{
	if (event_ops && event_ops->sm_sprint_info) {
		uint32_t rc = 0;
		atomic_inc (&event_use_count);

		if (enable_flag)
			rc = event_ops->sm_sprint_info(buf, buf_sz, ev, count);
		atomic_dec (&event_use_count);
		return rc;
	}

	return 0;
}
EXPORT_SYMBOL(sm_sprint_info);

int32_t __sm_add_event(uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len)
{
	if (event_ops && event_ops->sm_add_event) {
		uint32_t rc = 0;
		atomic_inc (&event_use_count);
		if (enable_flag)
			rc = event_ops->sm_add_event(event_id, param1, param2, data, data_len);
		atomic_dec (&event_use_count);
		return rc;
	}
	return -EINVAL;
}
EXPORT_SYMBOL(__sm_add_event);

int32_t sm_get_event_and_data (sm_event_item_t *ev, uint32_t *start, uint32_t *count, uint32_t flag)
{
	if (event_ops && event_ops->sm_get_event_and_data) {
		int32_t rc = 0;
		atomic_inc (&event_use_count);
		if (enable_flag)
			rc = event_ops->sm_get_event_and_data(ev, start, count, flag);
		atomic_dec (&event_use_count);
		return rc;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(sm_get_event_and_data);

void sm_set_event_mask(uint32_t mask)
{
	event_mask = mask;
}
EXPORT_SYMBOL(sm_set_event_mask);

uint32_t sm_get_event_mask(void)
{
	return event_mask;
}
EXPORT_SYMBOL(sm_get_event_mask);

void sm_set_system_state(int want_state)
{
	if (event_ops && event_ops->sm_set_system_state) {
		atomic_inc (&event_use_count);
		if (enable_flag)
			event_ops->sm_set_system_state(want_state);
		atomic_dec (&event_use_count);
	}
}
EXPORT_SYMBOL(sm_set_system_state);
