INCLUDES = -I$(top_srcdir)/lib @LIBSYNCE_CFLAGS@
METASOURCES = AUTO
bin_PROGRAMS = vdccm
vdccm_LDADD = $(top_builddir)/lib/libdescriptor.la
vdccm_LDFLAGS = @LIBSYNCE_LIBS@

noinst_HEADERS = windowscedevice.h devicemanager.h synceclient.h dccmserver.h \
					localserver.h connectionfilemanager.h cmdlineargs.h utils.h rapiserver.h rapiclient.h \
					windowscedevicefactory.h synceclientfactory.h rapihandshakeclient.h rapiprovisioningclient.h \
					rapihandshakeclientfactory.h rapiprovisioningclientfactory.h rapimessages.h rapiconnection.h \
			rapiproxy.h rapiproxyfactory.h rapiproxyconnection.h windowscedevicebase.h

vdccm_SOURCES = windowscedevice.cpp devicemanager.cpp synceclient.cpp \
	dccmserver.cpp localserver.cpp connectionfilemanager.cpp cmdlineargs.cpp utils.cpp \
	vdccm.cpp rapiserver.cpp rapiclient.cpp windowscedevicefactory.cpp \
	synceclientfactory.cpp rapihandshakeclient.cpp rapiprovisioningclient.cpp \
	rapihandshakeclientfactory.cpp rapiprovisioningclientfactory.cpp rapiconnection.cpp rapiproxy.cpp \
	rapiproxyfactory.cpp rapiproxyconnection.cpp

AM_CFLAGS = -pedantic-errors -Wall
AM_CXXFLAGS = -pedantic-errors -Wall

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/vdccm
	chmod u+s $(DESTDIR)$(bindir)/vdccm

man_MANS = vdccm.1

EXTRA_DIST = $(man_MANS)