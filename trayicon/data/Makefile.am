
gladedir = $(datadir)/synce/
glade_DATA = synce_trayicon_properties.glade

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

EXTRA_DIST = \
	$(glade_DATA) \
	$(schema_in_files)

DISTCLEANFILES = \
	$(schema_DATA)
