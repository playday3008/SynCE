AM_CXXFLAGS = @LIBSYNCE_CFLAGS@ -Wall
AM_CFLAGS = @LIBSYNCE_CFLAGS@ -Wall
AM_LDFLAGS = @LIBSYNCE_LIBS@

INCLUDES = -I$(top_srcdir)/src

noinst_PROGRAMS = CeRapiInvoke

CeRapiInvoke_SOURCES = CeRapiInvoke.cc ../../tools/pcommon.c
CeRapiInvoke_LDADD = ../../src/librapi.la

##TESTS = CeRapiInvoke

SUBDIRS = dll
