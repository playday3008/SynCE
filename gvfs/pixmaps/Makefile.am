
pixmapdir = $(datadir)/icons/gnome/48x48/apps
pixmap_DATA = synce-gvfs.png
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/gnome
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

EXTRA_DIST = $(pixmap_DATA)
