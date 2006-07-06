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
 * @file wbxml_tree_clb_xml.h
 * @ingroup wbxml_tree
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 03/03/11
 *
 * @brief WBXML Tree Callbacks for XML Parser (Expat)
 */

#ifndef WBXML_TREE_CLB_XML_H
#define WBXML_TREE_CLB_XML_H

#if defined( HAVE_EXPAT )

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup wbxml_tree 
 *  @{ 
 */

/**
 * @brief Doctype Declaration Callback
 * @param ctx User data
 * @param doctypeName Doctype Name
 * @param sysid System ID
 * @param pubid Public ID
 * @param has_internal_subset Non-zero if the DOCTYPE declaration has an internal subset
 */
void wbxml_tree_clb_xml_doctype_decl(void *ctx, const XML_Char *doctypeName, 
                                     const XML_Char *sysid, const XML_Char *pubid, 
                                     int has_internal_subset);

/**
 * @brief Start Element Callback
 * @param ctx User data
 * @param localName The local tag name
 * @param attrs The attributes attached to the element
 */
void wbxml_tree_clb_xml_start_element(void *ctx, const XML_Char *localName, const XML_Char **attrs);

/**
 * @brief End Element Callback
 * @param ctx User data
 * @param localName The local tag name
 */
void wbxml_tree_clb_xml_end_element(void *ctx, const XML_Char *localName);

/**
 * @brief Start of CDATA Section Callback
 * @param ctx User data
 */
void wbxml_tree_clb_xml_start_cdata(void *ctx);

/**
 * @brief End of CDATA Section Callback
 * @param ctx User data
 */
void wbxml_tree_clb_xml_end_cdata(void *ctx);

/**
 * @brief Characters Callback
 * @param ctx User data
 * @param ch The characters array
 * @param len The number of characters to read from the array
 */
void wbxml_tree_clb_xml_characters(void *ctx, const XML_Char *ch, int len);

/**
 * @brief Processing Instruction Callback
 * @param ctx User data
 * @param target The processing instruction target.
 * @param data The processing instruction data, or null if  none was supplied. The data does
 *             not include any whitespace separating it from the target
 */
void wbxml_tree_clb_xml_pi(void *ctx, const XML_Char *target, const XML_Char *data);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* HAVE_EXPAT */

#endif /* WBXML_TREE_CLB_XML_H */
