@SET_MAKE@

AM_CFLAGS = @LIBSYNCE_CFLAGS@ -Wall
AM_LDFLAGS = @LIBSYNCE_LIBS@

INCLUDES = -I$(top_srcdir)/src/support

## create this library
lib_LTLIBRARIES = librapi.la

## use these sources
librapi_la_SOURCES = rapi.c connection.c window.c rapi_indirection.c rapi2.c \
	misc.c
librapi_la_LDFLAGS = -no-undefined -version-info 2:0:0
librapi_la_LIBADD = $(top_builddir)/src/support/libsupport.la \
	$(top_builddir)/src/rapi2/librapi2.la \
	$(top_builddir)/src/rapi/librapi1.la \
	@LIBSYNCE_LIBS@

include_HEADERS = rapi.h
noinst_HEADERS = rapi_internal.h rapi_types.h rapi_indirection.h

SUBDIRS = support rapi rapi2 config
