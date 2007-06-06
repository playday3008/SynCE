#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gnome.h>
#include <synce_log.h>

#include "keyring.h"
#include "utils.h"

char *
get_key_from_keyring (int *item_id)
{
  GnomeKeyringResult      ret;
  GList *                 found_list = NULL;
  GnomeKeyringFound *     found;
  char *                  key;

  ret = gnome_keyring_find_itemsv_sync (GNOME_KEYRING_ITEM_GENERIC_SECRET,
                                        &found_list,
					/* attr.name */
                                        "synce",
                                        GNOME_KEYRING_ATTRIBUTE_TYPE_STRING,
					/* attr.value.string */
                                        "synce",
                                        NULL);
  if (ret != GNOME_KEYRING_RESULT_OK)
  {
    synce_trace("Error retrieving password from keyring. Ret=%d", ret);
    return NULL;
  }    

  found = (GnomeKeyringFound *) found_list->data;
  key = g_strdup (found->secret);
  if (item_id)
    *item_id = found->item_id;
  gnome_keyring_found_list_free (found_list);

  synce_trace("Password from keyring: %s", key);

  return key;
}


void
set_key_in_keyring (const char *key)
{
  GnomeKeyringAttributeList *     attributes;
  GnomeKeyringAttribute           attr;
  GnomeKeyringResult              ret;
  const char *                    display_name;
  guint32                         item_id;

  display_name = g_strdup_printf (_("Passphrase for SynCE Mobile Device"));

  attributes = gnome_keyring_attribute_list_new ();
  attr.name = g_strdup ("synce");
  attr.type = GNOME_KEYRING_ATTRIBUTE_TYPE_STRING;
  attr.value.string = g_strdup ("synce");
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
      synce_trace("Error storing password in keyring. Ret=%d", ret);
      synce_error_dialog(_("Can't store the password in your keyring,\nmake sure you have Gnome keyring installed"));
    }

  gnome_keyring_attribute_list_free (attributes);
}
