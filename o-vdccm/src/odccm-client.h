#ifndef ODCCM_CLIENT_H
#define ODCCM_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _OdccmClient OdccmClient;
struct _OdccmClient {
  GObject parent;
};

typedef struct _OdccmClientClass OdccmClientClass;
struct _OdccmClientClass {
  GObjectClass parent_class;
};

GType odccm_client_get_type (void);

#define ODCCM_CLIENT_TYPE (odccm_client_get_type())
#define ODCCM_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ODCCM_CLIENT_TYPE, OdccmClient))
#define ODCCM_CLIENT_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), ODCCM_CLIENT_TYPE, OdccmClientClass))
#define IS_ODCCM_CLIENT(obj) (G_TYPE_CHECK_TYPE ((obj), ODCCM_CLIENT_TYPE))
#define IS_ODCCM_CLIENT_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), ODCCM_CLIENT_TYPE))
#define ODCCM_CLIENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ODCCM_CLIENT_TYPE, OdccmClientClass))

G_END_DECLS

#endif /* ODCCM_CLIENT_H */
