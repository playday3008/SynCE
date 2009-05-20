/***************************************************************************
 * Copyright (c) 2009 Mark Ellis <mark_ellis@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/
#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum _ImageViewerSignals ImageViewerSignals;
enum _ImageViewerSignals
{
  MOUSE_BUTTON_PRESSED,
  MOUSE_BUTTON_RELEASED,
  MOUSE_MOVED,
  WHEEL_ROLLED,
  KEY_PRESSED,
  KEY_RELEASED,
  IMAGE_VIEWER_NUM_SIGNALS
};

typedef struct _ImageViewer ImageViewer;
struct _ImageViewer
{
        GtkEventBox parent;
};

typedef struct _ImageViewerClass ImageViewerClass;
struct _ImageViewerClass {
        GtkEventBoxClass parent_class;
        guint signals[IMAGE_VIEWER_NUM_SIGNALS];
};

GType image_viewer_get_type (void);

#define IMAGE_VIEWER_TYPE \
        (image_viewer_get_type())
#define IMAGE_VIEWER(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST((obj), IMAGE_VIEWER_TYPE, ImageViewer))
#define IMAGE_VIEWER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass), IMAGE_VIEWER_TYPE, ImageViewerClass))
#define IS_IMAGE_VIEWER(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj), IMAGE_VIEWER_TYPE))
#define IS_IMAGE_VIEWER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE((klass), IMAGE_VIEWER_TYPE))
#define IMAGE_VIEWER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS((obj), IMAGE_VIEWER_TYPE, ImageViewerClass))

void image_viewer_draw_image(ImageViewer *self);
void image_viewer_load_image(ImageViewer *self, guchar *data, gsize size);

void image_viewer_print_image(ImageViewer *self);
void image_viewer_save_image(ImageViewer *self, const gchar *filename);
void image_viewer_set_pda_size(ImageViewer *self, int x, int y);

G_END_DECLS

#endif /* IMAGEVIEWER_H */
