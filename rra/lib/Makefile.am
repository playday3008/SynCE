## useful flags
AM_CFLAGS = -g -Wall -Wno-long-long -Werror -ansi @CFLAGS@ -I..
## @APPLE_CFLAGS@

## create this library
lib_LTLIBRARIES = librra.la

## use these sources
librra_la_SOURCES = \
	appointment.c \
	dbstream.h dbstream.c \
	common_handlers.h common_handlers.c \
	contact.c \
	generator.h generator.c \
	librra.h   librra.c \
	parser.h   parser.c \
	recurrence.h recurrence_internal.h recurrence.c \
	rrac.h     rrac.c \
	strbuf.h   strbuf.c \
	task.c \
	timezone.h timezone.c

librra_la_LDFLAGS = -no-undefined -version-info 0:0:0
librra_la_LIBADD   = @LTLIBOBJS@

include_HEADERS = \
	librra.h

