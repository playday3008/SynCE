@SET_MAKE@

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/support

## create this library
noinst_LTLIBRARIES = librapiops2.la

## use these sources
librapiops2_la_SOURCES = file_management2.c file_access2.c misc2.c registry2.c password2.c database2.c invoke2.c
librapiops2_la_CFLAGS = @LIBSYNCE_CFLAGS@
librapiops2_la_LIBADD = @LIBSYNCE_LIBS@

noinst_HEADERS = backend_ops_2.h
