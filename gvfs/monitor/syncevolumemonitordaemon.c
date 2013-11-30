/*
 * gvfs/monitor/syncevolumemonitordaemon.c
 *
 * Copyright (c) 2013 Mark Ellis <mark@mpellis.org.uk>
 */

#include <config.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <gio/gio.h>
#include <locale.h>

#include <gvfsproxyvolumemonitordaemon.h>

#include "syncevolumemonitor.h"

int
main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  bindtextdomain (GETTEXT_PACKAGE, GVFS_LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  g_vfs_proxy_volume_monitor_daemon_init ();
  return g_vfs_proxy_volume_monitor_daemon_main (argc,
                                                 argv,
                                                 "org.gtk.Private.SynceVolumeMonitor",
                                                 G_VFS_TYPE_SYNCE_VOLUME_MONITOR);
}

