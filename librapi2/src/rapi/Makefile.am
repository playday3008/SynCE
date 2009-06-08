@SET_MAKE@

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/support

## create this library
noinst_LTLIBRARIES = librapi1.la

## use these sources
librapi1_la_SOURCES = database.c file_access.c file_management.c \
		invoke.c misc.c password.c registry.c
librapi1_la_CFLAGS = @LIBSYNCE_CFLAGS@
librapi1_la_LIBADD = @LIBSYNCE_LIBS@

noinst_HEADERS = rapi_api.h
