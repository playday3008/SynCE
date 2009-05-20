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
#ifndef RLEDECODER_H
#define RLEDECODER_H

#include <decoder.h>

G_BEGIN_DECLS

typedef struct _RleDecoder RleDecoder;
struct _RleDecoder
{
        Decoder parent;
};

typedef struct _RleDecoderClass RleDecoderClass;
struct _RleDecoderClass {
        DecoderClass parent_class;
};

GType rle_decoder_get_type (void);

#define RLE_DECODER_TYPE \
        (rle_decoder_get_type())
#define RLE_DECODER(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST((obj), RLE_DECODER_TYPE, RleDecoder))
#define RLE_DECODER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass), RLE_DECODER_TYPE, RleDecoderClass))
#define IS_RLE_DECODER(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj), RLE_DECODER_TYPE))
#define IS_RLE_DECODER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE((klass), RLE_DECODER_TYPE))
#define RLE_DECODER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS((obj), RLE_DECODER_TYPE, RleDecoderClass))

G_END_DECLS

#endif /* RLEDECODER_H */
