####### kdevelop will overwrite this part!!! (begin)##########
lib_LTLIBRARIES = liblibrapi.la

INCLUDES = -I$(top_srcdir)/libwine -I$(top_srcdir)/libchunk $(all_includes)

LDFLAGS = 

liblibrapi_la_METASOURCES = AUTO

liblibrapi_la_SOURCES = rapi.c 

EXTRA_DIST = rapi.c rapi.h main.c 

####### kdevelop will overwrite this part!!! (end)############
SOMAJOR = 1
SOMINOR = 0
SOSUBMINOR = 0

# here you should add all header files, which contains the functionality
# of your shared lib (published for the users)
include_HEADERS = rapi.h

# here you should add all header files, which shouldn´t be installed
noinst_HEADERS =

liblibrapi_la_LDFLAGS = $(all_libraries) -no-undefined -version-info $(SOMAJOR):$(SOMINOR):$(SOSUBMINOR) $(LDFLAGS) 

# add here the libraries which have to be include
# for building the shared lib
#
#  please consider also the usage of the make environment variables
#  like
#  $(LIB_KDEUI), $(LIB_QT), $(LIB_HTML), etc.
#  instead of the hardcoded entries, which have placed
#  in here.
#  The following line is only a suggestion!
#
#liblibrapi_la_LIBADD =  -lkfile -lkdeui -lkdecore $(LIB_QT)


bin_PROGRAMS = main
main_SOURCES = main.c
main_LDADD = $(top_builddir)/libchunk/libchunk.la $(top_builddir)/client/librapi/liblibrapi.la $(top_builddir)/libwine/libwine.la
AM_CFLAGS = -fshort-wchar -Wall -DDEBUG
SUBDIRS = 
