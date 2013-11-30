/*
 * gvfs/monitor/syncevolumemonitor.h
 *
 * Copyright (c) 2013 Mark Ellis <mark@mpellis.org.uk>
 */

#ifndef SYNCE_VOLUME_MONITOR_H
#define SYNCE_VOLUME_MONITOR_H

#include <glib-object.h>
#include <gio/gio.h>

G_BEGIN_DECLS

#define G_VFS_TYPE_SYNCE_VOLUME_MONITOR   (g_vfs_synce_volume_monitor_get_type())
#define G_VFS_SYNCE_VOLUME_MONITOR(o) (G_TYPE_CHECK_INSTANCE_CAST((o), G_VFS_TYPE_SYNCE_VOLUME_MONITOR, GVfsSynceVolumeMonitor))
#define G_VFS_SYNCE_VOLUME_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_CAST((k), G_VFS_TYPE_SYNCE_VOLUME_MONITOR, GVfsSynceVolumeMonitorClass))
#define G_VFS_IS_SYNCE_VOLUME_MONITOR(o) (G_TYPE_CHECK_INSTANCE_TYPE((o), G_VFS_TYPE_SYNCE_VOLUME_MONITOR))
#define G_VFS_IS_SYNCE_VOLUME_MONITOR_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE((k), G_VFS_TYPE_SYNCE_VOLUME_MONITOR))
#define G_VFS_SYNCE_VOLUME_MONITOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), G_VFS_TYPE_SYNCE_VOLUME_MONITOR, GVfsSynceVolumeMonitorClass))

typedef struct _GVfsSynceVolumeMonitor GVfsSynceVolumeMonitor;
typedef struct _GVfsSynceVolumeMonitorClass GVfsSynceVolumeMonitorClass;

struct _GVfsSynceVolumeMonitorClass {
  GVolumeMonitorClass parent_class;
};

GType g_vfs_synce_volume_monitor_get_type (void) G_GNUC_CONST;

GVolumeMonitor *g_vfs_synce_volume_monitor_new (void);

G_END_DECLS

#endif /* SYNCE_VOLUME_MONITOR_H */

