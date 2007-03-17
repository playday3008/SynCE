rrabindingsdir = $(pyexecdir)
rrabindings_LTLIBRARIES = pyrra.la

pyrra_la_CFLAGS = @PYTHON_INCLUDES@ @LIBRRA_CFLAGS@
pyrra_la_LDFLAGS = -module -avoid-version -fPIC -lrra -lrapi
nodist_pyrra_la_SOURCES = pyrra.c

EXTRA_DIST = pyrra.pyx

pyrra.c: $(srcdir)/pyrra.pyx
	pyrexc $(srcdir)/pyrra.pyx -I/usr/local/include/rra -o ./pyrra.c
