AM_CFLAGS = -Wall \
	-DLOCALSTATEDIR=\""$(localstatedir)"\"

libexec_PROGRAMS = hal-dccm synce-serial-chat

BUILT_SOURCES = synce-device-signals-marshal.h \
		synce-device-signals-marshal.c

if USE_HAL
BUILT_SOURCES += hal-interface-glue.h
else
BUILT_SOURCES += udev-interface-glue.h
endif

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
	$(DBUS_CFLAGS)

hal_dccm_LDADD = \
	$(SYNCE_LIBS) \
	$(GLIB_LIBS) \
	$(GNET_LIBS) \
	$(DBUS_LIBS)

if USE_HAL
hal_dccm_CPPFLAGS += $(HAL_CFLAGS)
hal_dccm_LDADD += $(HAL_LIBS)
else
hal_dccm_CPPFLAGS += $(GUDEV_CFLAGS) -DG_UDEV_API_IS_SUBJECT_TO_CHANGE
hal_dccm_LDADD += $(GUDEV_LIBS)
endif

synce_serial_chat_SOURCES = synce-serial-chat.c

EXTRA_DIST = synce-device-signals-marshal.list \
		hal-interface.xml \
		udev-interface.xml

dist_bin_SCRIPTS = synce-unlock.py

%-glue.h: %.xml
	dbus-binding-tool --mode=glib-server --output=$@ --prefix=synce_device $<
#	dbus-binding-tool --mode=glib-server --output=$@ --prefix=$(subst -,_,$*) $<

%-signals-marshal.h: %-signals-marshal.list
	glib-genmarshal --header --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.h

%-signals-marshal.c: %-signals-marshal.list
	glib-genmarshal --body --prefix=$(subst -,_,$*)_marshal $< > $*-signals-marshal.c
