INCLUDES = -I$(top_srcdir)/lib/rapi

## create this library
noinst_LTLIBRARIES = libsupport.la

## use these sources
libsupport_la_SOURCES = \
	rapi_buffer.h   rapi_buffer.c  \
	rapi_context.h  rapi_context.c
libsupport_la_CFLAGS = @DBUS_CFLAGS@ @HAL_CFLAGS@ @LIBSYNCE_CFLAGS@
libsupport_la_LIBADD  = @DBUS_LIBS@ @HAL_LIBS@ @LIBSYNCE_LIBS@
