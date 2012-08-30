# -*- coding: utf-8 -*-
###############################################################################
# RAPICONTEXT.py
#
# Converted from the original rapiutil.py to provide an object with methods
# for each RAPI/ProcessConfig request
#
# Original rapiutil: (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
# Dr J A Gow 28/11/2007
#
###############################################################################

import libxml2
import xml2util
import characteristics
import logging
import pyrapi2

logger = logging.getLogger("engine.rapicontext")

QUERY_TYPE_GET      = 0
QUERY_TYPE_SET      = 1
QUERY_TYPE_REMOVE   = 2

class RapiContext(pyrapi2.RAPISession):

	def __init__(self,device,loglevel):
		pyrapi2.RAPISession.__init__(self,device,loglevel)
		
	def GetConfig(self,path,leaf,recursive=False):
		return self._ConfigQuery(QUERY_TYPE_GET, path, leaf, recursive)
	
	def SetConfig(self,path,ctic):
		return self._ConfigQuery(QUERY_TYPE_SET, path, None, ctic=ctic)

	def RemoveConfig(self,path,leaf):
		return self._ConfigQuery(QUERY_TYPE_REMOVE, path, leaf)

	def _ConfigQuery(self, query_type, path, leaf=None, recursive=False, ctic=None):
	
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

		reply = self.process_config(doc_node.serialize("utf-8",0), 1)
		reply_doc = libxml2.parseDoc(reply)

		logger.debug("_config_query: CeProcessConfig response is \n%s", reply_doc.serialize("utf-8",1))

		reply_node = xml2util.GetNodeOnLevel(reply_doc, 2 + len(tokens))

		if query_type != QUERY_TYPE_SET:
        		return characteristics.characteristic_tree_from_xml(reply_node)
		else:
        		return None
