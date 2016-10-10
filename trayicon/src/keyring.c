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
#include <glib/gi18n.h>

#include "keyring.h"


#if USE_LIBSECRET


const SecretSchema *
synce_get_schema (void)
{
  static const SecretSchema the_schema = {
    "org.synce.Password",
    SECRET_SCHEMA_NONE,
    {
      {  "device", SECRET_SCHEMA_ATTRIBUTE_STRING },
      {  NULL, 0 },
    }
  };
  return &the_schema;
}

#endif


#if !USE_LIBSECRET

GnomeKeyringResult
keyring_get_key(const gchar *name, gchar **key)
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
    g_debug("%s: failed to retrieve password from keyring: %d: %s", G_STRFUNC, ret, gnome_keyring_result_to_message(ret));
    return ret;
  }    

  found = (GnomeKeyringFound *) found_list->data;
  *key = g_strdup (found->secret);
  gnome_keyring_found_list_free (found_list);

  return ret;
}


GnomeKeyringResult
keyring_delete_key(const gchar *name)
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

  if (ret != GNOME_KEYRING_RESULT_OK) {
    g_debug("%s: failed to retrieve password from keyring: %d: %s", G_STRFUNC, ret, gnome_keyring_result_to_message(ret));
    return ret;
  }    

  found = (GnomeKeyringFound *) found_list->data;

  ret = gnome_keyring_item_delete_sync(found->keyring, found->item_id);
  if (ret != GNOME_KEYRING_RESULT_OK)
    g_debug("%s: failed to delete password from keyring: %d: %s", G_STRFUNC, ret, gnome_keyring_result_to_message(ret));

  gnome_keyring_found_list_free (found_list);
  return ret;
}


GnomeKeyringResult
keyring_set_key(const gchar *name, const gchar *key)
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

  gnome_keyring_attribute_list_free (attributes);
  if (ret != GNOME_KEYRING_RESULT_OK)
          g_warning("%s: failed to store password in keyring: %d: %s", G_STRFUNC, ret, gnome_keyring_result_to_message(ret));

  return ret;
}

#else


gchar *
keyring_get_key(const gchar *name, GError **error)
{
  gchar *password = NULL;

  password = secret_password_lookup_sync(SYNCE_SCHEMA,
					 NULL,
					 error,
					 "device", name,
					 NULL);

  if ((!password) && (*error != NULL))
    g_warning("%s: failed to retrieve password from keyring: %s", G_STRFUNC, (*error)->message);

  return password;  
}


gboolean
keyring_delete_key(const gchar *name, GError **error)
{
  gboolean result = FALSE;

  result = secret_password_clear_sync(SYNCE_SCHEMA,
				      NULL,
				      error,
				      "device", name,
				      NULL);
				      
  if (!result)
    g_warning("%s: failed to delete password from keyring: %s", G_STRFUNC, (*error)->message);

  return result;
}


gboolean
keyring_set_key(const gchar *name, const gchar *key, GError **error)
{

  gboolean result = FALSE;
  gchar *display_name = NULL;

  display_name = g_strdup_printf (_("Passphrase for SynCE Mobile Device %s"), name);

  result = secret_password_store_sync(SYNCE_SCHEMA,
    SECRET_COLLECTION_DEFAULT,
    display_name,
    key,
    NULL,
    error,
    "device", name,
    NULL);

  g_free(display_name);
  if (!result)
    g_warning("%s: failed to store password in keyring: %s", G_STRFUNC, (*error)->message);

  return result;
}

#endif
