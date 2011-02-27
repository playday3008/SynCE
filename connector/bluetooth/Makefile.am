if ENABLE_BLUETOOTH_SUPPORT


pppdir = ${sysconfdir}/ppp
pppupdir = ${pppdir}/ip-up.d
pppdowndir = ${pppdir}/ip-down.d
ppppeerdir = ${pppdir}/peers

EXTRA_DIST = README
ppppeer_DATA = synce-bt-peer

if USE_HAL
halscriptsdir = @hal_addon_dir@
halscripts_SCRIPTS = hal-synce-bluetooth

pppup_SCRIPTS = synce-hal-bt-ipup
pppdown_SCRIPTS = synce-hal-bt-ipdown
else
syncedatadir = ${datadir}/@PACKAGE_NAME@

pppup_SCRIPTS = synce-udev-bt-ipup
endif

else

EXTRA_DIST = synce-bt-peer \
	synce-hal-bt-ipup.in \
	synce-hal-bt-ipdown.in \
	README \
	hal-synce-bluetooth.in \
	synce-udev-bt-ipup.in

endif
