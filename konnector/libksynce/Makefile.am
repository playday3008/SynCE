INCLUDES = -I$(top_srcdir)/libksynce -I/home/voc/usr/src/kde/kdepim-3.5.0 \
	$(all_includes) $(RRA_INCLUDES) $(RAPI_INCLUDES) $(SYNCE_INCLUDES) \
	$(KITCHENSYNC_INCLUDES)

METASOURCES = AUTO
lib_LTLIBRARIES = libksynce.la
libksynce_la_LDFLAGS = $(all_libraries)
libksynce_la_SOURCES = eventsyncee.cpp todosyncee.cpp syncekonnectorbase.cpp \
	syncekonnectorconfigbase.cpp
noinst_HEADERS = syncekonnectorbase.h syncekonnectorconfigbase.h
