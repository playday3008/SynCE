/*
 * gvfs/monitor/syncevolume.h
 *
 * Copyright (c) 2013 Mark Ellis <mark@mpellis.org.uk>
 */

#ifndef GVFS_MONITOR_SYNCE_SYNCE_VOLUME_H
#define GVFS_MONITOR_SYNCE_SYNCE_VOLUME_H

#include <glib-object.h>
#include <gio/gio.h>
#include <rapi2.h>

#include "syncevolumemonitor.h"

G_BEGIN_DECLS

#define G_VFS_TYPE_SYNCE_VOLUME   (g_vfs_synce_volume_get_type())
#define G_VFS_SYNCE_VOLUME(o) (G_TYPE_CHECK_INSTANCE_CAST((o), G_VFS_TYPE_SYNCE_VOLUME, GVfsSynceVolume))
#define G_VFS_SYNCE_VOLUME_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), G_VFS_TYPE_SYNCE_VOLUME, GVfsSynceVolumeClass))
#define G_VFS_IS_SYNCE_VOLUME(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), G_VFS_TYPE_SYNCE_VOLUME))
#define G_VFS_IS_SYNCE_VOLUME_CLASS(k) ((G_TYPE_CHECK_CLASS_TYPE((k), G_VFS_TYPE_SYNCE_VOLUME))
#define G_VFS_SYNCE_VOLUME_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), G_VFS_TYPE_SYNCE_VOLUME, GVfsSynceVolumeClass))

typedef struct _GVfsSynceVolume GVfsSynceVolume;
typedef struct _GVfsSynceVolumeClass GVfsSynceVolumeClass;

struct _GVfsSynceVolumeClass {
  GObjectClass parent_class;
};

GType g_vfs_synce_volume_get_type (void) G_GNUC_CONST;

GVfsSynceVolume *g_vfs_synce_volume_new (GVolumeMonitor *monitor,
                                         IRAPIDevice *device);

G_END_DECLS

#endif /* GVFS_MONITOR_SYNCE_SYNCE_VOLUME_H */

