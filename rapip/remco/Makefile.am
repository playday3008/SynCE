SUBDIRS = screensnap

INCLUDES = $(all_includes)
METASOURCES = AUTO
bin_PROGRAMS = pdamirror



noinst_HEADERS = imageviewer.h pdascreenimpl.h rapiwrapper.h

pdamirror_SOURCES = imageviewer.cpp pdamirror.cpp pdascreen.ui pdascreenimpl.cpp rapiwrapper.cpp huffman.c
pdamirror_LDADD = -lkio $(LIB_KDEUI)
