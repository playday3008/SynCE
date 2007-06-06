#include <gnome-keyring.h>

char *
get_key_from_keyring (int *item_id);

void
set_key_in_keyring (const char *key);
