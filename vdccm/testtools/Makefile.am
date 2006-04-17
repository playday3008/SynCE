INCLUDES = -I$(top_srcdir)/lib $(SYNCE_INCLUDES)
METASOURCES = AUTO
bin_PROGRAMS = proxyclient triggerconnection
proxyclient_SOURCES = proxyclient.cpp proxyclientsocket.cpp
noinst_HEADERS = proxyclientsocket.h
proxyclient_LDADD = $(top_builddir)/lib/libdescriptor.la $(SYNCE_LIB)
proxyclient_LDFLAGS = $(SYNCE_LDFLAGS)
triggerconnection_SOURCES = triggerconnection.cpp
