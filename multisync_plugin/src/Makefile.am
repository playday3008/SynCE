## useful flags
AM_CFLAGS = -g -Wall -Werror @GTK2_CFLAGS@ -I@MULTISYNC_INCLUDE@
LDADD = @GTK2_LIBS@ @LDFLAGS@


## install with other Multisync plugins
libdir = @libdir@/multisync

lib_LTLIBRARIES = libsynce_plugin.la

libsynce_plugin_la_SOURCES = public.c internal.c

libsynce_plugin_la_LIBADD = @GTK2_LIBS@

