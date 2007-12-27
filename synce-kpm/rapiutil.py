# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>         #
#    Dr J A Gow 22/11/2007 - Converted all DOM usage to libxml2            #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################

import libxml2
import xml2util
import characteristics
import logging

logger = logging.getLogger("engine.xmlutil")


QUERY_TYPE_GET    = 0
QUERY_TYPE_SET    = 1
QUERY_TYPE_REMOVE = 2

def config_query_get(session, path, leaf, recursive=False):
	return _config_query(session, QUERY_TYPE_GET, path, leaf, recursive)

def config_query_set(session, path, ctic):
	return _config_query(session, QUERY_TYPE_SET, path, None, ctic=ctic)

def config_query_remove(session, path, leaf):
	return _config_query(session, QUERY_TYPE_REMOVE, path, leaf)

def _config_query(session, query_type, path, leaf=None, recursive=False, ctic=None):
	
	doc = libxml2.newDoc("1.0")
	doc_node = doc.newChild(None,"wap-provisioningdoc",None)
	parent=doc_node
	
	if path is not None:
	
		tokens = path.split(".")

        	for token in tokens:
            		node = parent.newChild(None,"characteristic",None)
            		node.setProp("type", token)
            		parent = node
    	else:
        	tokens = []

	if query_type == QUERY_TYPE_GET:
		
        	node = libxml2.newNode("characteristic-query")
        	if not recursive:
            		node.setProp("recursive", "false")
        	node.setProp("type", leaf)
	
    	elif query_type == QUERY_TYPE_REMOVE:
	    
        	node = libxml2.newNode("nocharacteristic")
        	node.setProp("type", leaf)
	
	elif query_type == QUERY_TYPE_SET:
		node = characteristics.characteristic_tree_to_xml(ctic, doc)

	parent.addChild(node)

	logger.debug("_config_query: CeProcessConfig request is \n%s", doc_node.serialize("utf-8",1))

	reply = session.process_config(doc_node.serialize("utf-8",0), 1)
	reply_doc = libxml2.parseDoc(reply)

	logger.debug("_config_query: CeProcessConfig response is \n%s", reply_doc.serialize("utf-8",1))

	reply_node = xml2util.GetNodeOnLevel(reply_doc, 2 + len(tokens))

	if query_type != QUERY_TYPE_SET:
        	return characteristics.characteristic_tree_from_xml(reply_node)
	else:
        	return None
