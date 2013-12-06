/*
 * gvfs/monitor/syncevolume.c
 *
 * Copyright (c) 2013 Mark Ellis <mark@mpellis.org.uk>
 */

#include <config.h>
#include <string.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n.h>

#include <rapi2.h>

#include "syncevolume.h"

struct _GVfsSynceVolume {
  GObject parent;

  GVolumeMonitor *monitor;

  char *uuid;

  char *name;
  char *icon;
  char *symbolic_icon;
  char *icon_fallback;
  IRAPIDevice *device;
};

static void g_vfs_synce_volume_iface_init (GVolumeIface *iface);

G_DEFINE_TYPE_EXTENDED(GVfsSynceVolume, g_vfs_synce_volume, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(G_TYPE_VOLUME, g_vfs_synce_volume_iface_init))

static char *
g_vfs_synce_volume_get_name (GVolume *volume)
{
  GVfsSynceVolume *synce_volume = G_VFS_SYNCE_VOLUME (volume);
  char *name;

  name = g_strdup (synce_volume->name);

  return name;
}

static GIcon *
g_vfs_synce_volume_get_icon (GVolume *volume)
{
  GVfsSynceVolume *synce_volume = G_VFS_SYNCE_VOLUME (volume);
  GIcon *icon;

  icon = g_themed_icon_new_with_default_fallbacks (synce_volume->icon);

  return icon;
}

static GIcon *
g_vfs_synce_volume_get_symbolic_icon (GVolume *volume)
{
  GVfsSynceVolume *synce_volume = G_VFS_SYNCE_VOLUME (volume);
  GIcon *icon;

  icon = g_themed_icon_new_with_default_fallbacks (synce_volume->symbolic_icon);

  return icon;
}

static char *
g_vfs_synce_volume_get_uuid (GVolume *volume)
{
  GVfsSynceVolume *synce_volume = G_VFS_SYNCE_VOLUME (volume);

  return NULL;
}

static gboolean
g_vfs_synce_volume_can_mount (GVolume *volume)
{
  return TRUE;
}

static gboolean
g_vfs_synce_volume_can_eject (GVolume *volume)
{
  return FALSE;
}

static gboolean
g_vfs_synce_volume_should_automount (GVolume *volume)
{
  return FALSE;
}

static GDrive *
g_vfs_synce_volume_get_drive (GVolume *volume)
{
  return NULL;
}

static GMount *
g_vfs_synce_volume_get_mount (GVolume *volume)
{
  return NULL;
}

typedef struct
{
  GVfsSynceVolume *enclosing_volume;
  GAsyncReadyCallback  callback;
  GFile *root;
  gpointer user_data;
} ActivationMountOp;

static void
mount_callback (GObject *source_object,
                GAsyncResult *res,
                gpointer user_data)
{
  ActivationMountOp *data = user_data;
  data->callback (G_OBJECT (data->enclosing_volume), res, data->user_data);
  g_object_unref (data->root);
  g_free (data);
}

static void
g_vfs_synce_volume_mount (GVolume             *volume,
                          GMountMountFlags     flags,
                          GMountOperation     *mount_operation,
                          GCancellable        *cancellable,
                          GAsyncReadyCallback  callback,
                          gpointer             user_data)
{
  GVfsSynceVolume *synce_volume = G_VFS_SYNCE_VOLUME (volume);
  ActivationMountOp *data;
  GFile *root;

  g_print ("g_vfs_synce_volume_mount (can_mount=%d uuid=%s)\n",
           g_vfs_synce_volume_can_mount (volume),
           synce_volume->uuid);

  root = g_object_get_data (G_OBJECT (volume), "root");

  data = g_new0 (ActivationMountOp, 1);
  data->enclosing_volume = synce_volume;
  data->callback = callback;
  data->user_data = user_data;
  data->root = root;
  g_object_ref(data->root);

  g_file_mount_enclosing_volume (root,
                                 0,
                                 mount_operation,
                                 cancellable,
                                 mount_callback,
                                 data);
}

static gboolean
g_vfs_synce_volume_mount_finish (GVolume       *volume,
                                 GAsyncResult  *result,
                                 GError       **error)
{
  GFile *root;
  gboolean res;

  root = g_object_get_data (G_OBJECT (volume), "root");
  res = g_file_mount_enclosing_volume_finish (root, result, error);

  return res;
}

static char *
g_vfs_synce_volume_get_identifier (GVolume              *volume,
                                   const char          *kind)
{
  GVfsSynceVolume *synce_volume = G_VFS_SYNCE_VOLUME (volume);
  char *id;

  id = NULL;
  if (g_str_equal (kind, G_VOLUME_IDENTIFIER_KIND_LABEL) != FALSE)
    id = g_strdup (synce_volume->uuid);

  return id;
}

static char **
g_vfs_synce_volume_enumerate_identifiers (GVolume *volume)
{
  GVfsSynceVolume *synce_volume = G_VFS_SYNCE_VOLUME (volume);
  GPtrArray *res;

  res = g_ptr_array_new ();

  if (synce_volume->name)
    {
        g_ptr_array_add (res,
                         g_strdup (G_VOLUME_IDENTIFIER_KIND_LABEL));
    }

  /* Null-terminate */
  g_ptr_array_add (res, NULL);

  return (char **)g_ptr_array_free (res, FALSE);
}

static GFile *
g_vfs_synce_volume_get_activation_root (GVolume *volume)
{
  GFile *root;

  root = g_object_get_data (G_OBJECT (volume), "root");
  if (root == NULL)
    return NULL;

  return g_object_ref (root);
}

static const gchar *
g_vfs_synce_volume_get_sort_key (GVolume *volume)
{
  return NULL;
}

GVfsSynceVolume *
g_vfs_synce_volume_new (GVolumeMonitor *monitor,
                        IRAPIDevice *device)
{
  GVfsSynceVolume *self;
  GFile *root;
  char *uri;

  self = G_VFS_SYNCE_VOLUME(g_object_new (G_VFS_TYPE_SYNCE_VOLUME, NULL));
  self->monitor = monitor;

  uri = g_strdup_printf ("synce://%s", IRAPIDevice_get_name(device));
  root = g_file_new_for_uri (uri);
  g_free (uri);
  g_object_set_data_full (G_OBJECT(self), "root", root, g_object_unref);

  self->device = device;
  IRAPIDevice_AddRef(device);
  self->name = g_strdup(IRAPIDevice_get_name(device));

  return self;
}

static void
g_vfs_synce_volume_finalize (GObject *object)
{
  GVfsSynceVolume *self = NULL;

  self = G_VFS_SYNCE_VOLUME(object);

  self->monitor = NULL;
  g_free (self->uuid);
  g_free (self->name);
  g_free (self->icon);
  g_free (self->symbolic_icon);
  g_free (self->icon_fallback);

  if (self->device) {
    IRAPIDevice_Release(self->device);
    self->device = NULL;
  }

  if (G_OBJECT_CLASS(g_vfs_synce_volume_parent_class)->finalize)
    (*G_OBJECT_CLASS(g_vfs_synce_volume_parent_class)->finalize) (G_OBJECT(self));
}

static void
g_vfs_synce_volume_init (GVfsSynceVolume *self)
{
  self->monitor = NULL;
  self->uuid = NULL;
  self->name = NULL;
  self->device = NULL;

  self->icon = g_strdup ("synce-gvfs");
  self->symbolic_icon = g_strdup ("synce-gvfs");
  self->icon_fallback = NULL;
}

static void
g_vfs_synce_volume_class_init (GVfsSynceVolumeClass *klass)
{
  GObjectClass *gobject_class;
  gobject_class = G_OBJECT_CLASS(klass);
  gobject_class->finalize = g_vfs_synce_volume_finalize;
}

static void
g_vfs_synce_volume_iface_init (GVolumeIface *iface)
{
  iface->get_name = g_vfs_synce_volume_get_name;
  iface->get_icon = g_vfs_synce_volume_get_icon;
#if GLIB_CHECK_VERSION(2,34,0)
  iface->get_symbolic_icon = g_vfs_synce_volume_get_symbolic_icon;
#endif
  iface->get_uuid = g_vfs_synce_volume_get_uuid;
  iface->get_drive = g_vfs_synce_volume_get_drive;
  iface->get_mount = g_vfs_synce_volume_get_mount;
  iface->can_mount = g_vfs_synce_volume_can_mount;
  iface->can_eject = g_vfs_synce_volume_can_eject;
  iface->should_automount = g_vfs_synce_volume_should_automount;
  iface->mount_fn = g_vfs_synce_volume_mount;
  iface->mount_finish = g_vfs_synce_volume_mount_finish;
  iface->eject = NULL;
  iface->eject_finish = NULL;
#if GLIB_CHECK_VERSION(2,22,0)
  iface->eject_with_operation = NULL;
  iface->eject_with_operation_finish = NULL;
#endif
  iface->get_identifier = g_vfs_synce_volume_get_identifier;
  iface->enumerate_identifiers = g_vfs_synce_volume_enumerate_identifiers;
  iface->get_activation_root = g_vfs_synce_volume_get_activation_root;
#if GLIB_CHECK_VERSION(2,32,0)
  iface->get_sort_key = g_vfs_synce_volume_get_sort_key;
#endif
}

