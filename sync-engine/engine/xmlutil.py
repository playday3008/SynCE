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

import logging

from exceptions import ValueError

logger = logging.getLogger("engine.xmlutil")

def get_node_on_level(parent, level):
    while level > 0:
        for el in parent.childNodes:
            if el.nodeType == el.ELEMENT_NODE:
                parent = el
                level -= 1
                if level > 0:
                    break
                else:
                    return el

def node_find_child(node, name):
    for n in node.childNodes:
        if n.nodeType == n.ELEMENT_NODE and n.localName == name:
            return n
    return None

def node_get_child(node, name):
    child = node_find_child(node, name)
    if child is None:
        raise ValueError("child not found")
    return child

def node_get_value(node):
    for n in node.childNodes:
        if n.nodeType == n.TEXT_NODE:
            return n.nodeValue.strip()
    return None

def node_get_value_of_child(node, name):
    child = node_find_child(node, name)
    if child == None:
        return None
    return node_get_value(child)

def node_set_value(node, value):
    for n in node.childNodes:
        if n.nodeType == n.TEXT_NODE:
            n.replaceWholeText(value)
            return
    raise ValueError("node has no value")

def node_append_child(parent, name, value=None, always_create=True):
    doc = parent.ownerDocument
    node = None

    for token in name.split("/"):
        if not always_create:
            nodes = xpath.Evaluate(token, parent)
            if nodes:
                parent = nodes[0]
                node = parent
                continue

        node = doc.createElement(token)
        if value is not None:
            value_node = doc.createTextNode(unicode(value))
            node.appendChild(value_node)
        parent.appendChild(node)
        parent = node

    return node


class Characteristic:
    def __init__(self, type, parent=None):
        self.type = type
        self.params = {}
        self.parent = None
        self.children = {}

    def __getitem__(self, key):
        return self.params[key]

    def __setitem__(self, key, value):
        self.params[key] = value

    def __str__(self):
        return self.to_string()

    def to_string(self, level=0):
        indent = "%*s" % (4 * level, "")
        str = "%s* %s\n" % (indent, self.type)
        if self.params:
            for name, value in self.params.items():
                str += "%s    %s = %s\n" % (indent, name, value)
        else:
            str += "%s    [no params]\n" % indent

        for child in self.children.values():
            str += "\n"
            str += child.to_string(level + 1)

        return str

    def add_child(self, ctic):
        self.children[ctic.type] = ctic


def characteristic_tree_from_xml(node):
    return characteristic_from_xml(node)

def characteristic_tree_to_xml(ctic, doc):
    return characteristic_to_xml(ctic, doc)

def characteristic_from_xml(xml_node, parent=None):
    ctic = Characteristic(xml_node.getAttribute("type"), parent)
    for node in xml_node.childNodes:
        if node.nodeType == node.ELEMENT_NODE:
            if node.localName == "characteristic":
                ctic.children[node.getAttribute("type")] = characteristic_from_xml(node, ctic)
            elif node.localName == "parm":
                ctic[node.getAttribute("name")] = node.getAttribute("value")
            else:
                logger.warning("characteristic_from_xml: Unhandled node '%s'", node.localName)

    return ctic

def characteristic_to_xml(ctic, doc, parent=None):
    node = doc.createElement("characteristic")
    node.setAttribute("type", ctic.type)

    for child in ctic.children.values():
        characteristic_to_xml(child, doc, node)

    for name, value in ctic.params.items():
        parm = doc.createElement("parm")
        parm.setAttribute("name", name)
        parm.setAttribute("value", value)
        node.appendChild(parm)

    if parent != None:
        parent.appendChild(node)

    return node
