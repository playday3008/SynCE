/***************************************************************************
 * Copyright (c) 2009 Mark Ellis <mark_ellis@users.sourceforge.net>         *
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
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

#include "xordecoder.h"
#include <string.h>

G_DEFINE_TYPE (XorDecoder, xor_decoder, DECODER_TYPE)

typedef struct _XorDecoderPrivate XorDecoderPrivate;
struct _XorDecoderPrivate
{
        gboolean disposed;

        guchar *old_data;
};

#define XOR_DECODER_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE((o), XOR_DECODER_TYPE, XorDecoderPrivate))


static gboolean
xor_decoder_decode_impl(XorDecoder *self, guchar *raw_data, gsize raw_size, guchar *enc_data, gsize enc_size)
{
        XorDecoderPrivate *priv = XOR_DECODER_GET_PRIVATE (self);
        gsize i;

        if (priv->old_data == NULL) {
                priv->old_data = g_malloc0(sizeof(guchar) * enc_size);
                memset(priv->old_data, 0, enc_size);
        }

        if (priv->old_data != NULL) {
                for (i = 0; i < enc_size; i++) {
                        priv->old_data[i] = raw_data[i] = enc_data[i] ^ priv->old_data[i];
                }
        }

        return enc_size == raw_size;
}

static void
xor_decoder_init(XorDecoder *self)
{
        XorDecoderPrivate *priv = XOR_DECODER_GET_PRIVATE (self);
        priv->disposed = FALSE;

        priv->old_data = NULL;
}

static void
xor_decoder_dispose(GObject *obj)
{
        XorDecoder *self = XOR_DECODER(obj);
        XorDecoderPrivate *priv = XOR_DECODER_GET_PRIVATE (self);

        if (priv->disposed)
                return;
        priv->disposed = TRUE;

        g_free(priv->old_data);
        priv->old_data = NULL;

        if (G_OBJECT_CLASS (xor_decoder_parent_class)->dispose)
                G_OBJECT_CLASS (xor_decoder_parent_class)->dispose (obj);
}

static void
xor_decoder_finalize (GObject *obj)
{
        if (G_OBJECT_CLASS (xor_decoder_parent_class)->finalize)
                G_OBJECT_CLASS (xor_decoder_parent_class)->finalize (obj);
}

static void
xor_decoder_class_init (XorDecoderClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (XorDecoderPrivate));

        gobject_class->dispose = xor_decoder_dispose;
        gobject_class->finalize = xor_decoder_finalize;

        DECODER_CLASS(klass)->decode = (DecodeFunc)xor_decoder_decode_impl;
}


