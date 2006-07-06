cdef extern from "stddef.h":
    ctypedef unsigned int       size_t

cdef extern from "wbxml.h":
    ctypedef unsigned char      WB_BOOL
    ctypedef unsigned char      WB_UTINY
    ctypedef char               WB_TINY
    ctypedef unsigned int       WB_ULONG
    ctypedef int                WB_LONG

    ctypedef enum WBXMLVersion:
        WBXML_VERSION_UNKNOWN = -1
        WBXML_VERSION_10 = 0x00
        WBXML_VERSION_11 = 0x01
        WBXML_VERSION_12 = 0x02
        WBXML_VERSION_13 = 0x03

    ctypedef enum WBXMLLanguage:
        WBXML_LANG_UNKNOWN = 0
        WBXML_LANG_WML10
        WBXML_LANG_WML11
        WBXML_LANG_WML12
        WBXML_LANG_WML13
        WBXML_LANG_WTA10
        WBXML_LANG_WTAWML12
        WBXML_LANG_CHANNEL11
        WBXML_LANG_CHANNEL12
        WBXML_LANG_SI10
        WBXML_LANG_SL10
        WBXML_LANG_CO10
        WBXML_LANG_PROV10
        WBXML_LANG_EMN10
        WBXML_LANG_DRMREL10
        WBXML_LANG_OTA_SETTINGS
        WBXML_LANG_SYNCML_SYNCML10
        WBXML_LANG_SYNCML_DEVINF10
        WBXML_LANG_SYNCML_METINF10
        WBXML_LANG_SYNCML_SYNCML11
        WBXML_LANG_SYNCML_DEVINF11
        WBXML_LANG_SYNCML_METINF11
        WBXML_LANG_WV_CSP11
        WBXML_LANG_WV_CSP12
        WBXML_LANG_AIRSYNC

cdef extern from "wbxml_errors.h":
    ctypedef unsigned int       WBXMLError

cdef extern from "wbxml_encoder.h":
    ctypedef enum WBXMLEncoderXMLGenType:
        WBXML_ENCODER_XML_GEN_COMPACT = 0
        WBXML_ENCODER_XML_GEN_INDENT
        WBXML_ENCODER_XML_GEN_CANONICAL

cdef extern from "wbxml_conv.h":
    ctypedef struct WBXMLConvWBXML2XMLParams:
        WBXMLEncoderXMLGenType gen_type
        WBXMLLanguage lang
        WB_UTINY indent
        WB_BOOL keep_ignorable_ws

    ctypedef struct WBXMLConvXML2WBXMLParams:
        WBXMLVersion wbxml_version
        WB_BOOL keep_ignorable_ws
        WB_BOOL use_strtbl
        WB_BOOL produce_anonymous

