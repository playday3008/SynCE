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
#ifndef HUFFMANDECODER_H
#define HUFFMANDECODER_H

#include <decoder.h>

G_BEGIN_DECLS

typedef struct _HuffmanDecoder HuffmanDecoder;
struct _HuffmanDecoder
{
        Decoder parent;
};

typedef struct _HuffmanDecoderClass HuffmanDecoderClass;
struct _HuffmanDecoderClass {
        DecoderClass parent_class;
};

GType huffman_decoder_get_type (void);

#define HUFFMAN_DECODER_TYPE \
        (huffman_decoder_get_type())
#define HUFFMAN_DECODER(obj) \
        (G_TYPE_CHECK_INSTANCE_CAST((obj), HUFFMAN_DECODER_TYPE, HuffmanDecoder))
#define HUFFMAN_DECODER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_CAST((klass), HUFFMAN_DECODER_TYPE, HuffmanDecoderClass))
#define IS_HUFFMAN_DECODER(obj) \
        (G_TYPE_CHECK_INSTANCE_TYPE((obj), HUFFMAN_DECODER_TYPE))
#define IS_HUFFMAN_DECODER_CLASS(klass) \
        (G_TYPE_CHECK_CLASS_TYPE((klass), HUFFMAN_DECODER_TYPE))
#define HUFFMAN_DECODER_GET_CLASS(obj) \
        (G_TYPE_INSTANCE_GET_CLASS((obj), HUFFMAN_DECODER_TYPE, HuffmanDecoderClass))

G_END_DECLS

#endif /* HUFFMANDECODER_H */
