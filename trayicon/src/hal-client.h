/*
Copyright (c) 2008 Mark Ellis <mark@mpellis.org.uk>

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

#ifndef HAL_CLIENT_H
#define HAL_CLIENT_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _HalClient HalClient;
struct _HalClient {
  GObject parent;
};

typedef struct _HalClientClass HalClientClass;
struct _HalClientClass {
  GObjectClass parent_class;
};

GType hal_client_get_type (void);

#define HAL_CLIENT_TYPE (hal_client_get_type())
#define HAL_CLIENT(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), HAL_CLIENT_TYPE, HalClient))
#define HAL_CLIENT_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), HAL_CLIENT_TYPE, HalClientClass))
#define IS_HAL_CLIENT(obj) (G_TYPE_CHECK_TYPE ((obj), HAL_CLIENT_TYPE))
#define IS_HAL_CLIENT_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), HAL_CLIENT_TYPE))
#define HAL_CLIENT_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), HAL_CLIENT_TYPE, HalClientClass))

G_END_DECLS

#endif /* HAL_CLIENT_H */
