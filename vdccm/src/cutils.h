#ifndef CUTILS_H
#define CUTILS_H

#include <glib.h>

G_BEGIN_DECLS

void _vdccm_acquire_root_privileges ();
void _vdccm_drop_root_privileges ();

void _vdccm_ce_device_base_disconnect (gpointer ce_device_base);
gchar *_vdccm_ce_device_base_get_name (gpointer ce_device_base);

G_END_DECLS

#endif /* CUTILS_H */
