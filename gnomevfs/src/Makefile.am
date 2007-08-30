
INCLUDES  = \
	-DDATADIR=\"$(datadir)\"                        \
	-DPREFIX=\""$(prefix)"\"                        \
	-DSYSCONFDIR=\""$(sysconfdir)"\"                \
	-DLIBDIR=\""$(libdir)"\"                        \
	-DSYNCE_IN_COMPUTER_FOLDER_SH=\""$(datadir)/synce/synce-in-computer-folder.sh"\"

module_flags = -export_dynamic -avoid-version -module -no-undefined

vfsmodulesconfdir=$(sysconfdir)/gnome-vfs-2.0/modules
vfsmodulesconf_DATA=synce-module.conf


vfsmodulesdir=$(libdir)/gnome-vfs-2.0/modules

vfsmodules_LTLIBRARIES=libsyncevfs.la

libsyncevfs_la_SOURCES = libsyncevfs.c
libsyncevfs_la_CPPFLAGS = $(LIBSYNCE_CFLAGS) $(LIBRAPI2_CFLAGS) $(VFS_CFLAGS)
libsyncevfs_la_LDFLAGS = $(module_flags)
libsyncevfs_la_LIBADD = $(LIBSYNCE_LIBS) $(LIBRAPI2_LIBS) $(VFS_LIBS)


bin_PROGRAMS = synce-in-computer-folder

synce_in_computer_folder_SOURCES = synce-in-computer-folder.c
synce_in_computer_folder_CPPFLAGS = $(LIBSYNCE_CFLAGS) $(LIBRAPI2_CFLAGS) $(VFS_CFLAGS)
synce_in_computer_folder_LDADD = $(LIBSYNCE_LIBS) $(LIBRAPI2_LIBS) $(VFS_LIBS)

if ENABLE_TRAYICON_PLUGIN

traymoduledir = $(libdir)/synce-trayicon/modules
traymodule_LTLIBRARIES = gnomevfs-trayicon-module.la
gnomevfs_trayicon_module_la_SOURCES = gnomevfs-trayicon-module.c
gnomevfs_trayicon_module_la_CPPFLAGS = \
        $(GMODULE_CFLAGS) \
        $(VFS_CFLAGS)
gnomevfs_trayicon_module_la_LDFLAGS = -module -avoid-version
gnomevfs_trayicon_module_la_LIBADD = \
        $(GMODULE_LIBS) \
        $(VFS_LIBS)

endif

EXTRA_DIST=synce-module.conf
