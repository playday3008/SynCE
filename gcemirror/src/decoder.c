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

#include "decoder.h"

#include <netinet/in.h>
#include <string.h>

G_DEFINE_TYPE (Decoder, decoder, G_TYPE_OBJECT)

/* properties */
enum
{
        PROP_CHAIN = 1,
        PROP_ENC_SIZE,
        PROP_RAW_SIZE,
        PROP_ENC_DATA,
        PROP_RAW_DATA,
        LAST_PROPERTY
};

typedef struct _DecoderPrivate DecoderPrivate;

struct _DecoderPrivate
{
        gboolean disposed;

        Decoder *chain;

        gsize enc_size;
        gsize raw_size;
        guchar *enc_data;
        guchar *raw_data;
};

#define DECODER_GET_PRIVATE(o) \
        (G_TYPE_INSTANCE_GET_PRIVATE((o), DECODER_TYPE, DecoderPrivate))

gboolean decoder_decode(Decoder *self, guchar *raw_data, gsize raw_size, guchar *enc_data, gsize enc_size);
static gboolean decoder_read_data(Decoder *self, GIOChannel *chan, gsize enc_size);
static void decoder_cleanup(Decoder *self);

static void decoder_chain_cleanup(Decoder *self);
static gboolean decoder_read_size(Decoder *self, GIOChannel *chan);
static gboolean decoder_chain_read_size(Decoder *self, GIOChannel *chan);


/* methods */

gboolean
decoder_decode(Decoder *self, guchar *rawData, gsize rawSize, guchar *encData, gsize encSize)
{
        return DECODER_GET_CLASS(self)->decode(self, rawData, rawSize, encData, encSize);
}


static gboolean
decoder_decode_impl(Decoder *self, guchar *rawData, gsize rawSize, guchar *encData, gsize encSize)
{
        memcpy(rawData, encData, encSize);
        return TRUE;
}


gboolean
decoder_chain_decode(Decoder *self, guchar *rawData, gsize rawSize)
{
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);
        gboolean ret = TRUE;

        if (priv->chain != NULL) {
                priv->enc_data = g_malloc0(sizeof(guchar) * priv->enc_size);
                ret = decoder_chain_decode(priv->chain, priv->enc_data, priv->enc_size);
        }

        if (ret)
                ret = decoder_decode(self, rawData, rawSize, priv->enc_data, priv->enc_size);


        priv->raw_data = rawData;
        g_free(priv->enc_data);
        priv->enc_data = NULL;

        if (priv->chain != NULL)
                decoder_chain_cleanup(priv->chain);

        return ret;
}

static gsize
qsock_read_n(GIOChannel *sock, gchar *buffer, gsize num)
{
        guint32 readSize = 0;
        gsize n = 0;
        GError *error = NULL;
        GIOStatus status;

        do {
                status = g_io_channel_read_chars(sock, buffer + readSize, num - readSize, &n, &error);
                readSize += n;
                /*
                  if (readSize < num && n == 0)
                      sock->waitForReadyRead(3000);
                */
        } while (readSize < num && (status == G_IO_STATUS_NORMAL || status == G_IO_STATUS_AGAIN));

        return readSize;
}

static gboolean
decoder_read_size(Decoder *self, GIOChannel *sock)
{
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);
        guint32 encSizeN;
        gboolean ret = FALSE;
        guint32 readSize = 0;

        readSize = qsock_read_n(sock, (gchar *)&encSizeN, sizeof(guint32));

        if (readSize == sizeof(guint32)) {
                ret = TRUE;
                priv->enc_size = ntohl(encSizeN);
        }

        return ret;
}


static gboolean
decoder_chain_read_size(Decoder *self, GIOChannel *sock)
{
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);
        gboolean ret;

        ret = decoder_read_size(self, sock);

        if (priv->chain != NULL && ret) {
                ret = decoder_chain_read_size(priv->chain, sock);
        } else if (ret) {
                ret = decoder_read_data(self, sock, priv->enc_size);

                /*
                g_debug("%s: --- %zd", G_STRFUNC, priv->enc_size);
                */

        }
        return ret;
}


static gboolean
decoder_read_data(Decoder *self, GIOChannel *sock, gsize encSize)
{
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);
        guint32 readSize = 0;
        gboolean ret = FALSE;
        priv->enc_data = g_malloc0(sizeof(guchar) * encSize);

        readSize = qsock_read_n(sock, (gchar*)priv->enc_data, encSize);

        if (readSize == encSize)
                ret = TRUE;

        return ret;
}


gsize
decoder_chain_read(Decoder *self, GIOChannel *sock)
{
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);
        gboolean ret = FALSE;
        guint32 rawSizeN = 0;
        guint32 readSize = 0;

        priv->raw_size = 0;

        readSize = qsock_read_n(sock, (gchar*)&rawSizeN, sizeof(guint32));

        if (readSize == sizeof(guint32)) {
                priv->raw_size = ntohl(rawSizeN);
                if (!(ret = decoder_chain_read_size(self, sock))) {
                        g_debug("%s: Read sizes error", G_STRFUNC);
                        priv->raw_size = 0;
                }
        } else {
                g_debug("%s: Read raw-size error", G_STRFUNC);
        }

        return priv->raw_size;
}


static void
decoder_chain_cleanup(Decoder *self)
{
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);
        if (priv->chain != NULL) {
                decoder_chain_cleanup(priv->chain);
        }

        decoder_cleanup(self);
        return;
}


static void
decoder_cleanup(Decoder *self)
{
        return;
}


/* class & instance functions */

static void
decoder_init(Decoder *self)
{
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);
        priv->disposed = FALSE;

        priv->chain = NULL;
        priv->enc_data = NULL;
        priv->raw_data = NULL;

        priv->enc_size = 0;
        priv->raw_size = 0;
}

static void
decoder_get_property(GObject    *obj,
                     guint       property_id,
                     GValue     *value,
                     GParamSpec *pspec)
{
        Decoder *self = DECODER(obj);
        DecoderPrivate *priv = DECODER_GET_PRIVATE(self);

        switch (property_id) {

        case PROP_CHAIN:
                g_value_set_pointer(value, priv->chain);
                break;

        case PROP_ENC_SIZE:
                g_value_set_ulong(value, priv->enc_size);
                break;

        case PROP_RAW_SIZE:
                g_value_set_ulong(value, priv->raw_size);
                break;

        case PROP_ENC_DATA:
                g_value_set_pointer(value, priv->enc_data);
                break;

        case PROP_RAW_DATA:
                g_value_set_pointer(value, priv->raw_data);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID(obj, property_id, pspec);
                break;
        }
}


static void
decoder_set_property(GObject      *obj,
                     guint         property_id,
                     const GValue *value,
                     GParamSpec   *pspec)
{
        Decoder *self = DECODER(obj);
        DecoderPrivate *priv = DECODER_GET_PRIVATE(self);

        switch (property_id) {
        case PROP_CHAIN:
                if (priv->chain != NULL)
                        g_object_unref(priv->chain);
                priv->chain = g_value_get_pointer(value);
                break;

        case PROP_ENC_SIZE:
                priv->enc_size = g_value_get_ulong (value);
                break;

        case PROP_RAW_SIZE:
                priv->raw_size = g_value_get_ulong (value);
                break;

        case PROP_ENC_DATA:
                if (priv->enc_data != NULL)
                        g_free(priv->enc_data);
                priv->enc_data = g_value_get_pointer(value);
                break;

        case PROP_RAW_DATA:
                if (priv->raw_data != NULL)
                        g_free(priv->raw_data);
                priv->raw_data = g_value_get_pointer(value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (obj, property_id, pspec);
                break;
  }
}


static void
decoder_dispose(GObject *obj)
{
        Decoder *self = DECODER(obj);
        DecoderPrivate *priv = DECODER_GET_PRIVATE (self);

        if (priv->disposed)
                return;
        priv->disposed = TRUE;

        /* unref other objects */

        if (priv->chain)
                g_object_unref(priv->chain);
        g_free(priv->enc_data);

        /* dont free priv->raw_data, it is never allocated by us */

        if (G_OBJECT_CLASS (decoder_parent_class)->dispose)
                G_OBJECT_CLASS (decoder_parent_class)->dispose (obj);
}

static void
decoder_finalize(GObject *obj)
{
        if (G_OBJECT_CLASS(decoder_parent_class)->finalize)
                G_OBJECT_CLASS(decoder_parent_class)->finalize (obj);
}

static void
decoder_class_init(DecoderClass *klass)
{
        GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
        GParamSpec *param_spec;

        g_type_class_add_private(klass, sizeof(DecoderPrivate));
  
        gobject_class->get_property = decoder_get_property;
        gobject_class->set_property = decoder_set_property;

        gobject_class->dispose = decoder_dispose;
        gobject_class->finalize = decoder_finalize;

        klass->decode = &decoder_decode_impl;

        param_spec = g_param_spec_pointer ("chain", "decoder chain",
                                           "The chain of decoders",
                                           G_PARAM_READWRITE |
                                           G_PARAM_STATIC_NICK |
                                           G_PARAM_STATIC_BLURB);
        g_object_class_install_property (gobject_class, PROP_CHAIN, param_spec);

        param_spec = g_param_spec_ulong ("encsize", "encoded data size",
                                         "The size of the encoded data.",
                                         0, G_MAXSIZE, 0,
                                         G_PARAM_READWRITE |
                                         G_PARAM_STATIC_NICK |
                                         G_PARAM_STATIC_BLURB);
        g_object_class_install_property (gobject_class, PROP_ENC_SIZE, param_spec);

        param_spec = g_param_spec_ulong ("rawsize", "raw data size",
                                         "The size of the raw data.",
                                         0, G_MAXSIZE, 0,
                                         G_PARAM_READWRITE |
                                         G_PARAM_STATIC_NICK |
                                         G_PARAM_STATIC_BLURB);
        g_object_class_install_property (gobject_class, PROP_RAW_SIZE, param_spec);

        param_spec = g_param_spec_pointer ("encdata", "encoded data",
                                           "The encoded data.",
                                           G_PARAM_READWRITE |
                                           G_PARAM_STATIC_NICK |
                                           G_PARAM_STATIC_BLURB);
        g_object_class_install_property (gobject_class, PROP_ENC_DATA, param_spec);

        param_spec = g_param_spec_pointer ("rawdata", "raw data",
                                           "The raw data.",
                                           G_PARAM_READWRITE |
                                           G_PARAM_STATIC_NICK |
                                           G_PARAM_STATIC_BLURB);
        g_object_class_install_property (gobject_class, PROP_RAW_DATA, param_spec);
}


