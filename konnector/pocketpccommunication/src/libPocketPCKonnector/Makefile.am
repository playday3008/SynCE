# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/src/lib $(all_includes)

# these are the headers for your project


# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO


lib_LTLIBRARIES = libpocketpckonnector.la

libpocketpckonnector_la_SOURCES = PDAIdHelper.cpp PDAIdHelper.h\
	PocketPCKonnectorConfig.cpp PocketPCKonnectorConfig.h\
	PocketPCKonnector.cpp PocketPCKonnector.h
libpocketpckonnector_la_LDFLAGS = -module $(KDE_PLUGIN) $(KDE_RPATH) $(all_libraries) 
libpocketpckonnector_la_LIBADD = $(top_builddir)/src/lib/libkpocketpccomm.la -lsynce -lrapi -lrra -lkabc_file -lkonnector -lksync

servicedir   = $(kde_servicesdir)/kresources/konnector
service_DATA = libpocketpckonnector.desktop
