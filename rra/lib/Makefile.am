## useful flags
AM_CFLAGS = -g -Wall -Wsign-compare -Wno-long-long -Werror -ansi @CFLAGS@ -I..
## @APPLE_CFLAGS@

includedir = @includedir@/rra

include_HEADERS = \
	appointment.h \
	contact.h \
	matchmaker.h \
	syncmgr.h \
	timezone.h \
	task.h

## create this library
lib_LTLIBRARIES = librra.la

## use these sources
librra_la_SOURCES = \
	appointment.h      appointment.c \
	dbstream.h         dbstream.c \
	common_handlers.h  common_handlers.c \
	contact.h          contact.c \
	generator.h        generator.c \
	parser.h           parser.c \
	recurrence.h       recurrence_internal.h recurrence.c \
	matchmaker.h       matchmaker.c \
	rrac.h             rrac.c \
	strbuf.h           strbuf.c \
	syncmgr.h          syncmgr.c \
	task.h             task.c \
	timezone.h         timezone.c

librra_la_LDFLAGS = -no-undefined -version-info 0:0:0
librra_la_LIBADD   = @LTLIBOBJS@


