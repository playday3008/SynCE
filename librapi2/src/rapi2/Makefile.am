@SET_MAKE@

AM_CFLAGS = @LIBSYNCE_CFLAGS@ -Wall

INCLUDES = -I$(top_srcdir)/src -I$(top_srcdir)/src/support

## create this library
noinst_LTLIBRARIES = librapi2.la

## use these sources
librapi2_la_SOURCES = file_management2.c file_access2.c misc2.c registry2.c

noinst_HEADERS = rapi2_api.h
