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

#ifndef _SM_EVENT_H
#define _SM_EVENT_H

#include <linux/sm_event_log.h>

extern unsigned int event_mask;

typedef struct{
	int32_t (*sm_add_event)(uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len);
	int32_t (*sm_get_event_and_data)(sm_event_item_t *ev, uint32_t *start, uint32_t *count, uint32_t flag);
	void (*sm_set_system_state)(int want_state);
	int32_t (*sm_sprint_info)(char *buf, int32_t buf_sz, sm_event_item_t *ev, uint32_t count);
}sm_event_ops;

/*
 * should be called in the atomic context
 */
int32_t sm_event_register (sm_event_ops *ops, sm_event_item_t *event_pool);

/*
 * should be called in the atomic context
 */
int32_t sm_event_unregister (sm_event_ops *ops);

int32_t sm_get_event_and_data (sm_event_item_t *ev, uint32_t *start, uint32_t *count, uint32_t flag);
void sm_set_event_mask (uint32_t event_mask);
uint32_t sm_get_event_mask(void);
int32_t __sm_add_event(uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len);
void sm_set_system_state(int want_state);
int32_t sm_sprint_info (char *buf, int32_t buf_sz, sm_event_item_t *ev, uint32_t count);

static __always_inline int32_t sm_add_event(uint32_t event_id, uint32_t param1, int param2, void *data, uint32_t data_len)
{
	int ret;

	if (!((event_id & event_mask) == event_id))
		return 0;

	ret = __sm_add_event(event_id, param1, param2, data, data_len);

	return ret;
}
#endif
