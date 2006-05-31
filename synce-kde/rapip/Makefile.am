INCLUDES = $(all_includes)

AM_CXXFLAGS = @LIBRRA_CFLAGS@ @LIBORANGE_CFLAGS@
AM_LDFLAGS = @LIBRRA_LIBS@ @LIBORANGE_LIBS@

kde_module_LTLIBRARIES = kio_rapip.la

kio_rapip_la_SOURCES = rapip.cpp
kio_rapip_la_LIBADD = -lkio $(RAPI_LIB) $(SYNCE_LIB)
kio_rapip_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN) $(RAPI_LDFLAGS) $(SYNCE_LDFLAGS)

protocol_DATA = rapip.protocol
protocoldir = $(kde_servicesdir)

messages: rc.cpp
	$(XGETTEXT) -j *.cpp -o $(podir)/synce-kde.pot
