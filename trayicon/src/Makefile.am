
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
			properties.c properties.h \
			utils.c utils.h  \
			device.c device.h \
			device-manager.c device-manager.h \
			dccm-client.c dccm-client.h \
			keyring.c keyring.h \
			device-info.c device-info.h \
			module.c module.h \
			synce_app_man.c synce_app_man.h

if ENABLE_VDCCM_SUPPORT
synce_trayicon_SOURCES += vdccm-client.c vdccm-client.h \
			gtop_stuff.c gtop_stuff.h
endif

if ENABLE_ODCCM_SUPPORT
synce_trayicon_SOURCES += odccm-client.c odccm-client.h
endif

if ENABLE_HAL_SUPPORT
synce_trayicon_SOURCES += hal-client.c hal-client.h
endif

if ENABLE_UDEV_SUPPORT
synce_trayicon_SOURCES += udev-client.c udev-client.h
endif

EXTRA_DIST = dccm-client-signals-marshal.list \
		sync-engine.xml

synce_trayicon_CPPFLAGS = \
	$(GTK2_CFLAGS) \
	$(GCONF_CFLAGS) \
	$(GNOME_KEYRING_CFLAGS) \
	$(LIBSYNCE_CFLAGS) \
	$(LIBRRA_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(HAL_CFLAGS) \
	$(GTOP_CFLAGS) \
	$(LIBORANGE_CFLAGS) \
	$(NOTIFY_CFLAGS) \
	$(XML_CFLAGS) \
	$(APP_INDICATOR_CFLAGS)

synce_trayicon_LDADD = \
	$(GTK2_LIBS) \
	$(GCONF_LIBS) \
	$(GNOME_KEYRING_LIBS) \
	$(LIBSYNCE_LIBS) \
	$(LIBRRA_LIBS) \
	$(DBUS_LIBS) \
	$(HAL_LIBS) \
	$(GTOP_LIBS) \
	$(LIBORANGE_LIBS) \
	$(NOTIFY_LIBS) \
	$(XML_LIBS) \
	$(APP_INDICATOR_LIBS)


noinst_HEADERS = synce-trayicon-module.h


%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-client --ignore-unsupported --output=$@ --prefix=$(subst -,_,$*) $<

