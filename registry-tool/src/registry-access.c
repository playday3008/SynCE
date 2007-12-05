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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <rapi.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "registry-access.h"

static
extract_hive(const gchar *full_key_path, HKEY *hive, gchar **sub_key)
{
  if ((hive == NULL) || (sub_key == NULL))
    g_error("%s: return location is not valid", G_STRFUNC);

  gchar **key_units = g_strsplit(full_key_path, "\\", 2);

  if (strcmp("HKEY_CLASSES_ROOT", key_units[0]) == 0)
    *hive = HKEY_CLASSES_ROOT;
  else if (strcmp("HKEY_CURRENT_USER", key_units[0]) == 0)
    *hive = HKEY_CURRENT_USER;
  else if (strcmp("HKEY_LOCAL_MACHINE", key_units[0]) == 0)
    *hive = HKEY_LOCAL_MACHINE;
  else if (strcmp("HKEY_USERS", key_units[0]) == 0)
    *hive = HKEY_USERS;
  else
    g_error("%s: invalid hive name", G_STRFUNC);

  if (key_units[1])
    *sub_key = g_strdup(key_units[1]);
  else
    *sub_key = g_strdup("");

  g_strfreev(key_units);
}


GList *
enum_registry_values(GList *list, gchar *key_path)
{
  HKEY key, hkey;
  WCHAR* key_name = NULL;
  LONG result;
  DWORD index;
  DWORD num_subkeys, max_subkey_name_len, max_subkey_class_len,
    num_values, max_value_name_len, max_value_len;
  REG_VALUE_INFO *value_info;
  WCHAR *value_name;
  BYTE *data;

  gchar *tmp_subkey;
  extract_hive(key_path, &hkey, &tmp_subkey);

  key_name = wstr_from_ascii(tmp_subkey);

  result = CeRegOpenKeyEx(hkey, key_name, 0, 0, &key);
  if (result != ERROR_SUCCESS) {
    g_warning("%s: Unable to open key %s", G_STRFUNC, key_path);
    return(list);
  }

  result = CeRegQueryInfoKey(key,
			     NULL, NULL,   /* class name and name size */
			     NULL,         /* unused */
			     &num_subkeys, &max_subkey_name_len, &max_subkey_class_len,
			     &num_values, &max_value_name_len, &max_value_len,
			     NULL, NULL);  /* unused */
  if (result != ERROR_SUCCESS) {
    g_warning("%s: Unable to get info for key %s", G_STRFUNC, key_path);
    return(list);
  }

  value_name = g_malloc0(sizeof(WCHAR) * MAX_PATH);

  index = 0;
  while(TRUE)
  {
    DWORD name_size = MAX_PATH + 1;
    DWORD type;
    DWORD data_size = 0;

    result = CeRegEnumValue(key, index,
			    value_name, &name_size,
			    NULL, NULL,
			    NULL, &data_size);
    /* doesnt set data size !!! */

    if (result == ERROR_NO_MORE_ITEMS) break;

    if (result != ERROR_SUCCESS) {
      g_warning("%s: Unable to enumerate values for registry key %s: %s", G_STRFUNC, key_path,
		synce_strerror(result));
      return(list);
    }

    gchar *tmpname = wstr_to_utf8(value_name);

    result = CeRegQueryValueEx(key, value_name, NULL, &type, NULL, &data_size);
    if (result != ERROR_SUCCESS) {
      g_warning("%s: Failed to get value %s: %s", G_STRFUNC, tmpname,
		synce_strerror(result));
      return(list);
    }

    data = g_malloc0(data_size);
    result = CeRegQueryValueEx(key, value_name, NULL, &type, data, &data_size);
    if (result != ERROR_SUCCESS) {
      g_warning("%s: Failed to get value %s: %s", G_STRFUNC, tmpname,
		synce_strerror(result));
      return(list);
    }

    g_free(tmpname);

    value_info = (REG_VALUE_INFO*) g_malloc0(sizeof(REG_VALUE_INFO));
    value_info->name = wstr_to_utf8(value_name);
    value_info->type = type;
    value_info->data = data;
    value_info->data_len = data_size;
    list = g_list_append(list, value_info);

    data = NULL;

    index++;
  }

  g_free(value_name);

  CeRegCloseKey(key);
  return(list);
}


GList* enum_registry_key(GList *list, char *key_name, GtkWidget *progressbar) 
{
  LONG result;
  HKEY parent_key, hkey;
  WCHAR* parent_key_name = NULL;
  DWORD i;
  REG_KEY_INFO *key_info;

  /* Update the progressbar */
  if (progressbar != NULL) {
    gtk_progress_bar_pulse(GTK_PROGRESS_BAR(progressbar));
    while (gtk_events_pending()) {
      gtk_main_iteration();
    }
  }

  gchar *tmp_subkey;
  extract_hive(key_name, &hkey, &tmp_subkey);

  parent_key_name = wstr_from_ascii(tmp_subkey);

  result = CeRegOpenKeyEx(hkey, parent_key_name, 0, 0, &parent_key);

  if (result != ERROR_SUCCESS) {
      g_warning("%s: Unable to open registry key %s: %s", G_STRFUNC, key_name,
		synce_strerror(result));
      return(list);
  }

  for (i = 0; ; i++) {
    WCHAR wide_name[MAX_PATH];
    DWORD name_size = sizeof(wide_name);
    WCHAR wide_class[MAX_PATH];
    DWORD class_size = sizeof(wide_class);

    result = CeRegEnumKeyEx(parent_key, i, wide_name, &name_size, 
			    NULL, wide_class, &class_size, NULL);

    if (result == ERROR_NO_MORE_ITEMS) break;

    if (result != ERROR_SUCCESS) {
      g_warning("%s: Unable to enumerate keys in registry key %s: %s", G_STRFUNC, key_name,
		synce_strerror(result));
      return(list);
    }

    key_info = (REG_KEY_INFO*) g_malloc0(sizeof(REG_KEY_INFO));
    key_info->name = wstr_to_utf8(wide_name);
    key_info->class = wstr_to_utf8(wide_class);
    list = g_list_append(list,key_info);
  }

  CeRegCloseKey(parent_key);

  return (list);
}
