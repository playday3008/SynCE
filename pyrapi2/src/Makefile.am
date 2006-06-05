rapibindingsdir = $(pyexecdir)
rapibindings_LTLIBRARIES = pyrapi2.la

pyrapi2_la_CFLAGS = @PYTHON_INCLUDES@ @LIBRAPI2_CFLAGS@
pyrapi2_la_LDFLAGS = -module -avoid-version -fPIC @LIBRAPI2_LIBS@
nodist_pyrapi2_la_SOURCES = pyrapi2.c

EXTRA_DIST = types.pxi pyrapi2.pyx

pyrapi2.c: $(srcdir)/types.pxi $(srcdir)/pyrapi2.pyx
	pyrexc $(srcdir)/pyrapi2.pyx -o ./pyrapi2.c
