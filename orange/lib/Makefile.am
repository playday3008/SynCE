## useful flags
AM_CFLAGS = -g -ansi -Wall -Wsign-compare -Wno-long-long -Werror @CFLAGS@ @APPLE_CFLAGS@ -I..

## create this library
lib_LTLIBRARIES = liborange.la

## use these sources
liborange_la_SOURCES = \
	dllinflate.c \
	extract.c \
	helper.c \
	separate.c \
	squeeze.c \
	suf.c \
	synce-appmgr.h \
	synce-appmgr-internal.h \
	tomtom.c

EXTRA_liborange_la_SOURCES = inno.c vise.c
liborange_la_LIBADD = @INNO_LIBADD@ @VISE_LIBADD@
liborange_la_DEPENDENCIES = @INNO_LIBADD@ @VISE_LIBADD@

liborange_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = liborange.h

