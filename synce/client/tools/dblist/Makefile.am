bin_PROGRAMS = dblist
dblist_SOURCES = dblist.cpp
dblist_LDADD = $(top_builddir)/client/librapi/librapi.la $(top_builddir)/libchunk/libchunk.la $(top_builddir)/libwine/libwine.la -lxml2

# set the include path found by configure
INCLUDES = -I$(top_srcdir)/libwine -I$(top_srcdir)/client/librapi $(all_includes)

# the library search path.
dblist_LDFLAGS = $(all_libraries) -L../../librapi
SUBDIRS = 
AM_CFLAG = @SHORT_WCHAR_FLAGS@ -DUNICODE
AM_CXXFLAGS = @SHORT_WCHAR_FLAGS@ -DUNICODE

