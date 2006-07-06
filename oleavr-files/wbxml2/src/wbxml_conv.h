/*
 * libwbxml, the WBXML Library.
 * Copyright (C) 2002-2004 Aymerick Jéhanne <aymerick@jehanne.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * GPL v2: http://www.gnu.org/licenses/gpl.txt
 * 
 * Contact: libwbxml@aymerick.com
 * Home: http://libwbxml.aymerick.com
 */
 
/**
 * @file wbxml_conv.h
 * @ingroup wbxml_conv
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 03/02/23
 *
 * @brief WBXML Convertion Library (XML to WBXML, and WBXML to XML)
 */

#ifndef WBXML_CONV_H
#define WBXML_CONV_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup wbxml_conv 
 *  @{ 
 */

/** Parameters for xml2wbxml() function  */
typedef struct WBXMLConvWBXML2XMLParams_s {
    WBXMLEncoderXMLGenType gen_type;    /**< WBXML_ENCODER_XML_GEN_COMPACT | WBXML_ENCODER_XML_GEN_INDENT | WBXML_ENCODER_XML_GEN_CANONICAL (Default: WBXML_ENCODER_XML_GEN_INDENT) */
    WBXMLLanguage lang;                 /**< Force document Language (overwrite document Public ID) */
    WB_UTINY indent;                    /**< Indentation Delta, when using WBXML_ENCODER_XML_GEN_INDENT Generation Type (Default: 0) */
    WB_BOOL keep_ignorable_ws;          /**< Keep Ignorable Whitespaces (Default: FALSE) */
} WBXMLConvWBXML2XMLParams;

/** Parameters for wbxml2xml() function  */
typedef struct WBXMLConvXML2WBXMLParams_s {
    WBXMLVersion wbxml_version; /**< WBXML Version */
    WB_BOOL keep_ignorable_ws;  /**< Keep Ignorable Whitespaces (Default: FALSE) */
    WB_BOOL use_strtbl;         /**< Generate String Table (Default: TRUE) */
    WB_BOOL produce_anonymous;  /**< Produce an anonymous document (Default: FALSE) */
} WBXMLConvXML2WBXMLParams;


/**
 * @brief Convert WBXML to XML
 * @param wbxml     [in] WBXML Document to convert
 * @param wbxml_len [in] Length of WBXML Document
 * @param xml       [out] Resulting XML Document
 * @param params    [in] Parameters (if NULL, default values are used)
 * @return WBXML_OK if convertion succeeded, an Error Code otherwise
 */
WBXML_DECLARE(WBXMLError) wbxml_conv_wbxml2xml(WB_UTINY *wbxml, WB_ULONG wbxml_len, WB_UTINY **xml, WBXMLConvWBXML2XMLParams *params);

/**
 * @brief Convert XML to WBXML
 * @param xml       [in] XML Document to convert
 * @param wbxml     [out] Resulting WBXML Document
 * @param wbxml_len [out] Length of resulting WBXML Document
 * @param params    [in] Parameters (if NULL, default values are used)
 * @return WBXML_OK if convertion succeeded, an Error Code otherwise
 */
WBXML_DECLARE(WBXMLError) wbxml_conv_xml2wbxml(WB_UTINY *xml, WB_UTINY **wbxml, WB_ULONG *wbxml_len, WBXMLConvXML2WBXMLParams *params);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WBXML_CONV_H */
