SUBDIRS = screensnap

INCLUDES = $(all_includes)
METASOURCES = AUTO
bin_PROGRAMS = pdamirror



noinst_HEADERS = imageviewer.h rapiwrapper.h cescreen.h

pdamirror_SOURCES = imageviewer.cpp pdamirror.cpp rapiwrapper.cpp huffman.c cescreen.cpp
pdamirror_LDADD = -lkdeprint -lkio $(LIB_KDEUI)


