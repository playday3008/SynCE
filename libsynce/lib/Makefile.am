## useful flags
AM_CFLAGS = -ggdb -Wall -Wno-long-long -ansi @CFLAGS@ @APPLE_CFLAGS@ -I..

## create this library
lib_LTLIBRARIES = libsynce.la

## use these sources
libsynce_la_SOURCES = \
	bswap.c \
	datetype.c \
	errors.c \
	files.c \
	filetime.c \
	password.c \
	synce.h \
	synce_log.h synce_log.c \
	synce_socket.h synce_socket.c \
	wstr.c

libsynce_la_LDFLAGS = -no-undefined -version-info 0:0:0
libsynce_la_LIBADD  = @LTLIBICONV@ @LTLIBOBJS@ -lm

include_HEADERS = \
	synce.h synce_log.h synce_socket.h synce_types.h

noinst_PROGRAMS = ftconv

ftconv_SOURCES = ftconv.c
ftconv_LDADD = libsynce.la

