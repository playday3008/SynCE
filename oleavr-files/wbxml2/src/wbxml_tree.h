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
 * @file wbxml_tree.h
 * @ingroup wbxml_tree
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 03/02/16
 *
 * @brief WBXML Tree
 */

#ifndef WBXML_TREE_H
#define WBXML_TREE_H


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup wbxml_tree
 *  @{ 
 */


/****************************************************
 *	WBXML Tree Structures
 */
  

/**
 * @brief WBXML Tree Node Type
 */
typedef enum WBXMLTreeNodeType_e
{
    WBXML_TREE_ELEMENT_NODE = 0, /**< Element Node */
    WBXML_TREE_TEXT_NODE,        /**< Text Node */
    WBXML_TREE_CDATA_NODE,       /**< CDATA Node */
    WBXML_TREE_PI_NODE,          /**< PI Node */
    WBXML_TREE_TREE_NODE         /**< WBXML Tree Node */
} WBXMLTreeNodeType;


/**
 * @brief WBXML Tree Attribute structure
 */
typedef struct WBXMLTreeAttribute_s
{
    WBXMLAttribute  *attr;              /**< Attribute */
    struct WBXMLTreeAttribute_s  *next; /**< Next attribute */
} WBXMLTreeAttribute;


/**
 * @brief WBXML Tree Node structure
 */
typedef struct WBXMLTreeNode_s
{
    WBXMLTreeNodeType   type;       /**< Node Type */
    WBXMLTag            *name;      /**< Node Name (if type is 'WBXML_TREE_ELEMENT_NODE') */
    WBXMLTreeAttribute  *attrs;     /**< Node Attributes (if type is 'WBXML_TREE_ELEMENT_NODE') */
    WBXMLBuffer         *content;   /**< Node Content (if  type is 'WBXML_TREE_TEXT_NODE')  */
    struct WBXMLTree_s  *tree;      /**< Node Tree (if  type is 'WBXML_TREE_TREE_NODE') */
        
    struct WBXMLTreeNode_s  *parent;    /**< Parent Node */
    struct WBXMLTreeNode_s  *children;  /**< Children Node */
    struct WBXMLTreeNode_s  *next;      /**< Next sibling Node */
    struct WBXMLTreeNode_s  *prev;      /**< Previous sibling Node */
} WBXMLTreeNode;


/**
 * @brief WBXML Tree structure
 */
typedef struct WBXMLTree_s
{    
    const WBXMLLangEntry  *lang; /**< Language Table */
    WBXMLTreeNode   *root;       /**< Root Element */
} WBXMLTree;


/** 
 * WBXML Tree Clb Context Structure
 * @note Used by WBXML Tree Callbacks ('wbxml_tree_clb_wbxml.h' and 'wbxml_tree_clb_xml.h')
 */
typedef struct WBXMLTreeClbCtx_s {
    /* For XML and WBXML Clb: */
    WBXMLTree *tree;            /**< The WBXML Tree we are constructing */
    WBXMLTreeNode *current;     /**< Current Tree Node */
    WBXMLError error;           /**< Error while parsing Document */
    /* For XML Clb: */
    WB_ULONG skip_lvl;          /**< Used to skip a whole XML node (used for SyncML) */
    WB_LONG skip_start;         /**< Starting Skipping position in XML Document (used for SyncML) */
    WB_UTINY *input_buff;       /**< Pointer to Input Buffer */
#if defined( HAVE_EXPAT )
    XML_Parser xml_parser;      /**< Pointer to Expat XML Parser */
#endif /* HAVE_EXPAT */    
} WBXMLTreeClbCtx;


#if defined ( WBXML_SUPPORT_SYNCML )
/**
 * SyncML Data Type (the type of data inside <Data> element)
 */
typedef enum WBXMLSyncMLDataType_e {
    WBXML_SYNCML_DATA_TYPE_NORMAL = 0,      /**< Not specific Data Type */
    WBXML_SYNCML_DATA_TYPE_WBXML,           /**< application/vnd.syncml-devinf+wbxml (WBXML Document) */
    WBXML_SYNCML_DATA_TYPE_CLEAR,			/**< text/clear */
    WBXML_SYNCML_DATA_TYPE_DIRECTORY_VCARD, /**< text/directory;profile=vCard */
    WBXML_SYNCML_DATA_TYPE_VCARD,           /**< text/x-vcard */
    WBXML_SYNCML_DATA_TYPE_VCALENDAR        /**< text/x-vcalendar */
} WBXMLSyncMLDataType;
#endif /* WBXML_SUPPORT_SYNCML */


/****************************************************
 *  WBXML Tree Building Functions
 */

/**
 * @brief Parse a WBXML document, using internal callbacks, and construct a WBXML Tree
 * @param wbxml The WBXML document to parse
 * @param wbxml_len The WBXML document length
 * @param lang Can be used to force parsing of a given Language (set it to WBXML_LANG_UNKNOWN if you don't want to force anything)
 * @param tree [out] The resulting WBXML Tree 
 * @result Return WBXML_OK if no error, an error code otherwise
 */
WBXML_DECLARE(WBXMLError) wbxml_tree_from_wbxml(WB_UTINY *wbxml, WB_ULONG wbxml_len, WBXMLLanguage lang, WBXMLTree **tree);

/**
 * @brief Parse an XML document, using internal callbacks, and construct a WBXML Tree
 * @param xml The XML document to parse
 * @param tree [out] The resulting WBXML Tree 
 * @result Return WBXML_OK if no error, an error code otherwise
 * @note Needs 'HAVE_EXPAT' compile flag
 */
WBXML_DECLARE(WBXMLError) wbxml_tree_from_xml(WB_UTINY *xml, WBXMLTree **tree);


/****************************************************
 *	WBXML Tree Functions
 */

/* WBXMLTreeAttribute */

/**
 * @brief Create a Tree Attribute structure
 * @return The newly created Tree Attribute, or NULL if not enough memory
 */
WBXML_DECLARE(WBXMLTreeAttribute *) wbxml_tree_attribute_create(void);

/**
 * @brief Destroy a Tree Attribute structure
 * @param attr The Tree Attribute structure to destroy
 */
WBXML_DECLARE(void) wbxml_tree_attribute_destroy(WBXMLTreeAttribute *attr);


/* WBXMLTreeNode */

/**
 * @brief Create a Tree Node structure
 * @param type Node type
 * @return The newly created Tree Node, or NULL if not enough memory
 */
WBXML_DECLARE(WBXMLTreeNode *) wbxml_tree_node_create(WBXMLTreeNodeType type);

/**
 * @brief Destroy a Tree Node structure
 * @param node The Tree Node structure to destroy
 */
WBXML_DECLARE(void) wbxml_tree_node_destroy(WBXMLTreeNode *node);

/**
 * @brief Get an Element Node, given the Element Name
 * @param node   The Tree Node where to start searching
 * @param name   The Element Name we are searching
 * @param recurs If FALSE, only search into direct childs of 'node'
 * @return The found Tree Node, or NULL if not found
 */
WBXML_DECLARE(WBXMLTreeNode *) wbxml_tree_node_elt_get_from_name(WBXMLTreeNode *node, const char *name, WB_BOOL recurs);

#if defined ( WBXML_SUPPORT_SYNCML )
/**
 * @brief Get the SyncML Data Type of this Tree Node
 * @param node The Tree Node
 * @return The SyncML Data Type of this Tree Node (cf: WBXMLSyncMLDataType enum)
 * @note If no specific Data Type is found, this function returns 'WBXML_SYNCML_DATA_TYPE_NORMAL'
 */
WBXML_DECLARE(WBXMLSyncMLDataType) wbxml_tree_node_get_syncml_data_type(WBXMLTreeNode *node);
#endif /* WBXML_SUPPORT_SYNCML */


/* WBXMLTree */

/**
 * @brief Create a Tree structure
 * @return The newly created Tree, or NULL if not enough memory
 */
WBXML_DECLARE(WBXMLTree *) wbxml_tree_create(void);

/**
 * @brief Destroy a Tree structure
 * @param tree The Tree structure to destroy
 */
WBXML_DECLARE(void) wbxml_tree_destroy(WBXMLTree *tree);

/**
 * @brief Add a Tree Node to a Tree structure
 * @param tree   The Tree to modify
 * @param parent Parent of the new Tree Node (ie: Position where to add the new Tree Node in Tree structure)
 * @param node   The new Tree Node to add
 * @return TRUE is added, or FALSE if error.
 * @note If 'parent' is NULL: if 'tree' already have a Root Element this function returns FALSE, else 'node' becomes the Root Element of 'tree'
 */
WBXML_DECLARE(WB_BOOL) wbxml_tree_add_node(WBXMLTree *tree, WBXMLTreeNode *parent, WBXMLTreeNode *node);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WBXML_TREE_H */
