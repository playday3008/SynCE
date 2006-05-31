AM_CXXFLAGS = @LIBRRA_CFLAGS@
AM_LDFLAGS = @LIBRRA_LIBS@

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/libksynce -I$(top_srcdir)/includes \
	   $(KITCHENSYNC_INCLUDES) $(all_includes)

# these are the headers for your project

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/syncekonnector.pot


#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables

# the application source, library search path, and link libraries

METASOURCES = AUTO
lib_LTLIBRARIES = libsyncelocalkonnector.la
libsyncelocalkonnector_la_LDFLAGS = -avoid-version -module $(all_libraries)

servicedir = $(kde_servicesdir)/kresources/konnector
service_DATA = syncelocalkonnector.desktop

libsyncelocalkonnector_la_LIBADD = $(top_builddir)/libksynce/libksynce.la \
	$(LIB_KSYNC) $(LIB_KONNECTOR) $(LIB_KABC_FILE)
libsyncelocalkonnector_la_SOURCES = syncelocalkonnectorconfig.cpp\
	syncelocalkonnector.cpp
noinst_HEADERS = syncelocalkonnector.h syncelocalkonnectorconfig.h
