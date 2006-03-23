INCLUDES = -I$(top_srcdir)/lib $(SYNCE_INCLUDES)
METASOURCES = AUTO
bin_PROGRAMS = vdccm
vdccm_LDADD = $(top_builddir)/lib/libdescriptor.la $(SYNCE_LIB)
vdccm_LDFLAGS = $(SYNCE_LDFLAGS)
noinst_HEADERS = windowscedevice.h devicemanager.h synceclient.h dccmserver.h \
		localserver.h connectionfilemanager.h cmdlineargs.h utils.h rapiserver.h rapiclient.h \
		windowscedevicefactory.h synceclientfactory.h rapihandshakeclient.h rapiprovisioningclient.h \
		rapihandshakeclientfactory.h rapiprovisioningclientfactory.h rapimessages.h rapiconnection.h
vdccm_SOURCES = windowscedevice.cpp devicemanager.cpp synceclient.cpp \
		dccmserver.cpp localserver.cpp connectionfilemanager.cpp cmdlineargs.cpp utils.cpp \
		vdccm.cpp rapiserver.cpp rapiclient.cpp windowscedevicefactory.cpp \
		synceclientfactory.cpp rapihandshakeclient.cpp rapiprovisioningclient.cpp \
		rapihandshakeclientfactory.cpp rapiprovisioningclientfactory.cpp rapiconnection.cpp
AM_CFLAGS = -pedantic-errors -Wall
AM_CXXFLAGS = -pedantic-errors -Wall

install-exec-hook:
	chown root $(bindir)/vdccm
	chmod u+s $(bindir)/vdccm
