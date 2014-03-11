/*
 * Gadget Driver for Android ADB
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/delay.h>
#include <linux/wait.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/device.h>
#include <linux/miscdevice.h>

#define WIN_BULK_BUFFER_SIZE           4096

/* number of tx requests to allocate */
//#define TX_REQ_MAX  1 //4


/* String IDs */
#define INTERFACE_STRING_INDEX	0

/* values for win_dev.state */
#define STATE_OFFLINE               0   /* initial state, disconnected */
#define STATE_READY                 1   /* ready for userspace calls */
#define STATE_BUSY                  2   /* processing userspace calls */
#define STATE_CANCELED              3   /* transaction canceled by host */
#define STATE_ERROR                 4   /* error from completion routine */


#define RX_REQ_MAX 2


/* ID for Microsoft WIN OS String */
#define WIN_OS_STRING_ID   0xEE

/* WIN class reqeusts */
#define WIN_REQ_CANCEL              0x64
//#define WIN_REQ_GET_EXT_EVENT_DATA  0x65
//#define WIN_REQ_RESET               0x66
#define WIN_REQ_GET_DEVICE_STATUS   0x67

/* constants for device status */
#define WIN_RESPONSE_OK             0x2001
#define WIN_RESPONSE_DEVICE_BUSY    0x2019

//#define MICROSOFT_OS_DESCRIPTOR_INDEX   (unsigned char)0xEE //Magic string index number for the Microsoft OS descriptor
//#define GET_MS_DESCRIPTOR               (unsigned char)0xEE //(arbitarily assigned, but should not clobber/overlap normal bRequests)
//#define EXTENDED_COMPAT_ID              0x0004
//#define EXTENDED_PROPERTIES             0x0005

static const char win_shortname[] = "win_usb";

struct win_dev {
	struct usb_function function;
	struct usb_composite_dev *cdev;
	spinlock_t lock;

	struct usb_ep *ep_in;
	struct usb_ep *ep_out;

	atomic_t online;
	atomic_t error;
	
	int state;

	atomic_t read_excl;
	atomic_t write_excl;
	atomic_t open_excl;

	struct list_head tx_idle;

	wait_queue_head_t read_wq;
	wait_queue_head_t write_wq;
	struct usb_request *rx_req;
	int rx_done;
	bool notify_close;
	bool close_notified;
};

static struct usb_interface_descriptor win_interface_desc = {
	.bLength                = USB_DT_INTERFACE_SIZE,//9
	.bDescriptorType        = USB_DT_INTERFACE,//4
	//.bInterfaceNumber       = 0,
	.bAlternateSetting 		= 0,
	.bNumEndpoints          = 2,
	.bInterfaceClass        = USB_CLASS_VENDOR_SPEC,
	.bInterfaceSubClass     = USB_SUBCLASS_VENDOR_SPEC,
	.bInterfaceProtocol     = USB_CLASS_VENDOR_SPEC,
	.iInterface             = 0,
};


static struct usb_endpoint_descriptor win_highspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,//7
	.bDescriptorType        = USB_DT_ENDPOINT,//5
	.bEndpointAddress       = USB_DIR_IN,//0x80
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
	.bInterval 							=	0,
};

static struct usb_endpoint_descriptor win_highspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(512),
	.bInterval 							=	0,
};

static struct usb_endpoint_descriptor win_fullspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.bInterval 							=	0,
};

static struct usb_endpoint_descriptor win_fullspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
};


static struct usb_descriptor_header *fs_win_descs[] = {
	(struct usb_descriptor_header *) &win_interface_desc,
	(struct usb_descriptor_header *) &win_fullspeed_in_desc,
	(struct usb_descriptor_header *) &win_fullspeed_out_desc,
	NULL,
};

static struct usb_descriptor_header *hs_win_descs[] = {
	(struct usb_descriptor_header *) &win_interface_desc,
	(struct usb_descriptor_header *) &win_highspeed_in_desc,
	(struct usb_descriptor_header *) &win_highspeed_out_desc,
	NULL,
};

//static void win_ready_callback(void);
//static void win_closed_callback(void);


/* temporary variable used between win_open() and win_gadget_bind() */
static struct win_dev *_win_dev;


static struct usb_string win_string_defs[] = {
	/* Naming interface "WIN" so libwin will recognize us */
	[INTERFACE_STRING_INDEX].s	= "WINUSB",
	{  },	/* end of list */
};

static struct usb_gadget_strings win_string_table = {
	.language		= 0x0409,	/* en-US */ 
	.strings		= win_string_defs,
};

static struct usb_gadget_strings *win_strings[] = {
	&win_string_table,
	NULL,
};

/* Microsoft WIN OS String */
static u8 win_os_string[] = {
	18, /* sizeof(win_os_string) */
	USB_DT_STRING,
	/* Signature field: "MSFT100" */
	'M', 0, 'S', 0, 'F', 0, 'T', 0, '1', 0, '0', 0, '0', 0,
	/* vendor code */
	1,
	/* padding */
	0
};

/* Microsoft Extended Configuration Descriptor Header Section */
struct win_ext_config_desc_header {
	__le32	dwLength;
	__u16	bcdVersion;
	__le16	wIndex;
	__u8	bCount;
	__u8	reserved[7];
};

/* Microsoft Extended Configuration Descriptor Function Section */
struct win_ext_config_desc_function {
	__u8	bFirstInterfaceNumber;
	__u8	bInterfaceCount;
	__u8	compatibleID[8];
	__u8	subCompatibleID[8];
	__u8	reserved[6];
};

/* Microsoft Extended property Descriptor Header Section */
struct win_ext_property_desc_header {
	__le32	dwLength;
	__u16	bcdVersion;
	__le16	wIndex;
	__u16	wCount;
};
/* Microsoft Extended property Descriptor Function Section */
struct win_ext_property_desc_function {
	__le32	dwsize;
	__u8	dwPropertyDataType;
	__le32	wPropertyNameLength;
	__u16	bPropertyName[40];
	__le32	dwPropertyDataLength;
	__u16	bPropertyData[78];
};

/* WIN Extended Configuration Descriptor */
struct {
	struct win_ext_config_desc_header	header;
	struct win_ext_config_desc_function    function;	
} win_ext_config_desc = {
	.header = {
		.dwLength = __constant_cpu_to_le32(sizeof(win_ext_config_desc)),
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(4),
		.bCount = __constant_cpu_to_le16(1),
	},
	.function = {
		//.bFirstInterfaceNumber = 0,
		.bInterfaceCount = 1,
		.compatibleID = { 'W','I','N','U','S','B'},
	},
};

struct {
	//struct win_ext_property_desc_header	p_header;
	//struct win_ext_property_desc_function    p_function;
	__le32	dwLength;
	__le16	bcdVersion;
	__le16	wIndex;
	__le16	wCount;
	__le32	dwsize;
	__le32	dwPropertyDataType;
	__le16	wPropertyNameLength;
	__le16	bPropertyName[20];
	__le32	dwPropertyDataLength;
	__le16	bPropertyData[39];

		
} __attribute__((packed)) win_ext_prop_desc = {	
	//.p_header = {
		.dwLength = __constant_cpu_to_le32(sizeof(win_ext_prop_desc)),//142,
		.bcdVersion = __constant_cpu_to_le16(0x0100),
		.wIndex = __constant_cpu_to_le16(5),
		.wCount = 1,		
	//},
	//.p_function = {
		.dwsize = __constant_cpu_to_le16(132),
		.dwPropertyDataType = 1,
		.wPropertyNameLength = 40,
		.bPropertyName = {'D','e','v','i','c','e','I','n','t','e','r','f','a','c','e','G','U','I','D','\0'},
		.dwPropertyDataLength = 78,
		.bPropertyData =  {'{', '7', 'e', 'a', 'f', 'f', '7', '2', '6', '-', '3', '4', 'c', 'c', '-', '4', '2', '0', '4', '-', 'b', '0', '9', 'd', '-', 'f', '9', '5', '4', '7', '1', 'b', '8', '7', '3', 'c', 'f', '}','\0'},
	//},
};

struct win_device_status {
	__le16	wLength;
	__le16	wCode;
};

static inline struct win_dev *func_to_win(struct usb_function *f)
{
	return container_of(f, struct win_dev, function);
}


static struct usb_request *win_request_new(struct usb_ep *ep, int buffer_size)
{
	struct usb_request *req = usb_ep_alloc_request(ep, GFP_KERNEL);
	if (!req)
		return NULL;

	/* now allocate buffers for the requests */
	req->buf = kmalloc(buffer_size, GFP_KERNEL);
	if (!req->buf) {
		usb_ep_free_request(ep, req);
		return NULL;
	}

	return req;
}

static void win_request_free(struct usb_request *req, struct usb_ep *ep)
{
	if (req) {
		kfree(req->buf);
		usb_ep_free_request(ep, req);
	}
}

static inline int win_lock(atomic_t *excl)
{
	if (atomic_inc_return(excl) == 1) {
		return 0;
	} else {
		atomic_dec(excl);
		return -1;
	}
}

static inline void win_unlock(atomic_t *excl)
{
	atomic_dec(excl);
}

/* add a request to the tail of a list */
void win_req_put(struct win_dev *dev, struct list_head *head,
		struct usb_request *req)
{
	unsigned long flags;

	spin_lock_irqsave(&dev->lock, flags);
	list_add_tail(&req->list, head);
	spin_unlock_irqrestore(&dev->lock, flags);
}

/* remove a request from the head of a list */
struct usb_request *win_req_get(struct win_dev *dev, struct list_head *head)
{
	unsigned long flags;
	struct usb_request *req;
	spin_lock_irqsave(&dev->lock, flags);
	if (list_empty(head)) {
		req = 0;
	} else {
		req = list_first_entry(head, struct usb_request, list);
		list_del(&req->list);
	}
	spin_unlock_irqrestore(&dev->lock, flags);
	return req;
}

static void win_complete_in(struct usb_ep *ep, struct usb_request *req)
{
	struct win_dev *dev = _win_dev;
	if (req->status != 0) {
		printk("error in %s, req->status = 0x%08x\n",
			__func__, req->status);
		atomic_set(&dev->error, 1);
	}

	win_req_put(dev, &dev->tx_idle, req);

	wake_up(&dev->write_wq);
}

static void win_complete_out(struct usb_ep *ep, struct usb_request *req)
{
	struct win_dev *dev = _win_dev;

	dev->rx_done = 1;
	if (req->status != 0 && req->status != -ECONNRESET) {
		printk("error in %s, req->status = 0x%08x\n",
			__func__, req->status);
		atomic_set(&dev->error, 1);
	}

	wake_up(&dev->read_wq);
}

static int win_create_bulk_endpoints(struct win_dev *dev,
				struct usb_endpoint_descriptor *in_desc,
				struct usb_endpoint_descriptor *out_desc)
{
	struct usb_composite_dev *cdev = dev->cdev;
	struct usb_request *req;
	struct usb_ep *ep;
    int i;
	DBG(cdev, "create_bulk_endpoints dev: %p\n", dev);

	ep = usb_ep_autoconfig(cdev->gadget, in_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_in failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for ep_in got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_in = ep;

	ep = usb_ep_autoconfig(cdev->gadget, out_desc);
	if (!ep) {
		DBG(cdev, "usb_ep_autoconfig for ep_out failed\n");
		return -ENODEV;
	}
	DBG(cdev, "usb_ep_autoconfig for win ep_out got %s\n", ep->name);
	ep->driver_data = dev;		/* claim the endpoint */
	dev->ep_out = ep;

	/* now allocate requests for our endpoints */
	req = win_request_new(dev->ep_out, WIN_BULK_BUFFER_SIZE);
	if (!req)
		goto fail;
	req->complete = win_complete_out;
	dev->rx_req = req;

	for (i = 0; i < TX_REQ_MAX; i++) {
		req = win_request_new(dev->ep_in, WIN_BULK_BUFFER_SIZE);
		if (!req)
			goto fail;
		req->complete = win_complete_in;
		win_req_put(dev, &dev->tx_idle, req);
	}

	return 0;

fail:
	printk(KERN_ERR "win_bind() could not allocate requests\n");
	return -1;
}

static ssize_t win_read(struct file *fp, char __user *buf,
				size_t count, loff_t *pos)
{
	struct win_dev *dev = fp->private_data;
	struct usb_request *req;
	int r = count, xfer;
	int ret;
	pr_debug("win_read(%d)\n", count);
	if (!_win_dev) {
		printk("win device is not setup\n");
		return -ENODEV;
	}

	if (count > WIN_BULK_BUFFER_SIZE) {
		printk("win error: the read count %d is too large\n", count);
		return -EINVAL;
	}

	if (win_lock(&dev->read_excl)) {
		printk("win device is read busy\n");
		return -EBUSY;
	}
	/* we will block until we're online */
	while (!(atomic_read(&dev->online) || atomic_read(&dev->error))) {
		pr_debug("win_read: waiting for online state\n");
		ret = wait_event_interruptible(dev->read_wq,
			(atomic_read(&dev->online) ||
			atomic_read(&dev->error)));
		if (ret < 0) {
			win_unlock(&dev->read_excl);
			printk("win_read: while loop error: ret = %d\n", ret);
			return ret;
		}
	}
	if (atomic_read(&dev->error)) {
		printk("win read -EIO due to dev->error 1\n");
		r = -EIO;
		goto done;
	}

requeue_req:
	/* queue a request */
	req = dev->rx_req;
	req->length = count;
	dev->rx_done = 0;
	ret = usb_ep_queue(dev->ep_out, req, GFP_ATOMIC);
	if (ret < 0) {
		pr_debug("win_read: failed to queue req %p (%d)\n", req, ret);
		r = -EIO;
		printk("win_read: failed to queue req %p (%d)\n", req, ret);
		atomic_set(&dev->error, 1);
		goto done;
	} else {
		pr_debug("rx %p queue\n", req);
	}

	/* wait for a request to complete */
	ret = wait_event_interruptible(dev->read_wq, dev->rx_done);
	if (ret < 0) {
		if (ret != -ERESTARTSYS) {
			atomic_set(&dev->error, 1);
			printk("win_read: failed to wait_event_int\n");
		}
		r = ret;
		printk("win_read: interruptiable return ret = %d\n", ret);
		usb_ep_dequeue(dev->ep_out, req);
		goto done;
	}
	if (!atomic_read(&dev->error)) {
		/* If we got a 0-len packet, throw it back and try again. */
		if (req->actual == 0)
			goto requeue_req;

		pr_debug("rx %p %d\n", req, req->actual);
		xfer = (req->actual < count) ? req->actual : count;
		if (copy_to_user(buf, req->buf, xfer)) {
			printk("win_read: error in copy_to_user\n");
			r = -EFAULT;
		}
	} else {
		printk("win read -EIO due to dev->error 2\n");
		r = -EIO;
	}

done:
	win_unlock(&dev->read_excl);
	pr_debug("win_read returning %d\n", r);
	return r;
}

static ssize_t win_write(struct file *fp, const char __user *buf,
				 size_t count, loff_t *pos)
{
	struct win_dev *dev = fp->private_data;
	struct usb_request *req = 0;
	int r = count, xfer;
	int ret;
	if (!_win_dev)
		return -ENODEV;
	pr_debug("win_write(%d)\n", count);

	if (win_lock(&dev->write_excl))
		return -EBUSY;

	while (count > 0) {
		if (atomic_read(&dev->error)) {
			pr_debug("win_write dev->error\n");
			r = -EIO;
			break;
		}

		/* get an idle tx request to use */
		req = 0;
		ret = wait_event_interruptible(dev->write_wq,
			((req = win_req_get(dev, &dev->tx_idle)) ||
			 atomic_read(&dev->error)));

		if (ret < 0) {
			r = ret;
			break;
		}

		if (req != 0) {
			if (count > WIN_BULK_BUFFER_SIZE)
				xfer = WIN_BULK_BUFFER_SIZE;
			else
				xfer = count;
			if (copy_from_user(req->buf, buf, xfer)) {
				r = -EFAULT;
				break;
			}

			req->length = xfer;
			ret = usb_ep_queue(dev->ep_in, req, GFP_ATOMIC);
			if (ret < 0) {
				pr_debug("win_write: xfer error %d\n", ret);
				printk("win_write: xfer error %d\n", ret);
				atomic_set(&dev->error, 1);
				r = -EIO;
				break;
			}

			buf += xfer;
			count -= xfer;

			/* zero this so we don't try to free it on error exit */
			req = 0;
		}
	}

	if (req)
		win_req_put(dev, &dev->tx_idle, req);

	win_unlock(&dev->write_excl);
	pr_debug("win_write returning %d\n", r);
	return r;
}

static int win_open(struct inode *ip, struct file *fp)
{
	pr_info("win_open\n");
	if (!_win_dev)
		return -ENODEV;

	if (win_lock(&_win_dev->open_excl))
		return -EBUSY;

	fp->private_data = _win_dev;

	/* clear the error latch */
	atomic_set(&_win_dev->error, 0);

	if (_win_dev->close_notified) {
		_win_dev->close_notified = false;
	//	win_ready_callback();
	}

	_win_dev->notify_close = true;
	return 0;
}

static int win_release(struct inode *ip, struct file *fp)
{
	pr_info("win_release\n");

	/*
	 * WIN daemon closes the device file after I/O error.  The
	 * I/O error happen when Rx requests are flushed during
	 * cable disconnect or bus reset in configured state.  Disabling
	 * USB configuration and pull-up during these scenarios are
	 * undesired.  We want to force bus reset only for certain
	 * commands like "win root" and "win usb".
	 */
	if (_win_dev->notify_close) {
		//win_closed_callback();
		_win_dev->close_notified = true;
	}

	win_unlock(&_win_dev->open_excl);
	return 0;
}

/* file operations for WIN device /dev/android_win */
static const struct file_operations win_fops = {
	.owner = THIS_MODULE,
	.read = win_read,
	.write = win_write,
	.open = win_open,
	.release = win_release,
};

static struct miscdevice win_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = win_shortname,
	.fops = &win_fops,
};

static int win_ctrlrequest(struct usb_composite_dev *cdev,
				const struct usb_ctrlrequest *ctrl)
{
	struct win_dev *dev = _win_dev;
	int	value = -EOPNOTSUPP;
	u16	w_index = le16_to_cpu(ctrl->wIndex);
	u16	w_value = le16_to_cpu(ctrl->wValue);
	u16	w_length = le16_to_cpu(ctrl->wLength);
	unsigned long	flags;


	//printk("win_ctrlrequest %02x.%02x v%04x i%04x l%u\n ",ctrl->bRequestType, ctrl->bRequest,
	//		w_value, w_index, w_length);
	/* Handle WIN OS string */

	/* Handle WIN OS string */
	if (ctrl->bRequestType ==
			(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)
			&& ctrl->bRequest == USB_REQ_GET_DESCRIPTOR
			&& (w_value >> 8) == USB_DT_STRING
			&& (w_value & 0xFF) == WIN_OS_STRING_ID) {
		value = (w_length < sizeof(win_os_string)
				? w_length : sizeof(win_os_string));
		memcpy(cdev->req->buf, win_os_string, value);
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_VENDOR) {
		/* Handle WIN OS descriptor */
		DBG(cdev, "vendor request: %d index: %d value: %d length: %d\n",
			ctrl->bRequest, w_index, w_value, w_length);
		//win_ext_config_desc.function.bFirstInterfaceNumber = win_interface_desc.bInterfaceNumber;
		if (ctrl->bRequest == 1	&& (ctrl->bRequestType & USB_DIR_IN))
		{	if (w_index == 4 ) {
			value = (w_length < sizeof(win_ext_config_desc) ?
					w_length : sizeof(win_ext_config_desc));
			memcpy(cdev->req->buf, &win_ext_config_desc, value);
		}else if (w_index == 5 && win_interface_desc.bInterfaceNumber == w_value){
		value = (w_length < sizeof(win_ext_prop_desc) ?
					w_length : sizeof(win_ext_prop_desc));
			memcpy(cdev->req->buf, &win_ext_prop_desc, value);
		}
		}
	} else if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS) {
		DBG(cdev, "class request: %d index: %d value: %d length: %d\n",
			ctrl->bRequest, w_index, w_value, w_length);
		if (ctrl->bRequest == WIN_REQ_CANCEL && w_index == 0
				&& w_value == 0) {
			DBG(cdev, "WIN_REQ_CANCEL\n");

			spin_lock_irqsave(&dev->lock, flags);
			if (dev->state == STATE_BUSY) {
				dev->state = STATE_CANCELED;
				wake_up(&dev->read_wq);
				wake_up(&dev->write_wq);
			}
			spin_unlock_irqrestore(&dev->lock, flags);

			/* We need to queue a request to read the remaining
			 *  bytes, but we don't actually need to look at
			 * the contents.
			 */
			value = w_length;
		} else if (ctrl->bRequest == WIN_REQ_GET_DEVICE_STATUS
				&& w_value == 0) {
			struct win_device_status *status = cdev->req->buf;
			status->wLength =
				__constant_cpu_to_le16(sizeof(*status));

			DBG(cdev, "WIN_REQ_GET_DEVICE_STATUS\n");
			spin_lock_irqsave(&dev->lock, flags);
			/* device status is "busy" until we report
			 * the cancelation to userspace
			 */
			if (dev->state == STATE_CANCELED)
				status->wCode =
					__cpu_to_le16(WIN_RESPONSE_DEVICE_BUSY);
			else
				status->wCode =
					__cpu_to_le16(WIN_RESPONSE_OK);
			spin_unlock_irqrestore(&dev->lock, flags);
			value = sizeof(*status);
		}
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		int rc;
		cdev->req->zero = value < w_length;
		cdev->req->length = value;
		rc = usb_ep_queue(cdev->gadget->ep0, cdev->req, GFP_ATOMIC);
		if (rc < 0)
			ERROR(cdev, "%s: response queue error\n", __func__);
	}
	
	return value;
}



static int
win_function_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct win_dev	*dev = func_to_win(f);
	int			id;
	int			ret;

	dev->cdev = cdev;
	DBG(cdev, "win_function_bind dev: %p\n", dev);

		
	/* allocate interface ID(s) */
	id = usb_interface_id(c, f);
	if (id < 0)
		return id;
	win_interface_desc.bInterfaceNumber = id;
	win_ext_config_desc.function.bFirstInterfaceNumber = id;
	
	/* allocate endpoints */
	ret = win_create_bulk_endpoints(dev, &win_fullspeed_in_desc,
			&win_fullspeed_out_desc);
	if (ret)
		return ret;

	/* support high speed hardware */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		win_highspeed_in_desc.bEndpointAddress =
			win_fullspeed_in_desc.bEndpointAddress;
		win_highspeed_out_desc.bEndpointAddress =
			win_fullspeed_out_desc.bEndpointAddress;
	}

	DBG(cdev, "%s speed %s: IN/%s, OUT/%s\n",
			gadget_is_dualspeed(c->cdev->gadget) ? "dual" : "full",
			f->name, dev->ep_in->name, dev->ep_out->name);
	return 0;
}

static void
win_function_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct win_dev	*dev = func_to_win(f);
	struct usb_request *req;

	atomic_set(&dev->online, 0);
	printk("entering in %s\n", __func__);
	atomic_set(&dev->error, 1);

	wake_up(&dev->read_wq);

	win_request_free(dev->rx_req, dev->ep_out);
	while ((req = win_req_get(dev, &dev->tx_idle)))
		win_request_free(req, dev->ep_in);
}

static int win_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct win_dev	*dev = func_to_win(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	int ret;

	DBG(cdev, "win_function_set_alt intf: %d alt: %d\n", intf, alt);
	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_in);
	if (ret) {
		dev->ep_in->desc = NULL;
		ERROR(cdev, "config_ep_by_speed failes for ep %s, result %d\n",
				dev->ep_in->name, ret);
		return ret;
	}
	ret = usb_ep_enable(dev->ep_in);
	if (ret) {
		ERROR(cdev, "failed to enable ep %s, result %d\n",
			dev->ep_in->name, ret);
		return ret;
	}

	ret = config_ep_by_speed(cdev->gadget, f, dev->ep_out);
	if (ret) {
		dev->ep_out->desc = NULL;
		ERROR(cdev, "config_ep_by_speed failes for ep %s, result %d\n",
			dev->ep_out->name, ret);
		usb_ep_disable(dev->ep_in);
		return ret;
	}
	ret = usb_ep_enable(dev->ep_out);
	if (ret) {
		ERROR(cdev, "failed to enable ep %s, result %d\n",
				dev->ep_out->name, ret);
		usb_ep_disable(dev->ep_in);
		return ret;
	}
	atomic_set(&dev->online, 1);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);
	return 0;
}

static void win_function_disable(struct usb_function *f)
{
	struct win_dev	*dev = func_to_win(f);
	struct usb_composite_dev	*cdev = dev->cdev;

	DBG(cdev, "win_function_disable cdev %p\n", cdev);
	/*
	 * Bus reset happened or cable disconnected.  No
	 * need to disable the configuration now.  We will
	 * set noify_close to true when device file is re-opened.
	 */
	dev->notify_close = false;
	atomic_set(&dev->online, 0);
	atomic_set(&dev->error, 1);
	usb_ep_disable(dev->ep_in);
	usb_ep_disable(dev->ep_out);

	/* readers may be blocked waiting for us to go online */
	wake_up(&dev->read_wq);

	VDBG(cdev, "%s disabled\n", dev->function.name);
}

static int win_bind_config(struct usb_configuration *c)
{
	struct win_dev *dev = _win_dev;

	printk(KERN_INFO "win_bind_config start \n");

	dev->cdev = c->cdev;
	dev->function.name = "winusb";
	
	dev->function.strings = win_strings;
	
	dev->function.descriptors = fs_win_descs;
	dev->function.hs_descriptors = hs_win_descs;
	dev->function.bind = win_function_bind;
	dev->function.unbind = win_function_unbind;
	dev->function.set_alt = win_function_set_alt;
	dev->function.disable = win_function_disable;
	return usb_add_function(c, &dev->function);
}

static int win_setup(void)
{
	struct win_dev *dev;
	int ret;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	spin_lock_init(&dev->lock);

	init_waitqueue_head(&dev->read_wq);
	init_waitqueue_head(&dev->write_wq);

	atomic_set(&dev->open_excl, 0);
	atomic_set(&dev->read_excl, 0);
	atomic_set(&dev->write_excl, 0);

	/* config is disabled by default if win is present. */
	dev->close_notified = true;

	INIT_LIST_HEAD(&dev->tx_idle);

	_win_dev = dev;

	ret = misc_register(&win_device);
	if (ret)
		goto err;

	return 0;

err:
	kfree(dev);
	printk(KERN_ERR "win gadget driver failed to initialize\n");
	return ret;
}

static void win_cleanup(void)
{
	misc_deregister(&win_device);

	kfree(_win_dev);
	_win_dev = NULL;
}
