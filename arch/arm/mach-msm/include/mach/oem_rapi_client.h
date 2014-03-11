/* Copyright (c) 2009,2012 The Linux Foundation. All rights reserved.
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

#ifndef __ASM__ARCH_OEM_RAPI_CLIENT_H
#define __ASM__ARCH_OEM_RAPI_CLIENT_H

/*
 * OEM RAPI CLIENT Driver header file
 */

#include <linux/types.h>
#include <mach/msm_rpcrouter.h>

#ifdef CONFIG_MSM_AMSS_ENHANCE_DEBUG
#define NZI_ITEM_FILE_NAME_LENGTH	8
#define TASK_STRUCT_TAG			0x7461736b
#define MEM_INFO_TAG			0x6d657369
typedef struct {
	uint32_t len;
	uint32_t data[3];
} extent_t;
typedef struct {
	extent_t extension;
	uint32_t address;
	char file_name[NZI_ITEM_FILE_NAME_LENGTH];	/* This indicate the log type */
	uint32_t size; 					/* Size of the buffer */
} nzi_buf_item_type;
int send_modem_logaddr(nzi_buf_item_type *input);
#endif

enum {
	OEM_RAPI_CLIENT_EVENT_NONE = 0,

	/*
	 * list of oem rapi client events
	 */
	OEM_RAPI_CLIENT_EVENT_TRI_COLOR_LED_WORK = 21,
	OEM_RAPI_STREAMING_SILENT_PROFILE_SET = 30,
	OEM_RAPI_STREAMING_SILENT_PROFILE_GET = 31,

	OEM_RAPI_CLIENT_EVENT_QRDCOMPACTDUMP_NZIITEM_WRITE = 40,
	OEM_RAPI_CLIENT_EVENT_DEBUG_SLEEP_MONITOR = 41,

	OEM_RAPI_CLIENT_EVENT_MAX

};

struct oem_rapi_client_streaming_func_cb_arg {
	uint32_t  event;
	void      *handle;
	uint32_t  in_len;
	char      *input;
	uint32_t out_len_valid;
	uint32_t output_valid;
	uint32_t output_size;
};

struct oem_rapi_client_streaming_func_cb_ret {
	uint32_t *out_len;
	char *output;
};

struct oem_rapi_client_streaming_func_arg {
	uint32_t event;
	int (*cb_func)(struct oem_rapi_client_streaming_func_cb_arg *,
		       struct oem_rapi_client_streaming_func_cb_ret *);
	void *handle;
	uint32_t in_len;
	char *input;
	uint32_t out_len_valid;
	uint32_t output_valid;
	uint32_t output_size;
};

struct oem_rapi_client_streaming_func_ret {
	uint32_t *out_len;
	char *output;
};

int oem_rapi_client_streaming_function(
	struct msm_rpc_client *client,
	struct oem_rapi_client_streaming_func_arg *arg,
	struct oem_rapi_client_streaming_func_ret *ret);

int oem_rapi_client_close(void);

struct msm_rpc_client *oem_rapi_client_init(void);

#endif
