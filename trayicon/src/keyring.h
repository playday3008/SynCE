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

#if USE_LIBSECRET

#include <libsecret/secret.h>

const SecretSchema * synce_get_schema (void) G_GNUC_CONST;
#define SYNCE_SCHEMA  synce_get_schema ()

gchar *
keyring_get_key(const gchar *name, GError **error);

gboolean
keyring_delete_key(const gchar *name, GError **error);

gboolean
keyring_set_key(const gchar *name, const gchar *key, GError **error);

#else

#include <gnome-keyring.h>

GnomeKeyringResult
keyring_get_key(const gchar *name, gchar **key);

GnomeKeyringResult
keyring_delete_key(const gchar *name);

GnomeKeyringResult
keyring_set_key(const gchar *name, const gchar *key);

#endif
