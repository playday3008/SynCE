INCLUDES = $(all_includes)
METASOURCES = AUTO
bin_PROGRAMS = kcemirror

noinst_HEADERS = imageviewer.h rapiwrapper.h cescreen.h decoder.h rledecoder.h huffmandecoder.h xordecoder.h

kcemirror_SOURCES = imageviewer.cpp cescreen.cpp decoder.cpp rledecoder.cpp huffmandecoder.cpp xordecoder.cpp rapiwrapper.cpp kcemirror.cpp
kcemirror_LDADD = -lkdeprint -lkio $(LIB_KDEUI)

applnkUtilitiesdir = $(kde_appsdir)/Utilities
