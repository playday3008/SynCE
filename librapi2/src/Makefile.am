## useful flags
CFLAGS = -ggdb -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -Isupport
@SET_MAKE@

## create this library
lib_LTLIBRARIES = librapi.la

## use these sources
librapi_la_SOURCES = \
	file_access.c                  \
	file_management.c              \
	misc.c                         \
	rapi.c                         \
	rapi_buffer.h   rapi_buffer.c  \
	rapi_context.h  rapi_context.c \
	rapi_internal.h                \
	rapi_socket.h   rapi_socket.c

librapi_la_LDFLAGS = -no-undefined -version-info 2:0:0
librapi_la_LIBADD  = @LTLIBICONV@ config/libconfig.la support/libsupport.la

include_HEADERS = rapi.h rapi_types.h

SUBDIRS = config support .

