INCLUDES = -I$(top_srcdir)/lib

METASOURCES = AUTO

bin_PROGRAMS = proxyclient triggerconnection
noinst_HEADERS = proxyclientsocket.h

proxyclient_SOURCES = proxyclient.cpp proxyclientsocket.cpp
proxyclient_LDADD = $(top_builddir)/lib/libdescriptor.la $(SYNCE_LIB)
proxyclient_CXXFLAGS = @LIBSYNCE_CFLAGS@


triggerconnection_SOURCES = triggerconnection.cpp
proxyclient_LDFLAGS = $(SYNCE_LDFLAGS)
