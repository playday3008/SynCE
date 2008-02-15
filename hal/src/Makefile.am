AM_CFLAGS = -Wall

sbin_PROGRAMS = hal-dccm

BUILT_SOURCES = synce-device-signals-marshal.h \
		synce-device-signals-marshal.c \
		synce-device-glue.h

CLEANFILES = $(BUILT_SOURCES)

hal_dccm_SOURCES = $(BUILT_SOURCES) \
		main.c \
		log.c log.h \
		synce-device.c synce-device.h synce-device-internal.h \
		synce-device-rndis.c synce-device-rndis.h \
		synce-device-legacy.c synce-device-legacy.h \
		synce-connection-broker.c synce-connection-broker.h \
		synce-errors.c synce-errors.h \
		utils.c utils.h

hal_dccm_CPPFLAGS = \
	$(SYNCE_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GNET_CFLAGS) \
	$(HAL_CFLAGS) \
	$(DBUS_CFLAGS)

hal_dccm_LDADD = \
	$(SYNCE_LIBS) \
	$(GLIB_LIBS) \
	$(GNET_LIBS) \
	$(HAL_LIBS) \
	$(DBUS_LIBS)


libexec_PROGRAMS = synce-serial-chat

synce_serial_chat_SOURCES = synce-serial-chat.c

EXTRA_DIST = synce-device-signals-marshal.list \
		synce-device.xml

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $<

%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c
