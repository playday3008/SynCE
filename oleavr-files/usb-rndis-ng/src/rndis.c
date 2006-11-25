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
#include <string.h>
#include "rndis.h"

gboolean
_rndis_command (RNDISContext *ctx,
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

gboolean
_rndis_init (RNDISContext *ctx,
             struct rndis_init_c **response)
{
  struct rndis_init req;
  struct rndis_init_c *resp;

  req.msg_type = RNDIS_MSG_INIT;
  req.msg_len = sizeof (req);

  req.major_version = GUINT32_TO_LE (1);
  req.minor_version = GUINT32_TO_LE (0);
  req.max_transfer_size = GUINT32_TO_LE (ctx->host_max_transfer_size);

  if (!_rndis_command (ctx, (struct rndis_request *) &req,
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
_rndis_query (RNDISContext *ctx,
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

  if (!_rndis_command (ctx, (struct rndis_request *) &req,
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

gboolean
_rndis_set (RNDISContext *ctx,
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

  success = _rndis_command (ctx, (struct rndis_request *) req,
                            (struct rndis_response **) &resp);

  g_free (buf);

  return success;
}

gboolean
_rndis_keepalive (RNDISContext *ctx)
{
  struct rndis_keepalive req;
  void *resp;

  req.msg_type = RNDIS_MSG_KEEPALIVE;
  req.msg_len = sizeof (struct rndis_keepalive);

  return _rndis_command (ctx, (struct rndis_request *) &req,
                         (struct rndis_response **) &resp);
}

