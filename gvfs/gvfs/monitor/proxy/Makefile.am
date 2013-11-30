gvfs_src_dir = @with_gvfs_source@

NULL =

libgvfsproxyvolumemonitordaemon_sources = 		\
	$(gvfs_src_dir)/monitor/proxy/gvfsproxyvolumemonitordaemon.c	\
	$(gvfs_src_dir)/monitor/proxy/gvfsproxyvolumemonitordaemon.h	\
	$(NULL)

EXTRA_DIST =						\
	$(gvfs_src_dir)/monitor/proxy/dbus-interfaces.xml		\
	$(NULL)


if LOCAL_LIBDAEMON_SRC

EXTRA_DIST += $(libgvfsproxyvolumemonitordaemon_sources)

else

dist-hook:
	cp $(libgvfsproxyvolumemonitordaemon_sources) $(distdir)

endif
