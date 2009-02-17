syncedatadir = ${datadir}/synce-hal
syncedata_DATA = dhclient.conf

dbusservicedir = "`pkg-config --variable=sysconfdir dbus-1`/dbus-1/system.d"
dbusservice_DATA = org.freedesktop.Hal.Device.Synce.conf

EXTRA_DIST = $(syncedata_DATA) $(dbusservice_DATA)
