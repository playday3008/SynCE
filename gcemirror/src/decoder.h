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
#ifndef DECODER_H
#define DECODER_H

#include <sys/types.h>
#include <glib-object.h>
#include <glib.h>

G_BEGIN_DECLS
typedef struct _Decoder Decoder;
struct _Decoder
{
        GObject parent;
};

typedef struct _DecoderClass DecoderClass;
struct _DecoderClass {
        GObjectClass parent_class;

        gboolean (*decode) (Decoder *self, guchar *raw_data, gsize raw_size, guchar *enc_data, gsize enc_size);

};

GType decoder_get_type (void);

#define DECODER_TYPE \
        (decoder_get_type())
#define DECODER(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST((obj), DECODER_TYPE, Decoder))
#define DECODER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass), DECODER_TYPE, DecoderClass))
#define IS_DECODER(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj), DECODER_TYPE))
#define IS_DECODER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE((klass), DECODER_TYPE))
#define DECODER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS((obj), DECODER_TYPE, DecoderClass))


typedef gboolean (*DecodeFunc) (Decoder *self, guchar *raw_data, gsize raw_size, guchar *enc_data, gsize enc_size);

gboolean decoder_chain_decode(Decoder *self, guchar *raw_data, gsize chain_enc_size);
gsize decoder_chain_read(Decoder *self, GIOChannel *chan);

G_END_DECLS

#endif /* DECODER_H */
