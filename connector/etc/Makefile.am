syncedatadir = ${datadir}/@PACKAGE_NAME@
syncedata_DATA = dhclient.conf

halservicefile = org.freedesktop.Hal.Device.Synce.conf
udevservicefile = org.synce.dccm.conf

dbusservicedir = "`pkg-config --variable=sysconfdir dbus-1`/dbus-1/system.d"
if USE_HAL
dbusservice_DATA = $(halservicefile)
else
dbusservice_DATA = $(udevservicefile)
endif

EXTRA_DIST = $(syncedata_DATA) $(halservicefile) $(udevservicefile)
