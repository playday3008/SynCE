
gvfs_src_dir = $(top_srcdir)/@with_gvfs_source@

gvfs_headers = \
	$(gvfs_src_dir)/common/gsysutils.h \
	$(gvfs_src_dir)/common/gdbusutils.h \
	$(gvfs_src_dir)/common/gmountspec.h \
	$(gvfs_src_dir)/common/gmountoperationdbus.h \
	$(gvfs_src_dir)/common/gmountsource.h \
	$(gvfs_src_dir)/common/gmounttracker.h \
	$(gvfs_src_dir)/common/gvfsdaemonprotocol.h

EXTRA_DIST = 

if LOCAL_LIBDAEMON_SRC

EXTRA_DIST += $(gvfs_headers)

else

dist-hook:
	cp $(gvfs_headers) $(distdir)

endif

