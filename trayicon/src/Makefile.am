
bin_PROGRAMS = synce-trayicon

INCLUDES  = \
			$(GNOME2_CFLAGS) 						\
			-DDATADIR=\"$(datadir)\" 				\
			-DSYNCE_DATA=\""$(datadir)/synce/"\" 

synce_trayicon_SOURCES = eggtrayicon.c synce-trayicon.c gtop_stuff.c properties.c eggtrayicon.h gtop_stuff.h properties.h 

synce_trayicon_LDADD = $(GNOME2_LIBS) 
##$(LDFLAGS)
