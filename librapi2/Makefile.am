## Makefile.am -- Process this file with automake to produce Makefile.in
@SET_MAKE@

## don't complain about missing AUTHORS, NEWS, ChangeLog, etc
AUTOMAKE_OPTIONS = foreign 1.4

SUBDIRS = src script tools .
#tests not built at the moment

m4datadir = $(datadir)/aclocal
m4data_DATA = librapi2.m4

EXTRA_DIST = $(m4data_DATA)
