## useful flags
AM_CFLAGS = -g -ansi -Wall -Wsign-compare -Wno-long-long @CFLAGS@ @APPLE_CFLAGS@ -I.. \
	    @LIBOLE2_CFLAGS@ @LIBGSF_CFLAGS@

## create this library
lib_LTLIBRARIES = liborange.la

## use these sources
liborange_la_SOURCES = \
	dllinflate.c \
	extract.c \
	helper.c \
	installshield_sfx.c \
	pe.h pe.c \
	separate.c \
	squeeze.c \
	suf.c \
	synce-appmgr.h \
	synce-appmgr-internal.h \
	tomtom.c

EXTRA_liborange_la_SOURCES = inno.c vise.c msi.c msi-gsf.c
liborange_la_LIBADD = @INNO_LIBADD@ @VISE_LIBADD@ @MSI_LIBADD@ @LIBOLE2_LIBS@ @LIBGSF_LIBS@
liborange_la_DEPENDENCIES = @INNO_LIBADD@ @VISE_LIBADD@ @MSI_LIBADD@

liborange_la_LDFLAGS = -no-undefined -version-info 0:0:0

include_HEADERS = liborange.h

