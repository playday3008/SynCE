halfdi = 10-synce.fdi

syncedatadir = ${datadir}/@PACKAGE_NAME@
syncedata_DATA = synceconnector.py

if USE_HAL
haldatadir = ${datadir}/hal/fdi/policy/20thirdparty
haldata_DATA = $(halfdi)

halscriptsdir = @hal_addon_dir@
halscripts_SCRIPTS = \
	hal-synce-rndis \
	hal-synce-serial 

else

udevrulesdir = @udev_dir@/rules.d
udevrules_DATA = 85-synce.rules

udevstartersdir = @udev_dir@
udevstarters_SCRIPTS = \
	synce-udev-serial \
	synce-udev-rndis

syncedata_SCRIPTS = \
	udev-synce-rndis \
	udev-synce-serial

bin_SCRIPTS = synce-serial
endif

EXTRA_DIST = $(halfdi)

