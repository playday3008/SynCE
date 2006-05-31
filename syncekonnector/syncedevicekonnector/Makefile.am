AM_CXXFLAGS = @LIBRRA_CFLAGS@
AM_LDFLAGS = @LIBRRA_LIBS@

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/libksynce -I$(top_srcdir)/includes $(KITCHENSYNC_INCLUDES) $(all_includes)

# these are the headers for your project


# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(EXTRACTRC) `find . -name \*.ui -o -name \*.rc` > rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/syncekonnector.pot


#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables

# the application source, library search path, and link libraries



lib_LTLIBRARIES = libsyncedevicekonnector.la
libsyncedevicekonnector_la_LDFLAGS = -avoid-version -module --whole_archive \
	$(KDE_RPATH) $(all_libraries)
	
libsyncedevicekonnector_la_SOURCES = eventhandler.cpp todohandler.cpp \
	addressbookhandler.cpp pimhandler.cpp syncedevicekonnectorconfig.cpp syncedevicekonnector.cpp
	
noinst_HEADERS = eventhandler.h todohandler.h addressbookhandler.h pimhandler.h \
	recordtype.h syncedevicekonnectorconfig.h syncedevicekonnector.h

libsyncedevicekonnector_la_LIBADD = $(top_builddir)/libksynce/libksynce.la \
	$(LIB_KABC_FILE) $(LIB_KONNECTOR) $(LIB_KSYNC)
	
servicedir = $(kde_servicesdir)/kresources/konnector
service_DATA = syncedevicekonnector.desktop
