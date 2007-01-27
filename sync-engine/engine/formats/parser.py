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
from xml import xpath
from engine.xmlutil import *
from engine.util import *
import logging

logger = logging.getLogger("engine.formats.parser")

def set_children_namespace(node, namespace):
    for node in node.childNodes:
        if node.nodeType == node.ownerDocument.ELEMENT_NODE:
            node.setAttribute('xmlns', namespace)
        set_children_namespace(node, namespace)

def from_airsync(guid, app_node, root_name, conv_spec, ignore_spec):
    dom = minidom.getDOMImplementation()
    doc = dom.createDocument(None, root_name, None)
    root_node = doc.documentElement

    # FIXME: do this properly
    src_doc = minidom.parseString(app_node.toxml(encoding="utf-8"))

    #uid_node = node_append_child(root_node, "UID")
    #node_append_child(uid_node, "Content", guid)

    for src_spec, dst_spec in conv_spec:
        if isinstance(src_spec, basestring):
            matches = xpath.Evaluate(src_spec, app_node)
            for node in matches:

                value = node_get_value(node)
                if value == None and len(xpath.Evaluate("*", node)) == 0:
                    continue

                if dst_spec != None:
                    if callable(dst_spec):
                        value = dst_spec(doc, src_doc, node, value)
                        if isinstance(value, basestring):
                            name = node.localName
                        else:
                            name = None
                    elif isinstance(dst_spec, basestring):
                        name = dst_spec
                    else:
                        name = dst_spec[0]
                        if dst_spec[1] != None:
                            value = dst_spec[1](doc, src_doc, node, value)
                        else:
                            value = None

                    if name != None:
                        val_node = node_append_child(root_node, name, always_create=False)
                    else:
                        val_node = root_node

                    if value != None:
                        if isinstance(value, basestring):
                            node_append_child(val_node, "Content", value)
                        else:
                            for n in value:
                                val_node.appendChild(n)
                    else:
                        val_node.appendChild(node.cloneNode(True))
                else:
                    root_node.appendChild(node.cloneNode(True))

                app_node.removeChild(node)
        else:
            src_values = {}
            for i in xrange(len(src_spec)):
                matches = xpath.Evaluate(src_spec[i], app_node)
                if matches:
                    src_values[i] = node_get_value(matches[0])
                    app_node.removeChild(matches[0])

            if len(src_values) > 0:
                src_index = 0
                dst_parent_node = node_append_child(root_node, dst_spec[0])
                for dst_desc in dst_spec[1]:
                    if isinstance(dst_desc, basestring):
                        if src_index in src_values:
                            node_append_child(dst_parent_node, dst_desc, src_values[src_index])
                        src_index += 1
                    else:
                        node_append_child(dst_parent_node, dst_desc[0], dst_desc[1])

    if ignore_spec is not None:
        for entry in ignore_spec:
            nodes = xpath.Evaluate(entry, app_node)
            for node in nodes:
                app_node.removeChild(node)

    nodes = xpath.Evaluate("*", app_node)
    if nodes:
        logger.info("from_airsync: Unparsed XML = %s", app_node.toprettyxml())

    return doc

def to_airsync(os_doc, root_name, conv_spec, namespace):
    doc = minidom.getDOMImplementation().createDocument(None, "ApplicationData", None)
    app_node = doc.documentElement

    # FIXME: do this properly
    src_doc = minidom.parseString(os_doc.toxml(encoding="utf-8"))

    root_node = xpath.Evaluate("/%s" % root_name, os_doc)[0]
    for expr, mappings in conv_spec:
        i = 0

        for node in xpath.Evaluate(expr, root_node):
            if callable(mappings):
                mappings(doc, src_doc, node, None)
            elif mappings != None:
                mapping = mappings[i]

                if not callable(mapping):
                    if not isinstance(mapping, dict):
                        mapping = { "Content" : mapping }

                    for node_name, as_name in mapping.items():
                        nodes = xpath.Evaluate(node_name, node)
                        if nodes:
                            value = node_get_value(nodes[0])
                            if callable(as_name):
                                value = as_name(doc, src_doc, nodes[0], value)
                                as_name = node.localName
                            node_append_child(app_node, as_name, value)
                else:
                    for n in mapping(doc, src_doc, node, node_get_value(node)):
                        app_node.appendChild(n)
            else:
                app_node.appendChild(node.cloneNode(True))

            node.parentNode.removeChild(node)

            i += 1
            if not callable(mappings):
                if mappings == None or i >= len(mappings):
                    break

    # Fix up the namespace attributes of all the nodes in the Airsync doc if necessary
    if namespace != None and namespace != "":
        set_children_namespace(doc.documentElement, namespace)

    nodes = xpath.Evaluate("*/*", root_node)
    if nodes:
        logger.info("to_airsync: Unparsed XML = %s", root_node.toprettyxml())

    return doc

