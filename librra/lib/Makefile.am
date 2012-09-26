INCLUDES = -I..

includedir = @includedir@/rra

include_HEADERS = \
	appointment.h \
	contact.h \
	matchmaker.h \
	syncmgr.h \
	timezone.h \
	frontend.h \
	task.h \
	uint32vector.h

## create this library
lib_LTLIBRARIES = librra.la

## use these sources
librra_la_SOURCES = \
	appointment.h      appointment.c \
	dbstream.h         dbstream.c \
	common_handlers.h  common_handlers.c \
	contact.h          contact.c \
	environment.h      environment.c \
	generator.h        generator.c \
	parser.h           parser.c \
	recurrence.h       recurrence.c \
	recurrence_pattern.h recurrence_pattern.c \
	matchmaker.h       matchmaker.c \
	mdir_line_vector.h mdir_line_vector.c \
	rrac.h             rrac.c \
	strbuf.h           strbuf.c \
	strv.h             strv.c \
	syncmgr.h          syncmgr.c \
	task.h             task.c \
	file.h             file.c \
	timezone.h         timezone.c \
	frontend.h         frontend.c \
	uint32vector.h     uint32vector.c \
	synce_ids.h        appointment_ids.h \
	task_ids.h         internal.h \
	contact_ids.h      rrac_packet.h \
	matchmaker.h

librra_la_CFLAGS = @LIBSYNCE_CFLAGS@
librra_la_LDFLAGS = -no-undefined -version-info 0:0:0
librra_la_LIBADD   = @LTLIBOBJS@ @LIBSYNCE_LIBS@
