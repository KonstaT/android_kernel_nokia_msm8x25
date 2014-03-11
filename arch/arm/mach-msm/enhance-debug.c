/* Copyright (c) 2012, The Linux Foundation. All rights reserved.
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

#include <linux/string.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/err.h>
#include <mach/oem_rapi_client.h>

static struct msm_rpc_client *client = NULL;

int send_modem_logaddr(nzi_buf_item_type *input)
{
	int ret;

	struct oem_rapi_client_streaming_func_arg client_arg = {
		OEM_RAPI_CLIENT_EVENT_QRDCOMPACTDUMP_NZIITEM_WRITE,
		NULL,
		(void *)NULL,
		sizeof(*input),
		(void *)input,
		0,
		0,
		0
	};
	struct oem_rapi_client_streaming_func_ret client_ret = {
		(uint32_t *)NULL,
		(char *)NULL
	};

	if (client == NULL) {
		client = oem_rapi_client_init();
		if (IS_ERR(client) || (!client)) {
			client = NULL;
			return -ENODEV;
		}
	}

	ret = oem_rapi_client_streaming_function(client, &client_arg, &client_ret);

	return ret;
}
EXPORT_SYMBOL(send_modem_logaddr);
