
##############
# Glade files
##############
gladedir = $(datadir)/synce/
glade_DATA = synce_trayicon_properties.glade

##############
# GSettings schema
##############
# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = org.synce.SynceTrayicon.gschema.xml

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@

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
	$(gsettings_SCHEMAS) \
	$(desktop_in_in_files) \
	$(autostart_in_in_files)


DISTCLEANFILES = \
	$(desktop_DATA) \
	$(autostart_DATA)
