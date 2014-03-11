/* arch/arm/mach-msm/proc_comm.c
 *
 * Copyright (C) 2007-2008 Google, Inc.
 * Copyright (c) 2009-2012, The Linux Foundation. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/module.h>
#include <mach/msm_iomap.h>
#include <mach/system.h>
#include <mach/proc_comm.h>

#include "smd_private.h"

static inline void notify_other_proc_comm(void)
{
	/* Make sure the write completes before interrupt */
	wmb();
#if defined(CONFIG_ARCH_MSM7X30)
	__raw_writel(1 << 6, MSM_APCS_GCC_BASE + 0x8);
#elif defined(CONFIG_ARCH_MSM8X60)
	__raw_writel(1 << 5, MSM_GCC_BASE + 0x8);
#else
	__raw_writel(1, MSM_CSR_BASE + 0x400 + (6) * 4);
#endif
}

#define APP_COMMAND 0x00
#define APP_STATUS  0x04
#define APP_DATA1   0x08
#define APP_DATA2   0x0C

#define MDM_COMMAND 0x10
#define MDM_STATUS  0x14
#define MDM_DATA1   0x18
#define MDM_DATA2   0x1C

static DEFINE_SPINLOCK(proc_comm_lock);
static int msm_proc_comm_disable;

/* Poll for a state change, checking for possible
 * modem crashes along the way (so we don't wait
 * forever while the ARM9 is blowing up.
 *
 * Return an error in the event of a modem crash and
 * restart so the msm_proc_comm() routine can restart
 * the operation from the beginning.
 */
static int proc_comm_wait_for(unsigned addr, unsigned value)
{
	while (1) {
		/* Barrier here prevents excessive spinning */
		mb();
		if (readl_relaxed(addr) == value)
			return 0;

		if (smsm_check_for_modem_crash())
			return -EAGAIN;

		udelay(5);
	}
}

void msm_proc_comm_reset_modem_now(void)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;

	spin_lock_irqsave(&proc_comm_lock, flags);

again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel_relaxed(PCOM_RESET_MODEM, base + APP_COMMAND);
	writel_relaxed(0, base + APP_DATA1);
	writel_relaxed(0, base + APP_DATA2);

	spin_unlock_irqrestore(&proc_comm_lock, flags);

	/* Make sure the writes complete before notifying the other side */
	wmb();
	notify_other_proc_comm();

	return;
}
EXPORT_SYMBOL(msm_proc_comm_reset_modem_now);

int msm_proc_comm(unsigned cmd, unsigned *data1, unsigned *data2)
{
	unsigned base = (unsigned)MSM_SHARED_RAM_BASE;
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&proc_comm_lock, flags);

	if (msm_proc_comm_disable) {
		ret = -EIO;
		goto end;
	}


again:
	if (proc_comm_wait_for(base + MDM_STATUS, PCOM_READY))
		goto again;

	writel_relaxed(cmd, base + APP_COMMAND);
	writel_relaxed(data1 ? *data1 : 0, base + APP_DATA1);
	writel_relaxed(data2 ? *data2 : 0, base + APP_DATA2);

	/* Make sure the writes complete before notifying the other side */
	wmb();
	notify_other_proc_comm();

	if (proc_comm_wait_for(base + APP_COMMAND, PCOM_CMD_DONE))
		goto again;

	if (readl_relaxed(base + APP_STATUS) == PCOM_CMD_SUCCESS) {
		if (data1)
			*data1 = readl_relaxed(base + APP_DATA1);
		if (data2)
			*data2 = readl_relaxed(base + APP_DATA2);
		ret = 0;
	} else {
		ret = -EIO;
	}

	writel_relaxed(PCOM_CMD_IDLE, base + APP_COMMAND);

	switch (cmd) {
	case PCOM_RESET_CHIP:
	case PCOM_RESET_CHIP_IMM:
	case PCOM_RESET_APPS:
		msm_proc_comm_disable = 1;
		printk(KERN_ERR "msm: proc_comm: proc comm disabled\n");
		break;
	}
end:
	/* Make sure the writes complete before returning */
	wmb();
	spin_unlock_irqrestore(&proc_comm_lock, flags);
	return ret;
}
EXPORT_SYMBOL(msm_proc_comm);


/*
        Read NV item
        App ARM calls this API to read NV item from Modem ARM
*/
int msm_read_nv(unsigned int nv_item, void *buf)
{
        int ret = -1;
        uint32_t data1 = nv_item;
        uint32_t data2 ;
        unsigned char *dest = buf;
        unsigned int i;
        if (NULL == buf)
                return ret;
        ret = msm_proc_comm(PCOM_NV_READ, &data1, &data2);
        if (ret)
                return ret;
        switch (nv_item)
        {
        case 4678:
                for(i = 0; i < 6; i++ )
                {
                        if(i < 4)
                        {
                                *dest++ = (unsigned char)(data2 >> (i*8));
                        }
                        else
                        {
                                *dest++ = (unsigned char)(data1 >> ((i-4)*8));
                        }
                }
                break;
        default:
                printk(KERN_ERR "%s:nv item %d is not supported now\n",__func__,nv_item);
                ret = -EIO;
                break;
        }
        return ret;
}
#define NV_ITEM_WLAN_MAC_ADDR   4678
extern unsigned char wlan_mac_addr[6];
int read_nv(unsigned int nv_item, void *buf)
{
        int ret = -EIO;
        switch (nv_item)
        {
                case 4678:
                        if(memcmp(wlan_mac_addr,"\0\0\0\0\0\0",sizeof(wlan_mac_addr))!=0){
                                memcpy(buf,wlan_mac_addr,sizeof(wlan_mac_addr));
                                ret = 0;
                        } else {
                            msm_read_nv(NV_ITEM_WLAN_MAC_ADDR,wlan_mac_addr);
                            printk(KERN_ERR "%s: read mac addr from nv\n",__func__);
                            if(memcmp(wlan_mac_addr,"\0\0\0\0\0\0",sizeof(wlan_mac_addr))!=0){
                                memcpy(buf,wlan_mac_addr,sizeof(wlan_mac_addr));
                                ret = 0;
                            }
                        }
                        break;
                default:
                        printk(KERN_ERR "%s:nv item %d is not supported now\n",__func__,nv_item);
                        break;
        }
        return ret;
}
EXPORT_SYMBOL(read_nv);
