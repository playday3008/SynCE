## useful flags
CFLAGS = -ggdb -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -I..

## create this library
noinst_LTLIBRARIES = libsupport.la

## use these sources
libsupport_la_SOURCES = \
	rapi_endian.h rapi_endian.c \
	rapi_log.h rapi_log.c \
	rapi_wstr.h rapi_wstr.c

include_HEADERS = \
	rapi_wstr.h 
