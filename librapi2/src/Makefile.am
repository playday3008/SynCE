@SET_MAKE@

INCLUDES = -I$(top_srcdir)/src/support

## create this library
lib_LTLIBRARIES = librapi.la

## use these sources
librapi_la_SOURCES = rapi.c connection.c window.c rapi_indirection.c rapi2.c \
	misc.c irapistream.h irapistream.c
librapi_la_CFLAGS = @LIBSYNCE_CFLAGS@
librapi_la_LDFLAGS = -no-undefined -version-info 2:0:0
librapi_la_LIBADD = $(top_builddir)/src/support/libsupport.la \
	$(top_builddir)/src/rapi2/librapi2.la \
	$(top_builddir)/src/rapi/librapi1.la \
	@LIBSYNCE_LIBS@

include_HEADERS = rapi.h
noinst_HEADERS = rapi_internal.h rapi_indirection.h

SUBDIRS = support rapi rapi2 config
