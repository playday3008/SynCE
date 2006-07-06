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
 * @file wbxml_tree_clb_wbxml.c
 * @ingroup wbxml_tree
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 03/02/22
 *
 * @brief WBXML Tree Callbacks for WBXML Parser
 */

#include "wbxml.h"


/* Private Functions Prototypes */
static WBXMLTreeAttribute *construct_attribute_list(WBXMLAttribute **atts);


/***************************************************
 *  Public Functions
 */

void wbxml_tree_clb_wbxml_start_document(void *ctx, WB_LONG charset, const WBXMLLangEntry *lang)
{
    WBXMLTreeClbCtx *tree_ctx = (WBXMLTreeClbCtx *) ctx;

    if (tree_ctx->error != WBXML_OK)
        return;

    tree_ctx->tree->lang = lang;
}


void wbxml_tree_clb_wbxml_end_document(void *ctx)
{
    WBXMLTreeClbCtx *tree_ctx = (WBXMLTreeClbCtx *) ctx;

    if (tree_ctx->error != WBXML_OK)
        return;
}


void wbxml_tree_clb_wbxml_start_element(void *ctx, WBXMLTag *element, WBXMLAttribute **attrs, WB_BOOL empty)
{
    WBXMLTreeClbCtx *tree_ctx = (WBXMLTreeClbCtx *) ctx;
    WBXMLTreeNode *node = NULL;

    if (tree_ctx->error != WBXML_OK)
        return;

    /* Create a new Node */
    if ((node = wbxml_tree_node_create(WBXML_TREE_ELEMENT_NODE)) == NULL) {
        tree_ctx->error = WBXML_ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    /* Set Element */
    if ((node->name = wbxml_tag_duplicate(element)) == NULL) {
        wbxml_tree_node_destroy(node);
        tree_ctx->error = WBXML_ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    /* Set Attributes */
    if (attrs != NULL) {
        if ((node->attrs = construct_attribute_list(attrs)) == NULL) {
            wbxml_tree_node_destroy(node);
            tree_ctx->error = WBXML_ERROR_NOT_ENOUGH_MEMORY;
            return;
        }
    }

    /* Add this Node to Tree  */
    if (!wbxml_tree_add_node(tree_ctx->tree, tree_ctx->current, node)) {
        tree_ctx->error = WBXML_ERROR_INTERNAL;
        return;
    }

    /* This is the new Current Element */
    tree_ctx->current = node;
}


void wbxml_tree_clb_wbxml_end_element(void *ctx, WBXMLTag *element, WB_BOOL empty)
{
    WBXMLTreeClbCtx *tree_ctx = (WBXMLTreeClbCtx *) ctx;

    if (tree_ctx->error != WBXML_OK)
        return;

    if (tree_ctx->current == NULL) {
        tree_ctx->error = WBXML_ERROR_INTERNAL;
        return;
    }

    if (tree_ctx->current->parent == NULL) {
        /* This must be the Root Element */
        if (tree_ctx->current != tree_ctx->tree->root) {
            tree_ctx->error = WBXML_ERROR_INTERNAL;
        }
    }
    else {
#if defined ( WBXML_SUPPORT_SYNCML )
        /* Have we added a CDATA section ? 
         * If so, we assume that now that we have reached an end of Element, 
         * the CDATA section ended, and so we go back to parent.
         */
        if ((tree_ctx->current != NULL) && (tree_ctx->current->type == WBXML_TREE_CDATA_NODE))
            tree_ctx->current = tree_ctx->current->parent;
#endif /* WBXML_SUPPORT_SYNCML */

        /* Go back one step upper in the tree */
        tree_ctx->current = tree_ctx->current->parent;
    }
}


void wbxml_tree_clb_wbxml_characters(void *ctx, WB_UTINY *ch, WB_ULONG start, WB_ULONG length)
{
    WBXMLTreeClbCtx *tree_ctx = (WBXMLTreeClbCtx *) ctx;
    WBXMLTreeNode *node = NULL;
#if defined ( WBXML_SUPPORT_SYNCML )
    WBXMLTree *tmp_tree = NULL;
#endif /* WBXML_SUPPORT_SYNCML */

    if (tree_ctx->error != WBXML_OK)
        return;

#if defined ( WBXML_SUPPORT_SYNCML )
    /* Specific treatment for SyncML */
    switch (wbxml_tree_node_get_syncml_data_type(tree_ctx->current)) {
    case WBXML_SYNCML_DATA_TYPE_WBXML:
        /* Deal with Embedded SyncML Documents - Parse WBXML */
        if (wbxml_tree_from_wbxml(ch + start, length, WBXML_LANG_UNKNOWN, &tmp_tree) != WBXML_OK) {
            /* Not parsable ? Just add it as a Text Node... */
            goto text_node;
        }

        /* Create a new Node */
        if ((node = wbxml_tree_node_create(WBXML_TREE_TREE_NODE)) == NULL) {
            wbxml_tree_destroy(tmp_tree);
            tree_ctx->error = WBXML_ERROR_NOT_ENOUGH_MEMORY;            
            return;
        }

        /* Set Tree */
        node->tree = tmp_tree;
        break;

	case WBXML_SYNCML_DATA_TYPE_CLEAR:
    case WBXML_SYNCML_DATA_TYPE_DIRECTORY_VCARD:
    case WBXML_SYNCML_DATA_TYPE_VCALENDAR:
    case WBXML_SYNCML_DATA_TYPE_VCARD:
        /*
         * Add a CDATA section node
         *
         * Example:
	     * <Add>
		 *   <CmdID>6</CmdID>
		 *   <Meta><Type xmlns='syncml:metinf'>text/x-vcard</Type></Meta>
		 *   <Item>
		 *     <Source>
		 *	     <LocURI>pas-id-3F4B790300000000</LocURI>
		 *     </Source>         
         *     <Data><![CDATA[BEGIN:VCARD
         *  VERSION:2.1
         *  X-EVOLUTION-FILE-AS:Ximian, Inc.
         *  N:
         *  LABEL;WORK;ENCODING=QUOTED-PRINTABLE:401 Park Drive  3 West=0ABoston, MA
         *  02215=0AUSA
         *  TEL;WORK;VOICE:(617) 236-0442
         *  TEL;WORK;FAX:(617) 236-8630
         *  EMAIL;INTERNET:[EMAIL PROTECTED]
         *  URL:www.ximian.com/
         *  ORG:Ximian, Inc.
         *  NOTE:Welcome to the Ximian Addressbook.
         *  UID:pas-id-3F4B790300000000
         *  END:VCARD
         *  ]]>
         *     </Data>
         *   </Item>
         * </Add>
         *
         * The end of CDATA section is assumed to be reached when parsing the end 
         * of </Data> element.
         */

        /* Create a new CDATA Node */
        if ((node = wbxml_tree_node_create(WBXML_TREE_CDATA_NODE)) == NULL) {
            tree_ctx->error = WBXML_ERROR_NOT_ENOUGH_MEMORY;
            return;
    }

        /* Add this CDATA Node to Tree  */
        if (!wbxml_tree_add_node(tree_ctx->tree, tree_ctx->current, node)) {
            tree_ctx->error = WBXML_ERROR_INTERNAL;
            return;
        }

        /* This is the new Current Element */
        tree_ctx->current = node;

        /* Now we can add the Text Node */
        goto text_node;

        break;

    default:
text_node:
#endif /* WBXML_SUPPORT_SYNCML */

        /* Create a new Node */
        if ((node = wbxml_tree_node_create(WBXML_TREE_TEXT_NODE)) == NULL) {
            tree_ctx->error = WBXML_ERROR_NOT_ENOUGH_MEMORY;
            return;
        }

        /* Set Content */
        if ((node->content = wbxml_buffer_create_real(ch + start, length, length)) == NULL) {
            wbxml_tree_node_destroy(node);
            tree_ctx->error = WBXML_ERROR_NOT_ENOUGH_MEMORY;
            return;
        }

#if defined ( WBXML_SUPPORT_SYNCML )
    } /* switch */
#endif /* WBXML_SUPPORT_SYNCML */

    /* Add this Node to Tree  */
    if (!wbxml_tree_add_node(tree_ctx->tree, tree_ctx->current, node)) {
        tree_ctx->error = WBXML_ERROR_INTERNAL;
        return;
    }
}


void wbxml_tree_clb_wbxml_pi(void *ctx, const WB_UTINY *target, WB_UTINY *data)
{
    /** @todo wbxml_tree_clb_pi() */
}


/***************************************************
 *  Private Functions
 */

/**
 * @brief Construct a Tree Attribute List
 * @param attrs The Attribute Table to Duplicate
 * @return The Attribute List, or NULL if Error
 */
static WBXMLTreeAttribute *construct_attribute_list(WBXMLAttribute **attrs)
{
    WBXMLTreeAttribute *attr = NULL, *first = NULL, *curr = NULL;
    WB_ULONG i = 0;

    if (attrs == NULL)
        return NULL;

    while (attrs[i] != NULL) {
        /* Create Tree Attribute */
        if ((attr = wbxml_tree_attribute_create()) == NULL) {
            wbxml_tree_attribute_destroy(first);
            return NULL;
        }

        /* Link it to previous Attribute */
        if (curr != NULL)
            curr->next = attr;

        /* Duplicate Attribute */
        if ((attr->attr = wbxml_attribute_duplicate(attrs[i])) == NULL) {
            wbxml_tree_attribute_destroy(first);
            return NULL;
        }

        /* Keep the first one somewhere */
        if (i == 0)
            first = attr;

        /* Go to next Attribute */
        curr = attr;
        i++;
    }

    return first;
}
