AM_CXXFLAGS = @LIBRAPI2_CFLAGS@
AM_LDFLAGS = @LIBRAPI2_LIBS@

INCLUDES = $(all_includes)
METASOURCES = AUTO
bin_PROGRAMS = kcemirror

noinst_HEADERS = imageviewer.h rapiwrapper.h cescreen.h decoder.h rledecoder.h huffmandecoder.h xordecoder.h

kcemirror_SOURCES = imageviewer.cpp cescreen.cpp decoder.cpp rledecoder.cpp huffmandecoder.cpp xordecoder.cpp rapiwrapper.cpp kcemirror.cpp
kcemirror_LDADD = $(LIB_KDEPRINT) $(LIB_KIO) $(LIB_KDEUI) $(LIBRAPI2_LIBS)
kcemirror_LDFLAGS = $(KDE_RPATH)
applnkUtilitiesdir = $(kde_appsdir)/Utilities
