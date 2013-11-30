NULL =

gvfs_src_dir = @with_gvfs_source@

############################################################################

dbus_built_sources = gvfsvolumemonitordbus.c gvfsvolumemonitordbus.h
BUILT_SOURCES = $(dbus_built_sources)

$(dbus_built_sources) : Makefile.am $(gvfs_src_dir)/monitor/proxy/dbus-interfaces.xml
	$(AM_V_GEN) gdbus-codegen					\
		--interface-prefix org.gtk.Private.			\
		--c-namespace GVfs					\
		--generate-c-code gvfsvolumemonitordbus			\
		$(gvfs_src_dir)/monitor/proxy/dbus-interfaces.xml				\
		$(NULL)


noinst_LTLIBRARIES = libgvfsproxyvolumemonitordaemon-noin.la

libgvfsproxyvolumemonitordaemon_noin_la_SOURCES =		\
	$(gvfs_src_dir)/monitor/proxy/gvfsproxyvolumemonitordaemon.c	\
	$(gvfs_src_dir)/monitor/proxy/gvfsproxyvolumemonitordaemon.h	\
	$(NULL)

nodist_libgvfsproxyvolumemonitordaemon_noin_la_SOURCES =	\
	$(dbus_built_sources)					\
	$(NULL)

libgvfsproxyvolumemonitordaemon_noin_la_CFLAGS =		\
	-I$(gvfs_src_dir)/common                  		\
	$(GLIB_CFLAGS)                          		\
	-DG_LOG_DOMAIN=\"GVFS-RemoteVolumeMonitorDaemon\"	\
	-DGVFS_LOCALEDIR=\""$(localedir)"\"			\
	$(NULL)

libgvfsproxyvolumemonitordaemon_noin_la_LIBADD  =     		\
	$(GLIB_LIBS)                                 		\
	$(NULL)

CLEANFILES =							\
	$(dbus_built_sources)					\
	$(NULL)

############################################################################

libexec_PROGRAMS = gvfs-synce-volume-monitor

gvfs_synce_volume_monitor_SOURCES = \
	syncevolume.c		syncevolume.h \
	syncevolumemonitor.c 	syncevolumemonitor.h \
	syncevolumemonitordaemon.c \
	$(NULL)

gvfs_synce_volume_monitor_CFLAGS = \
	-DG_LOG_DOMAIN=\"GVFS-SYNCE\" \
	-I$(gvfs_src_dir)/common \
	-I$(gvfs_src_dir)/monitor/proxy \
	$(GLIB_CFLAGS) \
	$(LIBSYNCE_CFLAGS) \
	$(WARN_CFLAGS) \
	-DGIO_MODULE_DIR=\"$(GIO_MODULE_DIR)\" \
	-DGVFS_LOCALEDIR=\"$(localedir)\" \
	$(NULL)

gvfs_synce_volume_monitor_LDADD = \
	$(GLIB_LIBS) \
	$(LIBSYNCE_LIBS) \
	libgvfsproxyvolumemonitordaemon-noin.la \
	$(NULL)

remote_volume_monitorsdir = $(datadir)/gvfs/remote-volume-monitors
remote_volume_monitors_DATA = synce.monitor

servicedir = $(datadir)/dbus-1/services
service_in_files = org.gtk.Private.SynceVolumeMonitor.service.in
service_DATA = $(service_in_files:.service.in=.service)

$(service_DATA): $(service_in_files) Makefile
	$(AM_V_GEN) $(SED) -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

clean-local:
	rm -f *~ *.loT $(BUILT_SOURCES) $(service_DATA)

DISTCLEANFILES = $(service_DATA)

EXTRA_DIST = $(service_in_files) synce.monitor

