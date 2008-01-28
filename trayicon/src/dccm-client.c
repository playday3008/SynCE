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

#include "dccm-client.h"
#include "dccm-client-signals-marshal.h"

/* methods */

gboolean
dccm_client_init_comms(DccmClient *self)
{
  return DCCM_CLIENT_GET_INTERFACE (self)->dccm_client_init_comms(self);
}

gboolean
dccm_client_uninit_comms(DccmClient *self)
{
  return DCCM_CLIENT_GET_INTERFACE (self)->dccm_client_uninit_comms(self);
}

void
dccm_client_provide_password(DccmClient *self, gchar *pdaname, gchar *password)
{
  DCCM_CLIENT_GET_INTERFACE (self)->dccm_client_provide_password(self, pdaname, password);
}

gboolean
dccm_client_request_disconnect(DccmClient *self, gchar *pdaname)
{
  return DCCM_CLIENT_GET_INTERFACE (self)->dccm_client_request_disconnect(self, pdaname);
}



static void
dccm_client_base_init (gpointer klass)
{
  static gboolean initialized = FALSE;

  if (initialized)
    goto exit;

  /* create interface signals here. */
  initialized = TRUE;

  DccmClientInterface *iface = (DccmClientInterface *)klass;

  iface->signals[DEVICE_CONNECTED] = g_signal_new ("device-connected",
                  G_TYPE_FROM_INTERFACE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  dccm_client_marshal_VOID__STRING_POINTER,
                  G_TYPE_NONE, 2, G_TYPE_STRING, G_TYPE_POINTER);

  iface->signals[DEVICE_DISCONNECTED] = g_signal_new ("device-disconnected",
                  G_TYPE_FROM_INTERFACE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  iface->signals[PASSWORD_REQUIRED] = g_signal_new ("password-required",
                  G_TYPE_FROM_INTERFACE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  iface->signals[PASSWORD_REQUIRED_ON_DEVICE] = g_signal_new ("password-required-on-device",
                  G_TYPE_FROM_INTERFACE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  iface->signals[PASSWORD_REJECTED] = g_signal_new ("password-rejected",
                  G_TYPE_FROM_INTERFACE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE, 1, G_TYPE_STRING);

  iface->signals[SERVICE_STOPPING] = g_signal_new ("service-stopping",
                  G_TYPE_FROM_INTERFACE (klass),
                  G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

  iface->signals[DCCM_LAST_SIGNAL] = 0;

exit:
  return;
}

GType
dccm_client_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (DccmClientInterface),
      dccm_client_base_init,   /* base_init */
      NULL,   /* base_finalize */
      NULL,   /* class_init */
      NULL,   /* class_finalize */
      NULL,   /* class_data */
      0,
      0,      /* n_preallocs */
      NULL    /* instance_init */
    };
    type = g_type_register_static (G_TYPE_INTERFACE, "DccmClient", &info, 0);
  }
  return type;
}
