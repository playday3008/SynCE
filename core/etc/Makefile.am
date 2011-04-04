syncedatadir = ${datadir}/@PACKAGE_NAME@
syncedata_DATA = dhclient.conf

halservicefile = org.freedesktop.Hal.Device.Synce.conf
udevservicefile = org.synce.dccm.conf
udevservicestartfile = org.synce.dccm.service

dbusservicedir = "`pkg-config --variable=sysconfdir dbus-1`/dbus-1/system.d"
dbusservicestartdir = `pkg-config --variable=session_bus_services_dir dbus-1 | sed s/services/system-services/`
if USE_HAL
dbusservice_DATA = $(halservicefile)
else
dbusservice_DATA = $(udevservicefile)
dbusservicestart_DATA = $(udevservicestartfile)
endif

EXTRA_DIST = $(syncedata_DATA) $(halservicefile) $(udevservicefile)
