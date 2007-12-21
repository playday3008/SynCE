
bin_PROGRAMS = vdccm

INCLUDES  = \
	-DDATADIR=\"$(datadir)\"                        \
	-DPREFIX=\""$(prefix)"\"                        \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DLIBDIR=\""$(libdir)"\"                        \
	-DSYNCE_DATA=\""$(datadir)/synce/"\" 

BUILT_SOURCES = \
	odccm-client-signals-marshal.h \
	odccm-client-signals-marshal.c

CLEANFILES = $(BUILT_SOURCES)

vdccm_SOURCES = $(BUILT_SOURCES) \
		main.c \
		log.c log.h \
		odccm-client.c odccm-client.h

EXTRA_DIST = odccm-client-signals-marshal.list

vdccm_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(GNET_CFLAGS) \
	$(SYNCE_CFLAGS) \
	$(DBUS_CFLAGS)

vdccm_LDADD = \
	$(GLIB_LIBS) \
	$(GNET_LIBS) \
	$(SYNCE_LIBS) \
	$(DBUS_LIBS)

%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c
