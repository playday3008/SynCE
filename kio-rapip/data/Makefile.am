# The new mime system

mimedir = $(datadir)/mime/packages
mime_DATA = synce-kio-rapip.xml

if UPDATE_MIME_DATABASE
install-data-local:
	mkdir -p $(mimedir)
	update-mime-database $(datadir)/mime/

uninstall-local:
	update-mime-database $(datadir)/mime/
else
install-data-local:

uninstall-local:
endif

EXTRA_DIST = $(mime_DATA)
