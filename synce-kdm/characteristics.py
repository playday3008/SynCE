# -*- coding: utf-8 -*-
############################################################################
#    Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>         #
#    Dr J A Gow 22/11/07 : Split code into new 'Characteristics' module    #
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
import logging

from exceptions import ValueError

logger = logging.getLogger("engine.xmlutil")

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
			str += "\n" + child.to_string(level + 1)
			
        	return str

	def add_child(self, ctic):
		self.children[ctic.type] = ctic


def characteristic_from_xml(xml_node, parent=None):
	
	ctic = Characteristic(xml2util.GetNodeAttr(xml_node,"type"), parent)
	
	if xml_node.children != None:
		for child in xml_node.children:
        		if child.type == "element":
            			if child.name == "characteristic":
                			ctic.children[xml2util.GetNodeAttr(child,"type")] = characteristic_from_xml(child, ctic)
            			elif child.name == "parm":
                			ctic[xml2util.GetNodeAttr(child,"name")] = xml2util.GetNodeAttr(child,"value")
            			else:
                			logger.warning("characteristic_from_xml: Unhandled node '%s'", child.localName)
	return ctic


def characteristic_to_xml(ctic, doc, parent=None):
	
	node = libxml2.newNode("characteristic")
	node.setProp("type", ctic.type)

	for child in ctic.children.values():
        	characteristic_to_xml(child, doc, node)

	for name, value in ctic.params.items():
        	parm = libxml2.newNode("parm")
        	parm.setProp("name", name)
        	parm.setProp("value", value)
        	node.addChild(parm)

    	if parent != None:
        	parent.addChild(node)

	return node


def characteristic_tree_from_xml(node):
	return characteristic_from_xml(node)

def characteristic_tree_to_xml(ctic, doc):
	return characteristic_to_xml(ctic, doc)

