# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>       #
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

from xml.dom import minidom
from xmlutil import *

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
    doc = minidom.Document()
    doc_node = doc.createElement("wap-provisioningdoc")
    doc.appendChild(doc_node)

    parent = doc_node
    if path is not None:
        tokens = path.split(".")

        for token in tokens:
            node = doc.createElement("characteristic")
            node.setAttribute("type", token)
            parent.appendChild(node)
            parent = node
    else:
        tokens = []

    if query_type == QUERY_TYPE_GET:
        node = doc.createElement("characteristic-query")
        if not recursive:
            node.setAttribute("recursive", "false")
        node.setAttribute("type", leaf)
    elif query_type == QUERY_TYPE_REMOVE:
        node = doc.createElement("nocharacteristic")
        node.setAttribute("type", leaf)
    elif query_type == QUERY_TYPE_SET:
        node = characteristic_tree_to_xml(ctic, doc)

    parent.appendChild(node)

    logger.debug("_config_query: CeProcessConfig request is \n%s", doc_node.toprettyxml())

    reply = session.process_config(doc_node.toxml(), 1)
    reply_doc = minidom.parseString(reply)

    logger.debug("_config_query: CeProcessConfig response is \n%s", reply_doc.documentElement.toprettyxml())

    reply_node = get_node_on_level(reply_doc, 2 + len(tokens))

    if query_type != QUERY_TYPE_SET:
        return characteristic_tree_from_xml(reply_node)
    else:
        return None
