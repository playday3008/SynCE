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

#ifndef ODCCM_CLIENT_H
#define ODCCM_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _OdccmClient OdccmClient;
struct _OdccmClient {
  GObject parent;
};

typedef struct _OdccmClientClass OdccmClientClass;
struct _OdccmClientClass {
  GObjectClass parent_class;
};

GType odccm_client_get_type (void);

#define ODCCM_CLIENT_TYPE (odccm_client_get_type())
#define ODCCM_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ODCCM_CLIENT_TYPE, OdccmClient))
#define ODCCM_CLIENT_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), ODCCM_CLIENT_TYPE, OdccmClientClass))
#define IS_ODCCM_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ODCCM_CLIENT_TYPE))
#define IS_ODCCM_CLIENT_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), ODCCM_CLIENT_TYPE))
#define ODCCM_CLIENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ODCCM_CLIENT_TYPE, OdccmClientClass))

G_END_DECLS

#endif /* ODCCM_CLIENT_H */
