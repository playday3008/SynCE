
bin_PROGRAMS = synce-software-manager

INCLUDES  = \
	-DDATADIR=\"$(datadir)\"                        \
	-DPREFIX=\""$(prefix)"\"                        \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DLIBDIR=\""$(libdir)"\"                        \
	-DSYNCE_DATA=\""$(datadir)/synce/"\"            \
	-DSYNCE_SOFTWARE_MANAGER_GLADEFILE=\""$(datadir)/synce/synce_software_manager.glade"\"


synce_software_manager_SOURCES =	\
	main.c \
	software_manager.c software_manager.h \
	synce_app_man.c synce_app_man.h

synce_software_manager_CPPFLAGS = \
	$(GTK2_CFLAGS) \
	$(LIBRAPI2_CFLAGS) \
	$(LIBORANGE_CFLAGS)

synce_software_manager_LDADD = \
	$(GTK2_LIBS) \
	$(LIBRAPI2_LIBS) \
	$(LIBORANGE_LIBS)

