INCLUDES = -I$(top_srcdir)/raki -I$(top_srcdir)/raki/interfaces $(all_includes)
lib_LTLIBRARIES = librakikdeaddressbooksync.la
librakikdeaddressbooksync_la_LDFLAGS = -module $(all_libraries) --whole_archive
librakikdeaddressbooksync_la_SOURCES = kdeaddressbooksyncfactory.h kdeaddressbooksyncfactory.cpp kdeaddressbooksync.h kdeaddressbooksync.cpp

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

kde_services_DATA = rakikdeaddressbooksync.desktop
