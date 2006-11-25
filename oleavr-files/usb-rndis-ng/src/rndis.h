#ifndef RNDIS_H
#define RNDIS_H

#include <glib.h>
#include <usb.h>

#define RNDIS_TIMEOUT_MS                      5000

#define INTERRUPT_MAX_PACKET_SIZE              128
#define RESPONSE_BUFFER_SIZE                  1025

#define USB_DIR_OUT                              0 /* to device */
#define USB_DIR_IN                            0x80 /* to host */

#define RNDIS_MSG_COMPLETION            0x80000000

#define RNDIS_MSG_PACKET                         1
#define RNDIS_MSG_INIT                           2
#define RNDIS_MSG_QUERY                          4
#define RNDIS_MSG_SET                            5
#define RNDIS_MSG_KEEPALIVE                      8

#define RNDIS_STATUS_SUCCESS                     0

#define OID_802_3_PERMANENT_ADDRESS     0x01010101
#define OID_802_3_CURRENT_ADDRESS       0x01010102
#define OID_GEN_CURRENT_PACKET_FILTER   0x0001010E

#define NDIS_PACKET_TYPE_DIRECTED       0x00000001
#define NDIS_PACKET_TYPE_MULTICAST      0x00000002
#define NDIS_PACKET_TYPE_ALL_MULTICAST  0x00000004
#define NDIS_PACKET_TYPE_BROADCAST      0x00000008
#define NDIS_PACKET_TYPE_SOURCE_ROUTING 0x00000010
#define NDIS_PACKET_TYPE_PROMISCUOUS    0x00000020
#define NDIS_PACKET_TYPE_SMT            0x00000040
#define NDIS_PACKET_TYPE_ALL_LOCAL      0x00000080
#define NDIS_PACKET_TYPE_GROUP          0x00000100
#define NDIS_PACKET_TYPE_ALL_FUNCTIONAL 0x00000200
#define NDIS_PACKET_TYPE_FUNCTIONAL     0x00000400
#define NDIS_PACKET_TYPE_MAC_FRAME      0x00000800

struct rndis_message {
    guint32 msg_type;
    guint32 msg_len;
} __attribute__ ((packed));

struct rndis_request {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
} __attribute__ ((packed));

struct rndis_response {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
} __attribute__ ((packed));

struct rndis_init {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 major_version;
    guint32 minor_version;
    guint32 max_transfer_size;
} __attribute__ ((packed));

struct rndis_init_c {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
    guint32 major_version;
    guint32 minor_version;
    guint32 device_flags;
    guint32 medium;
    guint32 max_packets_per_message;
    guint32 max_transfer_size;
    guint32 packet_alignment;
    guint32 af_list_offset;
    guint32 af_list_size;
} __attribute__ ((packed));

struct rndis_query {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 oid;
    guint32 len;
    guint32 offset;
    guint32 handle;
} __attribute__ ((packed));

struct rndis_query_c {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
    guint32 len;
    guint32 offset;
} __attribute__ ((packed));

struct rndis_set {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 oid;
    guint32 len;
    guint32 offset;
    guint32 handle;
} __attribute__ ((packed));

struct rndis_set_c {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
    guint32 status;
} __attribute__ ((packed));

struct rndis_keepalive {
    guint32 msg_type;
    guint32 msg_len;
    guint32 request_id;
} __attribute__ ((packed));

struct rndis_data {
    guint32 msg_type;
    guint32 msg_len;
    guint32 data_offset;
    guint32 data_len;

    guint32 oob_data_offset;
    guint32 oob_data_len;
    guint32 num_oob;

    guint32 packet_data_offset;
    guint32 packet_data_len;

    guint32 vc_handle;
    guint32 reserved;
} __attribute__ ((packed));

typedef struct {
    usb_dev_handle *h;
    gint fd;
    guint host_max_transfer_size;
    guint device_max_transfer_size;
    guint alignment;

    struct usb_endpoint_descriptor *ep_int_in;
    struct usb_endpoint_descriptor *ep_bulk_in;
    struct usb_endpoint_descriptor *ep_bulk_out;
} RNDISContext;

gboolean _rndis_command (RNDISContext *ctx, struct rndis_request *req, struct rndis_response **resp);
gboolean _rndis_init (RNDISContext *ctx, struct rndis_init_c **response);
gboolean _rndis_query (RNDISContext *ctx, guint32 oid, guchar *result, guint *res_len);
gboolean _rndis_set (RNDISContext *ctx, guint32 oid, guchar *value, guint value_len);
gboolean _rndis_keepalive (RNDISContext *ctx);

#endif /* RNDIS_H */
