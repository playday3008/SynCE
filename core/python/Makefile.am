if HAVE_PYTHON

rapibindingsdir = $(pyexecdir)
rapibindings_LTLIBRARIES = pyrapi2.la

pyrapi2_la_CFLAGS = @PYTHON_INCLUDES@ @LIBSYNCE_CFLAGS@ -I../src
pyrapi2_la_LDFLAGS = -module -avoid-version -fPIC
pyrapi2_la_LIBADD = $(top_builddir)/src/librapi.la @LIBSYNCE_LIBS@
nodist_pyrapi2_la_SOURCES = pyrapi2.c

SUBDIRS = tests

endif

CLEANFILES = pyrapi2.c

EXTRA_DIST = types.pxi constants.pxi pyrapi2.pyx

pyrapi2.c: $(srcdir)/types.pxi $(srcdir)/pyrapi2.pyx
	pyrexc $(srcdir)/pyrapi2.pyx -o ./pyrapi2.c
