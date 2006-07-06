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
 * @file wbxml_conv.c
 * @ingroup wbxml_conv
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 03/02/23
 *
 * @brief WBXML Convertion Library (XML to WBXML, and WBXML to XML)
 */

#include "wbxml.h"


/************************************
 * Public Functions
 */

WBXML_DECLARE(WBXMLError) wbxml_conv_wbxml2xml(WB_UTINY *wbxml, WB_ULONG wbxml_len, WB_UTINY **xml, WBXMLConvWBXML2XMLParams *params)
{
    WBXMLEncoder *wbxml_encoder = NULL;
    WBXMLTree *wbxml_tree = NULL;
    WBXMLError ret = WBXML_OK;

    /* Check Parameters */
    if ((wbxml == NULL) || (wbxml_len == 0) || (xml == NULL))
        return WBXML_ERROR_BAD_PARAMETER;

    *xml = NULL;

    /* Parse WBXML to WBXML Tree */
    ret = wbxml_tree_from_wbxml(wbxml, wbxml_len, params ? params->lang : WBXML_LANG_UNKNOWN, &wbxml_tree);
    if (ret != WBXML_OK) {
        WBXML_ERROR((WBXML_CONV, "wbxml2xml convertion failed - WBXML Parser Error: %s",
                                 wbxml_errors_string(ret)));

        return ret;
    }
    else {
        /* Create WBXML Encoder */
        if ((wbxml_encoder = wbxml_encoder_create()) == NULL) {
            wbxml_tree_destroy(wbxml_tree);
            return WBXML_ERROR_NOT_ENOUGH_MEMORY;
        }

        /* Set the WBXML Tree to encode */
        wbxml_encoder_set_tree(wbxml_encoder, wbxml_tree);

        /* Set encoder parameters */
        if (params == NULL) {
            /* Default Values */

            /* Set XML Generation Type */
            wbxml_encoder_set_xml_gen_type(wbxml_encoder, WBXML_ENCODER_XML_GEN_INDENT);

            /* Set Indent */
            wbxml_encoder_set_indent(wbxml_encoder, 0);

            /* Skip Ignorable Whitespaces */
            wbxml_encoder_set_ignore_empty_text(wbxml_encoder, TRUE);
            wbxml_encoder_set_remove_text_blanks(wbxml_encoder, TRUE);
        }
        else {
            /* Set XML Generation Type */
            wbxml_encoder_set_xml_gen_type(wbxml_encoder, params->gen_type);

            /* Set Indent */
            if (params->gen_type == WBXML_ENCODER_XML_GEN_INDENT)
                wbxml_encoder_set_indent(wbxml_encoder, params->indent);

            /* Ignorable Whitespaces */
            if (params->keep_ignorable_ws) {
                wbxml_encoder_set_ignore_empty_text(wbxml_encoder, FALSE);
                wbxml_encoder_set_remove_text_blanks(wbxml_encoder, FALSE);
            }
            else {
                wbxml_encoder_set_ignore_empty_text(wbxml_encoder, TRUE);
                wbxml_encoder_set_remove_text_blanks(wbxml_encoder, TRUE);
            }
        }

        /* Encode WBXML Tree to XML */
        ret = wbxml_encoder_encode_to_xml(wbxml_encoder, xml);
        if (ret != WBXML_OK) {
            WBXML_ERROR((WBXML_CONV, "wbxml2xml convertion failed - WBXML Encoder Error: %s",
                                     wbxml_errors_string(ret)));
        }

        /* Clean-up */
        wbxml_encoder_destroy(wbxml_encoder);
        wbxml_tree_destroy(wbxml_tree);
        return ret;
    }
}


WBXML_DECLARE(WBXMLError) wbxml_conv_xml2wbxml(WB_UTINY *xml, WB_UTINY **wbxml, WB_ULONG *wbxml_len, WBXMLConvXML2WBXMLParams *params)
{
#if !defined( HAVE_EXPAT )

    return WBXML_ERROR_NO_XMLPARSER;

#else /* HAVE_EXPAT */

    WBXMLEncoder *wbxml_encoder = NULL;    
    WBXMLTree *wbxml_tree = NULL;
    WBXMLError ret = WBXML_OK;

    /* Check Parameters */
    if ((xml == NULL) || (wbxml == NULL) || (wbxml_len == NULL))
        return WBXML_ERROR_BAD_PARAMETER;

    *wbxml = NULL;
    *wbxml_len = 0;
    
    /* Parse XML to WBXML Tree */
    ret = wbxml_tree_from_xml(xml, &wbxml_tree);
    if (ret != WBXML_OK) {
        WBXML_ERROR((WBXML_CONV, "xml2wbxml convertion failed - Error: %s",
                                  wbxml_errors_string(ret)));

        return ret;
    }
    else {
        /* Encode WBXML Tree to WBXML Document */
        if ((wbxml_encoder = wbxml_encoder_create()) == NULL) {
            wbxml_tree_destroy(wbxml_tree);
            return WBXML_ERROR_NOT_ENOUGH_MEMORY;
        }
    
        /* Set the WBXML Tree to encode */
        wbxml_encoder_set_tree(wbxml_encoder, wbxml_tree);

        /* Set encoder parameters */
        if (params == NULL) {
            /* Default Parameters */

            /* Ignores "Empty Text" Nodes */
            wbxml_encoder_set_ignore_empty_text(wbxml_encoder, TRUE);

            /* Remove leading and trailing whitespaces in "Text Nodes" */
            wbxml_encoder_set_remove_text_blanks(wbxml_encoder, TRUE);

            /* Use String Table */
            wbxml_encoder_set_use_strtbl(wbxml_encoder, TRUE);

            /* Don't produce an anonymous document by default */
            wbxml_encoder_set_produce_anonymous(wbxml_encoder, FALSE);
        }
        else {
            /* WBXML Version */
            wbxml_encoder_set_wbxml_version(wbxml_encoder, params->wbxml_version);
            
            /* Keep Ignorable Whitespaces ? */
            if (!params->keep_ignorable_ws) {
                /* Ignores "Empty Text" Nodes */
                wbxml_encoder_set_ignore_empty_text(wbxml_encoder, TRUE);

                /* Remove leading and trailing whitespaces in "Text Nodes" */
                wbxml_encoder_set_remove_text_blanks(wbxml_encoder, TRUE);
            }

            /* String Table */
            wbxml_encoder_set_use_strtbl(wbxml_encoder, params->use_strtbl);

            /* Produce an anonymous document? */
            wbxml_encoder_set_produce_anonymous(wbxml_encoder,
                params->produce_anonymous);
        }

        /* Encode WBXML */
        ret = wbxml_encoder_encode_to_wbxml(wbxml_encoder, wbxml, wbxml_len);

        /* Clean-up */
        wbxml_tree_destroy(wbxml_tree);
        wbxml_encoder_destroy(wbxml_encoder);

        return ret;
    }

#endif /* HAVE_EXPAT */
}
