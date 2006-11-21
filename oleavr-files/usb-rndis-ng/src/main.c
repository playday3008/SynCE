/*
 * Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <glib.h>
#include <usb.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#define RNDIS_MSG_COMPLETION            0x80000000

#define RNDIS_MSG_INIT                           2
#define RNDIS_MSG_QUERY                          4
#define RNDIS_MSG_SET                            5

#define RNDIS_STATUS_SUCCESS                     0

#define RNDIS_TIMEOUT_MS                      5000

#define OID_802_3_CURRENT_ADDRESS       0x01010102
#define OID_GEN_CURRENT_PACKET_FILTER   0x0001010E

#define NDIS_PACKET_TYPE_DIRECTED       0x00000001
#define NDIS_PACKET_TYPE_MULTICAST      0x00000002
#define NDIS_PACKET_TYPE_ALL_MULTICAST  0x00000004
#define NDIS_PACKET_TYPE_BROADCAST      0x00000008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING 0x00000010
#define NDIS_PACKET_TYPE_PROMISCUOUS    0x00000020
#define NDIS_PACKET_TYPE_SMT            0x00000040
#define NDIS_PACKET_TYPE_ALL_LOCAL      0x00000080
#define NDIS_PACKET_TYPE_GROUP          0x00000100
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL 0x00000200
#define NDIS_PACKET_TYPE_FUNCTIONAL     0x00000400
#define NDIS_PACKET_TYPE_MAC_FRAME      0x00000800

#define RESPONSE_BUFFER_SIZE                  1025

#define USB_DIR_OUT                              0 /* to device */
#define USB_DIR_IN                            0x80 /* to host */

struct rndis_message {
    guint32 msg_type;
    guint32 msg_len;
} __attribute__ ((packed));

struct rndis_request {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
} __attribute__ ((packed));

struct rndis_response {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
} __attribute__ ((packed));

struct rndis_init {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 major_version;
    guint32 minor_version;
    guint32 max_transfer_size;
} __attribute__ ((packed));

struct rndis_init_c {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
    guint32 major_version;
    guint32 minor_version;
    guint32 device_flags;
    guint32 medium;
    guint32 max_packets_per_message;
    guint32 max_transfer_size;
    guint32 packet_alignment;
    guint32 af_list_offset;
    guint32 af_list_size;
} __attribute__ ((packed));

struct rndis_query {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 oid;
    guint32 len;
    guint32 offset;
    guint32 handle;
} __attribute__ ((packed));

struct rndis_query_c {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
    guint32 len;
    guint32 offset;
} __attribute__ ((packed));

struct rndis_set {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 oid;
    guint32 len;
    guint32 offset;
    guint32 handle;
} __attribute__ ((packed));

struct rndis_set_c {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
} __attribute__ ((packed));

void
log_data (const gchar *filename,
          const guchar *buf,
          int len)
{
  FILE *f = fopen (filename, "w");
  if (f == NULL)
    return;

  fwrite (buf, len, 1, f);

  fclose (f);
}

gboolean
rndis_command (usb_dev_handle *h,
               struct rndis_request *req,
               struct rndis_response **resp)
{
  gint len;
  guchar buf[8];
  static guchar response_buf[RESPONSE_BUFFER_SIZE];
  static guint id = 2;
  struct rndis_response *r;

  req->msg_type = GUINT32_TO_LE (req->msg_type);
  req->msg_len = GUINT32_TO_LE (req->msg_len);
  req->request_id = GUINT32_TO_LE (id++);

  len = usb_control_msg (h, USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                         USB_REQ_GET_STATUS, 0, 0, (gchar *) req, req->msg_len,
                         RNDIS_TIMEOUT_MS);
  if (len < 0)
    goto USB_ERROR;
  else if (len != req->msg_len)
    {
      fprintf (stderr, "short write, wrote %d out of %d\n", len,
               req->msg_len);
      goto ERROR;
    }

  len = usb_interrupt_read (h, 0x81, (gchar *) buf, sizeof(buf), RNDIS_TIMEOUT_MS);
  if (len < 0)
    goto USB_ERROR;
  else if (len != sizeof(buf))
    {
      fprintf (stderr, "read %d, expected %d\n", len, sizeof(buf));
      goto ERROR;
    }

  len = usb_control_msg (h, USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                         USB_REQ_CLEAR_FEATURE, 0, 0, (gchar *) response_buf,
                         sizeof(response_buf), RNDIS_TIMEOUT_MS);
  if (len < 0)
    goto USB_ERROR;
  else if (len < sizeof (struct rndis_response))
    goto ERROR;

  *resp = (struct rndis_response *) response_buf;
  r = *resp;

  r->msg_type = GUINT32_FROM_LE (r->msg_type);
  r->msg_len = GUINT32_FROM_LE (r->msg_len);
  r->request_id = GUINT32_FROM_LE (r->request_id);
  r->status = GUINT32_FROM_LE (r->status);

  if (r->msg_type !=
      (GUINT32_FROM_LE (req->msg_type) | RNDIS_MSG_COMPLETION))
    goto ERROR;
  else if (r->request_id != GUINT32_FROM_LE (req->request_id))
    goto ERROR;

  return TRUE;

USB_ERROR:
  fprintf (stderr, usb_strerror ());

ERROR:
  return FALSE;
}

gboolean
rndis_init (usb_dev_handle *h,
            struct rndis_init_c **response)
{
  struct rndis_init req;
  struct rndis_init_c *resp;

  req.msg_type = RNDIS_MSG_INIT;
  req.msg_len = sizeof (req);

  req.major_version = GUINT32_TO_LE (1);
  req.minor_version = GUINT32_TO_LE (0);
  req.max_transfer_size = GUINT32_TO_LE (16384);

  if (!rndis_command (h, (struct rndis_request *) &req,
                      (struct rndis_response **) &resp))
    return FALSE;

  resp->major_version = GUINT32_FROM_LE (resp->major_version);
  resp->minor_version = GUINT32_FROM_LE (resp->minor_version);
  resp->device_flags = GUINT32_FROM_LE (resp->device_flags);
  resp->medium = GUINT32_FROM_LE (resp->medium);
  resp->max_packets_per_message = GUINT32_FROM_LE (
                                    resp->max_packets_per_message);
  resp->max_transfer_size = GUINT32_FROM_LE (resp->max_transfer_size);
  resp->packet_alignment = GUINT32_FROM_LE (resp->packet_alignment);
  resp->af_list_offset = GUINT32_FROM_LE (resp->af_list_offset);
  resp->af_list_size = GUINT32_FROM_LE (resp->af_list_size);

  *response = resp;

  return TRUE;
}

gboolean
rndis_query (usb_dev_handle *h,
             guint32 oid,
             guchar **result,
             guint *res_len)
{
  struct rndis_query req;
  struct rndis_query_c *resp;

  req.msg_type = RNDIS_MSG_QUERY;
  req.msg_len = sizeof (req) + *res_len;

  req.oid = GUINT32_TO_LE (oid);
  req.len = GUINT32_TO_LE (*res_len);
  req.offset = GUINT32_TO_LE (sizeof (req) - sizeof (struct rndis_message));
  req.handle = GUINT32_TO_LE (0);

  if (!rndis_command (h, (struct rndis_request *) &req,
                      (struct rndis_response **) &resp))
    return FALSE;

  resp->len = GUINT32_FROM_LE (resp->len);
  resp->offset = GUINT32_FROM_LE (resp->offset);

  if (resp->len > *res_len)
    return FALSE;

  if (sizeof (struct rndis_message) + resp->offset + resp->len
      > RESPONSE_BUFFER_SIZE)
    {
      return FALSE;
    }

  *result = (guchar *) resp + sizeof (struct rndis_message) + resp->offset;
  *res_len = resp->len;

  return TRUE;
}

gboolean
rndis_set (usb_dev_handle *h,
           guint32 oid,
           guchar *value,
           guint value_len)
{
  struct rndis_set req;
  struct rndis_set_c *resp;

  req.msg_type = RNDIS_MSG_SET;
  req.msg_len = sizeof (req) + value_len;

  req.oid = GUINT32_TO_LE (oid);
  req.len = GUINT32_TO_LE (value_len);
  req.offset = GUINT32_TO_LE (sizeof (req) - sizeof (struct rndis_message));
  req.handle = GUINT32_TO_LE (0);

  if (!rndis_command (h, (struct rndis_request *) &req,
                      (struct rndis_response **) &resp))
    return FALSE;

  return TRUE;
}

void
handle_device (struct usb_device *dev)
{
  usb_dev_handle *h;
  struct rndis_init_c *resp;
  guint max_transfer_size;
  guchar *mac_addr;
  guint mac_addr_len;
  guint32 pf;
  guchar *buf;
  gint len, fd = -1, err;
  struct ifreq ifr;

  h = usb_open (dev);
  if (h == NULL)
    goto ERROR;

  if (dev->descriptor.bNumConfigurations > 1)
    {
      printf ("warning: more than one configuration found -- using "
              "configuration 0\n");
    }

  if (dev->config->bNumInterfaces != 2)
    {
      printf ("warning: bNumInterfaces != 2 but %d\n",
              dev->config->bNumInterfaces);
    }

  if (usb_claim_interface (h, 0) != 0)
    goto ERROR;

  if (usb_claim_interface (h, 1) != 0)
    goto ERROR;

  /*
  if (usb_set_configuration (h, 1) != 0)
    goto ERROR;*/

  /*
   * initialize
   */

  puts ("doing rndis_init");
  if (!rndis_init (h, &resp))
    goto OUT;

  printf ("rndis_init succeeded:\n");
  printf ("  major_version = %d\n"
          "  minor_version = %d\n"
          "  device_flags = 0x%08x\n"
          "  medium = %d\n"
          "  max_packets_per_message = %d\n"
          "  max_transfer_size = %d\n"
          "  packet_alignment = %d\n"
          "  af_list_offset = %d\n"
          "  af_list_size = %d\n",
          resp->major_version, resp->minor_version,
          resp->device_flags, resp->medium,
          resp->max_packets_per_message,
          resp->max_transfer_size,
          resp->packet_alignment,
          resp->af_list_offset,
          resp->af_list_size);

  max_transfer_size = resp->max_transfer_size;

  puts ("doing rndis_query for OID_802_3_CURRENT_ADDRESS");
  mac_addr_len = 6;
  if (!rndis_query (h, OID_802_3_CURRENT_ADDRESS, &mac_addr, &mac_addr_len))
    goto OUT;

  printf ("rndis_query succeeded, got MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
          mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4],
          mac_addr[5]);

  puts ("setting packet filter");
  pf = GUINT32_TO_LE (NDIS_PACKET_TYPE_DIRECTED
                      | NDIS_PACKET_TYPE_MULTICAST
                      | NDIS_PACKET_TYPE_BROADCAST);
  rndis_set (h, OID_GEN_CURRENT_PACKET_FILTER, (guchar *) &pf, sizeof (pf));

  puts ("packet filter set");

  if ((fd = open ("/dev/net/tun", O_RDWR)) < 0)
    goto SYS_ERROR;

  memset (&ifr, 0, sizeof (ifr));
  ifr.ifr_flags = IFF_TAP;

  if ((err = ioctl (fd, TUNSETIFF, (void *) &ifr)) < 0) {
      goto SYS_ERROR;
  }

  printf ("got '%s'\n", ifr.ifr_name);

  buf = g_new (guchar, max_transfer_size);

  puts ("doing bulk read");
  len = usb_bulk_read (h, 0x82, (gchar *) buf, max_transfer_size, 60 * 1000);
  printf ("usb_bulk_read returned %d\n", len);

  g_free (buf);

  goto OUT;

ERROR:
  fprintf (stderr, "error occurred: %s\n", usb_strerror ());
  goto OUT;

SYS_ERROR:
  fprintf (stderr, "error occurred: %s\n", strerror (errno));
  if (fd > 0)
      close (fd);

OUT:
  return;
}

gint
main(gint argc, gchar *argv[])
{
  GMainContext *ctx;
  GMainLoop *loop;
  struct usb_bus *busses, *bus;

  ctx = g_main_context_new ();
  loop = g_main_loop_new (ctx, TRUE);

  usb_init ();
  usb_find_busses ();
  usb_find_devices ();

  busses = usb_get_busses ();
  for (bus = busses; bus; bus = bus->next)
    {
      struct usb_device *dev;

      for (dev = bus->devices; dev; dev = dev->next)
        {
          struct usb_device_descriptor *desc = &dev->descriptor;

          if (desc->bDeviceClass == 0xef &&
              desc->bDeviceSubClass == 0x01 &&
              desc->bDeviceProtocol == 0x01)
            {
              handle_device (dev);
            }
        }
    }

  g_main_loop_run (loop);

  return 0;
}

