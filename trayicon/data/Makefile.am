
##############
# Glade files
##############
gladedir = $(datadir)/synce/
glade_DATA = synce_trayicon_properties.glade

##############
# GConf schema
##############
schemadir = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = synce-trayicon.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)
@INTLTOOL_SCHEMAS_RULE@

if GCONF_SCHEMAS_INSTALL
gconf_schema_install = "GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA)"

install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Installing GConf schemas"; \
		-$(gconf_schema_install); \
	else \
		echo "*** GConf schema not installed. After install, run this:"; \
		echo "***   $(gconf_schema_install)"; \
	fi
endif

if GCONF_SCHEMAS_INSTALL
gconf_schema_uninstall = "GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-uninstall-rule $(schema_DATA)"

uninstall-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Uninstalling GConf schemas"; \
		-$(gconf_schema_install); \
	else \
		echo "*** GConf schema not uninstalled. After uninstall, run this:"; \
		echo "***   $(gconf_schema_uninstall)"; \
	fi
endif

############################
# Desktop and autostart file
############################
desktopdir = $(datadir)/applications
desktop_in_in_files = synce-trayicon.desktop.in.in
desktop_in_files = synce-trayicon.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autostartdir = $(datadir)/gnome/autostart
autostart_in_in_files = synce-trayicon-autostart.desktop.in.in
autostart_in_files = synce-trayicon-autostart.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

@INTLTOOL_DESKTOP_RULE@



EXTRA_DIST = \
	$(glade_DATA) \
	$(schema_in_files) \
	$(desktop_in_in_files) \
	$(autostart_in_in_files)


DISTCLEANFILES = \
	$(schema_DATA) \
	$(desktop_DATA) \
	$(autostart_DATA)
