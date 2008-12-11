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

#ifndef DCCM_CLIENT_H
#define DCCM_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum _DccmClientSignals DccmClientSignals;
enum _DccmClientSignals
{
  DEVICE_CONNECTED,
  DEVICE_DISCONNECTED,
  PASSWORD_REQUIRED,
  PASSWORD_REQUIRED_ON_DEVICE,
  PASSWORD_REJECTED,
  DEVICE_UNLOCKED,
  SERVICE_STARTING,
  SERVICE_STOPPING,
  DCCM_NUM_SIGNALS
};

typedef struct _DccmClient DccmClient;

typedef struct _DccmClientInterface DccmClientInterface;
struct _DccmClientInterface {
  GTypeInterface parent;

  guint signals[DCCM_NUM_SIGNALS];

  gboolean (*dccm_client_init_comms) (DccmClient *self);
  gboolean (*dccm_client_uninit_comms) (DccmClient *self);
  void (*dccm_client_provide_password) (DccmClient *self, const gchar *pdaname, const gchar *password);
  gboolean (*dccm_client_request_disconnect) (DccmClient *self, const gchar *pdaname);
};

GType dccm_client_get_type (void);

#define DCCM_CLIENT_TYPE (dccm_client_get_type())
#define DCCM_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DCCM_CLIENT_TYPE, DccmClient))
#define IS_DCCM_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DCCM_CLIENT_TYPE))
#define DCCM_CLIENT_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), DCCM_CLIENT_TYPE, DccmClientInterface))

gboolean
dccm_client_init_comms(DccmClient *self);

gboolean
dccm_client_uninit_comms(DccmClient *self);

void
dccm_client_provide_password(DccmClient *self, const gchar *pdaname, const gchar *password);

gboolean
dccm_client_request_disconnect(DccmClient *self, const gchar *pdaname);

G_END_DECLS

#endif /* DCCM_CLIENT_H */
