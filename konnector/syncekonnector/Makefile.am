# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/libksynce -I$(top_srcdir)/includes $(all_includes) \
	$(RRA_INCLUDES) $(RAPI_INCLUDES) $(SYNCE_INCLUDES) $(KITCHENSYNC_INCLUDES)

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



lib_LTLIBRARIES = libsyncekonnector.la
libsyncekonnector_la_LDFLAGS = -avoid-version -module --whole_archive \
	$(KDE_RPATH) $(RRA_LDFLAGS) $(RAPI_LDFLAGS) $(SYNCE_LDFLAGS) $(all_libraries)
	
libsyncekonnector_la_SOURCES = AddressBookHandler.cpp matchmaker.cpp \
	PimHandler.cpp PocketPCKonnector.cpp PocketPCKonnectorConfig.cpp eventhandler.cpp \
	todohandler.cpp
	
noinst_HEADERS = AddressBookHandler.h matchmaker.h PimHandler.h \
	PocketPCKonnector.h PocketPCKonnectorConfig.h RecordType.h eventhandler.h todohandler.h

servicedir = $(kde_servicesdir)/kresources/konnector
service_DATA = syncekonnector.desktop
libsyncekonnector_la_LIBADD = $(top_builddir)/libksynce/libksynce.la $(RRA_LIB) \
	$(RAPI_LIB) $(SYNCE_LIB) $(LIB_KABC_FILE) $(LIB_KONNECTOR) $(LIB_KSYNC)
AM_CFLAGS = -pedantic-errors -Wall
AM_CXXFLAGS = -pedantic-errors -Wall
