AM_CXXFLAGS = @LIBRRA_CFLAGS@
AM_LDFLAGS = @LIBRRA_LIBS@

INCLUDES = -I$(top_srcdir)/libksynce -I$(top_srcdir)/includes \
	   $(KITCHENSYNC_INCLUDES) $(all_includes)

METASOURCES = AUTO
lib_LTLIBRARIES = libksynce.la
libksynce_la_LDFLAGS = $(all_libraries)
libksynce_la_SOURCES = eventsyncee.cpp todosyncee.cpp syncekonnectorbase.cpp \
	syncekonnectorconfigbase.cpp
noinst_HEADERS = syncekonnectorbase.h syncekonnectorconfigbase.h
