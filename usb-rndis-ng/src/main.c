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

#include <config.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/if.h>
#include <linux/if_arp.h>
#include <linux/if_tun.h>
#include <linux/usbdevice_fs.h>
#ifdef HAVE_HAL
#include <libhal.h>
#endif
#include "rndis.h"

static gboolean run_as_daemon = TRUE, hispeed_capable = TRUE;
static gint bus_id = -1, dev_id = -1;

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

static gint
handle_sys_error (const gchar *sender, gint code)
{
  fprintf (stderr, "%s: error occurred: code=%d, msg=%s\n",
           sender, code, strerror (errno));

  return 1;
}

static gint
handle_usb_error (const gchar *sender, gint code)
{
  if (code == -ENODEV)
    {
      printf ("device disconnected, exiting\n");
      return 0;
    }
  else
    {
      fprintf (stderr, "%s: error occurred: code=%d, msg=%s\n",
               sender, code, usb_strerror ());
    }

  return 1;
}

static gpointer
recv_thread (gpointer data)
{
  RNDISContext *ctx = data;
  gint exit_code = 0;
  guchar *buf;
  gint len;

  buf = g_new (guchar, ctx->host_max_transfer_size);

  while (TRUE)
    {
      gint remaining;
      guchar *p;

      do
        {
          len = usb_bulk_read (ctx->h, ctx->ep_bulk_in->bEndpointAddress,
                               (gchar *) buf, ctx->host_max_transfer_size, 60000);
        }
      while (len == 0);

      if (len < 0)
        {
          /* not a timeout? */
          if (len != -ETIMEDOUT)
            {
              exit_code = handle_usb_error (__FUNCTION__, len);
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

      while (remaining > 0)
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
              exit_code = handle_sys_error (__FUNCTION__, len);
              goto OUT;
            }
          else if (len != hdr->data_len)
            {
              fprintf (stderr, "recv_thread: short write, %d out of %d bytes\n",
                       len, hdr->data_len);
            }

          p += hdr->msg_len;
          remaining -= hdr->msg_len;
        }
    }

OUT:
  g_free (buf);

  exit (exit_code);

  return NULL;
}

static gpointer
send_thread (gpointer data)
{
  RNDISContext *ctx = data;
  gint exit_code = 0;
  guchar *buf;
  gint len;
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
          exit_code = handle_sys_error (__FUNCTION__, len);
          goto OUT;
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

      do
        {
          len = usb_bulk_write (ctx->h, ctx->ep_bulk_out->bEndpointAddress,
                                (gchar *) buf, msg_len, RNDIS_TIMEOUT_MS);
        }
      while (len == 0);

      if (len < 0)
        {
          exit_code = handle_usb_error (__FUNCTION__, len);
          goto OUT;
        }
      else if (len != msg_len)
        {
          fprintf (stderr, "send_thread: short write, %d out of %d bytes\n",
                   len, msg_len);
        }

#ifdef INSANE_DEBUG
      sprintf (str, "/tmp/sent_packet_%04d.bin", ++i);
      log_data (str, buf, msg_len);
#endif
    }

OUT:
  g_free (buf);

  exit (exit_code);

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
has_hi_speed_connection (struct usb_device *dev)
{
  /**
   * FIXME: Is there any way we can auto-detect this?
   *
   * The ioctl USBDEVFS_CONNECTINFO's usbdevfs_connectinfo struct has a member
   * called slow, which isn't currently exposed by libusb, however, this member
   * seems to be set to 0 even for 11 Mbps, so it's probably for < 11 Mbps
   * and thus useless...
   */
  return hispeed_capable;
}

#ifdef HAVE_HAL

#define HAL_PROP_USB_BUSNO "usb_device.bus_number"
#define HAL_PROP_USB_DEVNO "usb_device.linux.device_number"

static gboolean
notify_hal (RNDISContext *ctx, gint bus_no, gint dev_no)
{
  gboolean success = FALSE;
  const gchar *func_name = "";
  DBusError error;
  LibHalContext *hal_ctx = NULL;
  gboolean initialized = FALSE;
  gchar **devices = NULL;
  gint num_devices, i;
  gchar *nwif_udi = NULL;

  dbus_error_init (&error);

  hal_ctx = libhal_ctx_new ();
  if (hal_ctx == NULL)
    goto OUT;

  if (!libhal_ctx_set_dbus_connection (hal_ctx,
        dbus_bus_get (DBUS_BUS_SYSTEM, &error)))
    {
      func_name = "dbus_bus_get";
      goto DBUS_ERROR;
    }

  if (!libhal_ctx_init (hal_ctx, &error))
    {
      func_name = "libhal_ctx_init";
      goto DBUS_ERROR;
    }

  initialized = TRUE;

  devices = libhal_find_device_by_capability (hal_ctx, "pda", &num_devices,
                                              &error);
  if (devices == NULL)
    goto DBUS_ERROR;

  for (i = 0; i < num_devices; i++)
    {
      gchar *udi = devices[i];
      gchar str[64], *p;

      if (!libhal_device_property_exists (hal_ctx, udi, HAL_PROP_USB_DEVNO,
                                          NULL))
        {
          continue;
        }

      if (libhal_device_get_property_int (hal_ctx, udi, HAL_PROP_USB_DEVNO,
                                          NULL) != dev_no)
        {
          continue;
        }

      if (libhal_device_get_property_int (hal_ctx, udi, HAL_PROP_USB_BUSNO,
                                          NULL) != bus_no)
        {
          continue;
        }

      nwif_udi = libhal_new_device (hal_ctx, &error);
      if (nwif_udi == NULL)
        {
          func_name = "libhal_new_device";
          goto DBUS_ERROR;
        }

      func_name = "libhal_device_set_property_string";

      sprintf (str, "/sys/class/net/%s", ctx->ifname);
      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "linux.sysfs_path",
                                              str, &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "linux.subsystem",
                                              "net", &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "info.parent",
                                              udi, &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "info.product",
                                              "Networking Interface", &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "info.category",
                                              "net.80203", &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "net.physical_device",
                                              udi, &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "net.interface",
                                              ctx->ifname, &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_string (hal_ctx, nwif_udi, "net.address",
                                              ctx->mac_addr_str, &error))
        goto DBUS_ERROR;

      func_name = "libhal_device_set_property_int";

      if (!libhal_device_set_property_int (hal_ctx, nwif_udi, "net.linux.ifindex",
                                           ctx->ifindex, &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_int (hal_ctx, nwif_udi, "net.arp_proto_hw_id",
                                           ARPHRD_ETHER, &error))
        goto DBUS_ERROR;

      if (!libhal_device_set_property_bool (hal_ctx, nwif_udi, "net.interface_up",
                                            TRUE, &error))
        {
          func_name = "libhal_device_set_property_bool";
          goto DBUS_ERROR;
        }

      if (!libhal_device_set_property_uint64 (hal_ctx, nwif_udi, "net.80203.mac_address",
                                              ctx->mac_addr, &error))
        {
          func_name = "libhal_device_set_property_uint64";
          goto DBUS_ERROR;
        }

      func_name = "libhal_device_add_capability";

      if (!libhal_device_add_capability (hal_ctx, nwif_udi, "net", &error))
        goto DBUS_ERROR;

      if (!libhal_device_add_capability (hal_ctx, nwif_udi, "net.80203", &error))
        goto DBUS_ERROR;

      sprintf (str, "/org/freedesktop/Hal/devices/net_%s", ctx->mac_addr_str);
      for (p = str; *p != '\0'; p++)
        {
          if (*p == ':')
            *p = '_';
        }

      if (!libhal_device_commit_to_gdl (hal_ctx, nwif_udi, str, &error))
        {
          func_name = "libhal_device_commit_to_gdl";
          goto DBUS_ERROR;
        }

      success = TRUE;
    }

  if (!success)
    {
      g_warning ("device not found by HAL");
    }

  goto OUT;

DBUS_ERROR:
  g_warning ("%s failed with D-Bus error %s: %s\n",
             func_name, error.name, error.message);

  dbus_error_free (&error);

OUT:
  g_free (nwif_udi);

  if (devices != NULL)
    libhal_free_string_array (devices);

  if (initialized)
    libhal_ctx_shutdown (hal_ctx, NULL);

  if (hal_ctx != NULL)
    libhal_ctx_free (hal_ctx);

  return success;
}
#endif

static gboolean
handle_device (struct usb_device *dev,
               gint bus_no, gint dev_no)
{
  gboolean success = TRUE;
  usb_dev_handle *h = NULL;
  struct rndis_init_c *resp;
  guint32 mtu;
  guchar mac_addr[6];
  guint mac_addr_len, mtu_len;
  guint32 pf;
  gint fd = -1, sock_fd = -1, err, i;
  struct ifreq ifr;
  pid_t pid;

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
  device_ctx.host_max_transfer_size = (has_hi_speed_connection (dev)) ? 16384 : 8192;

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
  if (!_rndis_init (&device_ctx, &resp))
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

  device_ctx.device_max_transfer_size = resp->max_transfer_size;
  device_ctx.alignment = 1 << resp->packet_alignment;

  printf ("rndis_query(OID_GEN_MAXIMUM_FRAME_SIZE) => ");
  mtu_len = sizeof (mtu);
  if (!_rndis_query (&device_ctx, OID_GEN_MAXIMUM_FRAME_SIZE, (guchar *) &mtu,
                     &mtu_len))
    {
      goto RNDIS_ERROR;
    }

  mtu = GUINT32_FROM_LE (mtu);
  printf ("%d\n", mtu);

  printf ("rndis_query(OID_802_3_PERMANENT_ADDRESS) => ");
  mac_addr_len = 6;
  if (!_rndis_query (&device_ctx, OID_802_3_PERMANENT_ADDRESS, mac_addr,
                     &mac_addr_len))
    {
      goto RNDIS_ERROR;
    }

  device_ctx.mac_addr = (guint64) mac_addr[0] << 40 |
                        (guint64) mac_addr[1] << 32 |
                        (guint64) mac_addr[2] << 24 |
                        (guint64) mac_addr[3] << 16 |
                        (guint64) mac_addr[4] <<  8 |
                        (guint64) mac_addr[5];

  sprintf (device_ctx.mac_addr_str, "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
           mac_addr[4], mac_addr[5]);
  printf ("%s\n", device_ctx.mac_addr_str);

  printf ("rndis_set(OID_GEN_CURRENT_PACKET_FILTER) => ");

  pf = GUINT32_TO_LE (NDIS_PACKET_TYPE_DIRECTED
                      | NDIS_PACKET_TYPE_MULTICAST
                      | NDIS_PACKET_TYPE_BROADCAST);
  /* pf = GUINT32_TO_LE (NDIS_PACKET_TYPE_PROMISCUOUS); */
  if (!_rndis_set (&device_ctx, OID_GEN_CURRENT_PACKET_FILTER, (guchar *) &pf,
                   sizeof (pf)))
    {
      goto RNDIS_ERROR;
    }

  printf ("ok\n");

  /* create a tap device */
  if ((fd = open ("/dev/net/tun", O_RDWR)) < 0)
    goto SYS_ERROR;

  device_ctx.fd = fd;

  memset (&ifr, 0, sizeof (ifr));
  ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

  if ((err = ioctl (fd, TUNSETIFF, &ifr)) < 0)
    {
      goto SYS_ERROR;
    }

  if ((sock_fd = socket (PF_INET, SOCK_STREAM, 0)) < 0)
    {
      goto SYS_ERROR;
    }

  /* name it */
  for (i = 0; i < 100; i++)
    {
      sprintf (ifr.ifr_newname, "rndis%d", i);

      err = ioctl (sock_fd, SIOCSIFNAME, &ifr);
      if (err == 0)
        break;
      else if (errno != -EEXIST)
        goto SYS_ERROR;
    }

  strcpy (ifr.ifr_name, ifr.ifr_newname);
  strcpy (device_ctx.ifname, ifr.ifr_name);

  /* change the MAC address */
  ifr.ifr_hwaddr.sa_family = ARPHRD_ETHER;
  memcpy (ifr.ifr_hwaddr.sa_data, mac_addr, sizeof (mac_addr));
  if ((err = ioctl (sock_fd, SIOCSIFHWADDR, &ifr)) < 0)
    goto SYS_ERROR;

  /* set the MTU */
  ifr.ifr_mtu = mtu - ETH_HLEN;
  if ((err = ioctl (sock_fd, SIOCSIFMTU, &ifr)) < 0)
    {
      if (errno == EINVAL && mtu > ETH_DATA_LEN)
        {
          fprintf (stderr, "failed to set jumbo MTU (%d), patched tun driver "
                   "required\n", mtu - ETH_HLEN);
        }
      else
        {
          goto SYS_ERROR;
        }
    }

  /* get the interface index */
  if ((err = ioctl (sock_fd, SIOCGIFINDEX, &ifr)) < 0)
    goto SYS_ERROR;

  device_ctx.ifindex = ifr.ifr_ifindex;

  /* bring the interface up */
  if ((err = ioctl (sock_fd, SIOCGIFFLAGS, &ifr)) < 0)
    goto SYS_ERROR;

  ifr.ifr_flags |= IFF_UP;

  if ((err = ioctl (sock_fd, SIOCSIFFLAGS, &ifr)) < 0)
    goto SYS_ERROR;

  close (sock_fd);
  sock_fd = -1;

  printf ("%s is now up and running\n", ifr.ifr_name);

  if (run_as_daemon)
    {
      pid = fork ();
      if (pid < 0)
        goto SYS_ERROR;

      if (pid > 0)
        goto OUT;

      if (setsid () < 0)
        goto SYS_ERROR;

      if (chdir ("/") < 0)
        goto SYS_ERROR;

      close (STDIN_FILENO);
      close (STDOUT_FILENO);
      close (STDERR_FILENO);
    }

  if (!g_thread_create (recv_thread, &device_ctx, TRUE, NULL))
    goto SYS_ERROR;

  if (!g_thread_create (send_thread, &device_ctx, TRUE, NULL))
    goto SYS_ERROR;

#ifdef HAVE_HAL
  if (!notify_hal (&device_ctx, bus_no, dev_no))
    goto SYS_ERROR;
#endif

  while (TRUE)
    {
      sleep (5);

#ifdef INSANE_DEBUG
      printf ("sending keepalive\n");
#endif

      if (!_rndis_keepalive (&device_ctx))
        goto ANY_ERROR;
    }

  goto OUT;

USB_ERROR:
  fprintf (stderr, "error occurred: %s\n", usb_strerror ());
  goto ANY_ERROR;

SYS_ERROR:
  fprintf (stderr, "error occurred: %s\n", strerror (errno));
  goto ANY_ERROR;

RNDIS_ERROR:
  printf ("failed\n");

ANY_ERROR:
  success = FALSE;

  if (h != NULL)
    usb_close (h);
  if (fd >= 0)
    close (fd);
  if (sock_fd >= 0)
    close (sock_fd);

OUT:
  return success;
}

static void
print_usage (const gchar *name)
{
  printf ("Usage:\n"
          "\t%s [-f] [-l] [bus-id device-id]\n\n"
          "\t-f           Do not run as a daemon\n"
          "\t-l           Device or hub is incapable of hi-speed operation\n\n",
          name);
}

gint
main(gint argc, gchar *argv[])
{
  gint c, nonopt_argc;
  struct usb_bus *busses, *bus;

  while ((c = getopt (argc, argv, "fl")) != -1)
    {
      switch (c)
        {
          case 'f':
            run_as_daemon = FALSE;
            break;
          case 'l':
            hispeed_capable = FALSE;
            break;
          default:
            print_usage (argv[0]);
            return EXIT_FAILURE;
        }
    }

  nonopt_argc = argc - optind;

  if (nonopt_argc)
    {
      if (nonopt_argc != 2)
        {
          print_usage (argv[0]);
          return EXIT_FAILURE;
        }

      bus_id = atoi (argv[optind]);
      dev_id = atoi (argv[optind + 1]);
    }
  else
    {
      printf ("scanning for a USB RNDIS device\n");
    }

  g_thread_init (NULL);

  /* make sure that the usb filesystem is up-to-date */
  sync ();

  usb_init ();

  usb_find_busses ();
  usb_find_devices ();

  busses = usb_get_busses ();
  for (bus = busses; bus; bus = bus->next)
    {
      struct usb_device *dev;
      gint cur_bus_id = atoi (bus->dirname);

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
              return (handle_device (dev, cur_bus_id, dev->devnum))
                ? EXIT_SUCCESS : EXIT_FAILURE;
            }
          else if (dev_id != -1)
            {
              fprintf (stderr, "device does not look like an RNDIS device\n");
              return EXIT_FAILURE;
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

  return EXIT_FAILURE;
}

