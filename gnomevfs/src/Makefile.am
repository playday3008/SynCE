module_flags = -export_dynamic -avoid-version -module -no-undefined

modulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
modulesconf_DATA=synce-module.conf

modulesdir=$(libdir)/gnome-vfs-2.0/modules


modules_LTLIBRARIES=libsyncevfs.la

libsyncevfs_la_SOURCES = libsyncevfs.c
libsyncevfs_la_CPPFLAGS = $(LIBSYNCE_CFLAGS) $(LIBRAPI2_CFLAGS) $(VFS_CFLAGS)
libsyncevfs_la_LDFLAGS = $(module_flags)
libsyncevfs_la_LIBADD = $(LIBSYNCE_LIBS) $(LIBRAPI2_LIBS) $(VFS_LIBS)


bin_PROGRAMS = synce-in-computer-folder

synce_in_computer_folder_SOURCES = synce-in-computer-folder.c
synce_in_computer_folder_CPPFLAGS = $(LIBSYNCE_CFLAGS) $(LIBRAPI2_CFLAGS) $(VFS_CFLAGS)
synce_in_computer_folder_LDADD = $(LIBSYNCE_LIBS) $(LIBRAPI2_LIBS) $(VFS_LIBS)


EXTRA_DIST=synce-module.conf
