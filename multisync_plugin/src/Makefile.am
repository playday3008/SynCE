## useful flags
AM_CFLAGS = -g -Wall @GTK2_CFLAGS@ -I@MULTISYNC_INCLUDE@
LDADD = @GTK2_LIBS@ @LDFLAGS@


## install with other Multisync plugins
libdir=$(prefix)/lib/multisync

lib_LTLIBRARIES = libsynce_plugin.la

libsynce_plugin_la_SOURCES = multisync_plugin.c get_changes.c gui.c

libsynce_plugin_la_LIBADD = @GTK2_LIBS@

