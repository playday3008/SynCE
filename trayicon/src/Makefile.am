AM_CFLAGS = @LIBRAPI2_CFLAGS@
AM_LDFLAGS = @LIBRAPI2_LIBS@

bin_PROGRAMS = synce-trayicon

INCLUDES  = \
			$(GNOME2_CFLAGS) $(GNOME_KEYRING_FLAGS) \
			-DDATADIR=\"$(datadir)\" 		\
			-DSYNCE_DATA=\""$(datadir)/synce/"\" 

synce_trayicon_SOURCES = eggtrayicon.c synce-trayicon.c gtop_stuff.c properties.c utils.c eggtrayicon.h gtop_stuff.h properties.h utils.h

if WITH_GNOME_KEYRING
synce_trayicon_SOURCES += keyring.c keyring.h
endif

synce_trayicon_LDADD = $(GNOME2_LIBS) $(GNOME_KEYRING_FLAGS)
##$(LDFLAGS)
