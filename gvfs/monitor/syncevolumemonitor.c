/*
 * gvfs/monitor/syncevolumemonitor.c
 *
 * Copyright (c) 2013 Mark Ellis <mark@mpellis.org.uk>
 */

#include <config.h>
#include <glib.h>
#include <gmodule.h>
/*
#include <gvfsproxyvolumemonitordaemon.h>
*/
#include <stdio.h>
#include <gio/gio.h>
#include <string.h>
#include <rapi2.h>
#include "syncevolume.h"
#include "syncevolumemonitor.h"

struct _GVfsSynceVolumeMonitor {
  GNativeVolumeMonitor parent;
  GList *volumes;
  IRAPIDesktop *desktop;
  DWORD context;
};

G_DEFINE_TYPE(GVfsSynceVolumeMonitor, g_vfs_synce_volume_monitor, G_TYPE_VOLUME_MONITOR)

static GList *
g_vfs_synce_volume_monitor_get_mounts (GVolumeMonitor *_self)
{
  return NULL;
}

static GList *
g_vfs_synce_volume_monitor_get_volumes (GVolumeMonitor *_self)
{
  GVfsSynceVolumeMonitor *self;
  GList *l;

  self = G_VFS_SYNCE_VOLUME_MONITOR (_self);

  l = g_list_copy (self->volumes);
  g_list_foreach (l, (GFunc)g_object_ref, NULL);

  return l;
}

static GList *
g_vfs_synce_volume_monitor_get_connected_drives (GVolumeMonitor *_self)
{
  return NULL;
}

static gboolean
g_vfs_synce_volume_monitor_is_supported (void)
{
  return TRUE;
}

static void
g_vfs_synce_monitor_create_volume (GVfsSynceVolumeMonitor *self,
                                   IRAPIDevice *device)
{
  GVfsSynceVolume *volume = NULL;

  g_print ("creating volume for device '%s'\n", IRAPIDevice_get_name(device));

  volume = g_vfs_synce_volume_new (G_VOLUME_MONITOR (self), device);
  if (volume != NULL)
    {
      self->volumes = g_list_prepend (self->volumes, volume);
      g_signal_emit_by_name (self, "volume-added", volume);
    }
}

static void
g_vfs_synce_monitor_remove_volume (GVfsSynceVolumeMonitor *self,
                                   IRAPIDevice *device)
{
  GVfsSynceVolume *volume = NULL;
  GList *l;

  for (l = self->volumes; l != NULL; l = l->next)
    {
      volume = l->data;
      gchar *vol_name = g_volume_get_name(G_VOLUME(volume));
      if (strcmp(vol_name, IRAPIDevice_get_name(device)) == 0) {
        g_print ("removing volume for device '%s'\n", vol_name);
        self->volumes = g_list_remove (self->volumes, volume);
        g_signal_emit_by_name (self, "volume-removed", volume);
        g_object_unref (volume);
        break;
      }
      g_free(vol_name);
    }
}

static HRESULT
g_vfs_synce_volume_monitor_device_connected(IRAPISink *sink, IRAPIDevice* device)
{
  GVfsSynceVolumeMonitor *self;

  self = G_VFS_SYNCE_VOLUME_MONITOR(sink->user_data);

  g_vfs_synce_monitor_create_volume (self, device);

  return S_OK;
}

static HRESULT
g_vfs_synce_volume_monitor_device_disconnected(IRAPISink *sink, IRAPIDevice* device)
{
  GVfsSynceVolumeMonitor *self;

  self = G_VFS_SYNCE_VOLUME_MONITOR(sink->user_data);

  g_vfs_synce_monitor_remove_volume (self, device);

  IRAPIDevice_Release(device);

  return S_OK;
}

static void
g_vfs_synce_volume_monitor_dispose (GObject *obj)
{
  GVfsSynceVolumeMonitor *self;

  self = G_VFS_SYNCE_VOLUME_MONITOR(obj);

  IRAPIDesktop_UnAdvise(self->desktop, self->context);
  IRAPIDesktop_Release(self->desktop);

  if (G_OBJECT_CLASS(g_vfs_synce_volume_monitor_parent_class)->finalize)
    (*G_OBJECT_CLASS(g_vfs_synce_volume_monitor_parent_class)->finalize)( G_OBJECT(self));
}

static void
g_vfs_synce_volume_monitor_finalize (GObject *obj)
{
  GVfsSynceVolumeMonitor *self;

  self = G_VFS_SYNCE_VOLUME_MONITOR(obj);

  if (self->volumes)
    g_list_free_full (self->volumes, g_object_unref);

  if (G_OBJECT_CLASS(g_vfs_synce_volume_monitor_parent_class)->finalize)
    (*G_OBJECT_CLASS(g_vfs_synce_volume_monitor_parent_class)->finalize)( G_OBJECT(self));
}

static void
g_vfs_synce_volume_monitor_init(GVfsSynceVolumeMonitor *self)
{
  IRAPIDesktop *desktop = NULL;
  IRAPISink *sink = NULL;
  HRESULT hr;
  DWORD context;

  self->volumes = NULL;
  self->desktop = NULL;
  self->context = 0;

  if (FAILED(hr = IRAPIDesktop_Get(&desktop))) {
    g_print("%s: failed to initialise RAPI: %d: %s\n", 
            G_STRFUNC, hr, synce_strerror_from_hresult(hr));
    return;
  }
  self->desktop = desktop;

  sink = g_malloc0(sizeof(IRAPISink));
  sink->IRAPISink_OnDeviceConnected = g_vfs_synce_volume_monitor_device_connected;
  sink->IRAPISink_OnDeviceDisconnected = g_vfs_synce_volume_monitor_device_disconnected;
  sink->user_data = self;

  if (FAILED(hr = IRAPIDesktop_Advise(desktop, sink, &context))) {
    g_print("%s: failed to register sink: %d: %s\n", 
            G_STRFUNC, hr, synce_strerror_from_hresult(hr));
    return;
  }
  self->context = context;

  g_print ("Volume monitor alive\n");

  return;
}

static void
g_vfs_synce_volume_monitor_class_init (GVfsSynceVolumeMonitorClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GVolumeMonitorClass *monitor_class = G_VOLUME_MONITOR_CLASS(klass);

  gobject_class->dispose = g_vfs_synce_volume_monitor_dispose;
  gobject_class->finalize = g_vfs_synce_volume_monitor_finalize;

  monitor_class->get_mounts = g_vfs_synce_volume_monitor_get_mounts;
  monitor_class->get_volumes = g_vfs_synce_volume_monitor_get_volumes;
  monitor_class->get_connected_drives = g_vfs_synce_volume_monitor_get_connected_drives;
  monitor_class->is_supported = g_vfs_synce_volume_monitor_is_supported;
}

GVolumeMonitor *
g_vfs_synce_volume_monitor_new (void)
{
  return G_VOLUME_MONITOR(g_object_new (G_VFS_TYPE_SYNCE_VOLUME_MONITOR,
                                        NULL));
}

