INCLUDES    = $(all_includes)

kde_module_LTLIBRARIES = kio_rapip.la

kio_rapip_la_SOURCES = rapip.cpp
kio_rapip_la_LIBADD = -lkio
kio_rapip_la_LDFLAGS = -avoid-version -module $(all_libraries) $(KDE_PLUGIN)

protocol_DATA = rapip.protocol
protocoldir = $(kde_servicesdir)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kio_rapip.pot
