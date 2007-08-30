#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gmodule.h>
#include <libgnomevfs/gnome-vfs-init.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-volume.h>
#include <libgnomevfs/gnome-vfs-volume-monitor.h>

#define DISPLAY_NAME "Mobile Device"
#define URI "synce:///"
#define ICON "synce-gnomevfs"

static GnomeVFSVolume *
find_volume(GnomeVFSVolumeMonitor *monitor)
{
  GnomeVFSVolume *volume, *found_vol = NULL;
  GList *volumes, *l;
  gchar *name;

  volumes = gnome_vfs_volume_monitor_get_mounted_volumes (monitor);

  for (l = volumes; l != NULL; l = l->next) {
    volume = l->data;
    name = gnome_vfs_volume_get_display_name(volume);

    if (strcmp(name, DISPLAY_NAME) == 0 ) {
      if (found_vol)
	gnome_vfs_volume_unref(found_vol);
      found_vol = gnome_vfs_volume_ref(volume);
    }
    g_free(name);
    gnome_vfs_volume_unref(volume);
  }

  g_list_free(volumes);
  return found_vol;
}

static void callback (gboolean succeeded,
		      char *error,
		      char *detailed_error,
		      gpointer data)
{
  gchar *name = (gchar *) data;

  if (!succeeded)
    g_warning("gnome-vfs plugin: Failed to disconnect %s: '%s' '%s'", name, error, detailed_error);
  g_free(name);
}


void
module_connect_func (const gchar *device_name)
{
  GnomeVFSVolumeMonitor *monitor;
  GnomeVFSVolume *volume;

  monitor = gnome_vfs_get_volume_monitor ();

  volume = find_volume(monitor);

  if (volume) {
    gnome_vfs_volume_unref(volume);
  } else {
    gnome_vfs_connect_to_server(URI, DISPLAY_NAME, ICON);
  }

  return;
}

void
module_disconnect_func (const gchar *device_name)
{
  GnomeVFSVolumeMonitor *monitor;
  GnomeVFSVolume *volume;

  monitor = gnome_vfs_get_volume_monitor ();

  volume = find_volume(monitor);

  if (volume) {
    gnome_vfs_volume_unmount(volume, callback, gnome_vfs_volume_get_display_name(volume));
    gnome_vfs_volume_unref(volume);
  }

  return;
}

const gchar*
g_module_check_init(GModule *module)
{
  if (!gnome_vfs_init()) {
    g_warning("gnome-vfs plugin: cannot initialize gnome-vfs");
    return "cannot initialize gnome-vfs";
  }

  return NULL;
}

void
g_module_unload(GModule *module)
{
  gnome_vfs_shutdown();

  return;
}
