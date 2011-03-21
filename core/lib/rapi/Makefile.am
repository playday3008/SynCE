@SET_MAKE@

INCLUDES = -I$(top_srcdir)/src/support

## create this library
lib_LTLIBRARIES = librapi.la

## use these sources
librapi_la_SOURCES = backend_ops_1.c backend_ops_2.c \
	connection.c window.c rapi_api.c \
	misc.c irapistream.h irapistream.c \
	rapi2_api.c
librapi_la_CFLAGS = @LIBSYNCE_CFLAGS@ \
	@DBUS_CFLAGS@ \
	@HAL_CFLAGS@
librapi_la_LDFLAGS = -no-undefined -version-info 2:0:0
librapi_la_LIBADD = $(top_builddir)/src/support/libsupport.la \
	$(top_builddir)/src/backend_ops_1/librapiops1.la \
	$(top_builddir)/src/backend_ops_2/librapiops2.la \
	@LIBSYNCE_LIBS@ \
	@DBUS_LIBS@ \
	@HAL_LIBS@

include_HEADERS = rapi.h \
		rapitypes.h \
		rapi2.h \
		rapitypes2.h \
		irapistream.h

noinst_HEADERS = rapi_internal.h \
		rapi_ops.h \
		irapistream_internal.h

SUBDIRS = support backend_ops_1 backend_ops_2
