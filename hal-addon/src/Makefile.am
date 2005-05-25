
INCLUDES = $(HAL_CFLAGS) $(GLIB_CFLAGS)

AM_CFLAGS = -Wall -g

noinst_PROGRAMS = hald-addon-pocketpc

hald_addon_pocketpc_SOURCES = \
	misc.h			\
	callout.c       \
	hal.h			\
	hal.c			\
	udevinfo.h		\
	udevinfo.c		\
	info.h			\
	info.c

hald_addon_pocketpc_LDFLAGS = 

hald_addon_pocketpc_LDADD = $(HAL_LIBS) $(GLIB_LIBS) $(synce_libs) $(rapi_libs) -lutil

install-exec-local:
	- $(INSTALL) -m 4755 -o root -g root hald-addon-pocketpc $(HAL_ADDONS_DIR)
