## useful flags
CFLAGS = -ggdb -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -I..

## create this library
lib_LTLIBRARIES = libsynce.la

## use these sources
libsynce_la_SOURCES = \
	bswap.c \
	filetime.c \
	synce.h \
	synce_log.h synce_log.c \
	synce_socket.h synce_socket.c \
	wstr.c

libsynce_la_LDFLAGS = -no-undefined -version-info 0:0:0
libsynce_la_LIBADD  = @LTLIBICONV@ config/libconfig.la

include_HEADERS = \
	synce.h synce_config.h synce_log.h synce_socket.h synce_types.h

SUBDIRS = config .

