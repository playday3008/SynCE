INCLUDES = -I$(top_srcdir)/lib

METASOURCES = AUTO

bin_PROGRAMS = proxyclient triggerconnection
noinst_HEADERS = proxyclientsocket.h

proxyclient_SOURCES = proxyclient.cpp proxyclientsocket.cpp
proxyclient_LDADD = $(top_builddir)/lib/libdescriptor.la
proxyclient_CXXFLAGS = @LIBSYNCE_CFLAGS@
proxyclient_LDFLAGS = @LIBSYNCE_LIBS@

triggerconnection_SOURCES = triggerconnection.cpp
