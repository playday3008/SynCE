SUBDIRS = screensnap

INCLUDES = $(all_includes)
METASOURCES = AUTO
bin_PROGRAMS = pdamirror



noinst_HEADERS = imageviewer.h rapiwrapper.h cescreen.h decoder.h rledecoder.h huffmandecoder.h

pdamirror_SOURCES = imageviewer.cpp pdamirror.cpp rapiwrapper.cpp cescreen.cpp decoder.cpp rledecoder.cpp huffmandecoder.cpp
pdamirror_LDADD = -lkdeprint -lkio $(LIB_KDEUI)




applnkUtilitiesdir = $(kde_appsdir)/Utilities
