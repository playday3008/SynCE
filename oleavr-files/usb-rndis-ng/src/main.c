/*
 * Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
 *
 * Special thanks to Brian Johnson and those who contributed to the
 * donation of a PXA27x-based PDA so that this driver could be made
 * reality...  You guys rock! :-)
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

//#define INSANE_DEBUG 1

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

#define RNDIS_MSG_PACKET                         1
#define RNDIS_MSG_INIT                           2
#define RNDIS_MSG_QUERY                          4
#define RNDIS_MSG_SET                            5
#define RNDIS_MSG_KEEPALIVE                      8

#define RNDIS_STATUS_SUCCESS                     0

#define RNDIS_TIMEOUT_MS                      5000

/* FIXME: don't assume USB 2.0 */
#define HOST_MAX_TRANSFER_SIZE               16384

#define OID_802_3_PERMANENT_ADDRESS     0x01010101
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

#define INTERRUPT_MAX_PACKET_SIZE              128
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

struct rndis_keepalive {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
} __attribute__ ((packed));

struct rndis_data {
    guint32 msg_type;
    guint32 msg_len;
    guint32 data_offset;
    guint32 data_len;

    guint32 oob_data_offset;
    guint32 oob_data_len;
    guint32 num_oob;

    guint32 packet_data_offset;
    guint32 packet_data_len;

    guint32 vc_handle;
    guint32 reserved;
} __attribute__ ((packed));

typedef struct {
    usb_dev_handle *h;
    gint fd;
    guint host_max_transfer_size;
    guint device_max_transfer_size;
    guint alignment;

    struct usb_endpoint_descriptor *ep_int_in;
    struct usb_endpoint_descriptor *ep_bulk_in;
    struct usb_endpoint_descriptor *ep_bulk_out;
} RNDISContext;

static RNDISContext device_ctx;

#ifdef INSANE_DEBUG
static void
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
#endif

static gboolean
rndis_command (RNDISContext *ctx,
               struct rndis_request *req,
               struct rndis_response **resp)
{
  gint len;
  guchar int_buf[INTERRUPT_MAX_PACKET_SIZE];
  static guchar response_buf[RESPONSE_BUFFER_SIZE];
  static guint id = 2;
  struct rndis_response *r;

  req->msg_type = GUINT32_TO_LE (req->msg_type);
  req->msg_len = GUINT32_TO_LE (req->msg_len);
  req->request_id = GUINT32_TO_LE (id++);

  len = usb_control_msg (ctx->h, USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                         USB_REQ_GET_STATUS, 0, 0, (gchar *) req, req->msg_len,
                         RNDIS_TIMEOUT_MS);
  if (len <= 0)
    goto USB_ERROR;
  else if (len != req->msg_len)
    {
      fprintf (stderr, "short write, wrote %d out of %d\n", len,
               req->msg_len);
      goto ERROR;
    }

  /**
   * Interrupt requests should always be wMaxPacketSize.
   * Thanks to Sander Hoentjen for assistance in tracking this down. :)
   */
  len = usb_interrupt_read (ctx->h, ctx->ep_int_in->bEndpointAddress,
                            (gchar *) int_buf,
                            ctx->ep_int_in->wMaxPacketSize,
                            RNDIS_TIMEOUT_MS);
  if (len <= 0)
    goto USB_ERROR;
  else if (len < 8)
    {
      fprintf (stderr, "read %d, expected 8 or more\n", len);
      goto ERROR;
    }

  len = usb_control_msg (ctx->h, USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
                         USB_REQ_CLEAR_FEATURE, 0, 0, (gchar *) response_buf,
                         sizeof(response_buf), RNDIS_TIMEOUT_MS);
  if (len <= 0)
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
  fprintf (stderr, "USB error: %s\n",
      (len == 0) ? "device disconnected" : usb_strerror ());

ERROR:
  return FALSE;
}

static gboolean
rndis_init (RNDISContext *ctx,
            struct rndis_init_c **response)
{
  struct rndis_init req;
  struct rndis_init_c *resp;

  req.msg_type = RNDIS_MSG_INIT;
  req.msg_len = sizeof (req);

  req.major_version = GUINT32_TO_LE (1);
  req.minor_version = GUINT32_TO_LE (0);
  req.max_transfer_size = GUINT32_TO_LE (HOST_MAX_TRANSFER_SIZE);

  if (!rndis_command (ctx, (struct rndis_request *) &req,
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

static gboolean
rndis_query (RNDISContext *ctx,
             guint32 oid,
             guchar *result,
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

  if (!rndis_command (ctx, (struct rndis_request *) &req,
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

  memcpy (result,
      (guchar *) resp + sizeof (struct rndis_message) + resp->offset,
      resp->len);
  *res_len = resp->len;

  return TRUE;
}

static gboolean
rndis_set (RNDISContext *ctx,
           guint32 oid,
           guchar *value,
           guint value_len)
{
  gboolean success;
  guchar *buf;
  guint msg_len;
  struct rndis_set *req;
  struct rndis_set_c *resp;

  msg_len = sizeof (struct rndis_set) + value_len;

  buf = g_new (guchar, msg_len);
  req = (struct rndis_set *) buf;

  req->msg_type = RNDIS_MSG_SET;
  req->msg_len = msg_len;

  req->oid = GUINT32_TO_LE (oid);
  req->len = GUINT32_TO_LE (value_len);
  req->offset = GUINT32_TO_LE (sizeof (struct rndis_set)
                               - sizeof (struct rndis_message));
  req->handle = GUINT32_TO_LE (0);

  memcpy (buf + sizeof (struct rndis_set), value, value_len);

  success = rndis_command (ctx, (struct rndis_request *) req,
                           (struct rndis_response **) &resp);

  g_free (buf);

  return success;
}

static gboolean
rndis_keepalive (RNDISContext *ctx)
{
  struct rndis_keepalive req;
  void *resp;

  req.msg_type = RNDIS_MSG_KEEPALIVE;
  req.msg_len = sizeof (struct rndis_keepalive);

  return rndis_command (ctx, (struct rndis_request *) &req,
                        (struct rndis_response **) &resp);
}

static gpointer
recv_thread (gpointer data)
{
  RNDISContext *ctx = data;
  guchar *buf;
  gint len;

  buf = g_new (guchar, ctx->host_max_transfer_size);

  while (TRUE)
    {
      gint remaining;
      guchar *p;

      len = usb_bulk_read (ctx->h, ctx->ep_bulk_in->bEndpointAddress,
                           (gchar *) buf, ctx->host_max_transfer_size, 1000);
      if (len <= 0)
        {
          /* not a timeout? */
          if (len != -110)
            {
              fprintf (stderr, "recv_thread: usb_bulk_read returned %d: %s\n",
                       len, usb_strerror ());
              goto OUT;
            }
          else
            {
              continue;
            }
        }

#ifdef INSANE_DEBUG
      printf ("recv_thread: usb_bulk_read read %d\n", len);
#endif

      p = buf;
      remaining = len;

      while (remaining)
        {
          struct rndis_data *hdr = (struct rndis_data *) p;
          guchar *eth_buf;

          if (remaining < 8)
            {
              fprintf (stderr, "ignoring short packet with remaining=%d\n",
                       remaining);
              break;
            }

          hdr->msg_type = GUINT32_FROM_LE (hdr->msg_type);
          hdr->msg_len = GUINT32_FROM_LE (hdr->msg_len);

#ifdef INSANE_DEBUG
          printf ("recv_thread: msg_len=%d\n", hdr->msg_len);
#endif

          if (hdr->msg_type != RNDIS_MSG_PACKET)
            {
              fprintf (stderr, "ignoring msg_type=%d\n", hdr->msg_type);
              break;
            }
          else if (hdr->msg_len > remaining)
            {
              fprintf (stderr, "msg_len=%d > remaining=%d\n",
                       hdr->msg_len, remaining);
              break;
            }
          else if (hdr->msg_len == 0)
            {
              fprintf (stderr, "ignoring short message\n");
              break;
            }

          hdr->data_offset = GUINT32_FROM_LE (hdr->data_offset);
          hdr->data_len = GUINT32_FROM_LE (hdr->data_len);

          if (hdr->oob_data_offset ||
              hdr->oob_data_len ||
              hdr->num_oob ||
              hdr->packet_data_offset ||
              hdr->packet_data_len)
            {
              hdr->oob_data_offset = GUINT32_FROM_LE (hdr->oob_data_offset);
              hdr->oob_data_len = GUINT32_FROM_LE (hdr->oob_data_len);
              hdr->num_oob = GUINT32_FROM_LE (hdr->num_oob);
              hdr->packet_data_offset = GUINT32_FROM_LE (hdr->packet_data_offset);
              hdr->packet_data_len = GUINT32_FROM_LE (hdr->packet_data_len);

              printf ("recv_thread: interesting packet:\n");
              printf ("  oob_data_offset=%d\n"
                      "  oob_data_len=%d\n"
                      "  num_oob=%d\n"
                      "  packet_data_offset=%d\n"
                      "  packet_data_len=%d\n",
                      hdr->oob_data_offset,
                      hdr->oob_data_len,
                      hdr->num_oob,
                      hdr->packet_data_offset,
                      hdr->packet_data_len);
            }

          if (sizeof (struct rndis_message) + hdr->data_offset
              + hdr->data_len > hdr->msg_len)
            {
              fprintf (stderr, "ignoring truncated message\n");
              break;
            }

          eth_buf = p + sizeof (struct rndis_message) + hdr->data_offset;

#ifdef INSANE_DEBUG
          printf ("writing ethernet frame with size=%d\n", hdr->data_len);
#endif

          len = write (ctx->fd, eth_buf, hdr->data_len);
          if (len <= 0)
            {
              fprintf (stderr, "recv failed because len = %d: %s\n", len,
                       strerror (errno));
              goto OUT;
            }
          else if (len != hdr->data_len)
            {
              fprintf (stderr, "short write, %d out of %d bytes written\n",
                       len, hdr->data_len);
            }

          p += hdr->msg_len;
          remaining -= hdr->msg_len;
        }
    }

  g_free (buf);

OUT:
  printf ("recv_thread exiting\n");

  /* just assume the device was disconnected for now */
  exit (0);

  return NULL;
}

static gpointer
send_thread (gpointer data)
{
  RNDISContext *ctx = data;
  guchar *buf;
  gint len, result;
#ifdef INSANE_DEBUG
  guint i = 0;
#endif

  buf = g_new (guchar, ctx->device_max_transfer_size);

  while (TRUE)
    {
      struct rndis_data *hdr = (struct rndis_data *) buf;
#ifdef INSANE_DEBUG
      gchar str[256];
#endif
      guint msg_len;

      len = read (ctx->fd, buf + sizeof (struct rndis_data),
                  ctx->device_max_transfer_size - sizeof (struct rndis_data));
      if (len <= 0)
        {
          fprintf (stderr, "recv failed because len = %d: %s\n", len,
                   strerror (errno));

          return NULL;
        }

#ifdef INSANE_DEBUG
      printf ("send_thread: relaying %d bytes\n", len);
#endif

      memset (hdr, 0, sizeof (struct rndis_data));

      msg_len = sizeof (struct rndis_data) + len;
      if (msg_len % ctx->alignment != 0)
        {
          guint padding = ctx->alignment - (msg_len % ctx->alignment);

#ifdef INSANE_DEBUG
          printf ("send_thread: message length changed from %d to %d\n",
                  msg_len, msg_len + padding);
#endif

          msg_len += padding;
        }

      hdr->msg_type = GUINT32_TO_LE (RNDIS_MSG_PACKET);
      hdr->msg_len = GUINT32_TO_LE (msg_len);

      hdr->data_offset = GUINT32_TO_LE (sizeof (struct rndis_data) -
                                        sizeof (struct rndis_message));
      hdr->data_len = GUINT32_TO_LE (len);

      result = usb_bulk_write (ctx->h, ctx->ep_bulk_out->bEndpointAddress,
                               (gchar *) buf, msg_len, RNDIS_TIMEOUT_MS);
      if (result <= 0)
        {
          fprintf (stderr, "send_thread: USB error occurred: %s\n",
                   usb_strerror ());
          return NULL;
        }

#ifdef INSANE_DEBUG
      sprintf (str, "/tmp/sent_packet_%04d.bin", ++i);
      log_data (str, buf, msg_len);
#endif
    }

  g_free (buf);

  printf ("send_thread exiting\n");

  /* just assume the device was disconnected for now */
  exit (0);

  return NULL;
}

static gboolean
find_endpoints (struct usb_device *dev,
                RNDISContext *ctx)
{
  gboolean found_all = FALSE;
  gint i, j;

  for (i = 0; i < dev->config->bNumInterfaces; i++)
    {
      struct usb_interface_descriptor *desc = dev->config->interface[i].altsetting;

      for (j = 0; j < desc->bNumEndpoints; j++)
        {
          struct usb_endpoint_descriptor *ep = &desc->endpoint[j];
          guchar type = ep->bmAttributes & USB_ENDPOINT_TYPE_MASK;

          if ((ep->bEndpointAddress & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_IN)
            {
              if (type == USB_ENDPOINT_TYPE_INTERRUPT)
                {
                  ctx->ep_int_in = ep;
                }
              else if (type == USB_ENDPOINT_TYPE_BULK)
                {
                  ctx->ep_bulk_in = ep;
                }
            }
          else
            {
              if (type == USB_ENDPOINT_TYPE_BULK)
                {
                  ctx->ep_bulk_out = ep;
                }
            }

          if (ctx->ep_int_in && ctx->ep_bulk_in && ctx->ep_bulk_out)
            {
              found_all = TRUE;
              goto OUT;
            }
        }
    }

OUT:
  return found_all;
}

static gboolean
handle_device (struct usb_device *dev)
{
  gboolean success = TRUE;
  usb_dev_handle *h = NULL;
  struct rndis_init_c *resp;
  guchar mac_addr[6];
  guint mac_addr_len;
  gchar mac_addr_str[20], str[64];
  guint32 pf;
  gint fd = -1, err;
  struct ifreq ifr;

  if (!find_endpoints (dev, &device_ctx))
    return FALSE;

  /* should never happen */
  if (device_ctx.ep_int_in->wMaxPacketSize > INTERRUPT_MAX_PACKET_SIZE)
    {
      fprintf (stderr, "Interrupt ep wMaxPacketSize=%d > %d\n",
          device_ctx.ep_int_in->wMaxPacketSize, INTERRUPT_MAX_PACKET_SIZE);
      return FALSE;
    }

  h = usb_open (dev);
  if (h == NULL)
    goto USB_ERROR;

  device_ctx.h = h;

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
    goto USB_ERROR;

  if (usb_claim_interface (h, 1) != 0)
    goto USB_ERROR;

  /*
  if (usb_set_configuration (h, 1) != 0)
    goto ERROR;*/

  /*
   * initialize
   */

  puts ("doing rndis_init");
  if (!rndis_init (&device_ctx, &resp))
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

  device_ctx.host_max_transfer_size = HOST_MAX_TRANSFER_SIZE;
  device_ctx.device_max_transfer_size = resp->max_transfer_size;
  device_ctx.alignment = 1 << resp->packet_alignment;

  puts ("doing rndis_query for OID_802_3_PERMANENT_ADDRESS");
  mac_addr_len = 6;
  if (!rndis_query (&device_ctx, OID_802_3_PERMANENT_ADDRESS, mac_addr,
                    &mac_addr_len))
    {
      goto ANY_ERROR;
    }

  sprintf (mac_addr_str, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
           mac_addr[4], mac_addr[5]);
  printf ("rndis_query succeeded, got MAC address: %s\n", mac_addr_str);

  puts ("setting packet filter");

  pf = GUINT32_TO_LE (NDIS_PACKET_TYPE_DIRECTED
                      | NDIS_PACKET_TYPE_MULTICAST
                      | NDIS_PACKET_TYPE_BROADCAST);
  //pf = GUINT32_TO_LE (NDIS_PACKET_TYPE_PROMISCUOUS);
  if (!rndis_set (&device_ctx, OID_GEN_CURRENT_PACKET_FILTER, (guchar *) &pf,
                  sizeof (pf)))
    {
      goto ANY_ERROR;
    }

  puts ("packet filter set");

  if ((fd = open ("/dev/net/tun", O_RDWR)) < 0)
    goto SYS_ERROR;

  memset (&ifr, 0, sizeof (ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

  if ((err = ioctl (fd, TUNSETIFF, (void *) &ifr)) < 0)
    {
      goto SYS_ERROR;
    }

  printf ("got device '%s'\n", ifr.ifr_name);

  device_ctx.fd = fd;

  /* hackish */
  sprintf (str, "ifconfig %s hw ether %s", ifr.ifr_name, mac_addr_str);
  system (str);

  sprintf (str, "ifconfig %s up", ifr.ifr_name);
  system (str);

  if (!g_thread_create (recv_thread, &device_ctx, TRUE, NULL))
    goto SYS_ERROR;

  if (!g_thread_create (send_thread, &device_ctx, TRUE, NULL))
    goto SYS_ERROR;

  printf ("the device is now up and running\n");

  while (TRUE)
    {
      sleep (5);

#ifdef INSANE_DEBUG
      printf ("sending keepalive\n");
#endif

      if (!rndis_keepalive (&device_ctx))
        {
          /* just assume the device was disconnected for now */
          break;
        }
    }

  goto OUT;

USB_ERROR:
  success = FALSE;
  fprintf (stderr, "error occurred: %s\n", usb_strerror ());
  goto ANY_ERROR;

SYS_ERROR:
  success = FALSE;
  fprintf (stderr, "error occurred: %s\n", strerror (errno));

ANY_ERROR:
  if (h != NULL)
    usb_close (h);
  if (fd > 0)
      close (fd);

OUT:
  return success;
}

gint
main(gint argc, gchar *argv[])
{
  gint bus_id = -1, dev_id = -1;
  struct usb_bus *busses, *bus;

  if (argc != 1 && argc != 3)
    {
      fprintf (stderr, "usage: %s [bus-id device-id]\n", argv[0]);
      return 1;
    }

  if (argc == 3)
    {
      bus_id = atoi (argv[1]);
      dev_id = atoi (argv[2]);
    }
  else
    {
      printf ("scanning for a USB RNDIS device\n");
    }

  g_thread_init (NULL);

  usb_init ();

  usb_find_busses ();
  usb_find_devices ();

  busses = usb_get_busses ();
  for (bus = busses; bus; bus = bus->next)
    {
      struct usb_device *dev;
      int cur_bus_id = atoi (bus->dirname);

      if (bus_id != -1 && cur_bus_id != bus_id)
        continue;

      for (dev = bus->devices; dev; dev = dev->next)
        {
          struct usb_device_descriptor *desc = &dev->descriptor;

          if (dev_id != -1 && dev->devnum != dev_id)
            {
              continue;
            }

          if (desc->bDeviceClass == 0xef &&
              desc->bDeviceSubClass == 0x01 &&
              desc->bDeviceProtocol == 0x01)
            {
              return handle_device (dev) != TRUE;
            }
          else if (dev_id != -1)
            {
              fprintf (stderr, "device does not look like an RNDIS device\n");
              return 1;
            }
        }
    }

  if (bus_id != -1)
    {
      fprintf (stderr, "device not found\n");
    }
  else
    {
      fprintf (stderr, "no devices found\n");
    }

  return 1;
}

