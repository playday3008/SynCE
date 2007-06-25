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

#include "vdccm-client.h"
#include "dccm-client-signals-marshal.h"
#include "dccm-client.h"

static void dccm_client_interface_init (gpointer g_iface, gpointer iface_data);
G_DEFINE_TYPE_EXTENDED (VdccmClient, vdccm_client, G_TYPE_OBJECT, 0, G_IMPLEMENT_INTERFACE (DCCM_CLIENT_TYPE, dccm_client_interface_init))

typedef struct _VdccmClientPrivate VdccmClientPrivate;
struct _VdccmClientPrivate {
  GIOChannel *csock;
  guint read_watch_id;
  guint hup_watch_id;

  gboolean disposed;
};

#define VDCCM_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), VDCCM_CLIENT_TYPE, VdccmClientPrivate))


/* methods */

gboolean
vdccm_client_uninit_comms_impl(VdccmClient *self)
{
  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return FALSE;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return FALSE;
  }
  if (!priv->csock) {
    g_warning("Uninitialised object passed: %s", G_STRFUNC);
    return TRUE;
  }
  g_source_remove(priv->read_watch_id);
  g_source_remove(priv->hup_watch_id);
  g_io_channel_unref(priv->csock);
  priv->csock = NULL;

  return TRUE;
}

gboolean
vdccm_client_provide_password_impl(VdccmClient *self, gchar *pdaname, gchar *password)
{
  gchar *command_str;
  gsize bytes_written = 0;
  GError *error = NULL;
  GIOStatus ret;
  gboolean result = FALSE;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }
  if (!priv->csock) {
    g_warning("Uninitialised object passed: %s", G_STRFUNC);
    return result;
  }

  command_str = g_strjoin(NULL, "R", pdaname, "=", password, NULL);

  ret = g_io_channel_write_chars(priv->csock, command_str, strlen(command_str), &bytes_written, &error);

  if (ret != G_IO_STATUS_NORMAL) {
    g_critical("Error on write to vdccm: %s: %s", error->message, G_STRFUNC);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
      dccm_client_uninit_comms(DCCM_CLIENT(self));
    g_error_free(error);
    goto exit;
  }

  if (g_io_channel_flush(priv->csock, &error) != G_IO_STATUS_NORMAL) {
    g_critical("Error flushing vdccm socket: %s: %s", error->message, G_STRFUNC);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
      dccm_client_uninit_comms(DCCM_CLIENT(self));
    g_error_free(error);
    goto exit;
  }

  result = TRUE;
exit:
  g_free(command_str);
  return result;
}

gboolean
vdccm_client_request_disconnect_impl(VdccmClient *self, gchar *pdaname)
{
  gchar *command_str;
  gsize bytes_written = 0;
  GError *error = NULL;
  GIOStatus ret;
  gboolean result = FALSE;

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }
  if (!priv->csock) {
    g_warning("Uninitialised object passed: %s", G_STRFUNC);
    return result;
  }

  command_str = g_strjoin(NULL, "D", pdaname, NULL);

  ret = g_io_channel_write_chars(priv->csock, command_str, strlen(command_str), &bytes_written, &error);

  if (ret != G_IO_STATUS_NORMAL) {
    g_critical("Error on write to vdccm: %s: %s", error->message, G_STRFUNC);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
      dccm_client_uninit_comms(DCCM_CLIENT(self));
    g_error_free(error);
    goto exit;
  }

  if (g_io_channel_flush(priv->csock, &error) != G_IO_STATUS_NORMAL) {
    g_critical("Error flushing vdccm socket: %s: %s", error->message, G_STRFUNC);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
      dccm_client_uninit_comms(DCCM_CLIENT(self));
    g_error_free(error);
    goto exit;
  }

  result = TRUE;
exit:
  g_free(command_str);
  return result;
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
  gchar *synce_dir, *synce_path;
  SynceInfo *synce_info = NULL;

  if (!data) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }

  VdccmClient *self = VDCCM_CLIENT(data);
  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }
  if (!priv->csock) {
    g_warning("Uninitialised object passed: %s", G_STRFUNC);
    return result;
  }

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
    g_critical("Lost connection to vdccm");
    g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
    dccm_client_uninit_comms(DCCM_CLIENT(self));
    g_error_free(error);
    goto exit;
  }

  if (read_result == G_IO_STATUS_ERROR) {
    g_critical("Error on read from vdccm: %s: %s", error->message, G_STRFUNC);
    if (error->code == G_IO_CHANNEL_ERROR_PIPE)
      g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
      dccm_client_uninit_comms(DCCM_CLIENT(self));
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
	g_debug("Run connect for %s: %s", pdaname, G_STRFUNC);

	if (!synce_get_directory(&synce_dir)) {
	  g_critical("Unable to obtain synce directory: %s", G_STRFUNC);
	  return result;
	}
	synce_path = g_strjoin("/", synce_dir, pdaname, NULL);
	synce_info = synce_info_new(synce_path);
	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_CONNECTED], 0, pdaname, (gpointer)synce_info);
	g_free(synce_path);

	break;
      case 'D':
	/* pdaname disconnected */
	g_debug("Run disconnect for %s: %s", pdaname, G_STRFUNC);

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[DEVICE_DISCONNECTED], 0, pdaname);
	break;
      case 'P':
	/* pdaname requires password */
	g_debug("Run password required for %s: %s", pdaname, G_STRFUNC);

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REQUIRED], 0, pdaname);
	break;
      case 'R':
	/* pdaname given invalid password */
	g_debug("Run password rejected for %s: %s", pdaname, G_STRFUNC);

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[PASSWORD_REJECTED], 0, pdaname);
	break;
      case 'S':
	/* dccm stopping */
	g_debug("Run dccm stopping: %s", G_STRFUNC);

	g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
	break;
      default:
	g_critical("Received invalid command %c for pda %s: %s", command, pdaname, G_STRFUNC);
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
vdccm_client_hup_cb(GIOChannel *source,
	      GIOCondition condition,
	      gpointer data)
{
  if (!data) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return FALSE;
  }

  VdccmClient *self = VDCCM_CLIENT(data);

  g_critical("Connection to vdccm lost: %s", G_STRFUNC);
  g_signal_emit (self, DCCM_CLIENT_GET_INTERFACE (self)->signals[SERVICE_STOPPING], 0);
  dccm_client_uninit_comms(DCCM_CLIENT(self));
  return TRUE;
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

  if (!self) {
    g_warning("Invalid object passed: %s", G_STRFUNC);
    return result;
  }

  VdccmClientPrivate *priv = VDCCM_CLIENT_GET_PRIVATE (self);

  if (priv->disposed) {
    g_warning("Disposed object passed: %s", G_STRFUNC);
    return result;
  }
  if (priv->csock) {
    g_warning("Initialised object passed: %s", G_STRFUNC);
    return result;
  }

  if (!(synce_get_directory(&synce_dir))) {
    g_critical("Cannot obtain synce directory: %s", G_STRFUNC);
    return result;
  }

  csock_fd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (csock_fd < 0) {
    g_critical("Cannot obtain communication socket: %s: %s", strerror(errno), G_STRFUNC);
    return result;
  }

  csockaddr.sun_family = AF_UNIX;
  
  csock_path_len = g_snprintf(csockaddr.sun_path, 108, "%s/csock", synce_dir);
  if (csock_path_len > 108) {
    g_critical("Cannot open vdccm communication socket %s/csock: Path too long: %s", synce_dir, G_STRFUNC);
    return result;
  }

  if (connect(csock_fd, (struct sockaddr *)&csockaddr, sizeof(csockaddr)) < 0) {
    g_critical("Cannot connect to vdccm communication socket %s: %s: %s", csockaddr.sun_path, strerror(errno), G_STRFUNC);
    return result;
  }

  if (!(priv->csock = g_io_channel_unix_new(csock_fd))) {
    g_critical("Cannot convert vdccm socket fd to GIOChannel: %s", G_STRFUNC);
    shutdown(csock_fd, SHUT_RDWR);
    return result;
  }

  g_io_channel_set_close_on_unref(priv->csock, TRUE);

  if (g_io_channel_get_buffered(priv->csock))
    g_io_channel_set_buffered(priv->csock, 0);

  flags = g_io_channel_get_flags(priv->csock);
  flags = flags | G_IO_FLAG_NONBLOCK;
  if (g_io_channel_set_flags (priv->csock, flags, &error) != G_IO_STATUS_NORMAL) {
    if (error) {
      g_warning("Error setting flags on vdccm comm socket: %s: %s", error->message, G_STRFUNC);
      g_error_free(error);
      return result;
    }
  }

  priv->read_watch_id = g_io_add_watch(priv->csock, G_IO_IN,
			    (GIOFunc) vdccm_client_read_cb,
			    self);
  priv->hup_watch_id = g_io_add_watch(priv->csock, G_IO_HUP,
			    (GIOFunc) vdccm_client_hup_cb,
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
  VdccmClient *self = VDCCM_CLIENT(obj);

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
  iface->dccm_client_provide_password = (gboolean (*) (DccmClient *self, gchar *pdaname, gchar *password)) vdccm_client_provide_password_impl;
  iface->dccm_client_request_disconnect = (gboolean (*) (DccmClient *self, gchar *pdaname)) vdccm_client_request_disconnect_impl;
}
