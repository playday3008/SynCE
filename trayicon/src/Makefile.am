AM_CFLAGS = @LIBRAPI2_CFLAGS@ @DBUS_CFLAGS@
AM_LDFLAGS = @LIBRAPI2_LIBS@ @DBUS_LIBS@

bin_PROGRAMS = synce-trayicon

INCLUDES  = \
			$(GNOME2_CFLAGS) $(GNOME_KEYRING_FLAGS) \
			-DDATADIR=\"$(datadir)\" 		\
			-DSYNCE_DATA=\""$(datadir)/synce/"\" 

BUILT_SOURCES = \
	dccm-client-signals-marshal.h \
	dccm-client-signals-marshal.c

CLEANFILES = $(BUILT_SOURCES)

synce_trayicon_SOURCES = $(BUILT_SOURCES) \
			synce-trayicon.c \
			eggtrayicon.c eggtrayicon.h \
			gtop_stuff.c gtop_stuff.h \
			properties.c properties.h \
			utils.c utils.h  \
			device.c device.h \
			device-manager.c device-manager.h \
			vdccm-client.c vdccm-client.h \
			odccm-client.c odccm-client.h \
			dccm-client.c dccm-client.h

if WITH_GNOME_KEYRING
synce_trayicon_SOURCES += keyring.c keyring.h
endif

EXTRA_DIST = vdccm-clientsignals-marshal.list

synce_trayicon_LDADD = $(GNOME2_LIBS) $(GNOME_KEYRING_FLAGS)
##$(LDFLAGS)

%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c
