
INCLUDES = -I$(top_srcdir)/raki -I$(top_srcdir)/raki/interfaces $(all_includes) $(RAPI_INCLUDES) $(SYNCE_INCLUDES)

if AGSYNC
kde_module_LTLIBRARIES = rakiagsync.la
endif

rakiagsync_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_RPATH) $(KDE_MT_LDFLAGS) \
	--whole_archive -rdynamic
rakiagsync_la_SOURCES = agsync.cpp agsyncfactory.cpp syncstream.cpp \
		agsyncconfig.ui agsyncconfigimpl.cpp serverconfig.ui serverconfigimpl.cpp \
		avantgoclientinstallationdialog.ui avantgoclientinstallationdialogimpl.cpp

# the libraries to link against.
rakiagsync_la_LIBADD = $(AGSYNC_lib)

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) -j *.cpp -o $(podir)/synce-kde.pot

if AGSYNC
kde_services_DATA =  rakiagsync.desktop
endif
noinst_HEADERS = agsyncconfigimpl.h agsync.h agsyncfactory.h syncstream.h \
		serverconfigimpl.h avantgoclientinstallationdialogimpl.h
