include "types.pxi"

cdef extern from "wbxml_conv.h":
    #WBXMLError wbxml_conv_wbxml2xml(WB_UTINY *wbxml, WB_ULONG wbxml_len, WB_UTINY **xml, WBXMLConvWBXML2XMLParams *params)
    #WBXMLError wbxml_conv_xml2wbxml(WB_UTINY *xml, WB_UTINY **wbxml, WB_ULONG *wbxml_len, WBXMLConvXML2WBXMLParams *params)
    WBXMLError wbxml_conv_wbxml2xml(char *wbxml, WB_ULONG wbxml_len, char **xml, WBXMLConvWBXML2XMLParams *params)
    WBXMLError wbxml_conv_xml2wbxml(char *xml, char **wbxml, WB_ULONG *wbxml_len, WBXMLConvXML2WBXMLParams *params)
    #WB_UTINY *wbxml_errors_string(WBXMLError error_code)
    char *wbxml_errors_string(WBXMLError error_code)

cdef extern from "stdlib.h":
    void *malloc(size_t size)
    void free(void *ptr)

cdef extern from "Python.h":
    object PyString_FromStringAndSize(char *s, int len)

class WBXMLParseError:
    def __init__(self, code):
        self.code = code
        self.description = wbxml_errors_string(code)

    def __str__(self):
        return "%s (%d)" % (self.description, self.code)

def wbxml2xml(wbxml):
    cdef char *xml
    cdef WBXMLConvWBXML2XMLParams params

    #params.gen_type = WBXML_ENCODER_XML_GEN_COMPACT
    params.gen_type = WBXML_ENCODER_XML_GEN_CANONICAL
    params.lang = WBXML_LANG_AIRSYNC
    params.indent = 0
    params.keep_ignorable_ws = 1

    retval = wbxml_conv_wbxml2xml(wbxml, len(wbxml), &xml, &params)
    if retval != 0:
        raise WBXMLParseError(retval)

    return xml

def xml2wbxml(xml):
    cdef char *bytes
    cdef WB_ULONG len
    cdef WBXMLConvXML2WBXMLParams params

    params.wbxml_version = WBXML_VERSION_13
    params.keep_ignorable_ws = 1
    params.use_strtbl = 0
    params.produce_anonymous = 1

    retval = wbxml_conv_xml2wbxml(xml, &bytes, &len, &params)
    if retval != 0:
        raise WBXMLParseError(retval)

    str = PyString_FromStringAndSize(bytes, len)
    free(bytes)

    return str
