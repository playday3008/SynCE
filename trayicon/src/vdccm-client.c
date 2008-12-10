/*
Copyright (c) 2007 Mark Ellis <mark@mpellis.org.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <glib.h>
#include <synce.h>
#include <rapi.h>

#include "vdccm-client.h"
#include "dccm-client-signals-marshal.h"
#include "dccm-client.h"
#include "device.h"
#include "utils.h"

static void dccm_client_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_EXTENDED (VdccmClient, vdccm_client, G_TYPE_OBJECT, 0, G_IMPLEMENT_INTERFACE (DCCM_CLIENT_TYPE, dccm_client_interface_init))

typedef struct _VdccmClientPrivate VdccmClientPrivate;
struct _VdccmClientPrivate {
  GIOChannel *csock;
  guint read_watch_id;

  gboolean disposed;
};

#define VDCCM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), VDCCM_CLIENT_TYPE, VdccmClientPrivate))


/* methods */

gboolean
vdccm_client_uninit_comms_impl(VdccmClient *self)
{
  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return FALSE;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return FALSE;
  }
  if (!priv->csock) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return TRUE;
  }
  g_source_remove(priv->read_watch_id);
  g_io_channel_unref(priv->csock);
  priv->csock = NULL;

  return TRUE;
}

void
vdccm_client_provide_password_impl(VdccmClient *self, const gchar *pdaname, const gchar *password)
{
  gchar *command_str;
  gsize bytes_written = 0;
  GError *error = NULL;
  GIOStatus ret;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return;
  }
  if (!priv->csock) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return;
  }

  command_str = g_strjoin(NULL, "R", pdaname, "=", password, NULL);

  ret = g_io_channel_write_chars(priv->csock, command_str, strlen(command_str), &bytes_written, &error);

  if (ret != G_IO_STATUS_NORMAL) {
    g_critical("%s: Error on write to vdccm: %s", G_STRFUNC, error->message);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
    g_error_free(error);
    goto exit;
  }

  if (g_io_channel_flush(priv->csock, &error) != G_IO_STATUS_NORMAL) {
    g_critical("%s: Error flushing vdccm socket: %s", G_STRFUNC, error->message);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
    g_error_free(error);
    goto exit;
  }

exit:
  g_free(command_str);
  return;
}

gboolean
vdccm_client_request_disconnect_impl(VdccmClient *self, const gchar *pdaname)
{
  gchar *command_str;
  gsize bytes_written = 0;
  GError *error = NULL;
  GIOStatus ret;
  gboolean result = FALSE;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return result;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return result;
  }
  if (!priv->csock) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return result;
  }

  command_str = g_strjoin(NULL, "D", pdaname, NULL);

  ret = g_io_channel_write_chars(priv->csock, command_str, strlen(command_str), &bytes_written, &error);

  if (ret != G_IO_STATUS_NORMAL) {
    g_critical("%s: Error on write to vdccm: %s", G_STRFUNC, error->message);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
    g_error_free(error);
    goto exit;
  }

  if (g_io_channel_flush(priv->csock, &error) != G_IO_STATUS_NORMAL) {
    g_critical("%s: Error flushing vdccm socket: %s", G_STRFUNC, error->message);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
    g_error_free(error);
    goto exit;
  }

  result = TRUE;
exit:
  g_free(command_str);
  return result;
}

static WmDevice *
vdccm_device_connected(const gchar *pdaname, gboolean locked)
{
  WmDevice *device;
  SynceInfo *info = NULL;

  if (locked) {
          device = g_object_new(WM_DEVICE_TYPE,
                                "object-name", pdaname,
                                "dccm-type", "vdccm",
                                "name", pdaname,
                                "connection_status", DEVICE_STATUS_PASSWORD_REQUIRED,
                                NULL);
          return device;
  }

  info = synce_info_new(pdaname);
  if (!info) {
    g_warning("%s: Error getting SynceInfo from vdccm new device", G_STRFUNC);
    return NULL;
  }

  device = g_object_new(WM_DEVICE_TYPE,
			"object-name", info->name,
                        "dccm-type", "vdccm",
                        "connection_status", DEVICE_STATUS_CONNECTED,
			"name", info->name,
			"os-major", info->os_version,
			"os-minor", 0,
			"build-number", info->build_number,
			"processor-type", info->processor_type,
			"partner-id-1", info->partner_id_1,
			"partner-id-2", info->partner_id_2,
			"class", info->os_name,
			"hardware", info->model,
			"password", info->password,
			"key", info->key,
			"dccm-pid", info->dccm_pid,
			"ip", info->ip,
			"transport", info->transport,
			NULL);

  synce_info_destroy(info);

  if (!device) {
    g_warning("%s: Error creating new device", G_STRFUNC);
    return NULL;
  }

  return device;
}


gboolean
vdccm_client_read_cb(GIOChannel *source,
	      GIOCondition condition,
	      gpointer data)
{
  GIOStatus read_result;
  GError *error = NULL;
  gsize bytes_read;
  gsize count = sizeof(gchar) * 256;
  gchar *buffer = g_malloc0(count);
  gchar **command_and_pda_list;
  gchar command;
  gchar *pdaname;
  gint i;
  gboolean result = FALSE;

  if (!data) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return result;
  }

  VdccmClient *self = VDCCM_CLIENT(data);
  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return result;
  }
  if (!priv->csock) {
    g_warning("%s: Uninitialised object passed", G_STRFUNC);
    return result;
  }

  WmDevice *new_dev = NULL;

  /* messages split by ;
     Cpdaname - connected to pdaname

     Dpdaname - disconnected from pdaname

     Ppdaname - need password for pdaname, respond with Rpdaname=password

     Rpdaname - invalid password for pdaname

     S       - dccm stopped ?

     send Dpdaname   - disconnect pdaname
  */

  read_result = g_io_channel_read_chars(source,
					buffer,
					count,
					&bytes_read,
					&error);

  if (read_result == G_IO_STATUS_EOF) {
    g_critical("%s: Lost connection to vdccm", G_STRFUNC);
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
    goto exit;
  }

  if (read_result == G_IO_STATUS_ERROR) {
    g_critical("%s: Error on read from vdccm: %s", G_STRFUNC, error->message);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
    g_error_free(error);
    goto exit;
  }

  command_and_pda_list = g_strsplit(buffer, ";", 0);

  i = 0;
  while (command_and_pda_list[i] && command_and_pda_list[i][0])
    {
      command = command_and_pda_list[i][0];
      pdaname = &(command_and_pda_list[i][1]);

      switch(command) {
      case 'C':
	/* pdaname connected */
	g_debug("%s: Run connect for %s", G_STRFUNC, pdaname);

	new_dev = vdccm_device_connected(pdaname, FALSE);

        /* 
           This is ugly, but vdccm doesn't really fit the model any more
        */

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_DISCONNECTED], 0, pdaname);
	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, pdaname, (gpointer)new_dev);

	break;
      case 'D':
	/* pdaname disconnected */
	g_debug("%s: Run disconnect for %s", G_STRFUNC, pdaname);

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_DISCONNECTED], 0, pdaname);
	break;
      case 'P':
	/* pdaname requires password */
	g_debug("%s: Run password required for %s", G_STRFUNC, pdaname);

	new_dev = vdccm_device_connected(pdaname, TRUE);
	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, pdaname, (gpointer)new_dev);
	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, pdaname);
	break;
      case 'R':
	/* pdaname given invalid password */
	g_debug("%s: Run password rejected for %s", G_STRFUNC, pdaname);

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REJECTED], 0, pdaname);
	break;
      case 'S':
	/* dccm stopping */
	g_debug("%s: Run dccm stopping", G_STRFUNC);

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
	break;
      default:
	g_critical("%s: Received invalid command %c for pda %s", G_STRFUNC, command, pdaname);
	break;
      }
      i++;
    }

  g_strfreev(command_and_pda_list);
  g_free(buffer);
  result = TRUE;
exit:
  return result;
}


gboolean
vdccm_client_init_comms_impl(VdccmClient *self)
{
  gboolean result = FALSE;
  gchar *synce_dir;
  GError *error = NULL;
  int csock_fd, csock_path_len;
  struct sockaddr_un csockaddr;
  GIOFlags flags;
  GIOStatus status;

  if (!self) {
    g_warning("%s: Invalid object passed", G_STRFUNC);
    return result;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("%s: Disposed object passed", G_STRFUNC);
    return result;
  }
  if (priv->csock) {
    g_warning("%s: Initialised object passed", G_STRFUNC);
    return result;
  }

  if (!(synce_get_directory(&synce_dir))) {
    g_critical("%s: Cannot obtain synce directory", G_STRFUNC);
    return result;
  }

  csock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (csock_fd < 0) {
    g_critical("%s: Cannot obtain communication socket: %s", G_STRFUNC, strerror(errno));
    return result;
  }

  csockaddr.sun_family = AF_UNIX;
  
  csock_path_len = g_snprintf(csockaddr.sun_path, 108, "%s/csock", synce_dir);
  if (csock_path_len > 108) {
    g_critical("%s: Cannot open vdccm communication socket %s/csock: Path too long", G_STRFUNC, synce_dir);
    return result;
  }

  if (connect(csock_fd, (struct sockaddr *)&csockaddr, sizeof(csockaddr)) < 0) {
    g_critical("%s: Cannot connect to vdccm communication socket %s: %s", G_STRFUNC, csockaddr.sun_path, strerror(errno));
    return result;
  }

  if (!(priv->csock = g_io_channel_unix_new(csock_fd))) {
    g_critical("%s: Cannot convert vdccm socket fd to GIOChannel", G_STRFUNC);
    shutdown(csock_fd, SHUT_RDWR);
    return result;
  }

  g_io_channel_set_close_on_unref(priv->csock, TRUE);

  if ((status = g_io_channel_set_encoding(priv->csock, NULL, &error)) != G_IO_STATUS_NORMAL) {
    g_critical("%s: Error setting NULL encoding on control socket: %s", G_STRFUNC, error->message);
    g_error_free(error);
  }

  if (g_io_channel_get_buffered(priv->csock))
    g_io_channel_set_buffered(priv->csock, 0);

  flags = g_io_channel_get_flags(priv->csock);
  flags = flags | G_IO_FLAG_NONBLOCK;
  if (g_io_channel_set_flags (priv->csock, flags, &error) != G_IO_STATUS_NORMAL) {
    if (error) {
      g_warning("%s: Error setting flags on vdccm comm socket: %s", G_STRFUNC, error->message);
      g_error_free(error);
      return result;
    }
  }

  priv->read_watch_id = g_io_add_watch(priv->csock, G_IO_IN,
			    (GIOFunc) vdccm_client_read_cb,
			    self);
  result = TRUE;
  return result;
}


/* class & instance functions */

static void
vdccm_client_init(VdccmClient *self)
{
  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);
  priv->csock = NULL;
}

static void
vdccm_client_dispose (GObject *obj)
{
  VdccmClient *self = VDCCM_CLIENT(obj);
  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (obj);

  if (priv->disposed) {
    return;
  }

  if (priv->csock)
    dccm_client_uninit_comms(DCCM_CLIENT(self));
  priv->disposed = TRUE;

  /* unref other objects */

  if (G_OBJECT_CLASS (vdccm_client_parent_class)->dispose)
    G_OBJECT_CLASS (vdccm_client_parent_class)->dispose (obj);
}

static void
vdccm_client_finalize (GObject *obj)
{
  if (G_OBJECT_CLASS (vdccm_client_parent_class)->finalize)
    G_OBJECT_CLASS (vdccm_client_parent_class)->finalize (obj);
}

static void
vdccm_client_class_init (VdccmClientClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = vdccm_client_dispose;
  gobject_class->finalize = vdccm_client_finalize;

  g_type_class_add_private (klass, sizeof (VdccmClientPrivate));
}

static void
dccm_client_interface_init (gpointer g_iface, gpointer iface_data)
{
  DccmClientInterface *iface = (DccmClientInterface *)g_iface;

  iface->dccm_client_init_comms = (gboolean (*) (DccmClient *self)) vdccm_client_init_comms_impl;
  iface->dccm_client_uninit_comms = (gboolean (*) (DccmClient *self)) vdccm_client_uninit_comms_impl;
  iface->dccm_client_provide_password = (void (*) (DccmClient *self, const gchar *pdaname, const gchar *password)) vdccm_client_provide_password_impl;
  iface->dccm_client_request_disconnect = (gboolean (*) (DccmClient *self, const gchar *pdaname)) vdccm_client_request_disconnect_impl;
}
