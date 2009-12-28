
halfdi = 10-synce.fdi

if USE_HAL
haldatadir = ${datadir}/hal/fdi/policy/20thirdparty
haldata_DATA = $(halfdi)

haldir = @hal_addon_dir@
else
haldir = $(libexecdir)
endif

hal_SCRIPTS = \
	hal-synce-rndis \
	hal-synce-serial 

EXTRA_DIST = $(halfdi)

