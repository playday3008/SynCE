/*
 *  Copyright (C) 2007 Mark Ellis
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef SYNCE_SOFTWARE_MANAGER_H
#define SYNCE_SOFTWARE_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SynceSoftwareManager SynceSoftwareManager;
struct _SynceSoftwareManager {
  GtkHBox parent;
};

typedef struct _SynceSoftwareManagerClass SynceSoftwareManagerClass;
struct _SynceSoftwareManagerClass {
  GtkHBoxClass parent_class;
};

GType synce_software_manager_get_type (void);

#define SYNCE_SOFTWARE_MANAGER_TYPE (synce_software_manager_get_type())
#define SYNCE_SOFTWARE_MANAGER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYNCE_SOFTWARE_MANAGER_TYPE, SynceSoftwareManager))
#define SYNCE_SOFTWARE_MANAGER_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), SYNCE_SOFTWARE_MANAGER_TYPE, SynceSoftwareManagerClass))
#define IS_SYNCE_SOFTWARE_MANAGER(obj) (G_TYPE_CHECK_TYPE ((obj), SYNCE_SOFTWARE_MANAGER_TYPE))
#define IS_SYNCE_SOFTWARE_MANAGER_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), SYNCE_SOFTWARE_MANAGER_TYPE))
#define SYNCE_SOFTWARE_MANAGER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SYNCE_SOFTWARE_MANAGER_TYPE, SynceSoftwareManagerClass))

G_END_DECLS

#endif /* SYNCE_SOFTWARE_MANAGER_H */
