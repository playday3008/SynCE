## useful flags
AM_CFLAGS = -ggdb -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -I..
## @APPLE_CFLAGS@

## create this library
lib_LTLIBRARIES = librra.la

## use these sources
librra_la_SOURCES = \
	appointment.c \
	dbstream.h dbstream.c \
	contact.c \
	librra.h   librra.c \
	rrac.h     rrac.c \
	strbuf.h   strbuf.c

librra_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = \
	librra.h


