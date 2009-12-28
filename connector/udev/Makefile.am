
udevfile = synce.rules
udevscripts = \
	synce-udev-serial \
	synce-udev-rndis

if !USE_HAL
udevrulesdir = ${sysconfdir}/udev/rules.d
udevrules_DATA = $(udevfile)
libexec_SCRIPTS = $(udevscripts)
endif

haldir = @hal_addon_dir@
##hal_SCRIPTS = \
##	hal-synce-rndis \
##	hal-synce-serial 

EXTRA_DIST = $(udevfile) $(udevscripts)

