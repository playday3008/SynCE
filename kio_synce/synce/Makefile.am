####### kdevelop will overwrite this part!!! (begin)##########
kde_module_LTLIBRARIES = kio_synce.la


INCLUDES = $(all_includes)


LDFLAGS = 

kio_synce_la_METASOURCES=AUTO

kio_synce_la_SOURCES = synce.cpp 
kio_synce_la_LIBADD = -lkio  $(LIB_KDEUI) $(LIB_KDECORE) $(LIB_QT) -lXext -lX11

kio_synce_la_LDFLAGS = -module $(KDE_PLUGIN)  



EXTRA_DIST = synce.cpp synce.h synce.protocol 

install-data-local:
	$(mkinstalldirs) 
	$(INSTALL_DATA) $(srcdir)/synce.protocol  $(kde_servicesdir)

uninstall-local:
	-rm -f  $(kde_servicesdir)

####### kdevelop will overwrite this part!!! (end)############
# this 10 paths are KDE specific. Use them:
# kde_htmldir       Where your docs should go to. (contains lang subdirs)
# kde_appsdir       Where your application file (.kdelnk) should go to. 
# kde_icondir       Where your icon should go to.
# kde_minidir       Where your mini icon should go to.
# kde_datadir       Where you install application data. (Use a subdir)
# kde_locale        Where translation files should go to.(contains lang subdirs)
# kde_cgidir        Where cgi-bin executables should go to.
# kde_confdir       Where config files should go to.
# kde_mimedir       Where mimetypes should go to.
# kde_toolbardir    Where general toolbar icons should go to.
# kde_wallpaperdir  Where general wallpapers should go to.
kio_synce_la_LIBADD = -lkio        
kio_synce_la_LDFLAGS = $(all_libraries) -module $(KDE_PLUGIN)
# make messages.po. Move this one to ../po/ and "make merge" in po
# the -x is for skipping messages already translated in kdelibs
messages:
	LIST=`find . -name \*.h -o -name \*.hh -o -name \*.H -o -name \*.hxx -o -name \*.hpp -o -name \*.cpp -o -name \*.cc -o -name \*.cxx -o -name \*.ecpp -o -name \*.C`; \
	if test -n "$$LIST"; then \
		 $(XGETTEXT) -C -ki18n -x $(kde_includes)/kde.pot $$LIST -o ../po/synce.pot; \
	fi

