/*
 * Copyright (C) 2006 Ole André Vadla Ravnås <oleavr@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "projname-objname.h"

G_DEFINE_TYPE (ProjnameObjname, projname_objname, G_TYPE_OBJECT)

/* private stuff */
typedef struct _ProjnameObjnamePrivate ProjnameObjnamePrivate;

struct _ProjnameObjnamePrivate
{
  gboolean dispose_has_run;
};

#define PROJNAME_OBJNAME_GET_PRIVATE(o) \
    (G_TYPE_INSTANCE_GET_PRIVATE((o), PROJNAME_TYPE_OBJNAME, ProjnameObjnamePrivate))

static void
projname_objname_init (ProjnameObjname *self)
{
}

static GObject *
projname_objname_constructor (GType type, guint n_props,
                                 GObjectConstructParam *props)
{
  GObject *obj;
  ProjnameObjnamePrivate *priv;

  obj = G_OBJECT_CLASS (projname_objname_parent_class)->
    constructor (type, n_props, props);

  priv = PROJNAME_OBJNAME_GET_PRIVATE (obj);

  g_assert (priv);

  return obj;
}

static void
projname_objname_dispose (GObject *obj)
{
  ProjnameObjname *self = PROJNAME_OBJNAME (obj);
  ProjnameObjnamePrivate *priv = PROJNAME_OBJNAME_GET_PRIVATE (self);

  if (priv->dispose_has_run)
    return;

  priv->dispose_has_run = TRUE;

  if (G_OBJECT_CLASS (projname_objname_parent_class)->dispose)
    G_OBJECT_CLASS (projname_objname_parent_class)->dispose (obj);
}

static void
projname_objname_finalize (GObject *obj)
{
  ProjnameObjname *self = PROJNAME_OBJNAME (obj);
  ProjnameObjnamePrivate *priv = PROJNAME_OBJNAME_GET_PRIVATE (self);

  g_assert (priv);

  G_OBJECT_CLASS (projname_objname_parent_class)->finalize (obj);
}

static void
projname_objname_class_init (ProjnameObjnameClass *control_channel_class)
{
  GObjectClass *obj_class = G_OBJECT_CLASS (control_channel_class);

  g_type_class_add_private (control_channel_class,
                            sizeof (ProjnameObjnamePrivate));

  obj_class->constructor = projname_objname_constructor;

  obj_class->dispose = projname_objname_dispose;
  obj_class->finalize = projname_objname_finalize;
}

