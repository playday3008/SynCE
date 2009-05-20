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

#include "rledecoder.h"

G_DEFINE_TYPE (RleDecoder, rle_decoder, DECODER_TYPE)

typedef struct _RleDecoderPrivate RleDecoderPrivate;
struct _RleDecoderPrivate
{
        gboolean disposed;
};

#define RLE_DECODER_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE((o), RLE_DECODER_TYPE, RleDecoderPrivate))


gboolean
rle_decoder_decode_impl(RleDecoder *self, guchar *target, gsize raw_size, guchar *source, gsize size)
{
    guchar *act1 = source;
    guchar *act2 = source + 1;
    guchar *act3 = source + 2;

    guchar val1 = *act1;
    guchar val2 = *act2;
    guchar val3 = *act3;

    guchar *tmp_target = target;

    gsize count = 0;
    gsize samcount = 0;
    do {
        if ((val1 == *act1) && (val2 == *act2) && (val3 == *act3)) {
            samcount++;
            *tmp_target++ = val1;
            *tmp_target++ = val2;
            *tmp_target++ = val3;
            act1 += 3;
            act2 += 3;
            act3 += 3;
            count += 3;
            if (samcount == 2) {
                samcount = 0;
                guchar samruncount = *act1;
                while (samruncount) {
                    *tmp_target++ = val1;
                    *tmp_target++ = val2;
                    *tmp_target++ = val3;
                    samruncount--;
                }
                act1 += 1;
                act2 += 1;
                act3 += 1;
                count += 1;
                if (count < size) {
                    val1 = *act1;
                    val2 = *act2;
                    val3 = *act3;
                }
            }
        } else {
            samcount = 0;
            *tmp_target++ = (val1 = *act1);
            *tmp_target++ = (val2 = *act2);
            *tmp_target++ = (val3 = *act3);
            act1 += 3;
            act2 += 3;
            act3 += 3;
            count += 3;
        }
    } while (count < size);

    return ((gsize) (tmp_target - target)) == raw_size;
}


static void
rle_decoder_init(RleDecoder *self)
{
        RleDecoderPrivate *priv = RLE_DECODER_GET_PRIVATE (self);
        priv->disposed = FALSE;
}

static void
rle_decoder_dispose(GObject *obj)
{
        RleDecoder *self = RLE_DECODER(obj);
        RleDecoderPrivate *priv = RLE_DECODER_GET_PRIVATE (self);

        if (priv->disposed)
                return;
        priv->disposed = TRUE;

        /* unref other objects */

        if (G_OBJECT_CLASS (rle_decoder_parent_class)->dispose)
                G_OBJECT_CLASS (rle_decoder_parent_class)->dispose (obj);
}

static void
rle_decoder_finalize (GObject *obj)
{
        if (G_OBJECT_CLASS (rle_decoder_parent_class)->finalize)
                G_OBJECT_CLASS (rle_decoder_parent_class)->finalize (obj);
}

static void
rle_decoder_class_init (RleDecoderClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (RleDecoderPrivate));

        gobject_class->dispose = rle_decoder_dispose;
        gobject_class->finalize = rle_decoder_finalize;

        DECODER_CLASS(klass)->decode = (DecodeFunc)rle_decoder_decode_impl;
}


