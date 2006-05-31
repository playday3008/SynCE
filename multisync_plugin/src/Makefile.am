AM_CFLAGS = @LIBSYNCE_CFLAGS@ @LIBRAPI2_CFLAGS@ @LIBRRA_CFLAGS@ @GTK2_CFLAGS@ -I@MULTISYNC_INCLUDE@

## install with other Multisync plugins
libdir = @libdir@/multisync

lib_LTLIBRARIES = libsynce_plugin.la

libsynce_plugin_la_SOURCES = public.c internal.c

libsynce_plugin_la_LIBADD = @GTK2_LIBS@

