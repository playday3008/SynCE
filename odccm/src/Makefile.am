AM_CFLAGS = -Wall @LIBSYNCE_CFLAGS@ @GLIB_CFLAGS@ @GNET_CFLAGS@ @DBUS_CFLAGS@ @HAL_CFLAGS@
AM_LDFLAGS = @LIBSYNCE_LIBS@ @GLIB_LIBS@ @GNET_LIBS@ @DBUS_LIBS@ @HAL_LIBS@

sbin_PROGRAMS = odccm

BUILT_SOURCES = \
	odccm-device-glue.h \
	odccm-device-signals-marshal.h \
	odccm-device-signals-marshal.c \
	odccm-device-manager-glue.h \
	odccm-device-manager-signals-marshal.h \
	odccm-device-manager-signals-marshal.c

CLEANFILES = $(BUILT_SOURCES)

odccm_SOURCES = $(BUILT_SOURCES) \
		util.c util.h \
		odccm-errors.c odccm-errors.h \
		odccm-connection-broker.c odccm-connection-broker.h \
		odccm-device.c odccm-device.h \
		odccm-device-manager.c odccm-device-manager.h \
		odccm.c

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $<

%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c
