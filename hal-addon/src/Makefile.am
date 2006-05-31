AM_CFLAGS = @LIBSYNCE_CFLAGS@ @LIBRAPI2_CFLAGS@ @GLIB_CFLAGS@ @HAL_CFLAGS@
AM_LDFLAGS = @LIBSYNCE_LIBS@ @LIBRAPI2_LIBS@ @GLIB_LIBS@ @HAL_LIBS@

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

hald_addon_pocketpc_LDADD = -lutil

install-exec-local:
	- $(INSTALL) -m 4755 -o root -g root hald-addon-pocketpc $(HAL_ADDONS_DIR)
