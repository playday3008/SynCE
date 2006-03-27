INCLUDES = -I$(top_srcdir)/lib
METASOURCES = AUTO
bin_PROGRAMS = proxyclient triggerconnection
proxyclient_SOURCES = proxyclient.cpp proxyclientsocket.cpp
noinst_HEADERS = proxyclientsocket.h
proxyclient_LDADD = $(top_builddir)/lib/libdescriptor.la -lsynce
triggerconnection_SOURCES = triggerconnection.cpp
