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

G_BEGIN_DECLS

typedef enum {
  SYNCE_AM_OK,
  SYNCE_AM_FAILED,
  SYNCE_AM_RAPI_ERROR,
  SYNCE_AM_RAPI_TERM_ERROR,
  SYNCE_AM_INVALID_INSTALL_FILE,
  SYNCE_AM_INVALID_PATH
} SynceAppManResult;

typedef void (*SynceAppManBusyFunc) (gpointer user_data);

SynceAppManResult
synce_app_man_install(const gchar *filepath, gchar **error_message, SynceAppManBusyFunc func, gpointer busy_data);

SynceAppManResult
synce_app_man_uninstall(const gchar *program, gchar **error_message);

SynceAppManResult
synce_app_man_create_program_list(GList **list, gchar **error_message, SynceAppManBusyFunc func, gpointer busy_data);

G_END_DECLS

#endif /* SYNCE_APP_MAN_H */
