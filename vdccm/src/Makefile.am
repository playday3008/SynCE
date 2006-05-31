AM_CXXFLAGS = @LIBSYNCE_CFLAGS@
AM_LDFLAGS = @LIBSYNCE_LIBS@

INCLUDES = -I$(top_srcdir)/lib

METASOURCES = AUTO
bin_PROGRAMS = vdccm

vdccm_LDADD = $(top_builddir)/lib/libdescriptor.la

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

install-exec-hook:
	chown root $(DESTDIR)$(bindir)/vdccm
	chmod u+s $(DESTDIR)$(bindir)/vdccm
