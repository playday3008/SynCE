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
 * @file wbxml_tree.c
 * @ingroup wbxml_tree
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 03/02/18
 *
 * @brief WBXML Tree
 */

#include "wbxml.h"


/***************************************************
 *    Public Functions
 */

/* WBXML Tree Building Functions */

WBXML_DECLARE(WBXMLError) wbxml_tree_from_wbxml(WB_UTINY *wbxml, WB_ULONG wbxml_len, WBXMLLanguage lang, WBXMLTree **tree)
{   
    WBXMLParser *wbxml_parser = NULL;
    WB_LONG error_index = 0;
    WBXMLTreeClbCtx wbxml_tree_clb_ctx;
    WBXMLError ret = WBXML_OK;
    WBXMLContentHandler wbxml_tree_content_handler = 
        {
            wbxml_tree_clb_wbxml_start_document,
            wbxml_tree_clb_wbxml_end_document,
            wbxml_tree_clb_wbxml_start_element,
            wbxml_tree_clb_wbxml_end_element,
            wbxml_tree_clb_wbxml_characters,
            wbxml_tree_clb_wbxml_pi
        };

    if (tree != NULL)
        *tree = NULL;

    /* Create WBXML Parser */
    if((wbxml_parser = wbxml_parser_create()) == NULL) {
        WBXML_ERROR((WBXML_PARSER, "Can't create WBXML Parser"));
        return WBXML_ERROR_NOT_ENOUGH_MEMORY;
    }

    /* Init context */
    wbxml_tree_clb_ctx.error = WBXML_OK;
    wbxml_tree_clb_ctx.current = NULL;
    if ((wbxml_tree_clb_ctx.tree = wbxml_tree_create()) == NULL) {
        wbxml_parser_destroy(wbxml_parser);
        WBXML_ERROR((WBXML_PARSER, "Can't create WBXML Tree"));
        return WBXML_ERROR_NOT_ENOUGH_MEMORY;
    }
    
    /* Set Handlers Callbacks */
    wbxml_parser_set_user_data(wbxml_parser, &wbxml_tree_clb_ctx);
    wbxml_parser_set_content_handler(wbxml_parser, &wbxml_tree_content_handler);

    /* Give the user the possibility to force Document Language */
    if (lang != WBXML_LANG_UNKNOWN)
        wbxml_parser_set_language(wbxml_parser, lang);

    /* Parse the WBXML document to WBXML Tree */
    ret = wbxml_parser_parse(wbxml_parser, wbxml, wbxml_len);
    if ((ret != WBXML_OK) || (wbxml_tree_clb_ctx.error != WBXML_OK)) 
    {
        error_index = wbxml_parser_get_current_byte_index(wbxml_parser);
        WBXML_ERROR((WBXML_PARSER, "WBXML Parser failed at %ld - token: %x (%s)", 
                                   error_index,
                                   wbxml[error_index],
                                   ret != WBXML_OK ? wbxml_errors_string(ret) : wbxml_errors_string(wbxml_tree_clb_ctx.error)));
        
        wbxml_tree_destroy(wbxml_tree_clb_ctx.tree);
    }
    else {
        *tree = wbxml_tree_clb_ctx.tree;
    }

    /* Clean-up */
    wbxml_parser_destroy(wbxml_parser);

    if (ret != WBXML_OK)
        return ret;
    else
        return wbxml_tree_clb_ctx.error;
}


WBXML_DECLARE(WBXMLError) wbxml_tree_from_xml(WB_UTINY *xml, WBXMLTree **tree)
{
#if !defined( HAVE_EXPAT )

    return WBXML_ERROR_NO_XMLPARSER;

#else /* HAVE_EXPAT */

    XML_Parser xml_parser = NULL;
    WBXMLTreeClbCtx wbxml_tree_clb_ctx;
    WBXMLError ret = WBXML_OK;

    if (tree != NULL)
        *tree = NULL;

    /* Create XML Parser */
    if ((xml_parser = XML_ParserCreate(NULL)) == NULL)
        return WBXML_ERROR_NOT_ENOUGH_MEMORY;
    
    /* Init context */
    wbxml_tree_clb_ctx.current = NULL;
    wbxml_tree_clb_ctx.error = WBXML_OK;
    wbxml_tree_clb_ctx.skip_lvl = 0;
    wbxml_tree_clb_ctx.skip_start = 0;
    wbxml_tree_clb_ctx.xml_parser = xml_parser;
    wbxml_tree_clb_ctx.input_buff = xml;    
    wbxml_tree_clb_ctx.cur_code_page = 0;

    if ((wbxml_tree_clb_ctx.tree = wbxml_tree_create()) == NULL) {
        XML_ParserFree(xml_parser);
        WBXML_ERROR((WBXML_PARSER, "Can't create WBXML Tree"));
        return WBXML_ERROR_NOT_ENOUGH_MEMORY;
    }
    
    /* Set Handlers Callbacks */
    XML_SetStartDoctypeDeclHandler(xml_parser, wbxml_tree_clb_xml_doctype_decl);
    XML_SetElementHandler(xml_parser, wbxml_tree_clb_xml_start_element, wbxml_tree_clb_xml_end_element);
	XML_SetCdataSectionHandler(xml_parser, wbxml_tree_clb_xml_start_cdata, wbxml_tree_clb_xml_end_cdata);
	XML_SetProcessingInstructionHandler(xml_parser , wbxml_tree_clb_xml_pi);
    XML_SetCharacterDataHandler(xml_parser, wbxml_tree_clb_xml_characters);
    XML_SetUserData(xml_parser, (void*)&wbxml_tree_clb_ctx);

    /* Parse the XML Document to WBXML Tree */
    if (XML_Parse(xml_parser, (WB_TINY*) xml, WBXML_STRLEN(xml), TRUE) == 0)
    {
        WBXML_ERROR((WBXML_CONV, "xml2wbxml convertion failed - expat error %i\n"
            "\tdescription: %s\n"
            "\tline: %i\n"
            "\tcolumn: %i\n"
            "\tbyte index: %i\n"
            "\ttotal bytes: %i\n%s",
            XML_GetErrorCode(xml_parser), 
            XML_ErrorString(XML_GetErrorCode(xml_parser)), 
            XML_GetCurrentLineNumber(xml_parser), 
            XML_GetCurrentColumnNumber(xml_parser), 
            XML_GetCurrentByteIndex(xml_parser), 
            XML_GetCurrentByteCount(xml_parser), xml));

        wbxml_tree_destroy(wbxml_tree_clb_ctx.tree);

        ret = WBXML_ERROR_XML_PARSING_FAILED;
    }
    else {
        if ((ret = wbxml_tree_clb_ctx.error) != WBXML_OK)
            wbxml_tree_destroy(wbxml_tree_clb_ctx.tree);
        else
            *tree = wbxml_tree_clb_ctx.tree;
    }

    /* Clean-up */
    XML_ParserFree(xml_parser);

    return ret;

#endif /* HAVE_EXPAT */
}


/* WBXMLTreeAttribute */

WBXML_DECLARE(WBXMLTreeAttribute *) wbxml_tree_attribute_create(void)
{
    WBXMLTreeAttribute *result = NULL;
    
    if ((result = (WBXMLTreeAttribute *) wbxml_malloc(sizeof(WBXMLTreeAttribute))) == NULL)
        return NULL;

    result->attr = NULL;
    result->next = NULL;

    return result;
}


WBXML_DECLARE(void) wbxml_tree_attribute_destroy(WBXMLTreeAttribute *attr)
{
    WBXMLTreeAttribute *next = NULL;

    while (attr != NULL) 
    {
        next = attr->next;

        wbxml_attribute_destroy(attr->attr);   
        wbxml_free(attr);

        attr = next;
    }
}


/* WBXMLTreeNode */

WBXML_DECLARE(WBXMLTreeNode *) wbxml_tree_node_create(WBXMLTreeNodeType type)
{
    WBXMLTreeNode *result = NULL;
    
    if ((result = (WBXMLTreeNode *) wbxml_malloc(sizeof(WBXMLTreeNode))) == NULL)
        return NULL;

    result->type = type;
    result->name = NULL;
    result->attrs = NULL;
    result->content = NULL;
    result->tree = NULL;

    result->parent = NULL;
    result->children = NULL;
    result->next = NULL;
    result->prev = NULL;

    return result;
}


WBXML_DECLARE(void) wbxml_tree_node_destroy(WBXMLTreeNode *node)
{
    if (node == NULL)
        return;

    wbxml_tag_destroy(node->name);
    wbxml_tree_attribute_destroy(node->attrs);
    wbxml_buffer_destroy(node->content);
    wbxml_tree_destroy(node->tree);

    wbxml_free(node);
}


WBXML_DECLARE(WBXMLTreeNode *) wbxml_tree_node_elt_get_from_name(WBXMLTreeNode *node, const char *name, WB_BOOL recurs)
{
    WBXMLTreeNode *current_node = NULL;
    WB_BOOL node_found = FALSE;

    if ((node == NULL) || (name == NULL))
        return NULL;

    /** @todo Handle 'recurs' TRUE */

	/* Let's go through the tree */
	current_node = node;

	while (current_node != NULL)
    {
		/* Is this the Node we searched ? */
        if ((current_node->type == WBXML_TREE_ELEMENT_NODE) && 
            (WBXML_STRCMP(wbxml_tag_get_xml_name(current_node->name), name) == 0))
        {
            node_found = TRUE;
            break;
        }
        else {
            /* Go to next Sibbling Node */
            current_node = current_node->next;
        }
	}

    if (node_found)
        return current_node;

    return NULL;
}


#if defined ( WBXML_SUPPORT_SYNCML )

WBXML_DECLARE(WBXMLSyncMLDataType) wbxml_tree_node_get_syncml_data_type(WBXMLTreeNode *node)
{
    WBXMLTreeNode *tmp_node = NULL;

    if (node == NULL)
        return WBXML_SYNCML_DATA_TYPE_NORMAL;

    /* Are we in a <Data> ? */
    if ((node->type == WBXML_TREE_ELEMENT_NODE) &&
        (node->name != NULL) &&
        (WBXML_STRCMP(wbxml_tag_get_xml_name(node->name), "Data") == 0))
    {
        /* Go to Parent element (or Parent of Parent) and search for <Meta> */
        if (((node->parent != NULL) && 
             (node->parent->children != NULL) && 
             ((tmp_node = wbxml_tree_node_elt_get_from_name(node->parent->children, "Meta", FALSE)) != NULL)) ||
            ((node->parent != NULL) && 
             (node->parent->parent != NULL) && 
             (node->parent->parent->children != NULL) && 
             ((tmp_node = wbxml_tree_node_elt_get_from_name(node->parent->parent->children, "Meta", FALSE)) != NULL))) 
        {
            /* Search for <Type> */
            if ((tmp_node = wbxml_tree_node_elt_get_from_name(tmp_node->children, "Type", FALSE)) != NULL) 
            {
                /* Check <Type> value */
                if ((tmp_node->children != NULL) && (tmp_node->children->type == WBXML_TREE_TEXT_NODE)) {
                    /* application/vnd.syncml-devinf+wbxml */
                    if (wbxml_buffer_compare_cstr(tmp_node->children->content, "application/vnd.syncml-devinf+wbxml") == 0) {
                        return WBXML_SYNCML_DATA_TYPE_WBXML;
                    }
                    
					/* text/clear */
                    if (wbxml_buffer_compare_cstr(tmp_node->children->content, "text/clear") == 0) {
					    return WBXML_SYNCML_DATA_TYPE_CLEAR;
					}
					
					/* text/directory;profile=vCard */
                    if (wbxml_buffer_compare_cstr(tmp_node->children->content, "text/directory;profile=vCard") == 0) {
					    return WBXML_SYNCML_DATA_TYPE_DIRECTORY_VCARD;
					}
					
					/* text/x-vcard */
                    if (wbxml_buffer_compare_cstr(tmp_node->children->content, "text/x-vcard") == 0) {
                        return WBXML_SYNCML_DATA_TYPE_VCARD;
                    }
					
					/* text/x-vcalendar */
                    if (wbxml_buffer_compare_cstr(tmp_node->children->content, "text/x-vcalendar") == 0) {
					    return WBXML_SYNCML_DATA_TYPE_VCALENDAR;
					}
                }
            }
        }
    }

    return WBXML_SYNCML_DATA_TYPE_NORMAL;
}

#endif /* WBXML_SUPPORT_SYNCML */



/* WBXMLTree */

WBXML_DECLARE(WBXMLTree *) wbxml_tree_create(void)
{
    WBXMLTree *result = NULL;
    
    if ((result = (WBXMLTree *) wbxml_malloc(sizeof(WBXMLTree))) == NULL)
        return NULL;

    result->lang = NULL;
    result->root = NULL;

    return result;
}


WBXML_DECLARE(void) wbxml_tree_destroy(WBXMLTree *tree)
{
    WBXMLTreeNode *current_node = NULL, *previous_node = NULL, *tmp_node = NULL;
    WB_BOOL end_of_walk= FALSE;

    if (tree == NULL)
        return;

	/* Let's go through the tree (iteratively) to free all the nodes */
	current_node = tree->root;

	while (!end_of_walk)
    {
		if (current_node == NULL) {
			if (previous_node == NULL) {
				end_of_walk = TRUE;
				break;
			}
			else {
				if (previous_node->parent == NULL) {
					/* End of parsing, we have parsed the last child of root node */
					end_of_walk = TRUE;
					break;
				}
				else {
					/* Let's parse next child of parent element */					
					current_node = previous_node->next;
					tmp_node = previous_node->parent;

					/* Destroy this node (leaf) */
					wbxml_tree_node_destroy(previous_node);

					previous_node = tmp_node;
				}
			}
		}
		else {
			previous_node = current_node;
			current_node = current_node->children;		
		}
	}

    wbxml_tree_node_destroy(tree->root);
    wbxml_free(tree);
}


WBXML_DECLARE(WB_BOOL) wbxml_tree_add_node(WBXMLTree *tree, WBXMLTreeNode *parent, WBXMLTreeNode *node)
{
    WBXMLTreeNode *tmp = NULL;

    if ((tree == NULL) || (node == NULL))
        return FALSE;

    /* Set parent to new node */
    node->parent = parent;    

    /* Check if this is the Root Element */
    if (parent != NULL) {
		/* This is not the Root Element... search for previous sibbling element */
		if (parent->children != NULL) {
            /* Add this Node to end of Sibbling Node list of Parent */
			tmp = parent->children;

			while (tmp->next != NULL)
				tmp = tmp->next;
			
			node->prev = tmp;
			tmp->next = node;
        }
        else {
			/* No previous sibbling element */
			parent->children = node;
        }
	}
    else {
        /* We do NOT allow replacement of an existing Tree Node */
        if (tree->root != NULL)
            return FALSE;

        /* This is the Root Element */
        tree->root = node;
    }

    return TRUE;
}
