INCLUDES = -I$(top_srcdir)/raki -I$(top_srcdir)/raki/interfaces $(all_includes)

if AGSYNC
lib_LTLIBRARIES = librakiagsync.la
endif

librakiagsync_la_LDFLAGS = -module $(all_libraries) --whole_archive
librakiagsync_la_SOURCES = agsync.cpp agsync.h agsyncfactory.cpp agsyncfactory.h syncstream.cpp syncstream.h

# the libraries to link against.
librakiagsync_la_LIBADD = $(AGSYNC_lib)

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

if AGSYNC
kde_services_DATA =  rakiagsync.desktop
endif
