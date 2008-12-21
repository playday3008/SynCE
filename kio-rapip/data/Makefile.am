# The new mime system

mimedir = $(datadir)/mime/packages
mime_DATA = synce-kio-rapip.xml

update_mime_db_cmd = update-mime-database $(datadir)/mime

if UPDATE_MIME_DATABASE
install-data-hook: update-mime-db

uninstall-hook: update-mime-db

else
install-data-hook:

uninstall-hook:
endif

update-mime-db:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating shared mime database."; \
		$(update_mime_db_cmd); \
	else \
		echo "*** Shared mime database not updated. After (un)install, run this:"; \
		echo "***   $(update_mime_db_cmd)"; \
	fi

EXTRA_DIST = $(mime_DATA)
