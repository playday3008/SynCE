
gvfs_src_dir = @with_gvfs_source@

mountdir = $(datadir)/gvfs/mounts

INCLUDES =					\
	-I$(gvfs_src_dir)			\
	-I$(gvfs_src_dir)/common		\
	-I$(gvfs_src_dir)/daemon		\
	$(GLIB_CFLAGS) $(DBUS_CFLAGS) 		\
	-DDBUS_API_SUBJECT_TO_CHANGE		\
	-DLIBEXEC_DIR=\"$(libexecdir)/\" 	\
	-DMOUNTABLE_DIR=\"$(mountdir)/\" 	\
	-DGVFS_LOCALEDIR=\""$(localedir)"\"     \
	-DG_DISABLE_DEPRECATED

libraries =						\
	$(gvfs_src_dir)/daemon/libdaemon.la		\
	-lgvfscommon					\
	$(GLIB_LIBS) $(DBUS_LIBS)

%.mount: %.mount.in ../config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

libexec_PROGRAMS=gvfsd-synce

mount_in_files = synce.mount.in
mount_DATA = synce.mount

EXTRA_DIST = $(mount_in_files)

DISTCLEANFILES = $(mount_DATA)

gvfsd_synce_SOURCES = \
	gvfsbackendsynce.c gvfsbackendsynce.h \
	$(gvfs_src_dir)/daemon/daemon-main.c $(gvfs_src_dir)/daemon/daemon-main.h \
	$(gvfs_src_dir)/daemon/daemon-main-generic.c 

gvfsd_synce_CPPFLAGS = \
	-DBACKEND_HEADER=gvfsbackendsynce.h \
	-DDEFAULT_BACKEND_TYPE=synce \
	-DMAX_JOB_THREADS=1 \
	$(LIBRAPI2_CFLAGS) \
	-DBACKEND_TYPES='"synce", G_VFS_TYPE_BACKEND_SYNCE,'

gvfsd_synce_LDADD = $(libraries) $(LIBRAPI2_LIBS) -L$(gvfs_src_dir)/daemon

