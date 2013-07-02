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

#include "cescreen.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <glib/gi18n.h>
#include <synce.h>
#include <rapi2.h>
#include <errno.h>

#include <gdk/gdk.h>

#include "rledecoder.h"
#include "huffmandecoder.h"
#include "xordecoder.h"
#include "imageviewer.h"
#include "gcemirror-signals-marshal.h"

#define XK_MISCELLANY
#define XK_LATIN1
#include <X11/keysymdef.h>


#define MOUSE_PRESSED   1
#define MOUSE_RELEASED  2
#define MOUSE_MOVED     3
#define MOUSE_WHEEL     4
#define KEY_PRESSED     5
#define KEY_RELEASED    6

#define LEFT_BUTTON     1
#define RIGHT_BUTTON    2
#define MID_BUTTON      3

#define SIZE_MESSAGE    1
#define XOR_IMAGE       2
#define KEY_IMAGE       3
#define BMP_HEADER      4

G_DEFINE_TYPE (CeScreen, ce_screen, GTK_TYPE_WINDOW)

typedef struct _CeScreenPrivate CeScreenPrivate;
struct _CeScreenPrivate
{
        gboolean disposed;

        /* private */
        GIOChannel *socket;
        gchar *name;
        gboolean have_header;
        ImageViewer *imageviewer;
        guint32 width;
        guint32 height;
        gboolean pause;
        GtkToolbar *tb;
        GtkStatusbar *statusbar;
        guint statusbar_context;
        GtkWidget *pause_menu_item;
        GtkWidget *pause_tb_item;
        Decoder *decoder_chain;
        guchar *bmp_data;
        guint32 header_size;
        guint32 bmp_size;
        gboolean force_install;
        GtkPrintSettings *print_settings;
        GtkPageSetup *default_print_page;
};

#define CE_SCREEN_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE((o), CE_SCREEN_TYPE, CeScreenPrivate))

/* methods */

static void
ce_screen_update_pause_cb(GtkWidget *widget, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        priv->pause = !priv->pause;

        if (priv->pause) {
                gtk_statusbar_push(priv->statusbar, priv->statusbar_context, _("Pause"));

                gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(priv->pause_tb_item), GTK_STOCK_MEDIA_PLAY);

        } else {
                gtk_statusbar_pop(priv->statusbar, priv->statusbar_context);

                gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(priv->pause_tb_item), GTK_STOCK_MEDIA_PAUSE);

                image_viewer_draw_image(priv->imageviewer);
        }
}

static gsize
sock_read_n(GIOChannel *sock, gchar *buffer, gsize num)
{
        guint32 read_size = 0;
        gsize n = 0;
        GIOStatus status;

        do {
                status = g_io_channel_read_chars(sock, buffer + read_size, num - read_size, &n, NULL);

                read_size += n;
        } while (read_size < num && (status == G_IO_STATUS_NORMAL || status == G_IO_STATUS_AGAIN) );

        return read_size;
}

static gboolean
ce_screen_read_encoded_image(CeScreen *self)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        guint32 raw_size = decoder_chain_read(priv->decoder_chain, priv->socket);
        if (raw_size > 0) {
                if (decoder_chain_decode(priv->decoder_chain, priv->bmp_data + priv->header_size, raw_size)) {

                        image_viewer_load_image(priv->imageviewer, priv->bmp_data, priv->bmp_size);
                        if (!priv->pause) {
                                image_viewer_draw_image(priv->imageviewer);
                        }
                } else {
                        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Decoding error");
                        gtk_dialog_run(GTK_DIALOG(dialog));
                        gtk_widget_destroy(dialog);
                        return false;
                }
        } else {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Conection to PDA broken");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return false;
        }

        return true;
}


static gboolean
ce_screen_read_bmp_header(CeScreen *self)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        guint32 headerSizeN;
        guint32 bmpSizeN;

        gint n = sock_read_n(priv->socket, (gchar*)&bmpSizeN, sizeof(guint32));

        if (n == sizeof(guint32)) {
                priv->bmp_size = ntohl(bmpSizeN);
        } else {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Conection to PDA broken");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return false;
        }

        n = sock_read_n(priv->socket, (gchar*)&headerSizeN, sizeof(guint32));

        if (n != sizeof(guint32)) {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Conection to PDA broken");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return false;
        }


        priv->header_size = ntohl(headerSizeN);
        g_debug("%s: Header size: %d", G_STRFUNC, priv->header_size);
        if (priv->bmp_data != NULL) {
                g_free(priv->bmp_data);
        }
        priv->bmp_data = g_malloc0(sizeof(guchar) * priv->bmp_size);

        n = sock_read_n(priv->socket, (gchar*)priv->bmp_data, priv->header_size);
        if (n <= 0) {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Conection to PDA broken");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                return false;
        }

        return true;
}


static gboolean
ce_screen_read_size_message(CeScreen *self)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        guint32 xN;
        guint32 yN;
        gboolean ret = true;

        gint m = sock_read_n(priv->socket, (gchar*)&xN, sizeof(guint32));
        gint k = sock_read_n(priv->socket, (gchar*)&yN, sizeof(guint32));
        if (m == sizeof(glong) && k == sizeof(guint32)) {
                guint32 x = ntohl(xN);
                guint32 y = ntohl(yN);

                gtk_widget_set_size_request(GTK_WIDGET(self), x, y);
                image_viewer_set_pda_size(priv->imageviewer, x, y);
        } else {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Conection to PDA broken");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                ret = false;
        }

        return ret;
}


static gboolean
ce_screen_read_socket_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
        CeScreen *self = CE_SCREEN(data);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        guchar package_type;

        if (!priv->have_header) {
                guint32 xN;
                guint32 yN;
                gint m;
                gint k;
                gint p;

                p = sock_read_n(priv->socket, (gchar*)&package_type, sizeof(guchar));
                m = sock_read_n(priv->socket, (gchar*)&xN, sizeof(guint32));
                k = sock_read_n(priv->socket, (gchar*)&yN, sizeof(guint32));

                if (m > 0 && k > 0) {
                        priv->width = ntohl(xN);
                        priv->height = ntohl(yN);
                        priv->have_header = true;

                        image_viewer_set_pda_size(priv->imageviewer, priv->width, priv->height);

                        gtk_statusbar_pop(priv->statusbar, priv->statusbar_context);

                        gchar *text = g_strdup_printf(_("Connected to %s"), priv->name);
                        gtk_statusbar_push(priv->statusbar, priv->statusbar_context, text);
                        g_free(text);

                } else {
                        g_debug("%s: Failed to read header", G_STRFUNC);
                        g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);

                        g_io_channel_unref(priv->socket);
                        priv->socket = NULL;
                }
                return TRUE;

        }


        gint p = sock_read_n(priv->socket, (gchar*)&package_type, sizeof(guchar));
        if (p != sizeof(guchar)) {
                GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Connection to PDA broken");
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);

                g_io_channel_unref(priv->socket);
                priv->socket = NULL;
        } else {
                switch(package_type) {
                case XOR_IMAGE:
                        if (!ce_screen_read_encoded_image(self)) {
                                g_io_channel_unref(priv->socket);
                                priv->socket = NULL;
                                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);
                        }
                        break;
                case SIZE_MESSAGE:
                        if (!ce_screen_read_size_message(self)) {
                                g_io_channel_unref(priv->socket);
                                priv->socket = NULL;
                                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);
                        }
                        break;
                case KEY_IMAGE:
                        g_debug("%s: no action for read key_image", G_STRFUNC);
                        break;
                case BMP_HEADER:
                        if (!ce_screen_read_bmp_header(self)) {
                                g_io_channel_unref(priv->socket);
                                priv->socket = NULL;
                                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);
                        }
                        break;
                }
        }
        return TRUE;
}


static gboolean
ce_screen_close_socket_cb(GIOChannel *source, GIOCondition condition, gpointer data)
{
        CeScreen *self = CE_SCREEN(data);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        if (priv->socket != NULL) {
                g_io_channel_unref(priv->socket);
                priv->socket = NULL;
        }
        return TRUE;
}


static void
ce_screen_send_mouse_event(CeScreen *self, guint32 button, guint32 cmd, gdouble x, gdouble y)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);
        GIOStatus status;
        gsize bytes_written;

        if (!priv->pause) {
                g_debug("%s: sending mouse event, button %u, cmd %u, x %f, y %f", G_STRFUNC, button, cmd, x, y);

                guchar buf[4 * sizeof(guint32)];

                *(guint32 *) &buf[sizeof(guint32) * 0] = htonl(cmd);
                *(guint32 *) &buf[sizeof(guint32) * 1] = htonl(button);

                *(guint32 *) &buf[sizeof(guint32) * 2] = htonl((guint32) (65535 * x / priv->width));
                *(guint32 *) &buf[sizeof(guint32) * 3] = htonl((guint32) (65535 * y / priv->height));

                g_debug("%s: sending mouse event, button %u, cmd %u, x %ld, y %ld", G_STRFUNC, button, cmd, ((glong) (65535 * x / priv->width)), ((glong) (65535 * y / priv->height)));
                status = g_io_channel_write_chars(priv->socket,
                                                            (gchar*)buf,
                                                            4 * sizeof(guint32),
                                                            &bytes_written,
                                                            NULL);

                if (bytes_written != (4 * sizeof(guint32)))
                        g_debug("%s: Failure sending mouse event", G_STRFUNC);
        }
}


static void
ce_screen_send_key_event(CeScreen *self, guint32 code, guint32 cmd)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);
        GIOStatus status;
        gsize bytes_written;

        if (!priv->pause) {
                guchar buf[4 * sizeof(guint32)];

                *(guint32 *) &buf[sizeof(guint32) * 0] = htonl(cmd);
                *(guint32 *) &buf[sizeof(guint32) * 1] = htonl(code);
                *(guint32 *) &buf[sizeof(guint32) * 2] = 0;
                *(guint32 *) &buf[sizeof(guint32) * 3] = 0;

                status = g_io_channel_write_chars(priv->socket,
                                                  (gchar*)buf,
                                                  4 * sizeof(guint32),
                                                  &bytes_written,
                                                  NULL);

                if (bytes_written != (4 * sizeof(uint32_t)))
                        g_debug("%s: Failure sending key event", G_STRFUNC);
        }
}


static void
ce_screen_mouse_pressed_cb(ImageViewer *imageviewer, guint button, gdouble x, gdouble y, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);

        glong button_number;

        switch(button) {
        case 1:
                button_number = LEFT_BUTTON;
                g_debug("%s: left button pressed, x = %f, y = %f", G_STRFUNC, x, y);
                break;
        case 2:
                button_number = MID_BUTTON;
                g_debug("%s: middle button pressed, x = %f, y = %f", G_STRFUNC, x, y);
                break;
        case 3:
                button_number = RIGHT_BUTTON;
                g_debug("%s: right button pressed, x = %f, y = %f", G_STRFUNC, x, y);
                break;
        default:
                button_number = 0;
                break;
        }

        ce_screen_send_mouse_event(self, button_number, MOUSE_PRESSED, x, y);
}


static void
ce_screen_mouse_released_cb(ImageViewer *imageviewer, guint button, gdouble x, gdouble y, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);

        glong button_number;

        switch(button) {
        case 1:
                button_number = LEFT_BUTTON;
                break;
        case 2:
                button_number = MID_BUTTON;
                break;
        case 3:
                button_number = RIGHT_BUTTON;
                break;
        default:
                button_number = 0;
                break;
        }

        ce_screen_send_mouse_event(self, button_number, MOUSE_RELEASED, x, y);
}


static void
ce_screen_mouse_moved_cb(ImageViewer *imageviewer, guint button, gdouble x, gdouble y, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);

        glong button_number;

        switch(button) {
        case 1:
                button_number = LEFT_BUTTON;
                break;
        case 2:
                button_number = MID_BUTTON;
                break;
        case 3:
                button_number = RIGHT_BUTTON;
                break;
        default:
                button_number = 0;
                break;
        }

        ce_screen_send_mouse_event(self, button_number, MOUSE_MOVED, x, y);
}


static void
ce_screen_wheel_rolled_cb(ImageViewer *imageviewer, gint delta, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);

        ce_screen_send_mouse_event(self, 0, MOUSE_WHEEL, delta, 0);
}


static guint32
to_key_sym(CeScreen *self, gint code)
{
        switch (code) {
        case GDK_KEY_a: return 'a';
        case GDK_KEY_b: return 'b';
        case GDK_KEY_c: return 'c';
        case GDK_KEY_d: return 'd';
        case GDK_KEY_e: return 'e';
        case GDK_KEY_f: return 'f';
        case GDK_KEY_g: return 'g';
        case GDK_KEY_h: return 'h';
        case GDK_KEY_i: return 'i';
        case GDK_KEY_j: return 'j';
        case GDK_KEY_k: return 'k';
        case GDK_KEY_l: return 'l';
        case GDK_KEY_m: return 'm';
        case GDK_KEY_n: return 'n';
        case GDK_KEY_o: return 'o';
        case GDK_KEY_p: return 'p';
        case GDK_KEY_q: return 'q';
        case GDK_KEY_r: return 'r';
        case GDK_KEY_s: return 's';
        case GDK_KEY_t: return 't';
        case GDK_KEY_u: return 'u';
        case GDK_KEY_v: return 'v';
        case GDK_KEY_w: return 'w';
        case GDK_KEY_x: return 'x';
        case GDK_KEY_y: return 'y';
        case GDK_KEY_z: return 'z';

        case GDK_KEY_A: return 'A';
        case GDK_KEY_B: return 'B';
        case GDK_KEY_C: return 'C';
        case GDK_KEY_D: return 'D';
        case GDK_KEY_E: return 'E';
        case GDK_KEY_F: return 'F';
        case GDK_KEY_G: return 'G';
        case GDK_KEY_H: return 'H';
        case GDK_KEY_I: return 'I';
        case GDK_KEY_J: return 'J';
        case GDK_KEY_K: return 'K';
        case GDK_KEY_L: return 'L';
        case GDK_KEY_M: return 'M';
        case GDK_KEY_N: return 'N';
        case GDK_KEY_O: return 'O';
        case GDK_KEY_P: return 'P';
        case GDK_KEY_Q: return 'Q';
        case GDK_KEY_R: return 'R';
        case GDK_KEY_S: return 'S';
        case GDK_KEY_T: return 'T';
        case GDK_KEY_U: return 'U';
        case GDK_KEY_V: return 'V';
        case GDK_KEY_W: return 'W';
        case GDK_KEY_X: return 'X';
        case GDK_KEY_Y: return 'Y';
        case GDK_KEY_Z: return 'Z';

        case GDK_KEY_Escape:    return XK_Escape;
        case GDK_KEY_Tab:       return XK_Tab;
        case GDK_KEY_BackSpace: return XK_BackSpace;
        case GDK_KEY_Return:    return XK_Return;
        case GDK_KEY_KP_Enter:  return XK_Return;
        case GDK_KEY_Insert:    return XK_Insert;
        case GDK_KEY_Delete:    return XK_Delete;
        case GDK_KEY_Pause:     return XK_Pause;
        case GDK_KEY_Print:     return XK_Print;
        case GDK_KEY_Sys_Req:   return XK_Sys_Req;
        case GDK_KEY_Home:      return XK_Home;
        case GDK_KEY_End:       return XK_End;
        case GDK_KEY_Left:      return XK_Left;
        case GDK_KEY_Up:        return XK_Up;
        case GDK_KEY_Right:     return XK_Right;
        case GDK_KEY_Down:      return XK_Down;
        case GDK_KEY_Prior:     return XK_Prior;
        case GDK_KEY_Next:      return XK_Next;

        case GDK_KEY_Shift_L:     return XK_Shift_L;
        case GDK_KEY_Control_L:   return XK_Control_L;
        case GDK_KEY_Meta_L:      return XK_Meta_L;
        case GDK_KEY_Alt_L:       return XK_Alt_L;
        case GDK_KEY_Caps_Lock:   return XK_Caps_Lock;
        case GDK_KEY_Num_Lock:    return XK_Num_Lock;
        case GDK_KEY_Scroll_Lock: return XK_Scroll_Lock;

        case GDK_KEY_F1:      return XK_F1;
        case GDK_KEY_F2:      return XK_F2;
        case GDK_KEY_F3:      return XK_F3;
        case GDK_KEY_F4:      return XK_F4;
        case GDK_KEY_F5:      return XK_F5;
        case GDK_KEY_F6:      return XK_F6;
        case GDK_KEY_F7:      return XK_F7;
        case GDK_KEY_F8:      return XK_F8;
        case GDK_KEY_F9:      return XK_F9;
        case GDK_KEY_F10:     return XK_F10;
        case GDK_KEY_F11:     return XK_F11;
        case GDK_KEY_F12:     return XK_F12;
        case GDK_KEY_F13:     return XK_F13;
        case GDK_KEY_F14:     return XK_F14;
        case GDK_KEY_F15:     return XK_F15;
        case GDK_KEY_F16:     return XK_F16;
        case GDK_KEY_F17:     return XK_F17;
        case GDK_KEY_F18:     return XK_F18;
        case GDK_KEY_F19:     return XK_F19;
        case GDK_KEY_F20:     return XK_F20;
        case GDK_KEY_F21:     return XK_F21;
        case GDK_KEY_F22:     return XK_F22;
        case GDK_KEY_F23:     return XK_F23;
        case GDK_KEY_F24:     return XK_F24;

        default:
                return 0;
        }

        return 0;
}


static void
ce_screen_key_pressed_cb(ImageViewer *imageviewer, guint code, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);

        g_debug("%s: keypress %d", G_STRFUNC, code);

        guint32 key_code = to_key_sym(self, code);

        if (key_code != 0) {
                ce_screen_send_key_event(self, key_code, KEY_PRESSED);
        } else {
                g_debug("%s: Key with code %d not found in map", G_STRFUNC, code);
        }
}


static void
ce_screen_key_released_cb(ImageViewer *imageviewer, guint code, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);

        guint32 key_code = to_key_sym(self, code);

        if (key_code != 0) {
                ce_screen_send_key_event(self, key_code, KEY_RELEASED);
        } else {
                g_debug("%s: Key with code %d not found in map", G_STRFUNC, code);
        }
}

static void
ce_screen_connect_socket(CeScreen *self, const gchar *device_address)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        gint connectcount = 0;
        gint sockfd = -1;
        struct sockaddr_in addr;
        gint ret;
        GError *error = NULL;

        /* create socket */

        sockfd = socket(PF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
                g_critical("%s: failed to create socket: %d: %s", G_STRFUNC, errno, g_strerror(errno));
                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);
                return;
        }

        memset (&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(1234);
        addr.sin_addr.s_addr = inet_addr(device_address);

        g_debug("%s: attempting connect to %s:%d", G_STRFUNC, device_address, 1234);
        do {
                ret = connect(sockfd, (struct sockaddr *)&addr, sizeof(addr));
                if (ret < 0)
                        g_warning("%s: socket connect failed: %d: %s", G_STRFUNC, errno, g_strerror(errno));

        } while (ret < 0 && connectcount++ < 10);

        if (ret < 0) {
                g_critical("%s: failed to connect to device: %d: %s", G_STRFUNC, errno, g_strerror(errno));
                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);
                return;
        }

        priv->socket = g_io_channel_unix_new(sockfd);
        if (g_io_channel_set_encoding(priv->socket, NULL, &error) != G_IO_STATUS_NORMAL) {
                g_warning("%s: failed to set raw encoding: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
        }
        g_io_channel_set_buffered(priv->socket, FALSE);

        g_io_add_watch(priv->socket, G_IO_IN, ce_screen_read_socket_cb, self);
        g_io_add_watch(priv->socket, G_IO_HUP, ce_screen_close_socket_cb, self);

        return;
}


static void
ce_screen_do_synce_connect(CeScreen *self, const gchar *pda_name)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        HRESULT hr;
        DWORD last_error;
	IRAPIDesktop *desktop = NULL;
	IRAPIEnumDevices *enumdev = NULL;
	IRAPIDevice *device = NULL;
	IRAPISession *session = NULL;
	RAPI_DEVICEINFO devinfo;
        SYSTEM_INFO system;
        PROCESS_INFORMATION proc_info = {0, 0, 0, 0};
        const gchar *device_address = NULL;
        const gchar *arch = NULL;
        WCHAR *widestr = NULL;
        GError *error = NULL;

	if (FAILED(hr = IRAPIDesktop_Get(&desktop)))
	{
	  g_critical("%s: failed to initialise RAPI: %d: %s",
		     G_STRFUNC, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

	if (FAILED(hr = IRAPIDesktop_EnumDevices(desktop, &enumdev)))
	{
	  g_critical("%s: failed to get connected devices: %d: %s",
		     G_STRFUNC, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

	while (SUCCEEDED(hr = IRAPIEnumDevices_Next(enumdev, &device)))
	{
	  if (FAILED(IRAPIDevice_GetDeviceInfo(device, &devinfo)))
	  {
	    g_critical("%s: failure to get device info", G_STRFUNC);
	    goto error_exit;
	  }
	  if (strcmp(pda_name, devinfo.bstrName) == 0)
	    break;
	}

	if (FAILED(hr))
	{
	  g_critical("%s: Could not find device '%s' in RAPI: %08x: %s",
		     G_STRFUNC, priv->name, hr, synce_strerror_from_hresult(hr));
	  device = NULL;
	  goto error_exit;
	}

	IRAPIDevice_AddRef(device);
	IRAPIEnumDevices_Release(enumdev);
	enumdev = NULL;

	if (FAILED(hr = IRAPIDevice_CreateSession(device, &session)))
	{
	  g_critical("%s: Could not create a session to device '%s': %08x: %s",
		     G_STRFUNC, priv->name, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

	if (FAILED(hr = IRAPISession_CeRapiInit(session)))
	{
	  g_critical("%s: Unable to initialize connection to device '%s': %08x: %s", 
		     G_STRFUNC, priv->name, hr, synce_strerror_from_hresult(hr));
	  goto error_exit;
	}

        device_address = IRAPIDevice_get_device_ip(device);

        if (!device_address) {
                g_critical("%s: device address is NULL", G_STRFUNC);
                goto error_exit;
        }

        gchar *remote_snap_uri = g_strdup_printf("synce://%s/Filesystem/Windows/screensnap.exe", pda_name);
        g_debug("%s: uri = %s", G_STRFUNC, remote_snap_uri);
        GFile *remote_snap = g_file_new_for_uri(remote_snap_uri);
        g_free(remote_snap_uri);


        GFileInfo *info = g_file_query_info(remote_snap, "", G_FILE_QUERY_INFO_NONE, NULL, &error);
        if ((info == NULL) && (error->code != G_IO_ERROR_NOT_FOUND)) {
                g_critical("%s: error checking for screensnap on device: %s", G_STRFUNC, error->message);
                g_error_free(error);
                g_object_unref(remote_snap);
                goto error_exit;
        }

        if (info == NULL) {
                g_debug("%s: expecting G_IO_ERROR_NOT_FOUND, got: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
        }

        if ((info == NULL) || (priv->force_install == TRUE)) {
                g_debug("%s: copying screensnap to device", G_STRFUNC);

                IRAPISession_CeGetSystemInfo(session, &system);

                switch(system.wProcessorArchitecture) {
                case PROCESSOR_ARCHITECTURE_MIPS:
                        arch = ".mips";
                        break;
                case PROCESSOR_ARCHITECTURE_SHX:
                        arch = ".shx";
                        break;
                case PROCESSOR_ARCHITECTURE_ARM:
                        arch = ".arm";
                        break;
                }

                gchar *filename = g_strdup_printf("%sscreensnap.exe%s", SYNCE_DATA, arch);
                g_debug("%s: local screensnap name: %s", G_STRFUNC, filename);
                GFile *local_snap = g_file_new_for_path(filename);
                g_free(filename);

                if (!g_file_copy(local_snap,
                                 remote_snap,
                                 G_FILE_COPY_OVERWRITE,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &error)) {
                        g_critical("%s: failed to copy screensnap to device: %s", G_STRFUNC, error->message);
                        g_error_free(error);
                        g_object_unref(remote_snap);
                        g_object_unref(local_snap);
                        goto error_exit;;
                }
                g_object_unref(local_snap);
        }

        g_object_unref(remote_snap);

        widestr = wstr_from_utf8("\\Windows\\screensnap.exe");

        if (!IRAPISession_CeCreateProcess(session, widestr, NULL, NULL, NULL, false, 0, NULL, NULL, NULL, &proc_info)) {
                wstr_free_string(widestr);

                if (FAILED(hr = IRAPISession_CeRapiGetError(session))) {
                        g_critical(_("%s: failed to start screensnap process: %s"), G_STRFUNC, synce_strerror(hr));
                        goto error_exit;
                }

                last_error = IRAPISession_CeGetLastError(session);
                g_critical(_("%s: failed to start screensnap process: %s"), G_STRFUNC, synce_strerror(last_error));
                goto error_exit;
        }
        wstr_free_string(widestr);

        ce_screen_connect_socket(self, device_address);

        IRAPISession_CeRapiUninit(session);
        IRAPISession_Release(session);
	IRAPIDevice_Release(device);
	IRAPIDesktop_Release(desktop);

        return;

 error_exit:
	if (session)
	{
	  IRAPISession_CeRapiUninit(session);
	  IRAPISession_Release(session);
	}

	if (device) IRAPIDevice_Release(device);
	if (enumdev) IRAPIEnumDevices_Release(enumdev);
	if (desktop) IRAPIDesktop_Release(desktop);
        g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);
        return;
}

static void
ce_screen_gvfs_mount_ready(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        GError *error = NULL;


        if (!(g_file_mount_enclosing_volume_finish(G_FILE(source_object), res, &error))) {
                g_critical("%s: failed to gvfs mount for device: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;

                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);

                return;
        }

        ce_screen_do_synce_connect(self, priv->name);

        return;
}


void
ce_screen_connect(CeScreen *self, const gchar *pda_name, gboolean is_synce_device, gboolean force_install)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);
        GError *error = NULL;

        priv->name = g_strdup(pda_name);
        priv->force_install = force_install;

        if (!is_synce_device)
                return ce_screen_connect_socket(self, pda_name);

        gchar *uri = NULL;
        GFile *remote_snap = NULL;
        GMount *mount = NULL;

        uri = g_strdup_printf("synce://%s/Filesystem/Windows/screensnap.exe", pda_name);
        g_debug("%s: uri = %s", G_STRFUNC, uri);
        remote_snap = g_file_new_for_uri(uri);
        g_free(uri);

        mount = g_file_find_enclosing_mount(remote_snap,
                                            NULL,
                                            &error);
        if ((!mount) && (error->code != G_IO_ERROR_NOT_MOUNTED)) {
                g_warning("%s: failed to get gvfs mount for device: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;
                g_signal_emit(self, CE_SCREEN_GET_CLASS(self)->signals[PDA_ERROR], 0);
                goto exit;
        }

        if (!mount) {
                g_debug("%s: failed to get gvfs mount for device: %s", G_STRFUNC, error->message);
                g_error_free(error);
                error = NULL;

                GMountOperation *mount_op = gtk_mount_operation_new(GTK_WINDOW(self));
                g_file_mount_enclosing_volume(remote_snap,
                                              G_MOUNT_MOUNT_NONE,
                                              mount_op,
                                              NULL,
                                              ce_screen_gvfs_mount_ready,
                                              self);

                goto exit;
        }

        ce_screen_do_synce_connect(self, pda_name);

 exit:
        if (remote_snap) g_object_unref(remote_snap);
        if (mount) g_object_unref(mount);

        return;
}


static void
ce_screen_file_save_cb(GtkMenuItem *menuitem, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        GtkWidget *dialog = NULL;
        dialog = gtk_file_chooser_dialog_new("Save Screenshot",
                                             GTK_WINDOW(self),
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                             GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                             NULL);

        gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);

        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), g_get_home_dir());
        gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (dialog), "cescreen");


        GtkFileFilter *filter = gtk_file_filter_new();
        gtk_file_filter_set_name(filter, "Image files");
        gtk_file_filter_add_pixbuf_formats(filter);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER (dialog), filter);

        if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
                gtk_widget_destroy(dialog);
                return;
        }

        gchar *filename;
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        gtk_widget_destroy(dialog);

        if (!filename)
                return;

        g_debug("%s: saving to file %s", G_STRFUNC, filename);

        image_viewer_save_image(priv->imageviewer, filename);
        g_free (filename);
}


static void
ce_screen_begin_print_cb(GtkPrintOperation *operation, GtkPrintContext *context, gpointer user_data)
{
        gtk_print_operation_set_n_pages(operation, 1);
}

static void
ce_screen_draw_page_cb(GtkPrintOperation *operation, GtkPrintContext *context, gint page_nr, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        g_debug("%s: printing page %d", G_STRFUNC, page_nr);

        cairo_t *cr = gtk_print_context_get_cairo_context(context);

        const GdkPixbuf *pixbuf = image_viewer_get_pixbuf(priv->imageviewer);

        gdk_cairo_set_source_pixbuf(cr, pixbuf, 30.0, 30.0);

        cairo_paint(cr);

        return;
}

static void
ce_screen_file_print_cb(GtkMenuItem *menuitem, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        GtkPrintOperation *printop = NULL;

        GtkPrintOperationResult res;
        GError *error = NULL;
        GtkWidget *dialog = NULL;

        printop = gtk_print_operation_new();

        if (priv->print_settings != NULL)
                gtk_print_operation_set_print_settings(printop, priv->print_settings);

        if (priv->default_print_page != NULL)
                gtk_print_operation_set_default_page_setup(printop, priv->default_print_page);

        g_signal_connect (printop, "begin-print", G_CALLBACK (ce_screen_begin_print_cb), self);
        g_signal_connect (printop, "draw-page", G_CALLBACK (ce_screen_draw_page_cb), self);

        res = gtk_print_operation_run (printop,
                                       GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                       GTK_WINDOW(self),
                                       &error);

        if (res == GTK_PRINT_OPERATION_RESULT_ERROR) {
                dialog = gtk_message_dialog_new(GTK_WINDOW(self), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "Error printing screenshot\n%s", error->message);
                gtk_dialog_run(GTK_DIALOG(dialog));
                gtk_widget_destroy(dialog);
                g_error_free(error);
                error = NULL;
        } else if (res == GTK_PRINT_OPERATION_RESULT_APPLY) {
                if (priv->print_settings != NULL)
                        g_object_unref(priv->print_settings);
                priv->print_settings = g_object_ref(gtk_print_operation_get_print_settings(printop));

                if (priv->default_print_page != NULL)
                        g_object_unref(priv->default_print_page);
                priv->default_print_page = g_object_ref(gtk_print_operation_get_default_page_setup(printop));
        }

        g_object_unref(printop);
        return;
}

static const gchar *MITlicense = N_(
    "Copyright (c) 2009 Mark Ellis\n"
    "Copyright (c) 2003, Volker Christian\n"
    "\n"
    "Permission is hereby granted, free of charge, to\n"
    "any person obtaining a copy of this software and\n"
    "associated documentation files (the \"Software\"), to\n"
    "deal in the Software without restriction, including\n"
    "without limitation the rights to use, copy, modify,\n"
    "merge, publish, distribute, sublicense, and/or sell\n"
    "copies of the Software, and to permit persons to whom\n"
    "the Software is furnished to do so, subject to the\n"
    "following conditions:\n"
    "\n"
    "The above copyright notice and this permission notice\n"
    "shall be included in all copies or substantial portions\n"
    "of the Software.\n"
    "\n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF\n"
    "ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED\n"
    "TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A\n"
    "PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT\n"
    "SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR\n"
    "ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN\n"
    "ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT\n"
    "OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
    "OTHER DEALINGS IN THE SOFTWARE.");

static void
ce_screen_show_about_cb(GtkMenuItem *menuitem, gpointer user_data)
{
        CeScreen *self = CE_SCREEN(user_data);

        GtkWidget *about;
        const gchar* authors[] = {
                "Mark Ellis <mark_ellis@users.sourceforge.net>",
                "Volker Christian <voc@users.sourceforge.net>",
                NULL
        };

        about = gtk_about_dialog_new();

        gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(about), g_get_application_name());
        gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), VERSION);
        gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), _("Copyright (c) 2009, Mark Ellis\n"
                                                                  "Copyright (c) 2003, Volker Christian"));
        gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about), MITlicense);

        gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), _("GCEMirror, a PocketPC Control Tool"));
        gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "http://www.synce.org");

        gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);

        gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(self));

        gtk_dialog_run (GTK_DIALOG (about));
        gtk_widget_destroy (GTK_WIDGET(about));

        return;
}

void
ce_screen_quit_cb(GtkMenuItem *menuitem, gpointer user_data)
{
        gtk_main_quit();
}


/* class & instance functions */


static void
ce_screen_init(CeScreen *self)
{
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        priv->socket = NULL;
        priv->have_header = FALSE;
        priv->pause = FALSE;
        priv->print_settings = NULL;
        priv->default_print_page = NULL;

        /*
        gtk_window_set_default_size(GTK_WINDOW(self), 200, 200);
        */
        gtk_window_set_resizable(GTK_WINDOW(self), FALSE);

        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

        /*
         * create menus
         */
        GtkMenuBar *menubar = GTK_MENU_BAR(gtk_menu_bar_new());
        GtkMenu *menu = GTK_MENU(gtk_menu_new());
        GtkWidget *entry = NULL;

        entry = gtk_image_menu_item_new_with_label(_("Screenshot"));
        gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(entry), gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU));
        g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(ce_screen_file_save_cb), self);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);

        entry = gtk_image_menu_item_new_from_stock(GTK_STOCK_PRINT, NULL);
        g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(ce_screen_file_print_cb), self);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);

        entry = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);

        priv->pause_menu_item = gtk_image_menu_item_new_from_stock(GTK_STOCK_MEDIA_PAUSE, NULL);
        g_signal_connect(G_OBJECT(priv->pause_menu_item), "activate", G_CALLBACK(ce_screen_update_pause_cb), self);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), priv->pause_menu_item);

        entry = gtk_separator_menu_item_new();
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);

        entry = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
        g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(ce_screen_quit_cb), self);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);

        entry = gtk_menu_item_new_with_label(_("File"));
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(entry), GTK_WIDGET(menu));

        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), entry);


        menu = GTK_MENU(gtk_menu_new());

        entry = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
        g_signal_connect(G_OBJECT(entry), "activate", G_CALLBACK(ce_screen_show_about_cb), self);
        gtk_menu_shell_append(GTK_MENU_SHELL(menu), entry);

        entry = gtk_menu_item_new_with_label(_("Help"));
        gtk_menu_item_set_submenu(GTK_MENU_ITEM(entry), GTK_WIDGET(menu));

        gtk_menu_shell_append(GTK_MENU_SHELL(menubar), entry);

        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(menubar), FALSE, FALSE, 3);

        /*
         * create toolbar
         */
        GtkToolbar *toolbar = GTK_TOOLBAR(gtk_toolbar_new());
        GtkToolItem *tool_entry = NULL;

        tool_entry = gtk_tool_button_new_from_stock(GTK_STOCK_QUIT);
        g_signal_connect(G_OBJECT(tool_entry), "clicked", G_CALLBACK(ce_screen_quit_cb), self);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_entry, -1);

        tool_entry = gtk_tool_button_new(gtk_image_new_from_stock(GTK_STOCK_SAVE, GTK_ICON_SIZE_MENU), _("Screenshot"));
        g_signal_connect(G_OBJECT(tool_entry), "clicked", G_CALLBACK(ce_screen_file_save_cb), self);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_entry, -1);

        tool_entry = gtk_tool_button_new_from_stock(GTK_STOCK_PRINT);
        g_signal_connect(G_OBJECT(tool_entry), "clicked", G_CALLBACK(ce_screen_file_print_cb), self);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), tool_entry, -1);

        priv->pause_tb_item = GTK_WIDGET(gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PAUSE));
        g_signal_connect(G_OBJECT(priv->pause_tb_item), "clicked", G_CALLBACK(ce_screen_update_pause_cb), self);
        gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(priv->pause_tb_item), -1);

        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(toolbar), FALSE, FALSE, 3);

        /*
         * 2 boxes required to ensure the ImageViewer event box doesn't get allocated extra space
         */
        GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        priv->imageviewer = g_object_new(IMAGE_VIEWER_TYPE, NULL);
        gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(priv->imageviewer), FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(hbox), FALSE, FALSE, 3);

        g_signal_connect(G_OBJECT(priv->imageviewer), "wheel-rolled", G_CALLBACK(ce_screen_wheel_rolled_cb), self);
        g_signal_connect(G_OBJECT(priv->imageviewer), "key-pressed", G_CALLBACK(ce_screen_key_pressed_cb), self);
        g_signal_connect(G_OBJECT(priv->imageviewer), "key-released", G_CALLBACK(ce_screen_key_released_cb), self);
        g_signal_connect(G_OBJECT(priv->imageviewer), "mouse-button-pressed", G_CALLBACK(ce_screen_mouse_pressed_cb), self);
        g_signal_connect(G_OBJECT(priv->imageviewer), "mouse-button-released", G_CALLBACK(ce_screen_mouse_released_cb), self);
        g_signal_connect(G_OBJECT(priv->imageviewer), "mouse-moved", G_CALLBACK(ce_screen_mouse_moved_cb), self);


        priv->statusbar = GTK_STATUSBAR(gtk_statusbar_new());
        priv->statusbar_context = gtk_statusbar_get_context_id(priv->statusbar, "gcemirror");
        gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(priv->statusbar), FALSE, FALSE, 3);
        gtk_statusbar_push(priv->statusbar, priv->statusbar_context, _("Connecting..."));

        gtk_container_add(GTK_CONTAINER(self), GTK_WIDGET(vbox));

        priv->decoder_chain = NULL;
        priv->decoder_chain = g_object_new(HUFFMAN_DECODER_TYPE, "chain", priv->decoder_chain, NULL);
        priv->decoder_chain = g_object_new(RLE_DECODER_TYPE, "chain", priv->decoder_chain, NULL);
        priv->decoder_chain = g_object_new(XOR_DECODER_TYPE, "chain", priv->decoder_chain, NULL);

        priv->bmp_data = NULL;
}

static void
ce_screen_dispose(GObject *obj)
{
        CeScreen *self = CE_SCREEN(obj);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        if (priv->disposed)
                return;
        priv->disposed = TRUE;

        /* unref other objects */

        if (priv->socket != NULL) {
                g_io_channel_shutdown(priv->socket, FALSE, NULL);
                g_io_channel_unref(priv->socket);
        }

        g_object_unref(priv->decoder_chain);
        if (priv->print_settings) g_object_unref(priv->print_settings);
        if (priv->default_print_page) g_object_unref(priv->default_print_page);

        if (G_OBJECT_CLASS (ce_screen_parent_class)->dispose)
                G_OBJECT_CLASS (ce_screen_parent_class)->dispose (obj);
}

static void
ce_screen_finalize(GObject *obj)
{
        CeScreen *self = CE_SCREEN(obj);
        CeScreenPrivate *priv = CE_SCREEN_GET_PRIVATE (self);

        g_free(priv->name);
        g_free(priv->bmp_data);

        if (G_OBJECT_CLASS(ce_screen_parent_class)->finalize)
                G_OBJECT_CLASS(ce_screen_parent_class)->finalize (obj);
}

static void
ce_screen_class_init(CeScreenClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS(klass);

        g_type_class_add_private(klass, sizeof(CeScreenPrivate));
  
        gobject_class->dispose = ce_screen_dispose;
        gobject_class->finalize = ce_screen_finalize;

        klass->signals[PDA_ERROR] = g_signal_new ("pda-error",
                                                     G_OBJECT_CLASS_TYPE (klass),
                                                     G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                                                     0,
                                                     NULL, NULL,
                                                     g_cclosure_marshal_VOID__VOID,
                                                     G_TYPE_NONE, 0, G_TYPE_NONE);
}


