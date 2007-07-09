/*
Copyright (c) 2007 Mark Ellis <mark@mpellis.org.uk>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to
deal in the Software without restriction, including without limitation the
rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gnome.h>

#include "keyring.h"
#include "utils.h"


GnomeKeyringResult
keyring_get_key(gchar *name, gchar **key)
{
  GnomeKeyringResult      ret;
  GList *                 found_list = NULL;
  GnomeKeyringFound *     found;

  ret = gnome_keyring_find_itemsv_sync (GNOME_KEYRING_ITEM_GENERIC_SECRET,
                                        &found_list,
					/* attr.name */
                                        "synce",
                                        GNOME_KEYRING_ATTRIBUTE_TYPE_STRING,
					/* attr.value.string */
                                        name,
                                        NULL);
  if (ret != GNOME_KEYRING_RESULT_OK)
  {
    g_debug("%s: Error retrieving password from keyring. Ret=%d %s", G_STRFUNC, ret, keyring_strerror(ret));
    return ret;
  }    

  found = (GnomeKeyringFound *) found_list->data;
  *key = g_strdup (found->secret);
  gnome_keyring_found_list_free (found_list);

  return ret;
}


GnomeKeyringResult
keyring_delete_key(gchar *name)
{
  GnomeKeyringResult      ret;
  GList *                 found_list = NULL;
  GnomeKeyringFound *     found;
  guint32 item_id;


  ret = gnome_keyring_find_itemsv_sync (GNOME_KEYRING_ITEM_GENERIC_SECRET,
                                        &found_list,
					/* attr.name */
                                        "synce",
                                        GNOME_KEYRING_ATTRIBUTE_TYPE_STRING,
					/* attr.value.string */
                                        name,
                                        NULL);
  if (ret != GNOME_KEYRING_RESULT_OK)
  {
    g_debug("%s: Error retrieving password from keyring. Ret=%d %s", G_STRFUNC, ret, keyring_strerror(ret));
    return ret;
  }    

  found = (GnomeKeyringFound *) found_list->data;
  item_id = found->item_id;
  gnome_keyring_found_list_free (found_list);

  ret = gnome_keyring_item_delete_sync("default", item_id);
  return ret;
}


GnomeKeyringResult
keyring_set_key(gchar *name, const gchar *key)
{
  GnomeKeyringAttributeList *     attributes;
  GnomeKeyringAttribute           attr;
  GnomeKeyringResult              ret;
  const gchar *                   display_name;
  guint32                         item_id;

  display_name = g_strdup_printf (_("Passphrase for SynCE Mobile Device %s"), name);

  attributes = gnome_keyring_attribute_list_new ();
  attr.name = g_strdup ("synce");
  attr.type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  attr.value.string = g_strdup (name);
  g_array_append_val (attributes, attr);

  ret = gnome_keyring_item_create_sync (NULL,
					GNOME_KEYRING_ITEM_GENERIC_SECRET,
					display_name,
					attributes,
					key,
					TRUE,
					&item_id);
  if (ret != GNOME_KEYRING_RESULT_OK)
    {
      g_warning("%s: Error storing password in keyring. Ret=%d", G_STRFUNC, ret);
    }

  gnome_keyring_attribute_list_free (attributes);

  return ret;
}


const gchar *
keyring_strerror(GnomeKeyringResult error)
{
  gchar *message;

  switch(error) {
  case GNOME_KEYRING_RESULT_OK:
    message = "Gnome keyring: OK";
    break;
  case GNOME_KEYRING_RESULT_DENIED:
    message = "Gnome keyring: permission denied";
    break;
  case GNOME_KEYRING_RESULT_NO_KEYRING_DAEMON:
    message = "Gnome keyring: no keyring daemon running";
    break;
  case GNOME_KEYRING_RESULT_ALREADY_UNLOCKED:
    message = "Gnome keyring: already unlocked";
    break;
  case GNOME_KEYRING_RESULT_NO_SUCH_KEYRING:
    message = "Gnome keyring: no such keyring";
    break;
  case GNOME_KEYRING_RESULT_BAD_ARGUMENTS:
    message = "Gnome keyring: bad arguments";
    break;
  case GNOME_KEYRING_RESULT_IO_ERROR:
    message = "Gnome keyring: IO error";
    break;
  case GNOME_KEYRING_RESULT_CANCELLED:
    message = "Gnome keyring: cancelled";
    break;
  case GNOME_KEYRING_RESULT_ALREADY_EXISTS:
    message = "Gnome keyring: already exists";
    break;
  default:
    message = "Gnome keyring: unknown error";
    break;
  }
  return message;
}
