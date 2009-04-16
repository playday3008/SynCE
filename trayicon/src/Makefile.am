
bin_PROGRAMS = synce-trayicon

INCLUDES  = \
	-DDATADIR=\"$(datadir)\"                        \
	-DPREFIX=\""$(prefix)"\"                        \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DLIBDIR=\""$(libdir)"\"                        \
	-DSYNCE_DATA=\""$(datadir)/synce/"\"            \
	-DTRAYICON_MOD_DIR=\"@TRAYICON_MOD_DIR@\"

BUILT_SOURCES = \
	dccm-client-signals-marshal.h \
	dccm-client-signals-marshal.c \
	sync-engine-glue.h

CLEANFILES = $(BUILT_SOURCES)

synce_trayicon_SOURCES = $(BUILT_SOURCES) \
			main.c \
			stock-icons.h \
			synce-trayicon.c synce-trayicon.h \
			gtop_stuff.c gtop_stuff.h \
			properties.c properties.h \
			utils.c utils.h  \
			device.c device.h \
			device-manager.c device-manager.h \
			hal-client.c hal-client.h \
			vdccm-client.c vdccm-client.h \
			odccm-client.c odccm-client.h \
			dccm-client.c dccm-client.h \
			keyring.c keyring.h \
			device-info.c device-info.h \
			module.c module.h \
			synce_app_man.c synce_app_man.h

EXTRA_DIST = dccm-client-signals-marshal.list \
		sync-engine.xml

synce_trayicon_CPPFLAGS = \
	$(GNOME2_CFLAGS) \
	$(GNOME_KEYRING_CFLAGS) \
	$(LIBRAPI2_CFLAGS) \
	$(LIBRRA_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(HAL_CFLAGS) \
	$(LIBORANGE_CFLAGS) \
	$(NOTIFY_CFLAGS)

synce_trayicon_LDADD = \
	$(GNOME2_LIBS) \
	$(GNOME_KEYRING_LIBS) \
	$(LIBRAPI2_LIBS) \
	$(LIBRRA_LIBS) \
	$(DBUS_LIBS) \
	$(HAL_LIBS) \
	$(LIBORANGE_LIBS) \
	$(NOTIFY_LIBS)


noinst_HEADERS = synce-trayicon-module.h


%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-client --ignore-unsupported --output=$@ --prefix=$(subst -,_,$*) $<

