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

#include "huffmandecoder.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <netinet/in.h>

G_DEFINE_TYPE (HuffmanDecoder, huffman_decoder, DECODER_TYPE)

typedef struct _HuffmanDecoderPrivate HuffmanDecoderPrivate;
struct _HuffmanDecoderPrivate
{
        gboolean disposed;
};

#define HUFFMAN_DECODER_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE((o), HUFFMAN_DECODER_TYPE, HuffmanDecoderPrivate))


typedef struct huffman_node_tag
{
    guchar isLeaf;
    guint32 count;
    struct huffman_node_tag *parent;

    union _myunion
    {
        struct _mystruct
        {
            struct huffman_node_tag *zero, *one;
        } mystruct;
        guchar symbol;
    } myunion;
}
huffman_node;

typedef struct huffman_code_tag
{
    /* The length of this code in bits. */
    guint32 numbits;

    /* The bits that make up this code. The first
       bit is at position 0 in bits[0]. The second
       bit is at position 1 in bits[0]. The eighth
       bit is at position 7 in bits[0]. The ninth
       bit is at position 0 in bits[1]. */
    guchar *bits;
}
huffman_code;

static guint32
numbytes_from_numbits(guint32 numbits)
{
    return numbits / 8 + (numbits % 8 ? 1 : 0);
}

/*
 * get_bit returns the ith bit in the bits array
 * in the 0th position of the return value.
 */
static guchar
get_bit(guchar *bits, guint32 i)
{
    return (bits[i / 8] >> i % 8) & 1;
}

static void
reverse_bits(guchar* bits, guint32 numbits)
{
    guint32 numbytes = numbytes_from_numbits(numbits);
    guchar *tmp =
        (guchar*)alloca(numbytes);
    glong curbit;
    glong curbyte = 0;

    memset(tmp, 0, numbytes);

    for(curbit = 0; curbit < (glong) numbits; ++curbit) {
        guint bitpos = curbit % 8;

        if(curbit > 0 && curbit % 8 == 0)
            ++curbyte;

        tmp[curbyte] |= (get_bit(bits, numbits - curbit - 1) << bitpos);
    }

    memcpy(bits, tmp, numbytes);
}

/*
 * new_code builds a huffman_code from a leaf in
 * a Huffman tree.
 */
static huffman_code*
new_code(const huffman_node* leaf)
{
    /* Build the huffman code by walking up to
     * the root node and then reversing the bits,
     * since the Huffman code is calculated by
     * walking down the tree. */
    guint32 numbits = 0;
    guchar* bits = NULL;
    huffman_code *p;

    while(leaf && leaf->parent) {
        huffman_node *parent = leaf->parent;
        guchar cur_bit = numbits % 8;
        guint32 cur_byte = numbits / 8;

        /* If we need another byte to hold the code,
           then allocate it. */
        if(cur_bit == 0) {
            gsize newSize = cur_byte + 1;
            bits = (guchar*)realloc(bits, newSize);
            bits[newSize - 1] = 0; /* Initialize the new byte. */
        }

        /* If a one must be added then or it in. If a zero
         * must be added then do nothing, since the byte
         * was initialized to zero. */
        if(leaf == parent->myunion.mystruct.one)
            bits[cur_byte] |= 1 << cur_bit;

        ++numbits;
        leaf = parent;
    }

    if(bits)
        reverse_bits(bits, numbits);

    p = (huffman_code*)malloc(sizeof(huffman_code));
    p->numbits = numbits;
    p->bits = bits;
    return p;
}

#define MAX_SYMBOLS 256
typedef huffman_node* SymbolFrequencies[MAX_SYMBOLS];
typedef huffman_code* SymbolEncoder[MAX_SYMBOLS];

static huffman_node*
new_leaf_node(guchar symbol)
{
    huffman_node *p = (huffman_node*)malloc(sizeof(huffman_node));
    p->isLeaf = 1;
    p->myunion.symbol = symbol;
    p->count = 0;
    p->parent = 0;
    return p;
}

static huffman_node*
new_nonleaf_node(guint32 count, huffman_node *zero, huffman_node *one)
{
    huffman_node *p = (huffman_node*)malloc(sizeof(huffman_node));
    p->isLeaf = 0;
    p->count = count;
    p->myunion.mystruct.zero = zero;
    p->myunion.mystruct.one = one;
    p->parent = 0;

    return p;
}

static void
free_huffman_tree(huffman_node *subtree)
{
    if(subtree == NULL)
        return;

    if(!subtree->isLeaf) {
        free_huffman_tree(subtree->myunion.mystruct.zero);
        free_huffman_tree(subtree->myunion.mystruct.one);
    }

    free(subtree);
}


typedef struct buf_cache_tag
{
    guchar *cache;
    guint cache_len;
    guint cache_cur;
    guchar **pbufout;
    guint *pbufoutlen;
}
buf_cache;


/*
 * build_symbol_encoder builds a SymbolEncoder by walking
 * down to the leaves of the Huffman tree and then,
 * for each leaf, determines its code.
 */
static void
build_symbol_encoder(huffman_node *subtree, SymbolEncoder *pSF)
{
    if(subtree == NULL)
        return;

    if(subtree->isLeaf)
        (*pSF)[subtree->myunion.symbol] = new_code(subtree);
    else {
        build_symbol_encoder(subtree->myunion.mystruct.zero, pSF);
        build_symbol_encoder(subtree->myunion.mystruct.one, pSF);
    }
}


static int
memread(const guchar* buf,
        guint buflen,
        guint *pindex,
        void* bufout,
        guint readlen)
{
    assert(buf && pindex && bufout);
    assert(buflen >= *pindex);
    if(buflen < *pindex)
        return 1;
    if(readlen + *pindex >= buflen)
        return 1;
    memcpy(bufout, buf + *pindex, readlen);
    *pindex += readlen;
    return 0;
}

static huffman_node*
read_code_table_from_memory(const guchar* bufin,
                            guint bufinlen,
                            guint *pindex,
                            guint *pDataBytes)
{
    huffman_node *root = new_nonleaf_node(0, NULL, NULL);
    guint count;

    /* Read the number of entries.
       (it is stored in network byte order). */
    if(memread(bufin, bufinlen, pindex, &count, sizeof(count))) {
        free_huffman_tree(root);
        return NULL;
    }

    count = ntohl(count);

    /* Read the number of data bytes this encoding represents. */
    if(memread(bufin, bufinlen, pindex, pDataBytes, sizeof(*pDataBytes))) {
        free_huffman_tree(root);
        return NULL;
    }

    *pDataBytes = ntohl(*pDataBytes);

    /* Read the entries. */
    while(count-- > 0) {
        guint curbit;
        guchar symbol;
        guchar numbits;
        guchar numbytes;
        guchar *bytes;
        huffman_node *p = root;

        if(memread(bufin, bufinlen, pindex, &symbol, sizeof(symbol))) {
            free_huffman_tree(root);
            return NULL;
        }

        if(memread(bufin, bufinlen, pindex, &numbits, sizeof(numbits))) {
            free_huffman_tree(root);
            return NULL;
        }

        numbytes = numbytes_from_numbits(numbits);
        bytes = (guchar*)malloc(numbytes);
        if(memread(bufin, bufinlen, pindex, bytes, numbytes)) {
            free(bytes);
            free_huffman_tree(root);
            return NULL;
        }

        /*
         * Add the entry to the Huffman tree. The value
         * of the current bit is used switch between
         * zero and one child nodes in the tree. New nodes
         * are added as needed in the tree.
         */
        for(curbit = 0; curbit < numbits; ++curbit) {
            if(get_bit(bytes, curbit)) {
                if(p->myunion.mystruct.one == NULL) {
                    p->myunion.mystruct.one = (gint) curbit == numbits - 1
                                              ? new_leaf_node(symbol)
                                              : new_nonleaf_node(0, NULL, NULL);
                    p->myunion.mystruct.one->parent = p;
                }
                p = p->myunion.mystruct.one;
            } else {
                if(p->myunion.mystruct.zero == NULL) {
                    p->myunion.mystruct.zero = (gint) curbit == numbits - 1
                                               ? new_leaf_node(symbol)
                                               : new_nonleaf_node(0, NULL, NULL);
                    p->myunion.mystruct.zero->parent = p;
                }
                p = p->myunion.mystruct.zero;
            }
        }

        free(bytes);
    }

    return root;
}


#define CACHE_SIZE 1024


static gboolean
huffman_decode_memory(const guchar *bufin,
                      guint bufinlen,
                      guchar **pbufout,
                      gsize *pbufoutlen)
{
    huffman_node *root, *p;
    guint data_count;
    guint i = 0;
    guchar *buf;
    guint bufcur = 0;

    /* Ensure the arguments are valid. */
    if(!pbufout || !pbufoutlen)
        return FALSE;

    /* Read the Huffman code table. */
    root = read_code_table_from_memory(bufin, bufinlen, &i, &data_count);
    if(!root)
        return FALSE;

    buf = (guchar*)malloc(data_count);

    /* Decode the memory. */
    p = root;
    for(; i < bufinlen && data_count > 0; ++i) {
        guchar byte = bufin[i];
        guchar mask = 1;
        while(data_count > 0 && mask) {
            p = byte & mask ? p->myunion.mystruct.one : p->myunion.mystruct.zero;
            mask <<= 1;

            if(p->isLeaf) {
                buf[bufcur++] = p->myunion.symbol;
                p = root;
                --data_count;
            }
        }
    }

    free_huffman_tree(root);
    *pbufout = buf;
    *pbufoutlen = bufcur;
    return TRUE;
}


gboolean
huffman_decoder_decode_impl(HuffmanDecoder *self, guchar *raw_data, gsize raw_size, guchar *enc_data, gsize enc_size)
{
    gsize buf_out_len;
    guchar *buf_out = NULL;
    gboolean ret = TRUE;

    ret = huffman_decode_memory(enc_data, enc_size, &buf_out, &buf_out_len);

    if (ret) {
        if (buf_out_len == raw_size) {
            memcpy(raw_data, buf_out, raw_size);
        } else {
            ret = FALSE;
        }
    }

    g_free(buf_out);

    return ret;
}

/* class functions */

static void
huffman_decoder_init(HuffmanDecoder *self)
{
        HuffmanDecoderPrivate *priv = HUFFMAN_DECODER_GET_PRIVATE (self);
        priv->disposed = FALSE;
}

static void
huffman_decoder_dispose(GObject *obj)
{
        HuffmanDecoder *self = HUFFMAN_DECODER(obj);
        HuffmanDecoderPrivate *priv = HUFFMAN_DECODER_GET_PRIVATE (self);

        if (priv->disposed)
                return;
        priv->disposed = TRUE;

        /* unref other objects */

        if (G_OBJECT_CLASS (huffman_decoder_parent_class)->dispose)
                G_OBJECT_CLASS (huffman_decoder_parent_class)->dispose (obj);
}

static void
huffman_decoder_finalize (GObject *obj)
{
        if (G_OBJECT_CLASS (huffman_decoder_parent_class)->finalize)
                G_OBJECT_CLASS (huffman_decoder_parent_class)->finalize (obj);
}

static void
huffman_decoder_class_init (HuffmanDecoderClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

        g_type_class_add_private (klass, sizeof (HuffmanDecoderPrivate));

        gobject_class->dispose = huffman_decoder_dispose;
        gobject_class->finalize = huffman_decoder_finalize;

        DECODER_CLASS(klass)->decode = (DecodeFunc)huffman_decoder_decode_impl;
}

