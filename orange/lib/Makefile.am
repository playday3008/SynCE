## useful flags
AM_CFLAGS = -g -ansi -Wall -Wno-long-long -Werror @CFLAGS@ @APPLE_CFLAGS@ -I..

## create this library
lib_LTLIBRARIES = liborange.la

## use these sources
liborange_la_SOURCES = \
	dllinflate.c \
	extract.c \
	helper.c \
	inno.c \
	separate.c \
	squeeze.c \
	suf.c \
	synce-appmgr.h \
	synce-appmgr-internal.h

liborange_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = liborange.h

