
gvfs_src_dir = @with_gvfs_source@

mountdir = $(datadir)/gvfs/mounts

AM_CPPFLAGS =					\
	-I$(gvfs_src_dir)			\
	-I$(gvfs_src_dir)/common		\
	-I$(gvfs_src_dir)/daemon		\
	$(GLIB_CFLAGS) 				\
	-DLIBEXEC_DIR=\"$(libexecdir)/\" 	\
	-DMOUNTABLE_DIR=\"$(mountdir)/\" 	\
	-DGVFS_LOCALEDIR=\""$(localedir)"\"     \
	-DG_DISABLE_DEPRECATED

AM_CPPFLAGS += $(DBUS_CFLAGS) -DDBUS_API_SUBJECT_TO_CHANGE

noinst_LTLIBRARIES = libdaemon.la

%.mount: %.mount.in ../config.log
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

libexec_PROGRAMS=gvfsd-synce

libdaemon_la_SOURCES = \
	$(gvfs_src_dir)/daemon/gvfsdaemon.c $(gvfs_src_dir)/daemon/gvfsdaemon.h \
	$(gvfs_src_dir)/daemon/gvfsbackend.c $(gvfs_src_dir)/daemon/gvfsbackend.h \
	$(gvfs_src_dir)/daemon/gvfschannel.c $(gvfs_src_dir)/daemon/gvfschannel.h \
	$(gvfs_src_dir)/daemon/gvfsreadchannel.c $(gvfs_src_dir)/daemon/gvfsreadchannel.h \
	$(gvfs_src_dir)/daemon/gvfswritechannel.c $(gvfs_src_dir)/daemon/gvfswritechannel.h \
	$(gvfs_src_dir)/daemon/gvfsmonitor.c $(gvfs_src_dir)/daemon/gvfsmonitor.h \
	$(gvfs_src_dir)/daemon/gvfsdaemonutils.c $(gvfs_src_dir)/daemon/gvfsdaemonutils.h \
	$(gvfs_src_dir)/daemon/gvfsjob.c $(gvfs_src_dir)/daemon/gvfsjob.h \
	$(gvfs_src_dir)/daemon/gvfsjobsource.c $(gvfs_src_dir)/daemon/gvfsjobsource.h \
	$(gvfs_src_dir)/daemon/gvfsjobdbus.c $(gvfs_src_dir)/daemon/gvfsjobdbus.h \
	$(gvfs_src_dir)/daemon/gvfsjobmount.c $(gvfs_src_dir)/daemon/gvfsjobmount.h \
	$(gvfs_src_dir)/daemon/gvfsjobunmount.c $(gvfs_src_dir)/daemon/gvfsjobunmount.h \
	$(gvfs_src_dir)/daemon/gvfsjobmountmountable.c $(gvfs_src_dir)/daemon/gvfsjobmountmountable.h \
	$(gvfs_src_dir)/daemon/gvfsjobunmountmountable.c $(gvfs_src_dir)/daemon/gvfsjobunmountmountable.h \
	$(gvfs_src_dir)/daemon/gvfsjobopenforread.c $(gvfs_src_dir)/daemon/gvfsjobopenforread.h \
	$(gvfs_src_dir)/daemon/gvfsjobread.c $(gvfs_src_dir)/daemon/gvfsjobread.h \
	$(gvfs_src_dir)/daemon/gvfsjobseekread.c $(gvfs_src_dir)/daemon/gvfsjobseekread.h \
	$(gvfs_src_dir)/daemon/gvfsjobcloseread.c $(gvfs_src_dir)/daemon/gvfsjobcloseread.h \
	$(gvfs_src_dir)/daemon/gvfsjobopenforwrite.c $(gvfs_src_dir)/daemon/gvfsjobopenforwrite.h \
	$(gvfs_src_dir)/daemon/gvfsjobwrite.c $(gvfs_src_dir)/daemon/gvfsjobwrite.h \
	$(gvfs_src_dir)/daemon/gvfsjobseekwrite.c $(gvfs_src_dir)/daemon/gvfsjobseekwrite.h \
	$(gvfs_src_dir)/daemon/gvfsjobclosewrite.c $(gvfs_src_dir)/daemon/gvfsjobclosewrite.h \
	$(gvfs_src_dir)/daemon/gvfsjobqueryinfo.c $(gvfs_src_dir)/daemon/gvfsjobqueryinfo.h \
	$(gvfs_src_dir)/daemon/gvfsjobqueryfsinfo.c $(gvfs_src_dir)/daemon/gvfsjobqueryfsinfo.h \
	$(gvfs_src_dir)/daemon/gvfsjobenumerate.c $(gvfs_src_dir)/daemon/gvfsjobenumerate.h \
	$(gvfs_src_dir)/daemon/gvfsjobsetdisplayname.c $(gvfs_src_dir)/daemon/gvfsjobsetdisplayname.h \
	$(gvfs_src_dir)/daemon/gvfsjobtrash.c $(gvfs_src_dir)/daemon/gvfsjobtrash.h \
	$(gvfs_src_dir)/daemon/gvfsjobdelete.c $(gvfs_src_dir)/daemon/gvfsjobdelete.h \
	$(gvfs_src_dir)/daemon/gvfsjobcopy.c $(gvfs_src_dir)/daemon/gvfsjobcopy.h \
	$(gvfs_src_dir)/daemon/gvfsjobmove.c $(gvfs_src_dir)/daemon/gvfsjobmove.h \
	$(gvfs_src_dir)/daemon/gvfsjobmakedirectory.c $(gvfs_src_dir)/daemon/gvfsjobmakedirectory.h \
	$(gvfs_src_dir)/daemon/gvfsjobmakesymlink.c $(gvfs_src_dir)/daemon/gvfsjobmakesymlink.h \
	$(gvfs_src_dir)/daemon/gvfsjobsetattribute.c $(gvfs_src_dir)/daemon/gvfsjobsetattribute.h \
	$(gvfs_src_dir)/daemon/gvfsjobqueryattributes.c $(gvfs_src_dir)/daemon/gvfsjobqueryattributes.h \
	$(gvfs_src_dir)/daemon/gvfsjobcreatemonitor.c $(gvfs_src_dir)/daemon/gvfsjobcreatemonitor.h \
	$(gvfs_src_dir)/daemon/gvfskeyring.h $(gvfs_src_dir)/daemon/gvfskeyring.c \
	$(NULL)

EXTRA_DIST = 

if GVFS_VERSION_1_0
libdaemon_la_SOURCES += \
	$(gvfs_src_dir)/daemon/gvfsjobpull.c $(gvfs_src_dir)/daemon/gvfsjobpull.h \
	$(gvfs_src_dir)/daemon/gvfsjobpush.c $(gvfs_src_dir)/daemon/gvfsjobpush.h
EXTRA_DIST += \
	../gvfs/daemon/gvfsjobupload.c ../gvfs/daemon/gvfsjobupload.h
else
EXTRA_DIST += \
	../gvfs/daemon/gvfsjobpull.c ../gvfs/daemon/gvfsjobpull.h \
	../gvfs/daemon/gvfsjobpush.c ../gvfs/daemon/gvfsjobpush.h
libdaemon_la_SOURCES += \
	$(gvfs_src_dir)/daemon/gvfsjobupload.c $(gvfs_src_dir)/daemon/gvfsjobupload.h
endif

if GVFS_VERSION_1_2
libdaemon_la_SOURCES += \
	$(gvfs_src_dir)/daemon/gvfsjobopeniconforread.c $(gvfs_src_dir)/daemon/gvfsjobopeniconforread.h \
	$(gvfs_src_dir)/daemon/gvfsjobqueryinforead.c $(gvfs_src_dir)/daemon/gvfsjobqueryinforead.h \
	$(gvfs_src_dir)/daemon/gvfsjobqueryinfowrite.c $(gvfs_src_dir)/daemon/gvfsjobqueryinfowrite.h
else
EXTRA_DIST += \
	../gvfs/daemon/gvfsjobopeniconforread.c ../gvfs/daemon/gvfsjobopeniconforread.h \
	../gvfs/daemon/gvfsjobqueryinforead.c ../gvfs/daemon/gvfsjobqueryinforead.h \
	../gvfs/daemon/gvfsjobqueryinfowrite.c ../gvfs/daemon/gvfsjobqueryinfowrite.h
endif

if GVFS_VERSION_1_4
libdaemon_la_SOURCES += \
	$(gvfs_src_dir)/daemon/gvfsjobstartmountable.c $(gvfs_src_dir)/daemon/gvfsjobstartmountable.h \
	$(gvfs_src_dir)/daemon/gvfsjobstopmountable.c $(gvfs_src_dir)/daemon/gvfsjobstopmountable.h \
	$(gvfs_src_dir)/daemon/gvfsjobpollmountable.c $(gvfs_src_dir)/daemon/gvfsjobpollmountable.h
else
EXTRA_DIST += \
	../gvfs/daemon/gvfsjobstartmountable.c $(gvfs_src_dir)/daemon/gvfsjobstartmountable.h \
	../gvfs/daemon/gvfsjobstopmountable.c $(gvfs_src_dir)/daemon/gvfsjobstopmountable.h \
	../gvfs/daemon/gvfsjobpollmountable.c $(gvfs_src_dir)/daemon/gvfsjobpollmountable.h
endif

if GVFS_VERSION_1_14
libdaemon_la_SOURCES += \
	$(gvfs_src_dir)/daemon/gvfsjobprogress.c $(gvfs_src_dir)/daemon/gvfsjobprogress.h
EXTRA_DIST += \
	../gvfs/daemon/dbus-gmain.c $(gvfs_src_dir)/daemon/dbus-gmain.h
else
libdaemon_la_SOURCES += \
	$(gvfs_src_dir)/daemon/dbus-gmain.c $(gvfs_src_dir)/daemon/dbus-gmain.h
EXTRA_DIST += \
	../gvfs/daemon/gvfsjobprogress.c $(gvfs_src_dir)/daemon/gvfsjobprogress.h
endif

if GVFS_VERSION_1_16
libdaemon_la_SOURCES += \
	$(gvfs_src_dir)/daemon/gvfsjoberror.c $(gvfs_src_dir)/daemon/gvfsjoberror.h
else
EXTRA_DIST += \
	../gvfs/daemon/gvfsjoberror.c $(gvfs_src_dir)/daemon/gvfsjoberror.h
endif


mount_in_files = synce.mount.in
mount_DATA = synce.mount

EXTRA_DIST += $(mount_in_files)

DISTCLEANFILES = $(mount_DATA)

gvfsd_synce_SOURCES = \
	gvfsbackendsynce.c gvfsbackendsynce.h \
	$(gvfs_src_dir)/daemon/daemon-main.c $(gvfs_src_dir)/daemon/daemon-main.h \
	$(gvfs_src_dir)/daemon/daemon-main-generic.c 

gvfsd_synce_CPPFLAGS = \
	-DBACKEND_HEADER=gvfsbackendsynce.h \
	-DDEFAULT_BACKEND_TYPE=synce \
	-DMAX_JOB_THREADS=1 \
	$(LIBSYNCE_CFLAGS) \
	-DBACKEND_TYPES='"synce", G_VFS_TYPE_BACKEND_SYNCE,' \
	-DGVFS_LOG_DOMAIN=\"synce-gvfs\" \
	-DG_LOG_DOMAIN=\"synce-gvfs\" \
	$(AM_CPPFLAGS)

gvfsd_synce_LDADD = libdaemon.la $(LIBSYNCE_LIBS) $(GLIB_LIBS)

gvfsd_synce_LDADD += $(DBUS_LIBS)
