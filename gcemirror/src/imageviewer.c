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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "imageviewer.h"
#include "gcemirror-signals-marshal.h"

G_DEFINE_TYPE (ImageViewer, image_viewer, GTK_TYPE_EVENT_BOX)

typedef struct _ImageViewerPrivate ImageViewerPrivate;
struct _ImageViewerPrivate
{
        gboolean disposed;

        GtkImage *image;
        GdkPixbuf *pixbuf;
        GdkPixbufLoader *loader;
        guint currentButton;
};

#define IMAGE_VIEWER_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE((o), IMAGE_VIEWER_TYPE, ImageViewerPrivate))


/* methods */

void
image_viewer_set_pda_size(ImageViewer *self, gint x, gint y)
{
        gtk_widget_set_size_request(GTK_WIDGET(self), x, y);
}


void
image_viewer_draw_image(ImageViewer *self)
{
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        gtk_image_set_from_pixbuf(priv->image, priv->pixbuf);

        if ((GTK_WIDGET_CAN_FOCUS(self)) && (!GTK_WIDGET_HAS_FOCUS(self)))
                gtk_widget_grab_focus(GTK_WIDGET(self));

        return;
}

static void
image_viewer_pixbuf_closed_cb(GdkPixbufLoader *loader, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        image_viewer_set_pda_size(self, gdk_pixbuf_get_width(priv->pixbuf), gdk_pixbuf_get_height(priv->pixbuf));
}

static void
image_viewer_pixbuf_updated_cb(GdkPixbufLoader *loader, gint x, gint y, gint width, gint height, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        if (priv->pixbuf)
                return;

        priv->pixbuf = gdk_pixbuf_loader_get_pixbuf(priv->loader);
        g_object_ref(priv->pixbuf);

        image_viewer_set_pda_size(self, gdk_pixbuf_get_width(priv->pixbuf), gdk_pixbuf_get_height(priv->pixbuf));
}

void
image_viewer_load_image(ImageViewer *self, guchar *data, size_t size)
{
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);
        GError *error = NULL;

        if (priv->pixbuf) {
                g_object_unref(priv->pixbuf);
                priv->pixbuf = NULL;
        }
        if (priv->loader) g_object_unref(priv->loader);

        priv->loader = gdk_pixbuf_loader_new();

        g_signal_connect(G_OBJECT(priv->loader), "area-updated", G_CALLBACK(image_viewer_pixbuf_updated_cb), self);
        g_signal_connect(G_OBJECT(priv->loader), "closed", G_CALLBACK(image_viewer_pixbuf_closed_cb), self);

        if (!gdk_pixbuf_loader_write(priv->loader, data, size, &error)) {
                g_critical("%s: failed to load image data: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
                return;
        }

        if (!gdk_pixbuf_loader_close(priv->loader, &error)) {
                g_critical("%s: failed to close loader: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
                return;
        }

        return;
}


void
image_viewer_print_image(ImageViewer *self)
{
        g_debug("%s: printing not yet implemented", G_STRFUNC);

        return;
}


void
image_viewer_save_image(ImageViewer *self, const gchar *filename)
{
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);
        GError *error = NULL;

        gchar *suffix;
        if ((suffix = g_strrstr(filename, ".")) == NULL) {
                g_warning("%s: cannot determine file type", G_STRFUNC);
                return;
        }

        suffix++;

        if (!gdk_pixbuf_save(priv->pixbuf, filename, suffix, &error, NULL)) {
                g_warning("%s: failed to save file: %s", G_STRFUNC, error->message);
                g_error_free(error);
        }

        return;
}


gboolean
image_viewer_mouse_press_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        g_signal_emit (self, IMAGE_VIEWER_GET_CLASS(self)->signals[MOUSE_BUTTON_PRESSED], 0, event->button, event->x, event->y);

        priv->currentButton = event->button;
        return TRUE;
}

gboolean
image_viewer_mouse_release_event_cb(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        g_signal_emit (self, IMAGE_VIEWER_GET_CLASS(self)->signals[MOUSE_BUTTON_RELEASED], 0, event->button, event->x, event->y);

        priv->currentButton = 0;
        return TRUE;
}

gboolean
image_viewer_mouse_move_event_cb(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        g_signal_emit (self, IMAGE_VIEWER_GET_CLASS(self)->signals[MOUSE_MOVED], 0, priv->currentButton, event->x, event->y);
        return TRUE;
}

gboolean
image_viewer_wheel_event_cb(GtkWidget *widget, GdkEventScroll *event, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);

        if (event->direction == GDK_SCROLL_UP)
                g_signal_emit (self, IMAGE_VIEWER_GET_CLASS(self)->signals[WHEEL_ROLLED], 0, 120);
        else if (event->direction == GDK_SCROLL_DOWN)
                g_signal_emit (self, IMAGE_VIEWER_GET_CLASS(self)->signals[WHEEL_ROLLED], 0, -120);

        return TRUE;
}

gboolean
image_viewer_key_press_event_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);

        g_debug("%s: keypress %d", G_STRFUNC, event->keyval);

        g_signal_emit (self, IMAGE_VIEWER_GET_CLASS(self)->signals[KEY_PRESSED], 0, event->keyval);

        return TRUE;
}


gboolean
image_viewer_key_release_event_cb(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
        ImageViewer *self = IMAGE_VIEWER(user_data);

        g_debug("%s: keyrelease %d", G_STRFUNC, event->keyval);

        g_signal_emit (self, IMAGE_VIEWER_GET_CLASS(self)->signals[KEY_RELEASED], 0, event->keyval);

        return TRUE;
}


/* class & instance functions */

static void
image_viewer_init(ImageViewer *self)
{
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        /*
        gtk_widget_add_events(GTK_WIDGET(self), GDK_KEY_PRESS_MASK);
        gtk_widget_add_events(GTK_WIDGET(self), GDK_KEY_RELEASE_MASK);
        */

        gint events_mask = gtk_widget_get_events(GTK_WIDGET(self));
        events_mask = events_mask | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK;
        gtk_widget_set_events(GTK_WIDGET(self), events_mask);

        GTK_WIDGET_SET_FLAGS(self, GTK_CAN_FOCUS);

        gtk_widget_grab_focus(GTK_WIDGET(self));
        gtk_widget_set_sensitive(GTK_WIDGET(self), TRUE);


        priv->image = GTK_IMAGE(gtk_image_new());
        priv->pixbuf = NULL;
        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(priv->image));

        g_signal_connect(G_OBJECT(self), "button-press-event",
                         G_CALLBACK(image_viewer_mouse_press_event_cb), self);
        g_signal_connect(G_OBJECT(self), "button-release-event",
                         G_CALLBACK(image_viewer_mouse_release_event_cb), self);
        g_signal_connect(G_OBJECT(self), "motion-notify-event",
                         G_CALLBACK(image_viewer_mouse_move_event_cb), self);
        g_signal_connect(G_OBJECT(self), "scroll-event",
                         G_CALLBACK(image_viewer_wheel_event_cb), self);
        g_signal_connect(G_OBJECT(self), "key-press-event",
                         G_CALLBACK(image_viewer_key_press_event_cb), self);
        g_signal_connect(G_OBJECT(self), "key-release-event",
                         G_CALLBACK(image_viewer_key_release_event_cb), self);
}

static void
image_viewer_dispose(GObject *obj)
{
        ImageViewer *self = IMAGE_VIEWER(obj);
        ImageViewerPrivate *priv = IMAGE_VIEWER_GET_PRIVATE (self);

        if (priv->disposed)
                return;
        priv->disposed = TRUE;

        /* unref other objects */

        if (G_OBJECT_CLASS (image_viewer_parent_class)->dispose)
                G_OBJECT_CLASS (image_viewer_parent_class)->dispose (obj);
}

static void
image_viewer_finalize(GObject *obj)
{
        if (G_OBJECT_CLASS(image_viewer_parent_class)->finalize)
                G_OBJECT_CLASS(image_viewer_parent_class)->finalize (obj);
}

static void
image_viewer_class_init(ImageViewerClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

        g_type_class_add_private(klass, sizeof(ImageViewerPrivate));
  
        gobject_class->dispose = image_viewer_dispose;
        gobject_class->finalize = image_viewer_finalize;

        klass->signals[MOUSE_BUTTON_PRESSED] = g_signal_new ("mouse-button-pressed",
                                                     G_OBJECT_CLASS_TYPE (klass),
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                     0,
                                                     NULL, NULL,
                                                     gcemirror_marshal_VOID__UINT_DOUBLE_DOUBLE,
                                                     G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

        klass->signals[MOUSE_BUTTON_RELEASED] = g_signal_new ("mouse-button-released",
                                                     G_OBJECT_CLASS_TYPE (klass),
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                     0,
                                                     NULL, NULL,
                                                     gcemirror_marshal_VOID__UINT_DOUBLE_DOUBLE,
                                                     G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

        klass->signals[MOUSE_MOVED] = g_signal_new ("mouse-moved",
                                                     G_OBJECT_CLASS_TYPE (klass),
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                     0,
                                                     NULL, NULL,
                                                     gcemirror_marshal_VOID__UINT_DOUBLE_DOUBLE,
                                                     G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_DOUBLE, G_TYPE_DOUBLE);

        klass->signals[WHEEL_ROLLED] = g_signal_new ("wheel-rolled",
                                                     G_OBJECT_CLASS_TYPE (klass),
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                     0,
                                                     NULL, NULL,
                                                     g_cclosure_marshal_VOID__INT,
                                                     G_TYPE_NONE, 1, G_TYPE_INT);

        klass->signals[KEY_PRESSED] = g_signal_new ("key-pressed",
                                                     G_OBJECT_CLASS_TYPE (klass),
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                     0,
                                                     NULL, NULL,
                                                     gcemirror_marshal_VOID__UINT,
                                                     G_TYPE_NONE, 1, G_TYPE_UINT);

        klass->signals[KEY_RELEASED] = g_signal_new ("key-released",
                                                     G_OBJECT_CLASS_TYPE (klass),
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                     0,
                                                     NULL, NULL,
                                                     gcemirror_marshal_VOID__UINT,
                                                     G_TYPE_NONE, 1, G_TYPE_UINT);
}


