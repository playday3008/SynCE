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

#ifndef SYNCE_TRAYICON_H
#define SYNCE_TRAYICON_H

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SynceTrayIcon SynceTrayIcon;
struct _SynceTrayIcon {
  GObject parent;
};

typedef struct _SynceTrayIconClass SynceTrayIconClass;
struct _SynceTrayIconClass {
  GObjectClass parent_class;
};

GType synce_trayicon_get_type (void);

#define SYNCE_TRAYICON_TYPE (synce_trayicon_get_type())
#define SYNCE_TRAYICON(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SYNCE_TRAYICON_TYPE, SynceTrayIcon))
#define SYNCE_TRAYICON_CLASS(c) (G_TYPE_CHECK_CLASS_CAST ((c), SYNCE_TRAYICON_TYPE, SynceTrayIconClass))
#define IS_SYNCE_TRAYICON(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SYNCE_TRAYICON_TYPE))
#define IS_SYNCE_TRAYICON_CLASS(c) (G_TYPE_CHECK_CLASS_TYPE ((c), SYNCE_TRAYICON_TYPE))
#define SYNCE_TRAYICON_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SYNCE_TRAYICON_TYPE, SynceTrayIconClass))

G_END_DECLS

#endif /* SYNCE_TRAYICON_H */
