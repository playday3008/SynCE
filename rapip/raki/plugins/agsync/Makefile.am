
INCLUDES = -I$(top_srcdir)/raki -I$(top_srcdir)/raki/interfaces $(all_includes) $(RAPI_INCLUDES)

if AGSYNC
lib_LTLIBRARIES = librakiagsync.la
endif

librakiagsync_la_LDFLAGS = -module $(all_libraries) --whole_archive
librakiagsync_la_SOURCES = agsync.cpp agsyncfactory.cpp syncstream.cpp \
		agsyncconfig.ui agsyncconfigimpl.cpp serverconfig.ui serverconfigimpl.cpp \
		avantgoclientinstallationdialog.ui avantgoclientinstallationdialogimpl.cpp

# the libraries to link against.
librakiagsync_la_LIBADD = $(AGSYNC_lib)

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) -j *.cpp -o $(podir)/synce-kde.pot

if AGSYNC
kde_services_DATA =  rakiagsync.desktop
endif
noinst_HEADERS = agsyncconfigimpl.h agsync.h agsyncfactory.h syncstream.h \
		serverconfigimpl.h avantgoclientinstallationdialogimpl.h
