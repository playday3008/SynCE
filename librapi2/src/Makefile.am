## useful flags
CFLAGS = -ggdb -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -Isupport
@SET_MAKE@

## create this library
lib_LTLIBRARIES = librapi.la

## use these sources
librapi_la_SOURCES = \
	database.c                     \
	file_access.c                  \
	file_management.c              \
	misc.c                         \
	rapi.c                         \
	registry.c                     \
	window.c                       \
	rapi_internal.h

librapi_la_LDFLAGS = -no-undefined -version-info 2:0:0
librapi_la_LIBADD  = config/libconfig.la support/libsupport.la

include_HEADERS = rapi.h

SUBDIRS = config support .

