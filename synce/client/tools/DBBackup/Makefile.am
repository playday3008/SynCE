bin_PROGRAMS = DBBackup
DBBackup_SOURCES = DBBackup.cpp
DBBackup_LDADD = $(top_builddir)/client/librapi/liblibrapi.la $(top_builddir)/libchunk/libchunk.la $(top_builddir)/libwine/libwine.la -lxml2

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/libwine -I$(top_srcdir)/client/librapi $(all_includes)

# the library search path.
DBBackup_LDFLAGS = $(all_libraries) -L../../librapi
SUBDIRS = 
AM_CFLAGS = -fshort-wchar -DUNICODE
AM_CXXFLAGS = -fshort-wchar -DUNICODE
