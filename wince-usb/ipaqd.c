/*
 *   ipaqd.c	A Linux userspace driver for the Compaq iPAQ running Win CE 3.0
 *  
 *   version: 0.1.0
 *
 *   Copyright (C) 2001	Ganesh Varadarajan <vganesh@users.sourceforge.net>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <termios.h>
#include <linux/usbdevice_fs.h>
#include <usb.h>
#include "usbi.h"

#define IPAQ_VENDOR_ID		0x49f
#define IPAQ_PRODUCT_ID		0x3
#define IPAQ_ENDPOINT_IN	0x82
#define IPAQ_ENDPOINT_OUT	0x1

#define PPPSCRIPT		"/usr/local/bin/cebox.sh"

#define URBRECVSIZE	(12 * 1024)	/* max chunk we will receive */
#define URBSENDSIZE	(12 * 1024)	/* max size ppp packet we expect */
#define CBSIZE		4096		/* chat buffer size */

#define DEFAULT_DELAY	0		/* delay seconds before sending first
					   bulk urb */
#define MSG_ERR		0
#define MSG_INFO	1
#define MSG_DUMP	2


#define msg(level, fmt, ...)						\
{									\
	int	error;							\
									\
	error = errno;							\
	if (level <= msglevel) {					\
		fprintf(stderr, __FUNCTION__ ":%d: " fmt "\n", __LINE__ , ##__VA_ARGS__);    \
		if (level == MSG_ERR && error) {			\
			fprintf(stderr, __FUNCTION__ ": %s\n",		\
				strerror(error));			\
			errno = 0;					\
		}							\
	}								\
}

pid_t forkpty(int *, char *, struct termios *, struct winsize *);

static usb_dev_handle	*bus_scan(unsigned int, unsigned int);
static void		do_stuff(void);
static void		cleanup(int);
static void		urb_dump(struct usbdevfs_urb *uurb);
static void		bulk_out_callback(struct usbdevfs_urb *uurb);

/*
 * stolen from kernel sources, because it's hidden under a #ifdef __KERNEL__
 */

typedef struct {
        __u8 requesttype;
        __u8 request;
        __u16 value;
        __u16 index;
        __u16 length;
} devrequest __attribute__ ((packed));

/*
 * we maintain two lists to accumulate incoming and outgoing "packets".
 */

struct packet {
	void		*data;
	size_t		len;
	size_t		done;
	struct packet	*next;
	struct packet	*prev;
};

struct packet	head_in = {NULL, 0, 0, &head_in, &head_in};
struct packet	head_out = {NULL, 0, 0, &head_out, &head_out};

/*
 * yes I use globals. sue me.
 */

int		ipaqfd;				/* fd for /proc/bus.. interface */
int		pppfd;				/* master side of pty */
usb_dev_handle	*ipaqdev;			/* libusb handle */
int		errcount_in = 0;
int		sendinprogress = 0;		/* used to serialize bulk sends */
int		opt_delay = DEFAULT_DELAY;	/* seconds to sleep before initiating
						   first bulk receive */
int		msglevel = MSG_DUMP;		/* debug level */

int
main(int argc, char **argv)
{
	pid_t	pid;
	int	on = 1;
	int	c;

	while ((c = getopt(argc, argv, "d:s:")) != -1) {
		switch (c) {
			case 's':
				opt_delay = atoi(optarg);
				break;
			case 'd':
				msglevel = atoi(optarg);
				break;
			default:
				msg(MSG_ERR, "Usage: %s [-s delay] "
				    "[-d debuglevel]\n", argv[0]);
				exit(1);
				break;
		}
	}

	usb_set_debug(4);
	usb_init();
	usb_find_busses();
	usb_find_devices();

	ipaqdev = bus_scan(IPAQ_VENDOR_ID, IPAQ_PRODUCT_ID);

	if (ipaqdev == NULL) {
		msg(MSG_ERR, "ERROR:Couldn't locate device\n");
		exit(1);
	}

	/*
	 * the config has already been set to 1 by the kernel, so
	 * all we need to do is grab the device.
	 */

	usb_claim_interface(ipaqdev, 0);
	ipaqfd = ipaqdev->fd;

	pid = forkpty(&pppfd, NULL, NULL, NULL);
	if (pid < 0) {
		if (errno == ENOENT) {
			msg(MSG_ERR, "ERROR:out of ptys\n");
			exit(1);
		}
		msg(MSG_ERR, "forkpty\n");
		exit(1);
	}
	if (pid == 0) {
		if (ipaqfd > 2) {
			close(ipaqfd);
		}
		msg(MSG_INFO, "execing %s\n", PPPSCRIPT);
		execl(PPPSCRIPT, PPPSCRIPT, NULL);
		exit(2);
	}
	signal(SIGCHLD, cleanup);
	ioctl(pppfd, FIONBIO, &on);
	do_stuff();
	cleanup(0);
}

static usb_dev_handle	*
bus_scan(unsigned int vendor, unsigned int product)
{
	struct usb_bus		*bus;
	struct usb_device	*device;
	usb_dev_handle		*result;

	for (bus = usb_busses; bus; bus = bus->next) {
		for (device = bus->devices; device; device = device->next) {
			if (device->descriptor.idVendor == vendor &&
			    device->descriptor.idProduct == product) {
				result = usb_open(device);
				return result;
			}
		}
	}
	return NULL;
}

/*
 * function used to submit all kinds of urbs.
 */

static void
urb_submit(struct usbdevfs_urb *uurb, unsigned char type, unsigned char endpoint,
	   caddr_t data, size_t size, void (*cb)(struct usbdevfs_urb *))
{
	msg(MSG_INFO, "sending down urb %p type %x endpoint %x\n", uurb, type, endpoint);
	uurb->type = type;
	uurb->endpoint = endpoint;
	uurb->buffer = data;
	uurb->buffer_length = size;
	uurb->usercontext = (void *)cb;
	if (msglevel >= MSG_DUMP) {
		urb_dump(uurb);
	}
	if (ioctl(ipaqfd, USBDEVFS_SUBMITURB, uurb) < 0) {
		msg(MSG_ERR, "ERROR:urb_submit");
		if (data) {
			free(data);
		}
		free(uurb);
	}
	return;
}

/*
 * submit data to be sent to the ipaq. if there is no in-flight packet, we
 * will send it immediately, otherwise it will be queued. when the in-flight
 * urb is reaped, its callback processing will take the first item off the
 * queue and send it.
 */

static void
bulk_urb_out(caddr_t data, size_t size)
{
	struct usbdevfs_urb	*uurb;
	struct packet		*entry;

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	if (!sendinprogress) {
		sendinprogress = 1;
		uurb = calloc(sizeof(struct usbdevfs_urb), 1);
		urb_submit(uurb, USBDEVFS_URB_TYPE_BULK, IPAQ_ENDPOINT_OUT,
			   data, size, bulk_out_callback);
		return;
	}

	msg(MSG_INFO, "%p added to list\n", data);
	entry = malloc(sizeof(struct packet));
	entry->data = data;
	entry->len = size;
	entry->done = 0;
	
	entry->prev = head_out.prev;
	head_out.prev->next = entry;
	head_out.prev = entry;
	entry->next = &head_out;

	return;
}

static void
hexdump(caddr_t data, size_t len)
{
	int i;

	fprintf (stderr, "dump of %p, %d bytes", data, len);
	for (i = 0; i < len; i++) {
		if (i % 16 == 0) {
			fprintf(stderr, "\n%02x: ", i / 16);
		}
		fprintf(stderr, "%02x ", (unsigned int)*((unsigned char *)data + i));
	}
	fprintf(stderr, "\n\n");
}

static void
urb_dump(struct usbdevfs_urb *uurb)
{
	int	len;

	fprintf(stderr, "uurb %p: type %x endpoint %x status %x flags %x buffer %x "
		"bufferlength %x actuallen %x startframe %x numpackets %x error_count %x "
		"signr %x usercontext %x\n", uurb, uurb->type, uurb->endpoint, uurb->status,
		uurb->flags, uurb->buffer, uurb->buffer_length, uurb->actual_length,
		uurb->start_frame, uurb->number_of_packets, uurb->error_count, uurb->signr,
		uurb->usercontext);
	if (uurb->type == USBDEVFS_URB_TYPE_BULK && uurb->endpoint == IPAQ_ENDPOINT_OUT) {
		len = uurb->buffer_length;
	} else {
		len = uurb->actual_length;
	}
	if (len == 0) {
		return;
	}
	hexdump(uurb->buffer, len);
}

/*
 * reap completed urbs, if any. non-blocking. call callback functions.
 */

static void
reapurb()
{
	struct usbdevfs_urb	*uurb;
	void			(*callback)(struct usbdevfs_urb *);

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	while (1) {
		if (ioctl(ipaqfd, USBDEVFS_REAPURBNDELAY, &uurb) < 0) {
			if (errno != EAGAIN) {
				msg(MSG_ERR, "ERROR:reapurb");
			}
			return;
		}
		callback = (void (*)(struct usbdevfs_urb *))uurb->usercontext;
		if (callback) {
			callback(uurb);
		} else if (msglevel >= MSG_INFO) {
			urb_dump(uurb);
		}
	}
}

/*
 * blocking version of above. will reap only 1 urb.
 */

static void
reapurb_block()
{
	struct usbdevfs_urb	*uurb;
	void			(*callback)(struct usbdevfs_urb *);

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	if (ioctl(ipaqfd, USBDEVFS_REAPURB, &uurb) < 0) {
		msg(MSG_ERR, "ERROR:reapurb_block");
		cleanup(0);
	}
	
	callback = (void (*)(struct usbdevfs_urb *))uurb->usercontext;
	if (callback) {
		callback(uurb);
	} else {
		urb_dump(uurb);
	}
}

/*
 * goodbye.
 */

static void
cleanup(int sig)
{
	if (sig) {
		fprintf(stderr, "terminating at signal %d\n", sig);
	}
	usb_release_interface(ipaqdev, 0);
	usb_close(ipaqdev);

	exit(0);
}

/*
 * callback for bulk out data. we check if there's any pending data from
 * pppd in the queue and send down 1.
 * NOTE: usb-uhci has some trouble delivering the first ppp (lcp) packet.
 * often fails with EPIPE. uhci seems to work ok.
 */

static void
bulk_out_callback(struct usbdevfs_urb *uurb)
{
	struct packet	*entry;

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	sendinprogress = 0;
	if (uurb->status != 0) {
		msg(MSG_ERR, "ERROR:bulk send of %p failed with %d\n",
			uurb, uurb->status);
		if (msglevel >= MSG_DUMP) {
			urb_dump(uurb);
		}
		if (uurb->status == -EPIPE) {
			cleanup(0);
		}
	}
	free(uurb->buffer);

	if (head_out.next != &head_out) {
		sendinprogress = 1;
		memset(uurb, 0, sizeof(struct usbdevfs_urb));
		entry = head_out.next;
		entry->prev->next = entry->next;
		entry->next->prev = entry->prev;
		msg(MSG_INFO, "sending %p as %p\n", entry->data, uurb);
		urb_submit(uurb, USBDEVFS_URB_TYPE_BULK, IPAQ_ENDPOINT_OUT, entry->data,
			   entry->len, bulk_out_callback);
		free(entry);
	} else {
		free(uurb);
	}

	return;
}

/*
 * callback for bulk data coming in from the ipaq. queue it up, issue another
 * bulk read.
 */

static void
bulk_in_callback(struct usbdevfs_urb *uurb)
{
	struct packet	*entry;
	caddr_t		buffer;

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	if (uurb->status != 0) {
		fprintf(stderr, "ERROR:bulk receive failed %d\n", uurb->status);
		if (uurb->status == -EILSEQ) {
			cleanup(0);
		}
		return;
	}

	if (msglevel >= MSG_DUMP) {
		urb_dump(uurb);
	}
	entry = (struct packet *)malloc(sizeof(struct packet));
	entry->data = uurb->buffer;
	entry->len = uurb->actual_length;
	entry->done = 0;

	entry->prev = head_in.prev;
	head_in.prev->next = entry;
	head_in.prev = entry;
	entry->next = &head_in;
	head_in.len += entry->len;

	buffer = calloc(URBRECVSIZE, 1);
	memset(uurb, 0, sizeof(struct usbdevfs_urb));
	urb_submit(uurb, USBDEVFS_URB_TYPE_BULK, IPAQ_ENDPOINT_IN, buffer, URBRECVSIZE,
		   bulk_in_callback);
}

/*
 * some vague packet which the windows host driver seems to send. don't know much
 * about it except two of them are sent one after the other, immediately after the
 * first bulk read is posted. removing these seems to cause errors. snoopy calls it an
 * URB_FUNCTION_CLASS_INTERFACE. looks like a DT_REPORT. anyway. ours not to reason
 * why, as long as it works.
 */

static void
hack_callback(struct usbdevfs_urb *uurb)
{
	int		status;
	static int	donethis = 1;
	devrequest	*dr;

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	status = uurb->status;
	if (uurb->buffer) {
		free(uurb->buffer);
	}
	free(uurb);
	if (status != 0) {
		msg(MSG_ERR, "ERROR:hack packet %p failed with %d\n", uurb, status);
	}
	if (donethis > 1) {
		return;
	}

	/*
	 * do this packet once more. that's it.
	 */

	donethis++;
	uurb = calloc(sizeof(struct usbdevfs_urb), 1);
	dr = calloc(sizeof(devrequest), 1);
	dr->requesttype = 0x21;
	dr->request = 0x22;
	dr->value = 0x1;
	dr->index = 0;
	dr->length = 0;
	urb_submit(uurb, USBDEVFS_URB_TYPE_CONTROL, 0, (void *)dr, 8, hack_callback);
	return;
}

/*
 * keep reading from pppd until we can assemble a packet. then send it to the
 * ipaq (or queue it if a send is in progress.
 */

static void
send_to_ipaq()
{
	caddr_t			data;
	int			len;
	static char		buffer[URBSENDSIZE];
	static int		pos = 0;
	unsigned char		byte;

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);

	while ((len = read(pppfd, &byte, 1)) == 1) {
		buffer[pos++] = byte;
		if (pos > URBSENDSIZE) {
			msg(MSG_ERR, "ERROR: send buffer overflowed !\n");
			cleanup(0);
		}
		if (pos > 1 && byte == 0x7e) {
			data = malloc(pos);
			memcpy(data, buffer, pos);
			bulk_urb_out(data, pos);
			pos = 0;
		}
#if 0
		/*
		 * what the heck ?! I thought ppp packets were supposed to start
		 * and end with 0x7e, but it doesn't seem to be true of pppd,
		 * at least my version (2.4.1) or are we dropping bytes somewhere ?
		 * termio settings ?
		 */

		if (pos == 1 && byte != 0x7e) {
			fprintf(stderr, "ERROR: garbage %x at ppp packet boundary\n", byte);
			hexdump(buffer, URBSENDSIZE);
			fprintf(debug, "\nscrewed here\n");
			len = read(pppfd, buffer, URBSENDSIZE);
			fprintf(stderr, "\nOK rest coming %d %d\n", len, errno);
			if (len > 0) {
				hexdump(buffer, len);
			}
			cleanup(0);
		}
#endif
	}
	if (errno != EAGAIN) {
		msg(MSG_ERR, "ERROR:error %d reading from pppd\n", errno);
		return;
	}
}

/*
 * shove in as much as pppd will take without blocking.
 */

static void
send_to_pppd()
{
	int		written;
	struct packet	*p, *tmp;

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	for (p = head_in.next; p != &head_in;) {
		written = write(pppfd, p->data + p->done, p->len - p->done);
		if (written < 0) {
			if (errno != EAGAIN) {
				msg(MSG_ERR, "ERROR:writing to pppd\n");
			}
			return;
		}
		p->done += written;
		if (p->done == p->len) {
			p->prev->next = p->next;
			p->next->prev = p->prev;
			tmp = p->next;
			free(p->data);
			free(p);
			p = tmp;
		} else {
			return;
		}
	}
}

/*
 * we do our own chat instead of letting ppp do it. this a. simplifies packet
 * boundary detection and b. we send the whole CLIENTSERVER in one packet as
 * opposed to 12 with pppd doing the chat. in any case, the CLIENT/CLIENTSERVER
 * handshake is constant and unlikely to change.
 */

static void
chat_callback(struct usbdevfs_urb *uurb)
{
	static char	chat_buffer[CBSIZE] = {0,};
	static int	pos = 0;
	caddr_t		buffer;
	size_t		len;
	char		*ret, *ptr = chat_buffer;

	msg(MSG_INFO, "\nentered %s\n", __FUNCTION__);
	urb_dump(uurb);
	if (uurb->status != 0) {
		msg(MSG_ERR, "ERROR:bulk receive failed %d\n", uurb->status);
		cleanup(0);
	}
	buffer = uurb->buffer;
	len = uurb->actual_length;
	memset(uurb, 0, sizeof(struct usbdevfs_urb));
	if (pos + len > CBSIZE) {
		msg(MSG_ERR, "ERROR:ran out of chat buffer space\n");
		if (msglevel >= MSG_DUMP) {
			hexdump(chat_buffer, pos);
		}
		cleanup(0);
	}
	memcpy(chat_buffer + pos, buffer, len);
	pos += len;
	while(ret = memchr(ptr, 'C', pos - (ptr - chat_buffer))) {
		if (strstr(ret, "CLIENT")) {
			bulk_urb_out(strdup("CLIENTSERVER"),
				  strlen("CLIENTSERVER"));
			urb_submit(uurb, USBDEVFS_URB_TYPE_BULK, IPAQ_ENDPOINT_IN,
				   buffer, URBRECVSIZE, bulk_in_callback);
			return;
		}
		ptr = ret + 1;
	}
	urb_submit(uurb, USBDEVFS_URB_TYPE_BULK, IPAQ_ENDPOINT_IN, buffer,
		   URBRECVSIZE, chat_callback);
}

static void
do_stuff()
{
	struct usbdevfs_urb	*uurb;
	caddr_t			*data;
	devrequest		*dr;
	struct pollfd		pfds[2];

	/*
	 * all this is unnecessary, strictly speaking. but it helps. something
	 * to do with the timing I guess. copied straight from vmware sniffs.
	 */

	uurb = calloc(sizeof(struct usbdevfs_urb), 1);
	dr = (devrequest *)calloc(1024, 1);

// first 6 80 100
	dr->requesttype = 0x80;
	dr->request = 0x6;
	dr->value = 0x100;
	dr->index = 0;
	dr->length = 0x40;
	urb_submit(uurb, 2, 0x80, (void *)dr, 0x48, 0);
	reapurb_block();

// second 6 80 100
	dr->requesttype = 0x80;
	dr->request = 0x6;
	dr->value = 0x100;
	dr->index = 0;
	dr->length = 0x12;
	urb_submit(uurb, 2, 0x80, (void *)dr, 0x1a, 0);
	reapurb_block();

// first 6 80 200
	dr->requesttype = 0x80;
	dr->request = 0x6;
	dr->value = 0x200;
	dr->index = 0;
	dr->length = 0x9;
	urb_submit(uurb, 2, 0x80, (void *)dr, 0x11, 0);
	reapurb_block();

#if 0
	/*
	 * it looks like we can do without these ...
	 */

// second 6 80 200
	dr->requesttype = 0x80;
	dr->request = 0x6;
	dr->value = 0x200;
	dr->index = 0;
	dr->length = 0xff;
	urb_submit(uurb, 2, 0x80, (void *)dr, 0x107, 0);
	reapurb_block();

// again 6 80 100
	dr->requesttype = 0x80;
	dr->request = 0x6;
	dr->value = 0x100;
	dr->index = 0;
	dr->length = 0x12;
	urb_submit(uurb, 2, 0x80, (void *)dr, 0x1a, 0);
	reapurb_block();

// second 6 80 200
	dr->requesttype = 0x80;
	dr->request = 0x6;
	dr->value = 0x200;
	dr->index = 0;
	dr->length = 0x109;
	urb_submit(uurb, 2, 0x80, (void *)dr, 0x111, 0);
	reapurb_block();
#endif

	msg(MSG_INFO, "sleeping %d seconds ...\n", opt_delay);
	sleep(opt_delay);	
	data = calloc(URBRECVSIZE, 1);

	/*
	 * start the bulk read and cross fingers ...
	 */

	msg(MSG_INFO, "starting bulk read\n");
	urb_submit(uurb, USBDEVFS_URB_TYPE_BULK, IPAQ_ENDPOINT_IN, (void *)data,
		   URBRECVSIZE, chat_callback);

	/*
	 * get some unknown info.
	 */

	msg(MSG_INFO, "starting unknown control request\n");
	uurb = calloc(sizeof(struct usbdevfs_urb), 1);
	dr = calloc(sizeof(devrequest), 1);
	dr->requesttype = 0x21;
	dr->request = 0x22;
	dr->value = 0x1;
	dr->index = 0;
	dr->length = 0;
	urb_submit(uurb, USBDEVFS_URB_TYPE_CONTROL, 0, (void *)dr, 8, hack_callback);

	while (1) {
		pfds[0].fd = ipaqfd;
		pfds[0].events = POLLOUT;
		pfds[0].revents = 0;
		pfds[1].fd = pppfd;
		pfds[1].events = POLLIN;
		if (head_in.next != &head_in) {
			pfds[1].events |= POLLOUT;
		}
		pfds[1].revents = 0;

		poll(pfds, 2, 1000);
		if (pfds[0].revents & (POLLHUP | POLLERR)) {
			fprintf(stderr, "ERROR:device hung up\n");
			cleanup(0);
		}
		reapurb();
		if (pfds[1].revents & (POLLHUP | POLLERR)) {
			fprintf(stderr, "ERROR:pppd died\n");
			cleanup(0);
		}
		if (pfds[1].revents & POLLIN) {
			send_to_ipaq();
		}
		if ((pfds[1].events & POLLOUT && pfds[1].revents & POLLOUT) ||
		    head_in.next != &head_in) {
			send_to_pppd();
		}
	}
}
