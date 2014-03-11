#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "synce_gerrors.h"
#include <gio/gio.h>

#if USE_GDBUS
static const GDBusErrorEntry synce_dccm_error_entries[] =
{
  {SYNCE_DCCM_ERROR_INVALID_ARGUMENT, "org.synce.dccm.Error.InvalidArgument"},
  {SYNCE_DCCM_ERROR_NOT_AVAILABLE,    "org.synce.dccm.Error.NotAvailable"},
  {SYNCE_DCCM_ERROR_DEVICE_LOCKED,    "org.synce.dccm.Error.DeviceLocked"},
};

/* Ensure that every error code has an associated D-Bus error name */
G_STATIC_ASSERT (G_N_ELEMENTS (synce_dccm_error_entries) == SYNCE_DCCM_N_ERRORS);
#endif

GQuark
synce_dccm_error_quark (void)
{
#if USE_GDBUS
  static volatile gsize quark_volatile = 0;
  g_dbus_error_register_error_domain ("synce-dccm-error-quark",
                                      &quark_volatile,
                                      synce_dccm_error_entries,
                                      G_N_ELEMENTS (synce_dccm_error_entries));
  return (GQuark) quark_volatile;
#else /* USE_GDBUS */
  static GQuark quark = 0;
  if (quark == 0) {
    quark = g_quark_from_static_string ("synce-dccm-error-quark");
    dbus_g_error_domain_register (SYNCE_DCCM_ERROR, "org.synce.dccm.Error", SYNCE_DCCM_TYPE_ERROR);
  }
  return quark;
#endif
}


#if !USE_GDBUS

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }

GType
synce_dccm_error_get_type (void)
{
  static GType etype = 0;

  if (etype == 0)
  {
      static const GEnumValue values[] =
      {

        ENUM_ENTRY (SYNCE_DCCM_ERROR_INVALID_ARGUMENT, "InvalidArgument"),
        ENUM_ENTRY (SYNCE_DCCM_ERROR_NOT_AVAILABLE, "NotAvailable"),
        ENUM_ENTRY (SYNCE_DCCM_ERROR_DEVICE_LOCKED, "DeviceLocked"),
        { 0, 0, 0 }
      };

      g_assert (SYNCE_DCCM_N_ERRORS == G_N_ELEMENTS (values) - 1);

      etype = g_enum_register_static ("SynceDccmError", values);
  }

  return etype;
}


#endif

