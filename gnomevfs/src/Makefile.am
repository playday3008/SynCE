AM_CFLAGS = -g -Wall -Werror
module_flags = -export_dynamic -avoid-version -module -no-undefined

INCLUDES=$(rapi_cflags) $(synce_cflags) $(VFS_CFLAGS)

modulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
modulesconf_DATA=synce-module.conf

modulesdir=$(libdir)/gnome-vfs-2.0/modules

modules_LTLIBRARIES=libsyncevfs.la

libsyncevfs_la_SOURCES = libsyncevfs.c
libsyncevfs_la_LDFLAGS = $(module_flags)
libsyncevfs_la_LIBADD = $(synce_libs) $(rapi_libs) $(VFS_LIBS)

bin_PROGRAMS = synce-in-computer-folder

synce_in_computer_folder_SOURCES = synce-in-computer-folder.c
synce_in_computer_folder_LDADD = $(synce_libs) $(rapi_libs) $(VFS_LIBS)


EXTRA_DIST=synce-module.conf
