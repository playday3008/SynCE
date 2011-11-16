if HAVE_PYTHON

rrabindingsdir = $(pyexecdir)
rrabindings_LTLIBRARIES = pyrra.la

pyrra_la_CFLAGS = @PYTHON_INCLUDES@ -I../lib $(LIBSYNCE_CFLAGS)
pyrra_la_LDFLAGS = -module -avoid-version -fPIC
pyrra_la_LIBADD  = $(top_builddir)/lib/librra.la $(LIBSYNCE_LIBS)
nodist_pyrra_la_SOURCES = pyrra.c

endif

EXTRA_DIST = pyrra.pyx

pyrra.c: $(srcdir)/pyrra.pyx
	pyrexc $(srcdir)/pyrra.pyx -I../lib -o ./pyrra.c
