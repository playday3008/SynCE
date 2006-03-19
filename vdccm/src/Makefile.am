INCLUDES = -I$(top_srcdir)/lib $(SYNCE_INCLUDES)
METASOURCES = AUTO
bin_PROGRAMS = vdccm
vdccm_LDADD = $(top_builddir)/lib/libdescriptor.la $(SYNCE_LIB)
vdccm_LDFLAGS = $(SYNCE_LDFLAGS)
noinst_HEADERS = windowscedevice.h devicemanager.h synceclient.h dccmserver.h \
		localserver.h connectionfilemanager.h cmdlineargs.h utils.h rapiserver.h rapiclient.h
vdccm_SOURCES = windowscedevice.cpp devicemanager.cpp synceclient.cpp \
		dccmserver.cpp localserver.cpp connectionfilemanager.cpp cmdlineargs.cpp utils.cpp \
		vdccm.cpp rapiserver.cpp rapiclient.cpp \
    rapihandshakeclient.cpp rapiprovisioningclient.cpp
AM_CFLAGS = -pedantic-errors -Wall
AM_CXXFLAGS = -pedantic-errors -Wall
