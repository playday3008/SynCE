INCLUDES = -I$(top_srcdir)/raki -I$(top_srcdir)/raki/interfaces $(all_includes)
noinst_LIBRARIES =  libinterfaces.a
libinterfaces_a_SOURCES = rakisyncfactory.cpp rakisyncplugin.cpp

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO
include_HEADERS =  rakisyncplugin.h rakisyncfactory.h
noinst_HEADERS =  rakisyncfactory.h rakisyncplugin.h
