
INCLUDES  = \
	-DDATADIR=\"$(datadir)\"                        \
	-DPREFIX=\""$(prefix)"\"                        \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DLIBDIR=\""$(libdir)"\"                        \
	-DSYNCE_DATA=\""$(datadir)/synce/"\" 

moduledir = @TRAYICON_MOD_DIR@

module_LTLIBRARIES = test-mod.la

test_mod_la_SOURCES = test-mod.c

test_mod_la_CPPFLAGS = \
	$(GLIB2_CFLAGS) \
	$(GTK3_CFLAGS) \
	$(LIBSYNCE_CFLAGS)

test_mod_la_LDFLAGS = -module -avoid-version

test_mod_la_LIBADD = \
	$(GLIB2_LIBS) \
	$(GTK3_LIBS) \
	$(LIBSYNCE_LIBS)


