INCLUDES = $(RAPI_INCLUDES) $(all_includes)

kde_module_LTLIBRARIES = kio_rapip.la

kio_rapip_la_SOURCES = rapip.cpp
kio_rapip_la_LIBADD = -lkio $(RAPI_LIB)
kio_rapip_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN) $(RAPI_LDFLAGS)

protocol_DATA = rapip.protocol
protocoldir = $(kde_servicesdir)

messages: rc.cpp
	$(XGETTEXT) -j *.cpp -o $(podir)/synce-kde.pot
