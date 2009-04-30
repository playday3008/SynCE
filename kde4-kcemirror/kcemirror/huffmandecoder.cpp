/***************************************************************************
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
#include "huffmandecoder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>
#include <kdebug.h>


typedef struct huffman_node_tag
{
    unsigned char isLeaf;
    uint32_t count;
    struct huffman_node_tag *parent;

    union _myunion
    {
        struct _mystruct
        {
            struct huffman_node_tag *zero, *one;
        } mystruct;
        unsigned char symbol;
    } myunion;
}
huffman_node;

typedef struct huffman_code_tag
{
    /* The length of this code in bits. */
    uint32_t numbits;

    /* The bits that make up this code. The first
       bit is at position 0 in bits[0]. The second
       bit is at position 1 in bits[0]. The eighth
       bit is at position 7 in bits[0]. The ninth
       bit is at position 0 in bits[1]. */
    unsigned char *bits;
}
huffman_code;

static uint32_t
numbytes_from_numbits(uint32_t numbits)
{
    return numbits / 8 + (numbits % 8 ? 1 : 0);
}

/*
 * get_bit returns the ith bit in the bits array
 * in the 0th position of the return value.
 */
static unsigned char
get_bit(unsigned char* bits, uint32_t i)
{
    return (bits[i / 8] >> i % 8) & 1;
}

static void
reverse_bits(unsigned char* bits, uint32_t numbits)
{
    uint32_t numbytes = numbytes_from_numbits(numbits);
    unsigned char *tmp =
        (unsigned char*)alloca(numbytes);
    long curbit;
    long curbyte = 0;

    memset(tmp, 0, numbytes);

    for(curbit = 0; curbit < (long) numbits; ++curbit) {
        unsigned int bitpos = curbit % 8;

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
    uint32_t numbits = 0;
    unsigned char* bits = NULL;
    huffman_code *p;

    while(leaf && leaf->parent) {
        huffman_node *parent = leaf->parent;
        unsigned char cur_bit = numbits % 8;
        uint32_t cur_byte = numbits / 8;

        /* If we need another byte to hold the code,
           then allocate it. */
        if(cur_bit == 0) {
            size_t newSize = cur_byte + 1;
            bits = (unsigned char*)realloc(bits, newSize);
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
new_leaf_node(unsigned char symbol)
{
    huffman_node *p = (huffman_node*)malloc(sizeof(huffman_node));
    p->isLeaf = 1;
    p->myunion.symbol = symbol;
    p->count = 0;
    p->parent = 0;
    return p;
}

static huffman_node*
new_nonleaf_node(uint32_t count, huffman_node *zero, huffman_node *one)
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
    unsigned char *cache;
    unsigned int cache_len;
    unsigned int cache_cur;
    unsigned char **pbufout;
    unsigned int *pbufoutlen;
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
memread(const unsigned char* buf,
        unsigned int buflen,
        unsigned int *pindex,
        void* bufout,
        unsigned int readlen)
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
read_code_table_from_memory(const unsigned char* bufin,
                            unsigned int bufinlen,
                            unsigned int *pindex,
                            unsigned int *pDataBytes)
{
    huffman_node *root = new_nonleaf_node(0, NULL, NULL);
    unsigned int count;

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
        unsigned int curbit;
        unsigned char symbol;
        unsigned char numbits;
        unsigned char numbytes;
        unsigned char *bytes;
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
        bytes = (unsigned char*)malloc(numbytes);
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
                    p->myunion.mystruct.one = (int) curbit == numbits - 1
                                              ? new_leaf_node(symbol)
                                              : new_nonleaf_node(0, NULL, NULL);
                    p->myunion.mystruct.one->parent = p;
                }
                p = p->myunion.mystruct.one;
            } else {
                if(p->myunion.mystruct.zero == NULL) {
                    p->myunion.mystruct.zero = (int) curbit == numbits - 1
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


static bool huffman_decode_memory(const unsigned char *bufin,
                          unsigned int bufinlen,
                          unsigned char **pbufout,
                          size_t *pbufoutlen)
{
    huffman_node *root, *p;
    unsigned int data_count;
    unsigned int i = 0;
    unsigned char *buf;
    unsigned int bufcur = 0;

    /* Ensure the arguments are valid. */
    if(!pbufout || !pbufoutlen)
        return false;

    /* Read the Huffman code table. */
    root = read_code_table_from_memory(bufin, bufinlen, &i, &data_count);
    if(!root)
        return false;

    buf = (unsigned char*)malloc(data_count);

    /* Decode the memory. */
    p = root;
    for(; i < bufinlen && data_count > 0; ++i) {
        unsigned char byte = bufin[i];
        unsigned char mask = 1;
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
    return true;
}


HuffmanDecoder::HuffmanDecoder(Decoder* chain): Decoder(chain)
{
}


HuffmanDecoder::HuffmanDecoder(): Decoder()
{
}


HuffmanDecoder::~HuffmanDecoder()
{
}


bool HuffmanDecoder::decode(unsigned char *rawData, size_t rawSize, unsigned char *encData, size_t encSize)
{
    size_t bufOutLen;
    unsigned char *bufOut = NULL;
    bool ret = true;

    ret = huffman_decode_memory(encData, encSize, &bufOut, &bufOutLen);

    if (ret) {
        if (bufOutLen == rawSize) {
            memcpy(rawData, bufOut, rawSize);
        } else {
            ret = false;
        }
    }

    free(bufOut);

    return ret;
}

