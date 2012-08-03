@SET_MAKE@

lib_LTLIBRARIES = libsynce.la
libsynce_la_SOURCES = 
libsynce_la_LDFLAGS = -no-undefined -version-info 0:0:0
libsynce_la_LIBADD  = -lm @LTLIBICONV@ @DBUS_LIBS@ \
		$(top_builddir)/lib/utils/libutils.la \
		$(top_builddir)/lib/rapi/librapi.la

SUBDIRS = utils rapi
