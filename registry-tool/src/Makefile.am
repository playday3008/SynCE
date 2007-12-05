
bin_PROGRAMS = synce-registry-tool

INCLUDES  = \
	-DDATADIR=\"$(datadir)\"                        \
	-DPREFIX=\""$(prefix)"\"                        \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DLIBDIR=\""$(libdir)"\"                        \
	-DSYNCE_DATA=\""$(datadir)/synce/"\"

synce_registry_tool_SOURCES =				\
		main.c 					\
		registry-access.c registry-access.h	\
		registry-list.c registry-list.h		\
		misc.c misc.h

synce_registry_tool_CPPFLAGS =		\
		$(GNOME2_CFLAGS) 	\
		$(LIBRAPI2_CFLAGS)

synce_registry_tool_LDADD = 		\
		$(GNOME2_LIBS) 		\
		$(LIBRAPI2_LIBS)
