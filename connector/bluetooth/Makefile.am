if ENABLE_BLUETOOTH_SUPPORT

haldir = @hal_addon_dir@
hal_SCRIPTS = \
        hal-synce-bluetooth

pppdir = ${sysconfdir}/ppp
pppupdir = ${pppdir}/ip-up.d
pppdowndir = ${pppdir}/ip-down.d
ppppeerdir = ${pppdir}/peers

pppup_SCRIPTS = synce-bt-ipup
pppdown_SCRIPTS = synce-bt-ipdown
ppppeer_DATA = synce-bt-peer

EXTRA_DIST = README

else

EXTRA_DIST = synce-bt-peer \
	synce-bt-ipup.in \
	synce-bt-ipdown.in \
	README \
	hal-synce-bluetooth.in

endif
