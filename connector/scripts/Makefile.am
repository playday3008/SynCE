# new one

halfdi = 10-synce.fdi
udevrules = synce.rules

udevscripts = \
	synce-udev-serial \
	synce-udev-rndis

halscripts = \
	hal-synce-rndis \
	hal-synce-serial 

if USE_HAL
haldatadir = ${datadir}/hal/fdi/policy/20thirdparty
haldata_DATA = $(halfdi)

haldir = @hal_addon_dir@
hal_SCRIPTS = $(halscripts)

else
haldir = $(libexecdir)
udevrulesdir = ${sysconfdir}/udev/rules.d
udevrules_DATA = $(udevrules)
libexec_SCRIPTS = $(udevscripts)
endif

EXTRA_DIST = $(halfdi)

