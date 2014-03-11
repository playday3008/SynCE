#ifndef SYNCE_ERRORS_H
#define SYNCE_ERRORS_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum
{
  SYNCE_DCCM_ERROR_INVALID_ARGUMENT,
  SYNCE_DCCM_ERROR_NOT_AVAILABLE,
  SYNCE_DCCM_ERROR_DEVICE_LOCKED,
  SYNCE_DCCM_N_ERRORS,
} SynceDccmError;

GQuark synce_dccm_error_quark (void);
#define SYNCE_DCCM_ERROR synce_dccm_error_quark ()


#if !USE_GDBUS
GType synce_dccm_error_get_type (void);
#define SYNCE_DCCM_TYPE_ERROR (synce_dccm_error_get_type ())
#endif

G_END_DECLS

#endif /* SYNCE_ERRORS_H */
