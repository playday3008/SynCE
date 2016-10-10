
## create this library
noinst_LTLIBRARIES = libsupport.la

## use these sources
libsupport_la_SOURCES = \
	rapi_buffer.h   rapi_buffer.c  \
	rapi_context.h  rapi_context.c
libsupport_la_CFLAGS = -I$(top_srcdir)/lib/utils \
			-I$(top_builddir)/lib/utils \
			-I$(top_srcdir)/lib/rapi \
			@GLIB_CFLAGS@

