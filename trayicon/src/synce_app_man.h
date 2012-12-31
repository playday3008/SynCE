/*
 *  Copyright (C) 2008 Mark Ellis
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

#ifndef SYNCE_APP_MAN_H
#define SYNCE_APP_MAN_H

#include <rapi2.h>

G_BEGIN_DECLS

typedef enum {
  SYNCE_APP_MAN_ERROR_FAILED,
  SYNCE_APP_MAN_ERROR_RAPI,
  SYNCE_APP_MAN_ERROR_RAPI_TERM,
  SYNCE_APP_MAN_ERROR_INVALID_INSTALL_FILE,
  SYNCE_APP_MAN_ERROR_INVALID_PATH
} SynceAppManError;

GQuark synce_app_man_error_quark (void);
#define SYNCE_APP_MAN_ERROR synce_app_man_error_quark ()


typedef void (*SynceAppManBusyFunc) (gpointer user_data);

gboolean
synce_app_man_install(IRAPISession *session, const gchar *filepath, SynceAppManBusyFunc func, gpointer busy_data, GError **error);

gboolean
synce_app_man_uninstall(IRAPISession *session, const gchar *program, GError **error);

gboolean
synce_app_man_create_program_list(IRAPISession *session, GList **list, SynceAppManBusyFunc func, gpointer busy_data, GError **error);

G_END_DECLS

#endif /* SYNCE_APP_MAN_H */
