## useful flags
CFLAGS = -ggdb -Wall -ansi -pedantic @CFLAGS@ 

## create this library
lib_LTLIBRARIES = librapi.la

## use these sources
librapi_la_SOURCES = \
	file.c                         \
	rapi_buffer.h   rapi_buffer.c  \
	rapi_context.h  rapi_context.c \
	rapi_internal.h                \
	rapi_socket.h   rapi_socket.c  \
	rapi_unicode.h  rapi_unicode.c

include_HEADERS = rapi.h rapi_win.h

