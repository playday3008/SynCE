#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "synce_gerrors.h"
#include <gio/gio.h>

static const GDBusErrorEntry synce_dccm_error_entries[] =
{
  {SYNCE_DCCM_ERROR_INVALID_ARGUMENT, "org.synce.dccm.Error.InvalidArgument"},
  {SYNCE_DCCM_ERROR_NOT_AVAILABLE,    "org.synce.dccm.Error.NotAvailable"},
  {SYNCE_DCCM_ERROR_DEVICE_LOCKED,    "org.synce.dccm.Error.DeviceLocked"},
};

/* Ensure that every error code has an associated D-Bus error name */
G_STATIC_ASSERT (G_N_ELEMENTS (synce_dccm_error_entries) == SYNCE_DCCM_N_ERRORS);

GQuark
synce_dccm_error_quark (void)
{
  static volatile gsize quark_volatile = 0;
  g_dbus_error_register_error_domain ("synce-dccm-error-quark",
                                      &quark_volatile,
                                      synce_dccm_error_entries,
                                      G_N_ELEMENTS (synce_dccm_error_entries));
  return (GQuark) quark_volatile;
}

