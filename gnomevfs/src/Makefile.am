
module_flags = -export_dynamic -avoid-version

INCLUDES = $(rapi_cflags) $(synce_cflags) $(VFS_CFLAGS)

modulesconfdir=@modulesconfdir@
modulesconf_DATA=synce-module.conf

modulesdir=$(VFS_PREFIX)/lib/gnome-vfs-2.0/modules

modules_LTLIBRARIES=libsyncevfs.la

libsyncevfs_la_SOURCES = libsyncevfs.c
libsyncevfs_la_LDFLAGS = $(module_flags)
libsyncevfs_la_LIBADD = $(synce_libs) $(rapi_libs)

