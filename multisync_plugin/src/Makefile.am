## useful flags
CFLAGS = -g -Wall @APPLE_CFLAGS@ @GNOME_INCLUDEDIR@ -I@MULTISYNC_INCLUDE@

## install with other Multisync plugins
libdir=$(prefix)/lib/multisync

lib_LTLIBRARIES = libsynce_plugin.la

libsynce_plugin_la_SOURCES = multisync_plugin.c get_changes.c gui.c

libsynce_plugin_la_LIBADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) @GTK_LIBS@

