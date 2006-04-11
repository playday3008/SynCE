
INCLUDES = -I$(top_srcdir)/raki -I$(top_srcdir)/raki/interfaces $(all_includes) $(RAPI_INCLUDES)

if PIMSYNC
lib_LTLIBRARIES = librakipimsync.la
endif

librakipimsync_la_LDFLAGS = -module $(all_libraries) --whole_archive
librakipimsync_la_SOURCES = pimsync.cpp pimsyncfactory.cpp pimsyncconfig.ui pimsyncconfigimpl.cpp 

# the libraries to link against.
librakipimsync_la_LIBADD = $(PIMSYNC_lib)

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) -j *.cpp -o $(podir)/synce-kde.pot

if PIMSYNC
kde_services_DATA =  rakipimsync.desktop
endif
noinst_HEADERS = pimsync.h pimsyncfactory.h pimsyncconfigimpl.h
