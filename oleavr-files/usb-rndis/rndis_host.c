/*
 * Host Side support for RNDIS Networking Links
 * Copyright (C) 2005 by David Brownell
 * Copyright (C) 2006 Ole Andre Vadla Ravnaas <oleavr@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define	DEBUG			// error path messages, extra info
#define	VERBOSE			// more; success messages

#include <linux/config.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/workqueue.h>
#include <linux/mii.h>
#include <linux/usb.h>
#include <linux/usb_cdc.h>

#include "ndis.h"
#include "usbnet.h"


#define DRIVER_REVISION		"r17"


static int use_status = 1;
module_param(use_status, int, 0);
MODULE_PARM_DESC(use_status, "Use the status endpoint");

static int send_keepalives = 1;
module_param(send_keepalives, int, 0);
MODULE_PARM_DESC(send_keepalives, "Send keepalives to make broken devices happy");


/*
 * RNDIS is NDIS remoted over USB.  It's a MSFT variant of CDC ACM ... of
 * course ACM was intended for modems, not Ethernet links!  USB's standard
 * for Ethernet links is "CDC Ethernet", which is significantly simpler.
 *
 * NOTE that Microsoft's "RNDIS 1.0" specification is incomplete.  Issues
 * include:
 *    - Power management in particular relies on information that's scattered
 *	through other documentation, and which is incomplete or incorrect even
 *	there.
 *    - There are various undocumented protocol requirements, such as the
 *	need to send unused garbage in control-OUT messages.
 *    - In some cases, MS-Windows will emit undocumented requests; this
 *	matters more to peripheral implementations than host ones.
 *
 * For these reasons and others, ** USE OF RNDIS IS STRONGLY DISCOURAGED ** in
 * favor of such non-proprietary alternatives as CDC Ethernet or the newer (and
 * currently rare) "Ethernet Emulation Model" (EEM).
 */

/*
 * CONTROL uses CDC "encapsulated commands" with funky notifications.
 *  - control-out:  SEND_ENCAPSULATED
 *  - interrupt-in:  RESPONSE_AVAILABLE
 *  - control-in:  GET_ENCAPSULATED
 */
struct rndis_msg_hdr {
	__le32	msg_type;			/* RNDIS_MSG_* */
	__le32	msg_len;
	// followed by data that varies between messages
	__le32	request_id;
	__le32	status;
	// ... and more
} __attribute__ ((packed));

#define CONTROL_BUFFER_SIZE		1025

/* RNDIS defines this (absurdly huge) control timeout */
#define	RNDIS_CONTROL_TIMEOUT_MS	(10 * 1000)


#define ccpu2 __constant_cpu_to_le32

#define RESPONSE_AVAILABLE	ccpu2(0x00000001)

#define RNDIS_MSG_COMPLETION	ccpu2(0x80000000)

/* codes for "msg_type" field of rndis messages;
 * only the data channel uses packet messages (maybe batched);
 * everything else goes on the control channel.
 */
#define RNDIS_MSG_PACKET	ccpu2(0x00000001)	/* 1-N packets */
#define RNDIS_MSG_INIT		ccpu2(0x00000002)
#define RNDIS_MSG_INIT_C	(RNDIS_MSG_INIT|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_HALT		ccpu2(0x00000003)
#define RNDIS_MSG_QUERY		ccpu2(0x00000004)
#define RNDIS_MSG_QUERY_C	(RNDIS_MSG_QUERY|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_SET		ccpu2(0x00000005)
#define RNDIS_MSG_SET_C		(RNDIS_MSG_SET|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_RESET		ccpu2(0x00000006)
#define RNDIS_MSG_RESET_C	(RNDIS_MSG_RESET|RNDIS_MSG_COMPLETION)
#define RNDIS_MSG_INDICATE	ccpu2(0x00000007)
#define RNDIS_MSG_KEEPALIVE	ccpu2(0x00000008)
#define RNDIS_MSG_KEEPALIVE_C	(RNDIS_MSG_KEEPALIVE|RNDIS_MSG_COMPLETION)

/* codes for "status" field of completion messages */
#define	RNDIS_STATUS_SUCCESS		ccpu2(0x00000000)
#define	RNDIS_STATUS_FAILURE		ccpu2(0xc0000001)
#define	RNDIS_STATUS_INVALID_DATA	ccpu2(0xc0010015)
#define	RNDIS_STATUS_NOT_SUPPORTED	ccpu2(0xc00000bb)
#define	RNDIS_STATUS_MEDIA_CONNECT	ccpu2(0x4001000b)
#define	RNDIS_STATUS_MEDIA_DISCONNECT	ccpu2(0x4001000c)


struct rndis_data_hdr {
	__le32	msg_type;		/* RNDIS_MSG_PACKET */
	__le32	msg_len;		// rndis_data_hdr + data_len + pad
	__le32	data_offset;		// 36 -- right after header
	__le32	data_len;		// ... real packet size

	__le32	oob_data_offset;	// zero
	__le32	oob_data_len;		// zero
	__le32	num_oob;		// zero
	__le32	packet_data_offset;	// zero

	__le32	packet_data_len;	// zero
	__le32	vc_handle;		// zero
	__le32	reserved;		// zero
} __attribute__ ((packed));

struct rndis_init {		/* OUT */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_INIT */
	__le32	msg_len;			// 24
	__le32	request_id;
	__le32	major_version;			// of rndis (1.0)
	__le32	minor_version;
	__le32	max_transfer_size;
} __attribute__ ((packed));

struct rndis_init_c {		/* IN */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_INIT_C */
	__le32	msg_len;
	__le32	request_id;
	__le32	status;
	__le32	major_version;			// of rndis (1.0)
	__le32	minor_version;
	__le32	device_flags;
	__le32	medium;				// zero == 802.3
	__le32	max_packets_per_message;
	__le32	max_transfer_size;
	__le32	packet_alignment;		// max 7; (1<<n) bytes
	__le32	af_list_offset;			// zero
	__le32	af_list_size;			// zero
} __attribute__ ((packed));

struct rndis_halt {		/* OUT (no reply) */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_HALT */
	__le32	msg_len;
	__le32	request_id;
} __attribute__ ((packed));

struct rndis_query {		/* OUT */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_QUERY */
	__le32	msg_len;
	__le32	request_id;
	__le32	oid;
	__le32	len;
	__le32	offset;
/*?*/	__le32	handle;				// zero
} __attribute__ ((packed));

struct rndis_query_c {		/* IN */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_QUERY_C */
	__le32	msg_len;
	__le32	request_id;
	__le32	status;
	__le32	len;
	__le32	offset;
} __attribute__ ((packed));

struct rndis_set {		/* OUT */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_SET */
	__le32	msg_len;
	__le32	request_id;
	__le32	oid;
	__le32	len;
	__le32	offset;
/*?*/	__le32	handle;				// zero
} __attribute__ ((packed));

struct rndis_set_c {		/* IN */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_SET_C */
	__le32	msg_len;
	__le32	request_id;
	__le32	status;
} __attribute__ ((packed));

struct rndis_reset {		/* IN */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_RESET */
	__le32	msg_len;
	__le32	reserved;
} __attribute__ ((packed));

struct rndis_reset_c {		/* OUT */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_RESET_C */
	__le32	msg_len;
	__le32	status;
	__le32	addressing_lost;
} __attribute__ ((packed));

struct rndis_indicate {		/* IN (unrequested) */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_INDICATE */
	__le32	msg_len;
	__le32	status;
	__le32	length;
	__le32	offset;
/**/	__le32	diag_status;
	__le32	error_offset;
/**/	__le32	message;
} __attribute__ ((packed));

struct rndis_keepalive {	/* OUT (optionally IN) */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_KEEPALIVE */
	__le32	msg_len;
	__le32	request_id;
} __attribute__ ((packed));

struct rndis_keepalive_c {	/* IN (optionally OUT) */
	// header and:
	__le32	msg_type;			/* RNDIS_MSG_KEEPALIVE_C */
	__le32	msg_len;
	__le32	request_id;
	__le32	status;
} __attribute__ ((packed));

/* internal state */
struct rndis_context {
	int			running;
	int			status_initialized;
	unsigned		packet_align;
	struct completion	compl;
	struct task_struct	*thread_task;
	struct completion	thread_compl;
};

/*
 * RNDIS notifications from device: command completion; "reverse"
 * keepalives; etc
 */
static void rndis_status(struct usbnet *dev, struct urb *urb)
{
	struct cdc_state	*info = (void *) &dev->data;
	struct rndis_context	*ctx = info->context;

	if (urb->actual_length == 8) {
		u32 response, reserved;

		response = *((u32 *) urb->transfer_buffer);
		reserved = *((u32 *) urb->transfer_buffer + 4);

		if (response == RESPONSE_AVAILABLE) {
			if (use_status)
				complete(&ctx->compl);
		} else {
			dev_dbg(&info->control->dev,
				"response = 0x%08x, reserved = 0x%08x\n",
				le32_to_cpu(response), le32_to_cpu(reserved));
		}
	} else {
		dev_dbg(&info->control->dev,
			"unexpected actual_length %d", urb->actual_length);
	}
}

/*
 * RPC done RNDIS-style.  Caller guarantees:
 * - message is properly byteswapped
 * - there's no other request pending
 * - buf can hold up to 1KB response (required by RNDIS spec)
 * On return, the first few entries are already byteswapped.
 *
 * Call context is likely probe(), before interface name is known,
 * which is why we won't try to use it in the diagnostics.
 */
static int rndis_command(struct usbnet *dev, struct rndis_msg_hdr *buf,
			 gfp_t flags)
{
	struct cdc_state	*info = (void *) &dev->data;
	struct rndis_context	*ctx = info->context;
	int			retval;
	unsigned		count;
	__le32			rsp;
	u32			xid = 0, msg_len, request_id;

	/* REVISIT when this gets called from contexts other than probe() or
	 * disconnect(): either serialize, or dispatch responses on xid
	 */
	
	/* Issue the request */
	if (likely(buf->msg_type != RNDIS_MSG_HALT
			&& buf->msg_type != RNDIS_MSG_RESET)) {
		xid = dev->xid++;
		if (!xid)
			xid = dev->xid++;
		buf->request_id = ccpu2(xid);
	}
	retval = usb_control_msg(dev->udev,
		usb_sndctrlpipe(dev->udev, 0),
		USB_CDC_SEND_ENCAPSULATED_COMMAND,
		USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		0, info->bMasterInterface0,
		buf, le32_to_cpu(buf->msg_len),
		RNDIS_CONTROL_TIMEOUT_MS);
	if (unlikely(retval < 0 || xid == 0))
		return retval;

	/* initialize status endpoint if not already done */
	if (unlikely(!ctx->status_initialized)) {
		retval = usbnet_init_status(dev, info->control, flags);
		if (unlikely(retval < 0))
			return retval;

		ctx->status_initialized = 1;

		/* poke status endpoint */
		memset(dev->interrupt->transfer_buffer, 0,
			dev->interrupt->transfer_buffer_length);
		retval = usb_submit_urb(dev->interrupt, flags);
		if (unlikely(retval != 0)) {
			dev_err(&info->control->dev, "failed to submit interrupt urb: %d\n", retval);
			return retval;
		}
	}

	if (use_status) {
		/* wait for RESPONSE_AVAILABLE notification */
		if (!wait_for_completion_timeout(&ctx->compl,
				msecs_to_jiffies(RNDIS_CONTROL_TIMEOUT_MS))) {
			dev_dbg(&info->control->dev, "rndis response timeout "
				"waiting for status ep\n");
			return -ETIMEDOUT;
		}
	}

#if 0
	/* do CLEAR_FEATURE like the Windows driver does */
	retval = usb_clear_halt(dev->udev, usb_pipeendpoint(usb_rcvctrlpipe(dev->udev, 0)));
	if (retval) {
		dev_err(&info->control->dev, "failed to perform CLEAR_FEATURE on ctrl ep\n");
		return retval;
	}
#endif

	/* get the response */
	rsp = buf->msg_type | RNDIS_MSG_COMPLETION;
	for (count = 0; count < 10; count++) {
		memset(buf, 0, CONTROL_BUFFER_SIZE);
		retval = usb_control_msg(dev->udev,
			usb_rcvctrlpipe(dev->udev, 0),
			USB_CDC_GET_ENCAPSULATED_RESPONSE,
			USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			0, info->bMasterInterface0,
			buf, CONTROL_BUFFER_SIZE,
			RNDIS_CONTROL_TIMEOUT_MS);
		if (likely(retval >= 8)) {
			msg_len = le32_to_cpu(buf->msg_len);
			request_id = le32_to_cpu(buf->request_id);
			if (likely(buf->msg_type == rsp)) {
				if (likely(request_id == xid)) {
					if (unlikely(rsp == RNDIS_MSG_RESET_C))
						return 0;
					if (likely(RNDIS_STATUS_SUCCESS
							== buf->status))
						return 0;
					dev_dbg(&info->control->dev,
						"rndis reply status %08x\n",
						le32_to_cpu(buf->status));
					return -EL3RST;
				}
				dev_dbg(&info->control->dev,
					"rndis reply id %d expected %d\n",
					request_id, xid);
				/* then likely retry */
			} else switch (buf->msg_type) {
			case RNDIS_MSG_INDICATE: {	/* fault */
				// struct rndis_indicate *msg = (void *)buf;
				dev_info(&info->control->dev,
					 "rndis fault indication\n");
				}
				break;
			case RNDIS_MSG_KEEPALIVE: {	/* ping */
				struct rndis_keepalive_c *msg = (void *)buf;

				msg->msg_type = RNDIS_MSG_KEEPALIVE_C;
				msg->msg_len = ccpu2(sizeof *msg);
				msg->status = RNDIS_STATUS_SUCCESS;
				retval = usb_control_msg(dev->udev,
					usb_sndctrlpipe(dev->udev, 0),
					USB_CDC_SEND_ENCAPSULATED_COMMAND,
					USB_TYPE_CLASS | USB_RECIP_INTERFACE,
					0, info->bMasterInterface0,
					msg, sizeof *msg,
					RNDIS_CONTROL_TIMEOUT_MS);
				if (unlikely(retval < 0))
					dev_dbg(&info->control->dev,
						"rndis keepalive err %d\n",
						retval);
				}
				break;
			default:
				dev_dbg(&info->control->dev,
					"unexpected rndis msg %08x len %d\n",
					le32_to_cpu(buf->msg_type), msg_len);
			}
		} else {
			/* device probably issued a protocol stall; ignore */
			dev_dbg(&info->control->dev,
				"rndis response error, code %d\n", retval);
		}
		msleep(2);
	}
	dev_dbg(&info->control->dev, "rndis response timeout\n");
	return -ETIMEDOUT;
}

static const char *oid_to_string(u32 oid)
{
	switch (oid) {
		case OID_GEN_SUPPORTED_LIST:
			return "OID_GEN_SUPPORTED_LIST";
		case OID_GEN_HARDWARE_STATUS:
			return "OID_GEN_HARDWARE_STATUS";
		case OID_GEN_MEDIA_SUPPORTED:
			return "OID_GEN_MEDIA_SUPPORTED";
		case OID_GEN_MEDIA_IN_USE:
			return "OID_GEN_MEDIA_IN_USE";
		case OID_GEN_MAXIMUM_LOOKAHEAD:
			return "OID_GEN_MAXIMUM_LOOKAHEAD";
		case OID_GEN_MAXIMUM_FRAME_SIZE:
			return "OID_GEN_MAXIMUM_FRAME_SIZE";
		case OID_GEN_LINK_SPEED:
			return "OID_GEN_LINK_SPEED";
		case OID_GEN_TRANSMIT_BUFFER_SPACE:
			return "OID_GEN_TRANSMIT_BUFFER_SPACE";
		case OID_GEN_RECEIVE_BUFFER_SPACE:
			return "OID_GEN_RECEIVE_BUFFER_SPACE";
		case OID_GEN_TRANSMIT_BLOCK_SIZE:
			return "OID_GEN_TRANSMIT_BLOCK_SIZE";
		case OID_GEN_RECEIVE_BLOCK_SIZE:
			return "OID_GEN_RECEIVE_BLOCK_SIZE";
		case OID_GEN_VENDOR_ID:
			return "OID_GEN_VENDOR_ID";
		case OID_GEN_VENDOR_DESCRIPTION:
			return "OID_GEN_VENDOR_DESCRIPTION";
		case OID_GEN_CURRENT_PACKET_FILTER:
			return "OID_GEN_CURRENT_PACKET_FILTER";
		case OID_GEN_CURRENT_LOOKAHEAD:
			return "OID_GEN_CURRENT_LOOKAHEAD";
		case OID_GEN_DRIVER_VERSION:
			return "OID_GEN_DRIVER_VERSION";
		case OID_GEN_MAXIMUM_TOTAL_SIZE:
			return "OID_GEN_MAXIMUM_TOTAL_SIZE";
		case OID_GEN_PROTOCOL_OPTIONS:
			return "OID_GEN_PROTOCOL_OPTIONS";
		case OID_GEN_MAC_OPTIONS:
			return "OID_GEN_MAC_OPTIONS";
		case OID_GEN_MEDIA_CONNECT_STATUS:
			return "OID_GEN_MEDIA_CONNECT_STATUS";
		case OID_GEN_MAXIMUM_SEND_PACKETS:
			return "OID_GEN_MAXIMUM_SEND_PACKETS";
		case OID_GEN_VENDOR_DRIVER_VERSION:
			return "OID_GEN_VENDOR_DRIVER_VERSION";
		case OID_GEN_SUPPORTED_GUIDS:
			return "OID_GEN_SUPPORTED_GUIDS";
		case OID_GEN_NETWORK_LAYER_ADDRESSES:
			return "OID_GEN_NETWORK_LAYER_ADDRESSES";
		case OID_GEN_TRANSPORT_HEADER_OFFSET:
			return "OID_GEN_TRANSPORT_HEADER_OFFSET";
		case OID_GEN_MACHINE_NAME:
			return "OID_GEN_MACHINE_NAME";
		case OID_GEN_RNDIS_CONFIG_PARAMETER:
			return "OID_GEN_RNDIS_CONFIG_PARAMETER";
		case OID_GEN_VLAN_ID:
			return "OID_GEN_VLAN_ID";
		case OID_GEN_MEDIA_CAPABILITIES:
			return "OID_GEN_MEDIA_CAPABILITIES";
		case OID_GEN_PHYSICAL_MEDIUM:
			return "OID_GEN_PHYSICAL_MEDIUM";
		case OID_GEN_XMIT_OK:
			return "OID_GEN_XMIT_OK";
		case OID_GEN_RCV_OK:
			return "OID_GEN_RCV_OK";
		case OID_GEN_XMIT_ERROR:
			return "OID_GEN_XMIT_ERROR";
		case OID_GEN_RCV_ERROR:
			return "OID_GEN_RCV_ERROR";
		case OID_GEN_RCV_NO_BUFFER:
			return "OID_GEN_RCV_NO_BUFFER";
		case OID_GEN_DIRECTED_BYTES_XMIT:
			return "OID_GEN_DIRECTED_BYTES_XMIT";
		case OID_GEN_DIRECTED_FRAMES_XMIT:
			return "OID_GEN_DIRECTED_FRAMES_XMIT";
		case OID_GEN_MULTICAST_BYTES_XMIT:
			return "OID_GEN_MULTICAST_BYTES_XMIT";
		case OID_GEN_MULTICAST_FRAMES_XMIT:
			return "OID_GEN_MULTICAST_FRAMES_XMIT";
		case OID_GEN_BROADCAST_BYTES_XMIT:
			return "OID_GEN_BROADCAST_BYTES_XMIT";
		case OID_GEN_BROADCAST_FRAMES_XMIT:
			return "OID_GEN_BROADCAST_FRAMES_XMIT";
		case OID_GEN_DIRECTED_BYTES_RCV:
			return "OID_GEN_DIRECTED_BYTES_RCV";
		case OID_GEN_DIRECTED_FRAMES_RCV:
			return "OID_GEN_DIRECTED_FRAMES_RCV";
		case OID_GEN_MULTICAST_BYTES_RCV:
			return "OID_GEN_MULTICAST_BYTES_RCV";
		case OID_GEN_MULTICAST_FRAMES_RCV:
			return "OID_GEN_MULTICAST_FRAMES_RCV";
		case OID_GEN_BROADCAST_BYTES_RCV:
			return "OID_GEN_BROADCAST_BYTES_RCV";
		case OID_GEN_BROADCAST_FRAMES_RCV:
			return "OID_GEN_BROADCAST_FRAMES_RCV";
		case OID_GEN_RCV_CRC_ERROR:
			return "OID_GEN_RCV_CRC_ERROR";
		case OID_GEN_TRANSMIT_QUEUE_LENGTH:
			return "OID_GEN_TRANSMIT_QUEUE_LENGTH";
		case OID_GEN_GET_TIME_CAPS:
			return "OID_GEN_GET_TIME_CAPS";
		case OID_GEN_GET_NETCARD_TIME:
			return "OID_GEN_GET_NETCARD_TIME";
		case OID_GEN_NETCARD_LOAD:
			return "OID_GEN_NETCARD_LOAD";
		case OID_GEN_DEVICE_PROFILE:
			return "OID_GEN_DEVICE_PROFILE";
		case OID_GEN_INIT_TIME_MS:
			return "OID_GEN_INIT_TIME_MS";
		case OID_GEN_RESET_COUNTS:
			return "OID_GEN_RESET_COUNTS";
		case OID_GEN_MEDIA_SENSE_COUNTS:
			return "OID_GEN_MEDIA_SENSE_COUNTS";
		case OID_GEN_FRIENDLY_NAME:
			return "OID_GEN_FRIENDLY_NAME";
		case OID_GEN_MINIPORT_INFO:
			return "OID_GEN_MINIPORT_INFO";
		case OID_GEN_RESET_VERIFY_PARAMETERS:
			return "OID_GEN_RESET_VERIFY_PARAMETERS";
		case NDIS_802_3_MAC_OPTION_PRIORITY:
			return "NDIS_802_3_MAC_OPTION_PRIORITY";
		case OID_802_3_PERMANENT_ADDRESS:
			return "OID_802_3_PERMANENT_ADDRESS";
		case OID_802_3_CURRENT_ADDRESS:
			return "OID_802_3_CURRENT_ADDRESS";
		case OID_802_3_MULTICAST_LIST:
			return "OID_802_3_MULTICAST_LIST";
		case OID_802_3_MAXIMUM_LIST_SIZE:
			return "OID_802_3_MAXIMUM_LIST_SIZE";
		case OID_802_3_MAC_OPTIONS:
			return "OID_802_3_MAC_OPTIONS";
		case OID_802_3_RCV_ERROR_ALIGNMENT:
			return "OID_802_3_RCV_ERROR_ALIGNMENT";
		case OID_802_3_XMIT_ONE_COLLISION:
			return "OID_802_3_XMIT_ONE_COLLISION";
		case OID_802_3_XMIT_MORE_COLLISIONS:
			return "OID_802_3_XMIT_MORE_COLLISIONS";
		case OID_802_3_XMIT_DEFERRED:
			return "OID_802_3_XMIT_DEFERRED";
		case OID_802_3_XMIT_MAX_COLLISIONS:
			return "OID_802_3_XMIT_MAX_COLLISIONS";
		case OID_802_3_RCV_OVERRUN:
			return "OID_802_3_RCV_OVERRUN";
		case OID_802_3_XMIT_UNDERRUN:
			return "OID_802_3_XMIT_UNDERRUN";
		case OID_802_3_XMIT_HEARTBEAT_FAILURE:
			return "OID_802_3_XMIT_HEARTBEAT_FAILURE";
		case OID_802_3_XMIT_TIMES_CRS_LOST:
			return "OID_802_3_XMIT_TIMES_CRS_LOST";
		case OID_802_3_XMIT_LATE_COLLISIONS:
			return "OID_802_3_XMIT_LATE_COLLISIONS";
		default:
			return NULL;
	}
}

/*
 * rndis_query:
 *
 * Performs a query for #oid along with 0 or more bytes of
 * payload as specified by #in_len. If #reply_len is not set
 * to -1 then the reply length is checked against this value,
 * resulting in an error if it doesn't match.
 *
 * NOTE: Adding a payload of n bytes seems to be the way to tell
 * the device the maximum number of bytes expected back, and is
 * something added by MSFT as of post-RNDIS 1.0.  This is
 * completely undocumented and was discovered through reverse
 * engineering the driver supplied with ActiveSync 4.1. It seems
 * to expose the fact that it is built atop some undocumented subset
 * of RNDIS.
 */
static int rndis_query(struct usbnet *dev, struct usb_interface *intf,
		       void *buf, u32 oid, u32 in_len,
		       void **reply, int *reply_len)
{
	int retval;
	union {
		void			*buf;
		struct rndis_msg_hdr	*header;
		struct rndis_query	*get;
		struct rndis_query_c	*get_c;
	} u;
	u32 off, len;
	const char *oid_str;

	u.buf = buf;

	memset(u.get, 0, sizeof *u.get + in_len);
	u.get->msg_type = RNDIS_MSG_QUERY;
	u.get->msg_len = ccpu2(sizeof *u.get + in_len);
	u.get->oid = ccpu2(oid);
	u.get->len = ccpu2(in_len);
	u.get->offset = ccpu2(20);
	
	retval = rndis_command(dev, u.header, GFP_KERNEL);
	if (unlikely(retval < 0)) {
		const char *oid_str = oid_to_string(oid);

		if (oid_str) {
			dev_err(&intf->dev, "RNDIS_MSG_QUERY(%s) failed, %d\n",
					oid_str, retval);
		} else {
			dev_err(&intf->dev, "RNDIS_MSG_QUERY(0x%08x) failed, %d\n",
					oid, retval);
		}

		return retval;
	}
	
	off = le32_to_cpu(u.get_c->offset);
	len = le32_to_cpu(u.get_c->len);
	if (unlikely((8 + off + len) > CONTROL_BUFFER_SIZE))
		goto response_error;

	if (*reply_len != -1 && len != *reply_len)
		goto response_error;

	*reply = (unsigned char *) &u.get_c->request_id + off;
	*reply_len = len;

	return retval;

response_error:
	oid_str = oid_to_string(oid);

	if (oid_str) {
		dev_err(&intf->dev, "RNDIS_MSG_QUERY(%s) failed because of an "
				"invalid response - off %d len %d\n",
			oid_str, off, len);
	} else {
		dev_err(&intf->dev, "RNDIS_MSG_QUERY(0x%08x) failed because of an "
				"invalid response - off %d len %d\n",
			oid, off, len);
	}
	
	return -EDOM;
}

#define MAC_OPTIONS_CHECK_FLAG(flag) \
	if ((tmp & flag) != 0) { \
		dev_dbg(&intf->dev, "  " #flag "\n"); \
		tmp ^= flag; \
	}

static int keepalive_thread(void *data)
{
	struct usbnet		*dev = data;
	struct cdc_state	*info = (void *) &dev->data;
	struct rndis_context	*ctx = info->context;
	union {
		void			*buf;
		struct rndis_msg_hdr	*header;
		struct rndis_query	*get;
		struct rndis_query_c	*get_c;
	} u;
	int			reply_len, retval;
	u32			*dwp;

	dev_dbg(&info->control->dev, "keepalive thread starting\n");

	u.buf = kmalloc(CONTROL_BUFFER_SIZE, GFP_KERNEL);
	if (!u.buf)
		goto out;

	while (ctx->running) {
		if (msleep_interruptible(187) != 0)
			break;

		/* Get link speed as keepalive ... */
		reply_len = 4;
		retval = rndis_query(dev, info->control, u.buf, OID_GEN_LINK_SPEED, 4,
				(void **) &dwp, &reply_len);
		if (retval < 0)
			break;
	}

	kfree(u.buf);

out:
	dev_dbg(&info->control->dev, "keepalive thread exiting\n");
	complete_and_exit(&ctx->thread_compl, 0);
}

static int rndis_bind(struct usbnet *dev, struct usb_interface *intf)
{
	int			retval, i;
	struct cdc_state	*info = (void *) &dev->data;
	struct rndis_context	*ctx;
	struct net_device	*net = dev->net;
	union {
		void			*buf;
		struct rndis_msg_hdr	*header;
		struct rndis_init	*init;
		struct rndis_init_c	*init_c;
		struct rndis_query	*get;
		struct rndis_query_c	*get_c;
		struct rndis_set	*set;
		struct rndis_set_c	*set_c;
	} u;
	u32			tmp, *dwp;
	int			reply_len;
	unsigned char		*bp;

	dev_dbg(&intf->dev, "rndis_host WM5 " DRIVER_REVISION "\n");
	dev_dbg(&intf->dev, "use_status = %d, send_keepalives = %d\n",
		use_status, send_keepalives);

	/* we can't rely on i/o from stack working, or stack allocation */
	u.buf = kmalloc(CONTROL_BUFFER_SIZE, GFP_KERNEL);
	if (!u.buf)
		return -ENOMEM;

	/* claim endpoints and such magic */
	retval = usbnet_generic_cdc_bind(dev, intf);
	if (retval < 0)
		goto done;
	
	/* allocate a per-device context */
	ctx = kmalloc(sizeof *ctx, GFP_KERNEL);
	if (!ctx) {
		retval = -ENOMEM;
		goto fail;
	}
	ctx->running = 1;
	ctx->status_initialized = 0;
	init_completion(&ctx->compl);
	init_completion(&ctx->thread_compl);
	info->context = ctx;

	net->hard_header_len += sizeof (struct rndis_data_hdr);
	
	dev_dbg(&intf->dev, "sending RNDIS_MSG_INIT\n");

	/* initialize; max transfer is 16KB at full speed */
	u.init->msg_type = RNDIS_MSG_INIT;
	u.init->msg_len = ccpu2(sizeof *u.init);
	u.init->major_version = ccpu2(1);
	u.init->minor_version = ccpu2(0);
	u.init->max_transfer_size = ccpu2(16384); /*ccpu2(net->mtu + net->hard_header_len);*/

	retval = rndis_command(dev, u.header, GFP_KERNEL);
	if (unlikely(retval < 0)) {
		/* it might not even be an RNDIS device!! */
		dev_err(&intf->dev, "RNDIS init failed, %d\n", retval);
fail:
		if (ctx)
			kfree(ctx);

		usbnet_cdc_unbind(dev, intf);

		goto done;
	}

	dev_dbg(&intf->dev, "REMOTE_NDIS_INITIALIZE_MSG reply:\n"
		"  Status = %d\n"
		"  MajorVersion = %d\n"
		"  MinorVersion = %d\n"
		"  DeviceFlags = %d\n"
		"  Medium = %d\n"
		"  MaxPacketsPerTransfer = %d\n"
		"  MaxTransferSize = %d\n"
		"  PacketAlignmentFactor = %d\n"
		"  AFListOffset = %d\n"
		"  AFListSize = %d\n",
		u.init_c->status, u.init_c->major_version,
		u.init_c->minor_version, u.init_c->device_flags,
		u.init_c->medium, u.init_c->max_packets_per_message,
		u.init_c->max_transfer_size, u.init_c->packet_alignment,
		u.init_c->af_list_offset, u.init_c->af_list_size);

	dev->hard_mtu = le32_to_cpu(u.init_c->max_transfer_size);
	ctx->packet_align = 1 << le32_to_cpu(u.init_c->packet_alignment);
	
	dev_dbg(&intf->dev, "hard mtu %u, align %d\n", dev->hard_mtu,
		1 << le32_to_cpu(u.init_c->packet_alignment));

	/* Get list of supported OIDs. */
	reply_len = -1;
	retval = rndis_query(dev, intf, u.buf, OID_GEN_SUPPORTED_LIST, 0,
			(void **) &dwp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;
	
	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_GEN_SUPPORTED_LIST) reply: %d OIDs\n",
			reply_len / 4);

	for (i = 0; i < reply_len / 4; i++) {
		u32 oid = le32_to_cpu(*dwp);
		const char *oid_str;

		oid_str = oid_to_string(oid);
		if (oid_str) {
			dev_dbg(&intf->dev, "  %s\n", oid_str);
		} else {
			dev_dbg(&intf->dev, "  0x%08x\n", oid);
		}

		dwp++;
	}

	/* Get vendor driver version. */
	reply_len = 2;
	retval = rndis_query(dev, intf, u.buf, OID_GEN_VENDOR_DRIVER_VERSION, 4,
			(void **) &bp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;

	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_GEN_VENDOR_DRIVER_VERSION) "
			"reply: %d.%d\n", bp[1], bp[0]);

	/* Get maximum lookahead. */
	reply_len = 4;
	retval = rndis_query(dev, intf, u.buf, OID_GEN_MAXIMUM_LOOKAHEAD, 4,
			(void **) &dwp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;

	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_GEN_MAXIMUM_LOOKAHEAD) "
			"reply: %d\n", le32_to_cpu(*dwp));

	/* Get MAC options. */
	reply_len = 4;
	retval = rndis_query(dev, intf, u.buf, OID_GEN_MAC_OPTIONS, 4,
			(void **) &dwp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;

	tmp = le32_to_cpu(*dwp);
	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_GEN_MAC_OPTIONS) reply:\n");

	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_COPY_LOOKAHEAD_DATA);
	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_RECEIVE_SERIALIZED);
	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_TRANSFERS_NOT_PEND);
	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_NO_LOOPBACK);
	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_FULL_DUPLEX);
	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_EOTX_INDICATION);
	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_8021P_PRIORITY);
	MAC_OPTIONS_CHECK_FLAG(NDIS_MAC_OPTION_RESERVED);
	if (tmp != 0) {
		dev_dbg(&intf->dev, "  ..unknown flag(s) left: 0x%08x\n", tmp);
	}

	/* Get maximum send packet descriptors. */
	reply_len = 4;
	retval = rndis_query(dev, intf, u.buf, OID_GEN_MAXIMUM_SEND_PACKETS, 4,
			(void **) &dwp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;
	
	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_GEN_MAXIMUM_SEND_PACKETS) "
			"reply: %d\n", le32_to_cpu(*dwp));
	
	/* Get maximum number of 802.3 multicast addresses. */
	reply_len = 4;
	retval = rndis_query(dev, intf, u.buf, OID_802_3_MAXIMUM_LIST_SIZE, 4,
			(void **) &dwp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;

	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_802_3_MAXIMUM_LIST_SIZE) "
			"reply: %d\n", le32_to_cpu(*dwp));

	/* Get current MAC address. */
	reply_len = ETH_ALEN;
	retval = rndis_query(dev, intf, u.buf, OID_802_3_CURRENT_ADDRESS, ETH_ALEN,
			(void **) &bp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;

	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_802_3_CURRENT_ADDRESS) "
			"reply: %02x:%02x:%02x:%02x:%02x:%02x\n",
			bp[0], bp[1], bp[2], bp[3], bp[4], bp[5]);
	
	/* Get maximum frame size. */
	reply_len = 4;
	retval = rndis_query(dev, intf, u.buf, OID_GEN_MAXIMUM_FRAME_SIZE, 4,
			(void **) &dwp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;

	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_GEN_MAXIMUM_FRAME_SIZE) "
			"reply: %d\n", le32_to_cpu(*dwp));
	
	/* Get link speed. */
	reply_len = 4;
	retval = rndis_query(dev, intf, u.buf, OID_GEN_LINK_SPEED, 4,
			(void **) &dwp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;

	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_GEN_LINK_SPEED) reply: %d Mbps\n",
			le32_to_cpu(*dwp) / 10000);

	/* Set current lookahead. */
	memset(u.set, 0, sizeof *u.set);
	u.set->msg_type = RNDIS_MSG_SET;
	u.set->msg_len = ccpu2(sizeof *u.set + 4);
	u.set->oid = ccpu2(OID_GEN_CURRENT_LOOKAHEAD);
	u.set->len = ccpu2(4);
	u.set->offset = ccpu2((sizeof *u.set) - 8);
	*(__le32 *)(u.buf + sizeof *u.set) = ccpu2(128);

	retval = rndis_command(dev, u.header, GFP_KERNEL);
	if (unlikely(retval < 0)) {
		dev_err(&intf->dev, "RNDIS_MSG_SET(OID_GEN_CURRENT_LOOKAHEAD = 128) "
				"failed: %d\n", retval);
		goto fail;
	}
	
	dev_dbg(&intf->dev, "RNDIS_MSG_SET(OID_GEN_CURRENT_LOOKAHEAD = 128) succeeded\n");

	/* Get designated host ethernet address. */
	reply_len = ETH_ALEN;
	retval = rndis_query(dev, intf, u.buf, OID_802_3_PERMANENT_ADDRESS, 48,
			(void **) &bp, &reply_len);
	if (unlikely(retval < 0))
		goto fail;
	
	dev_dbg(&intf->dev, "RNDIS_MSG_QUERY(OID_802_3_PERMANENT_ADDRESS) "
			"reply: %02x:%02x:%02x:%02x:%02x:%02x\n",
			bp[0], bp[1], bp[2], bp[3], bp[4], bp[5]);

	memcpy(net->dev_addr, bp, ETH_ALEN);

	/* set a nonzero filter to enable data transfers */
	dev_dbg(&intf->dev, "setting packet filter\n");
	
	memset(u.set, 0, sizeof *u.set);
	u.set->msg_type = RNDIS_MSG_SET;
	u.set->msg_len = ccpu2(4 + sizeof *u.set);
	u.set->oid = ccpu2(OID_GEN_CURRENT_PACKET_FILTER);
	u.set->len = ccpu2(4);
	u.set->offset = ccpu2((sizeof *u.set) - 8);
	*(__le32 *)(u.buf + sizeof *u.set) = ccpu2(
			NDIS_PACKET_TYPE_DIRECTED |
			NDIS_PACKET_TYPE_MULTICAST |
			NDIS_PACKET_TYPE_BROADCAST);

	retval = rndis_command(dev, u.header, GFP_KERNEL);
	if (unlikely(retval < 0)) {
		dev_err(&intf->dev, "rndis set packet filter, %d\n", retval);
		goto fail;
	}

	if (send_keepalives) {
		/* Start keepalive thread */
		ctx->thread_task = kthread_create(keepalive_thread, dev,
						  "rndis_host");
		if (IS_ERR(ctx->thread_task)) {
			dev_err(&intf->dev, "kthread_create failed\n");
			retval = PTR_ERR(ctx->thread_task);
			goto fail;
		}

		wake_up_process(ctx->thread_task);
	}
	
	dev_dbg(&intf->dev, "initialization completed successfully\n");

	retval = 0;
done:
	kfree(u.buf);
	return retval;
}

static void rndis_unbind(struct usbnet *dev, struct usb_interface *intf)
{
	struct rndis_halt	*halt;
	struct cdc_state	*info = (void *) &dev->data;
	struct rndis_context	*ctx = info->context;

	ctx->running = 0;

	if (send_keepalives) {
		/* wait for the keepalive thread to exit */
		wait_for_completion(&ctx->thread_compl);
	}
	
	/* try to clear any rndis state/activity (no i/o from stack!) */
	halt = kcalloc(1, sizeof *halt, SLAB_KERNEL);
	if (halt) {
		halt->msg_type = RNDIS_MSG_HALT;
		halt->msg_len = ccpu2(sizeof *halt);
		(void) rndis_command(dev, (void *)halt, SLAB_KERNEL);
		kfree(halt);
	}

	/* free the per-device context */
	kfree(ctx);

	return usbnet_cdc_unbind(dev, intf);
}

/*
 * DATA -- host must not write zlps
 */
static int rndis_rx_fixup(struct usbnet *dev, struct sk_buff *skb)
{
	/* peripheral may have batched packets to us... */
	while (likely(skb->len)) {
		struct rndis_data_hdr	*hdr = (void *)skb->data;
		struct sk_buff		*skb2;
		u32			msg_len, data_offset, data_len;

		msg_len = le32_to_cpu(hdr->msg_len);
		data_offset = le32_to_cpu(hdr->data_offset);
		data_len = le32_to_cpu(hdr->data_len);

		if (le32_to_cpu(hdr->oob_data_len) > 0) {
			devdbg(dev, "wow, got some OOB data\n");
		}

		/* don't choke if we see oob, per-packet data, etc */
		if (unlikely(hdr->msg_type != RNDIS_MSG_PACKET
			|| (data_offset + data_len + 8) > msg_len)) {
			dev->stats.rx_frame_errors++;
			devdbg(dev, "bad rndis message 0x%08x/%d/%d/%d, len %d",
				le32_to_cpu(hdr->msg_type),
				msg_len, data_offset, data_len, skb->len);
			return 0;
		}
		
		if (skb->len < msg_len) {
			/* need more data ... */
			return -1;
		}

		skb_pull(skb, 8 + data_offset);

		/* at most one packet left? */
		if (likely((data_len - skb->len) <= sizeof *hdr)) {
			skb_trim(skb, data_len);
			break;
		}

		/* try to return all the packets in the batch */
		skb2 = skb_clone(skb, GFP_ATOMIC);
		if (unlikely(!skb2))
			break;
		skb_trim(skb2, data_len);
		usbnet_skb_return(dev, skb2);

		skb_pull(skb, msg_len - sizeof *hdr);
	}

	/* caller will usbnet_skb_return the remaining packet */
	return 1;
}

static struct sk_buff *
rndis_tx_fixup(struct usbnet *dev, struct sk_buff *skb, gfp_t flags)
{
	struct cdc_state	*info = (void *) &dev->data;
	struct rndis_context	*ctx = info->context;
	struct rndis_data_hdr	*hdr;
	struct sk_buff		*skb2;
	unsigned		len = skb->len;
	unsigned		len_with_hdr = (sizeof *hdr) + len;
	unsigned		padding;

	padding = (len_with_hdr % ctx->packet_align == 0) ? 0 :
		ctx->packet_align - (len_with_hdr % ctx->packet_align);
	
	/* create a new skb, with the correct size and padding. */
	skb2 = skb_copy_expand(skb, sizeof *hdr, padding + 1, flags);
	dev_kfree_skb_any(skb);
	if (unlikely(!skb2))
		return skb2;
	skb = skb2;

	/* fill out the RNDIS header.  we won't bother trying to batch
	 * packets; Linux minimizes wasted bandwidth through tx queues.
	 */

	hdr = (void *) __skb_push(skb, sizeof *hdr);
	memset(hdr, 0, sizeof *hdr);
	hdr->msg_type = RNDIS_MSG_PACKET;
	hdr->msg_len = cpu_to_le32(skb->len);
	hdr->data_offset = ccpu2(sizeof(*hdr) - 8);
	hdr->data_len = cpu_to_le32(len);

	__skb_put(skb, padding);

	/* FIXME make the last packet always be short ... */
	return skb;
}

static const struct driver_info	rndis_info = {
	.description =	"RNDIS device",
	.flags =	FLAG_ETHER | FLAG_FRAMING_RN,
	.bind =		rndis_bind,
	.unbind =	rndis_unbind,
	.status =	rndis_status,
	.rx_fixup =	rndis_rx_fixup,
	.tx_fixup =	rndis_tx_fixup,
};

#undef ccpu2


/*-------------------------------------------------------------------------*/

#define WM5_SUB_CLASS 0x01
#define WM5_PROTOCOL  0x01

static const struct usb_device_id	products [] = {
{
	/* RNDIS is MSFT's un-official variant of CDC ACM */
	USB_INTERFACE_INFO(USB_CLASS_COMM, 2 /* ACM */, 0x0ff),
        USB_INTERFACE_INFO(USB_CLASS_MISC, WM5_SUB_CLASS, WM5_PROTOCOL),
	.driver_info = (unsigned long) &rndis_info,
},
	{ },		// END
};
MODULE_DEVICE_TABLE(usb, products);

static struct usb_driver rndis_driver = {
	.name =		"rndis_host",
	.id_table =	products,
	.probe =	usbnet_probe,
	.disconnect =	usbnet_disconnect,
	.suspend =	usbnet_suspend,
	.resume =	usbnet_resume,
};

static int __init rndis_init(void)
{
	return usb_register(&rndis_driver);
}
module_init(rndis_init);

static void __exit rndis_exit(void)
{
	usb_deregister(&rndis_driver);
}
module_exit(rndis_exit);

MODULE_AUTHOR("David Brownell");
MODULE_DESCRIPTION("USB Host side RNDIS driver");
MODULE_LICENSE("GPL");
