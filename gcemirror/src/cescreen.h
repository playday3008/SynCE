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
#ifndef CESCREEN_H
#define CESCREEN_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "imageviewer.h"
#include "decoder.h"

G_BEGIN_DECLS

typedef enum _CeScreenSignals CeScreenSignals;
enum _CeScreenSignals
{
  PDA_ERROR,
  CE_SCREEN_NUM_SIGNALS
};

typedef struct _CeScreen CeScreen;
struct _CeScreen
{
        GtkWindow parent;
};

typedef struct _CeScreenClass CeScreenClass;
struct _CeScreenClass {
        GtkWindowClass parent_class;
        guint signals[CE_SCREEN_NUM_SIGNALS];
};

GType ce_screen_get_type (void);

#define CE_SCREEN_TYPE \
        (ce_screen_get_type())
#define CE_SCREEN(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST((obj), CE_SCREEN_TYPE, CeScreen))
#define CE_SCREEN_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass), CE_SCREEN_TYPE, CeScreenClass))
#define IS_CE_SCREEN(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj), CE_SCREEN_TYPE))
#define IS_CE_SCREEN_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE((klass), CE_SCREEN_TYPE))
#define CE_SCREEN_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS((obj), CE_SCREEN_TYPE, CeScreenClass))

void ce_screen_connect(CeScreen *self, const gchar *pda_name, gboolean is_synce_device, gboolean force_install);

#endif /* CESCREEN_H */
