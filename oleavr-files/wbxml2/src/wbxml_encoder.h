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
 * @file wbxml_encoder.h
 * @ingroup wbxml_encoder
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 11/11/02
 *
 * @brief WBXML Encoder - Encodes a WBXML Tree to WBXML or to XML
 */

#ifndef WBXML_ENCODER_H
#define WBXML_ENCODER_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup wbxml_encoder
 *  @{ 
 */

/**
 * @brief WBXML Encoder
 */
typedef struct WBXMLEncoder_s WBXMLEncoder;

/**
 * @brief Type of XML Generation
 * @note Canonical Form is defined here: http://www.jclark.com/xml/canonxml.html
 */
typedef enum WBXMLEncoderXMLGenType_e {
    WBXML_ENCODER_XML_GEN_COMPACT = 0,  /**< Compact XML generation */
    WBXML_ENCODER_XML_GEN_INDENT,       /**< Indented XML generation */
    WBXML_ENCODER_XML_GEN_CANONICAL     /**< Canonical XML generation */
} WBXMLEncoderXMLGenType;


/* WBXMLEncoder Functions */

/**
 * @brief Create a WBXML Encoder
 * @result Return the newly created WBXMLEncoder, or NULL if not enough memory
 * @warning Do NOT use this function directly, use wbxml_encoder_create() macro instead
 */
WBXML_DECLARE(WBXMLEncoder *) wbxml_encoder_create_real(void);
#define wbxml_encoder_create() wbxml_mem_cleam(wbxml_encoder_create_real())

/**
 * @brief Destroy a WBXML Encoder
 * @param encoder The WBXMLEncoder to free
 */
WBXML_DECLARE(void) wbxml_encoder_destroy(WBXMLEncoder *encoder);


/* Possible options when generating WBXML or XML */

/**
 * @brief Set the WBXML Encoder to ignore empty texts (ie: ignorable Whitespaces) [Default: FALSE]
 * @param encoder [in] The WBXML Encoder to use
 * @param set_ignore [in] TRUE if ignore, FALSE otherwise
 * @warning This behaviour can me overriden by the WBXML_ENCODER_XML_GEN_CANONICAL mode (set by wbxml_encoder_set_xml_gen_type())
 */
WBXML_DECLARE(void) wbxml_encoder_set_ignore_empty_text(WBXMLEncoder *encoder, WB_BOOL set_ignore);

/**
 * @brief Set the WBXML Encoder to remove leading and trailing blanks in texts [Default: FALSE]
 * @param encoder [in] The WBXML Encoder to use
 * @param set_remove [in] TRUE if remove, FALSE otherwise
 * @warning This behaviour can me overriden by the WBXML_ENCODER_XML_GEN_CANONICAL mode (set by wbxml_encoder_set_xml_gen_type())
 */
WBXML_DECLARE(void) wbxml_encoder_set_remove_text_blanks(WBXMLEncoder *encoder, WB_BOOL set_remove);


/* Possible options when generating WBXML */

/**
 * @brief Set if we use String Table when Encoding into WBXML [Default: TRUE]
 * @param encoder [in] The WBXML Encoder
 * @param use_strtbl [in] TRUE if we use String Table, FALSE otherwise
 * @note This function has no effect if WBXML_ENCODER_USE_STRTBL compilation flag is not set
 */
WBXML_DECLARE(void) wbxml_encoder_set_use_strtbl(WBXMLEncoder *encoder, WB_BOOL use_strtbl);

/**
 * @brief Set the WBXML Version of the output document, when generating WBXML [Default: 'WBXML_VERSION_TOKEN_13' (1.3)]
 * @param encoder [in] The WBXML Encoder
 * @param version [in] The WBXML Version
 */
WBXML_DECLARE(void) wbxml_encoder_set_wbxml_version(WBXMLEncoder *encoder, WBXMLVersion version);


/* Possible options when generating XML */

/**
 * @brief Set the WBXML Encoder XML Generation Type, when generating XML [Default: WBXML_ENCODER_XML_GEN_COMPACT]
 * @param encoder [in] The WBXML Encoder
 * @param gen_type [in] Generation Type (cf. WBXMLEncoderXMLGen enum)
 */
WBXML_DECLARE(void) wbxml_encoder_set_xml_gen_type(WBXMLEncoder *encoder, WBXMLEncoderXMLGenType gen_type);

/**
 * @brief Set the WBXML Encoder indent, when generating XML in WBXML_ENCODER_XML_GEN_INDENT mode [Default: 0]
 * @param encoder [in] The WBXML Encoder
 * @param indent [in] If 'WBXML_ENCODER_XML_GEN_INDENT' type is used, this is the number of spaces for indent
 */
WBXML_DECLARE(void) wbxml_encoder_set_indent(WBXMLEncoder *encoder, WB_UTINY indent);


/* Encoding Functions */

/**
 * @brief Set the WBXML Tree to encode
 * @param encoder [in] The WBXML Encoder to use
 * @param tree [in] The WBXML Tree to encode
 * @note You MUST call this function before calling following wbxml_encoder_encode() or wbxml_encoder_encode_to_xml() function
 */
WBXML_DECLARE(void) wbxml_encoder_set_tree(WBXMLEncoder *encoder, WBXMLTree *tree);

/**
 * @brief Encode a WBXML Tree to WBXML
 * @param encoder [in] The WBXML Encoder to use
 * @param wbxml [out] Resulting WBXML document
 * @param wbxml_len [out] The resulting WBXML document length
 * @return Return WBXML_OK if no error, an error code otherwise
 * @warning The 'encoder->tree' WBXMLLib Tree MUST be already set with a call to wbxml_encoder_set_tree() function
 */
WBXML_DECLARE(WBXMLError) wbxml_encoder_encode_to_wbxml(WBXMLEncoder *encoder, WB_UTINY **wbxml, WB_ULONG *wbxml_len);

/**
 * @brief Encode a WBXML Tree to XML
 * @param encoder [in] The WBXML Encoder to use
 * @param xml [out] Resulting XML document
 * @return Return WBXML_OK if no error, an error code otherwise
 * @warning The 'encoder->tree' WBXMLLib Tree MUST be already set with a call to wbxml_encoder_set_tree() function
 */
WBXML_DECLARE(WBXMLError) wbxml_encoder_encode_to_xml(WBXMLEncoder *encoder, WB_UTINY **xml);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WBXML_ENCODER_H */
