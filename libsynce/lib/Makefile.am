AM_CFLAGS = @DBUS_CFLAGS@ @HAL_CFLAGS@

INCLUDES = -I..

## create this library
lib_LTLIBRARIES = libsynce.la

## use these sources
libsynce_la_SOURCES = \
	bswap.c \
	datetype.c \
	errors.c \
	files.c \
	filetime.c \
	info.c \
	password.c \
	synce.h \
	synce_hash.h synce_hash.c \
	synce_ini.h synce_ini.c \
	synce_log.h synce_log.c \
	synce_socket.h synce_socket.c \
	synce_vector_template.h \
	time_fields.c \
	wstr.c

libsynce_la_LDFLAGS = -no-undefined -version-info 0:0:0
libsynce_la_LIBADD  = config/libconfig.la -lm @LTLIBICONV@ @LTLIBOBJS@ @DBUS_LIBS@ @HAL_LIBS@

include_HEADERS = \
	synce.h \
	synce_hash.h \
	synce_ini.h \
	synce_log.h \
	synce_socket.h \
	synce_types.h \
	synce_sys_error.h \
	synce_vector_template.h

##noinst_PROGRAMS = ftconv

##ftconv_SOURCES = ftconv.c
##ftconv_LDADD = libsynce.la

SUBDIRS = config .

