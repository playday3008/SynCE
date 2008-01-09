INCLUDES = -I$(top_srcdir)/lib @LIBSYNCE_CFLAGS@
METASOURCES = AUTO
bin_PROGRAMS = vdccm
vdccm_LDADD = $(top_builddir)/lib/libdescriptor.la
vdccm_LDFLAGS = @LIBSYNCE_LIBS@

if ENABLE_DESKTOP_INTEGRATION
INCLUDES += @GLIB_CFLAGS@ @DBUS_CFLAGS@ @HAL_CFLAGS@
vdccm_LDFLAGS += @GLIB_LIBS@ @DBUS_LIBS@ @HAL_LIBS@
endif

noinst_HEADERS = windowscedevice.h devicemanager.h synceclient.h dccmserver.h \
					localserver.h connectionfilemanager.h cmdlineargs.h utils.h rapiserver.h rapiclient.h \
					windowscedevicefactory.h synceclientfactory.h rapihandshakeclient.h rapiprovisioningclient.h \
					rapihandshakeclientfactory.h rapiprovisioningclientfactory.h rapimessages.h rapiconnection.h \
			rapiproxy.h rapiproxyfactory.h rapiproxyconnection.h windowscedevicebase.h

if ENABLE_DESKTOP_INTEGRATION
BUILT_SOURCES = \
	eventmanager-glue.h \
	eventmanager-signals-marshal.h \
	eventmanager-signals-marshal.c

CLEANFILES = $(BUILT_SOURCES)
endif

vdccm_SOURCES = windowscedevice.cpp devicemanager.cpp synceclient.cpp \
	dccmserver.cpp localserver.cpp connectionfilemanager.cpp cmdlineargs.cpp utils.cpp \
	vdccm.cpp rapiserver.cpp rapiclient.cpp windowscedevicefactory.cpp \
	synceclientfactory.cpp rapihandshakeclient.cpp rapiprovisioningclient.cpp \
	rapihandshakeclientfactory.cpp rapiprovisioningclientfactory.cpp rapiconnection.cpp rapiproxy.cpp \
	rapiproxyfactory.cpp rapiproxyconnection.cpp

if ENABLE_DESKTOP_INTEGRATION
vdccm_SOURCES += cutils.cpp cutils.h eventmanager.c eventmanager.h $(BUILT_SOURCES)
endif

AM_CFLAGS = -Wall
AM_CXXFLAGS = -pedantic-errors -Wall

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/vdccm
	chmod u+s $(DESTDIR)$(bindir)/vdccm

man_MANS = vdccm.1

EXTRA_DIST = $(man_MANS) eventmanager.xml

if ENABLE_DESKTOP_INTEGRATION
%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $<

%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c
endif
