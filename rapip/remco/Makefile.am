SUBDIRS = screensnap

INCLUDES = $(all_includes)
METASOURCES = AUTO
bin_PROGRAMS = remco pdamirror

remco_SOURCES = main.cpp remco.cpp imageviewer.cpp

noinst_HEADERS = remco.h imageviewer.h pdascreenimpl.h
remco_LDADD = $(LIB_KDEUI)

pdamirror_SOURCES = imageviewer.cpp pdamirror.cpp pdascreen.ui pdascreenimpl.cpp
pdamirror_LDADD = $(LIB_KDEUI)
