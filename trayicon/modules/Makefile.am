
INCLUDES  = \
	-DDATADIR=\"$(datadir)\"                        \
	-DPREFIX=\""$(prefix)"\"                        \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DLIBDIR=\""$(libdir)"\"                        \
	-DSYNCE_DATA=\""$(datadir)/synce/"\" 

moduledir = $(libdir)/synce-trayicon/modules

module_LTLIBRARIES = test-mod.la

test_mod_la_SOURCES = test-mod.c

test_mod_la_CPPFLAGS = \
	$(GNOME2_CFLAGS) \
	$(LIBRAPI2_CFLAGS)

test_mod_la_LDFLAGS = -module -avoid-version

test_mod_la_LIBADD = \
	$(GNOME2_LIBS) \
	$(LIBRAPI2_LIBS)


