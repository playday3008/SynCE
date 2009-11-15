gvfs_src_dir = $(top_srcdir)/@with_gvfs_source@

libdaemon_sources = \
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
	$(gvfs_src_dir)/daemon/gvfsjobpush.c $(gvfs_src_dir)/daemon/gvfsjobpush.h \
	$(gvfs_src_dir)/daemon/gvfsjobpull.c $(gvfs_src_dir)/daemon/gvfsjobpull.h \
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
	$(gvfs_src_dir)/daemon/dbus-gmain.h $(gvfs_src_dir)/daemon/dbus-gmain.c \
	$(gvfs_src_dir)/daemon/gvfskeyring.h $(gvfs_src_dir)/daemon/gvfskeyring.c

daemon_sources = \
	$(gvfs_src_dir)/daemon/daemon-main.c \
	$(gvfs_src_dir)/daemon/daemon-main.h \
	$(gvfs_src_dir)/daemon/daemon-main-generic.c 

EXTRA_DIST = 


if GVFS_VERSION_1_2
libdaemon_sources += \
	$(gvfs_src_dir)/daemon/gvfsjobopeniconforread.c $(gvfs_src_dir)/daemon/gvfsjobopeniconforread.h \
	$(gvfs_src_dir)/daemon/gvfsjobqueryinforead.c $(gvfs_src_dir)/daemon/gvfsjobqueryinforead.h \
	$(gvfs_src_dir)/daemon/gvfsjobqueryinfowrite.c $(gvfs_src_dir)/daemon/gvfsjobqueryinfowrite.h
else
EXTRA_DIST += \
	gvfsjobopeniconforread.c gvfsjobopeniconforread.h \
	gvfsjobqueryinforead.c gvfsjobqueryinforead.h \
	gvfsjobqueryinfowrite.c gvfsjobqueryinfowrite.h
endif

if GVFS_VERSION_1_4
libdaemon_sources += \
	$(gvfs_src_dir)/daemon/gvfsjobstartmountable.c $(gvfs_src_dir)/daemon/gvfsjobstartmountable.h \
	$(gvfs_src_dir)/daemon/gvfsjobstopmountable.c $(gvfs_src_dir)/daemon/gvfsjobstopmountable.h \
	$(gvfs_src_dir)/daemon/gvfsjobpollmountable.c $(gvfs_src_dir)/daemon/gvfsjobpollmountable.h
else
EXTRA_DIST += \
	gvfsjobstartmountable.c gvfsjobstartmountable.h \
	gvfsjobstopmountable.c gvfsjobstopmountable.h \
	gvfsjobpollmountable.c gvfsjobpollmountable.h
endif


if LOCAL_LIBDAEMON_SRC

EXTRA_DIST += $(libdaemon_sources) $(daemon_sources)

else

dist-hook:
	cp $(libdaemon_sources) $(daemon_sources) $(distdir)

endif

