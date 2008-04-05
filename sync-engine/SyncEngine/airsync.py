# -*- coding: utf-8 -*-
############################################################################
# AIRSYNC.py
#
# Airsync server module for sync-engine
#
# This is a rewritten and updated version of the Airsync module from the
# original sync-engine. This version is no longer reliant on Twisted
# libraries or on libwbxml - sync-engine now has its own wbxml codecs.
#
# Original airsync.py (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
#
############################################################################

import gobject

import libxml2
import xml2util

import socket
import urlparse
import cgi
import httplib
import BaseHTTPServer
import threading
import logging
import util

import wbxml

import formatapi
from constants import *

AIRSYNC_DOC_NAME = "AirSync"
AIRSYNC_PUBLIC_ID = "-//AIRSYNC//DTD AirSync//EN"
AIRSYNC_SYSTEM_ID = "http://www.microsoft.com/"

CHANGE_TYPE_TO_NODE_NAME = {
	
	CHANGE_ADDED    : "Add",
	CHANGE_MODIFIED : "Change",
	CHANGE_DELETED  : "Delete",

}

CHANGE_TYPE_FROM_NODE_NAME = {

	"Add"    : CHANGE_ADDED,
	"Change" : CHANGE_MODIFIED,
	"Delete" : CHANGE_DELETED,

}

#
# class AirsyncHandler
#
# AirsyncHandler is the request handler class containing the specific Airsync
# processing code.
#

class AirsyncHandler(BaseHTTPServer.BaseHTTPRequestHandler):

	sys_version      = "" # Don't let it report that its Python. This is just to match the headers sent by ActiveSync
	server_version   = "ActiveSync/4.1 Microsoft-HTTPAPI/1.0"
	protocol_version = "HTTP/1.1"

	def __init__(self, request, client_address, server):
		
		BaseHTTPServer.BaseHTTPRequestHandler.__init__(self, request, client_address, server)

	#
	# log_message
	#
	# Log all HTTP requests coming from the remote
	#

	def log_message(self, format, *args):
		
		self.server.logger.debug("HTTP Request: " + format, *args)

	#
	# do_QUIT
	#
	# OVERLOADED
	#
	# Standard server method called when server is stopped
	#

	def do_QUIT (self):
		
		self.server.logger.debug("do_QUIT: setting server 'stopped' flag")
		self.send_response(200)
		self.end_headers()
		self.server.stopped = True

	#
	# do_OPTIONS
	#
	# OVERLOADED
	#
	# Standard server method called in response to HTTP OPTIONS command
	#

	def do_OPTIONS(self):

		req_path, req_params = self._ParsePath()

		if req_path !=  "/Microsoft-Server-ActiveSync":
			self.server.logger.warning("do_OPTIONS: Returning 404 for path %s", req_path)
			self._SendEmptyResponse(404)
		else:
			self._SendEmptyResponse(200, (("Allow", "OPTIONS, POST"),
						      ("Public", "OPTIONS, POST"),
						      ("MS-ASProtocolVersions", "2.5"),
						      ("MS-ASProtocolCommands", "Sync,SendMail,SmartForward,SmartReply,GetAttachment,FolderSync,FolderCreate,FolderUpdate,MoveItems,GetItemEstimate,MeetingResponse")))

	#
	# do_POST
	#
	# OVERLOADED
	#
	# Standard server method called in response to HTTP POST request. This is the
	# main entry point for Airsync requests
	#

	def do_POST(self):

		req_path, req_params = self._ParsePath()

		if req_path == "/Microsoft-Server-ActiveSync":

			if req_params.has_key("Cmd") and len(req_params["Cmd"]) > 0:

				cmd = req_params["Cmd"][0]

				self.server.logger.info("do_POST: received %s command", cmd)

				# For AS (not including mail) we support the following
				# three commands only

				if cmd == "Sync":
					self._ProcessSync()
					return
				elif cmd == "FolderSync":
					self._ProcessFolderSync()
					return
				elif cmd == "GetItemEstimate":
					self._ProcessGetItemEstimate()
					return
		
		elif req_path == "/Microsoft-Server-ActiveSync/SyncStat.dll":

			self.server.logger.info("do_POST: received Status command")
			self._ProcessStatus()
			return
		
		self._SendEmptyResponse(500)

	############################################################
	##
	## Internal functions handling the specifics of Airsync/HTTP
	##

	#
	# _InitHeaders
	#
	# INTERNAL
	#
	# Common headers to all responses
	#

	def _InitHeaders(self):
		
		self.send_header("MS-Server-ActiveSync", "4.2.4876.0")
		self.send_header("Pragma", "no-cache")

		# Don't support persistent connections.  If the device expects
		# the connection to be closed, we don't always receivet the required
		# "Connection: close" header in the request.  Therefore, we just
		# tell the device that we're closing the connection after each request
		# it makes.

		self.send_header('Connection', 'close')

	#
	# _SendEmptyResponse
	#
	# INTERNAL
	#
	# Return a response (of specified code) but containing no data block
	#

	def _SendEmptyResponse(self, code, headers = ()):
		
		self.server.logger.debug("_SendEmptyResponse: Emitting response %d code to client", code)
		self.send_response(code)
		self.server.logger.debug("_SendEmptyResponse: Finished emitting response %d code to client", code)
		self._InitHeaders()

		for header, value in headers:
			self.send_header(header, value)

		self.send_header("Content-Type", "text/plain")
		self.send_header("Content-Length", "0")
		self.end_headers()

	#
	# _SendWBXMLResponse
	#
	# INTERNAL
	#
	# Send a standard response containing WBXML data. The XML document should be passed
	# to the function as a libwbxml data structure, not as a string
	#

	def _SendWBXMLResponse(self, xml):

		self.server.logger.debug("_SendWBXMLResponse: Emitting response %d code to client", 200)
		self.send_response(200)
		self.server.logger.debug("_SendWBXMLResponse: Finished emitting response %d code to client", 200)
		self._InitHeaders()

		self.server.logger.debug("_SendWBXMLResponse: starting document conversion")
		
		wbxmldata = wbxml.XMLToWBXML(xml)

		self.send_header("Content-Type", "application/vnd.ms-sync.wbxml")
		self.send_header("Content-Length", len(wbxmldata))
		self.end_headers()

		self.server.logger.debug("_SendWBXMLResponse: Emitting wbxml (length = %d)", len(wbxmldata))
		self.wfile.write(wbxmldata)
		self.server.logger.debug("_SendWBXMLResponse: Finished emitting wbxml")

	#
	# _ParsePath
	#
	# INTERNAL
	#
	# Handle the inbound URL
	#

	def _ParsePath(self):
		p = urlparse.urlparse(self.path)
		return p[2], cgi.parse_qs(p[4], True)

	#
	# _ReadXMLRequest
	#
	# INTERNAL
	#
	# Parse out and translate an inbound XML document, returning the document as a
	# libxml2 data structure. This also handles WBXML content, performing the translation
	# appropriately/
	#

	def _ReadXMLRequest(self):

		if self.headers.has_key("Content-Length"):
			req = self.rfile.read(int(self.headers["Content-Length"])).rstrip("\0")
			if self.headers.has_key("Content-Type") and self.headers["Content-Type"] == "application/vnd.ms-sync.wbxml":
				self.server.logger.debug("_ReadXMLRequest: converting request from wbxml")
				req = wbxml.WBXMLToXML(req)
			else:
				req = libxml2.parseDoc(req)	# not wbxml (status etc)
			return req
		else:
			raise ValueError("Request did not specify Content-Length header")

	#
	# _CreateWBXMLDoc
	#
	# INTERNAL
	#
	# Initialize the libxml2 structures for a document in a manner appropriate
	# for wbxml conversion.
	#

	def _CreateWBXMLDoc(self, root_node_name, namespace):

		doc = libxml2.newDoc("1.0")
		doc.createIntSubset(AIRSYNC_DOC_NAME, AIRSYNC_PUBLIC_ID, AIRSYNC_SYSTEM_ID)
		root=doc.newChild(None,root_node_name,None)
		ns=root.newNs(namespace,None)
		root.setNs(ns)
		return doc

	#
	# _ProcessSync
	#
	# INTERNAL
	#
	# Process Airsync 'Sync' commands
	#

	def _ProcessSync(self):
 
		req_doc = self._ReadXMLRequest()
		self.server.logger.debug("_ProcessSync: request document is \n%s", req_doc.serialize("utf-8",1))

		rsp_doc = self._CreateWBXMLDoc("Sync", "http://synce.org/formats/airsync_wm5/airsync")

		rsp_colls_node = rsp_doc.getRootElement().newChild(None,"Collections",None)

		AvailableItemDBs = self.server.engine.PshipManager.GetCurrentPartnership().deviceitemdbs

		xp = req_doc.xpathNewContext()
		xp.xpathRegisterNs("s","http://synce.org/formats/airsync_wm5/airsync")
		
		for n in xp.xpathEval("/s:Sync/s:Collections/s:Collection"):

			coll_cls = xml2util.GetNodeValue(xml2util.FindChildNode(n,"Class"))
			coll_key = xml2util.GetNodeValue(xml2util.FindChildNode(n,"SyncKey"))
			coll_id  = xml2util.GetNodeValue(xml2util.FindChildNode(n,"CollectionId"))

			first_request = (coll_key == "0")

			coll_key = "%s%d" % (coll_id, int(coll_key.split("}")[-1]) + 1)

			rsp_coll_node = rsp_colls_node.newChild(None,"Collection",None)
			rsp_coll_node.newChild(None,"Class",coll_cls)
			rsp_coll_node.newChild(None,"SyncKey",coll_key)
			rsp_coll_node.newChild(None,"CollectionId",coll_id)
			rsp_coll_node.newChild(None,"Status","1")

			itemDB = AvailableItemDBs[SYNC_ITEM_CLASS_TO_ID[coll_cls]]

			if not first_request and itemDB.GetLocalChangeCount() > 0:

				window_size = int(xml2util.GetNodeValue(xml2util.FindChildNode(n,"WindowSize")))

				changes = itemDB.QueryLocalChanges(window_size)

				if itemDB.GetLocalChangeCount() > 0:
					rsp_coll_node.newChild(None,"MoreAvailable",None)

				rsp_cmd_node = rsp_coll_node.newChild(None,"Commands",None)

				for itemID,change in changes:

					remID, change_type, data = change

					if change_type == CHANGE_ADDED:
						if remID == None:
							remID = util.generate_guid()

					rsp_change_node = rsp_cmd_node.newChild(None,CHANGE_TYPE_TO_NODE_NAME[change_type],None)
					rsp_change_node.newChild(None,"ServerId",remID)

					if change_type != CHANGE_DELETED:

						os_doc = libxml2.parseDoc(data)

						self.server.logger.debug("_ProcessSync: converting item to airsync, source is \n%s", os_doc.serialize("utf-8",1))

						as_doc=formatapi.ConvertFormat(DIR_TO_AIRSYNC,
						                               itemDB.type,
						                               os_doc,
						                               self.server.engine.config.config_Global.cfg["OpensyncXMLFormat"])

						self.server.logger.debug("_ProcessSync: converting item to airsync, result is \n%s", as_doc.serialize("utf-8",1))
					
						rsp_change_node.addChild(as_doc.getRootElement())

					itemDB.AcknowledgeLocalChanges([(itemID,remID)])

			rsp_responses_node = rsp_doc.getRootElement().newChild(None,"Responses",None)

			cmd_count = 0


			xp.setContextNode(n)
			for req_cmd_node in xp.xpathEval("s:Commands/*"):

				cmd_name = req_cmd_node.name
				chg_type = CHANGE_TYPE_FROM_NODE_NAME[cmd_name]

				if chg_type == CHANGE_ADDED:

					rsp_response_node = rsp_responses_node.newChild(None,cmd_name,None)

					cid  = xml2util.GetNodeValue(xml2util.FindChildNode(req_cmd_node,"ClientId"))
					rsp_response_node.newChild(None,"ClientId",cid)

					remID = util.generate_guid()

					rsp_response_node.newChild(None,"ServerId",remID)
					rsp_response_node.newChild(None,"Status","1")

				else:

					remID = xml2util.GetNodeValue(xml2util.FindChildNode(req_cmd_node,"ServerId"))

				xml = u""
				if chg_type in (CHANGE_ADDED, CHANGE_MODIFIED):

					app_node = xml2util.FindChildNode(req_cmd_node,"ApplicationData")

					self.server.logger.debug("_ProcessSync: converting item from airsync, source is \n%s", app_node.serialize("utf-8",1))

					os_doc=formatapi.ConvertFormat(DIR_FROM_AIRSYNC,
					                               itemDB.type,
					                               app_node,
					                               self.server.engine.config.config_Global.cfg["OpensyncXMLFormat"])

					self.server.logger.debug("_ProcessSync: converting item from airsync, result is \n%s", os_doc.serialize("utf-8",1))

					xml=os_doc.serialize("utf-8",0)

				itemDB.AddRemoteChanges([(remID, chg_type, xml)])
				cmd_count += 1

			if rsp_responses_node.children:
				rsp_coll_node.addChild(rsp_responses_node)

		self.server.logger.debug("_ProcessSync: response document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc)

	#
	# _ProcessFolderSync
	#
	# INTERNAL
	#
	# Process the FolderSync command.
	#

	def _ProcessFolderSync(self):

		req_doc = self._ReadXMLRequest()

		self.server.logger.debug("_ProcessFolderSync: request document is\n %s", req_doc.serialize("utf-8",1))

		req_folder_node = xml2util.FindChildNode(req_doc,"FolderSync")

		req_key = xml2util.GetNodeValue(xml2util.FindChildNode(req_folder_node,"SyncKey"))

		if req_key != "0":
			raise ValueError("SyncKey in FolderSync request is not 0")

		rsp_doc = self._CreateWBXMLDoc("FolderSync", "http://synce.org/formats/airsync_wm5/folderhierarchy")

		rsp_folder_node = rsp_doc.getRootElement()

		rsp_folder_node.newChild(None,"Status","1")
		rsp_folder_node.newChild(None,"SyncKey","{00000000-0000-0000-0000-000000000000}1")

		rsp_changes_node = rsp_folder_node.newChild(None,"Changes",None)

		cpship = self.server.engine.PshipManager.GetCurrentPartnership()

		rsp_changes_node.newChild(None,"Count",str(len(cpship.info.folders)))

		for server_id, data in cpship.info.folders.items():

			parent_id, display_name, type = data

			add_node = rsp_changes_node.newChild(None,"Add",None)

			add_node.newChild(None,"ServerId", server_id)
			add_node.newChild(None,"ParentId", str(parent_id))
			add_node.newChild(None,"DisplayName",display_name)
			add_node.newChild(None,"Type",str(type))

		self.server.logger.debug("_ProcessFolderSync: response document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc)

	#
	# _ProcessGetItemEstimate
	#
	# INTERNAL
	#
	# Process the GetItemEstimate Airsync command
	#

	def _ProcessGetItemEstimate(self):

		req_doc = self._ReadXMLRequest()

		self.server.logger.debug("_ProcessGetItemEstimate: request document is \n%s", req_doc.serialize("utf-8",1))

		rsp_doc = self._CreateWBXMLDoc("GetItemEstimate", "http://synce.org/formats/airsync_wm5/getitemestimate")

		AvailableItemDBs = self.server.engine.PshipManager.GetCurrentPartnership().deviceitemdbs

		xp=req_doc.xpathNewContext()
		xp.xpathRegisterNs("e","http://synce.org/formats/airsync_wm5/getitemestimate")

		for n in xp.xpathEval("/e:GetItemEstimate/e:Collections/e:Collection"):

			rsp_node = rsp_doc.getRootElement().newChild(None,"Response",None)

			rsp_node.newChild(None,"Status", "1")

			coll_cls    = xml2util.GetNodeValue(xml2util.FindChildNode(n,"Class"))
			coll_id     = xml2util.GetNodeValue(xml2util.FindChildNode(n,"CollectionId"))
			coll_filter = xml2util.GetNodeValue(xml2util.FindChildNode(n,"FilterType"))
			coll_key    = xml2util.GetNodeValue(xml2util.FindChildNode(n,"SyncKey"))

			coll_node = rsp_node.newChild(None,"Collection",None)
			coll_node.newChild(None,"Class",coll_cls)
			coll_node.newChild(None,"CollectionId",coll_id)

			itemDB = AvailableItemDBs[SYNC_ITEM_CLASS_TO_ID[coll_cls]]
			coll_node.newChild(None,"Estimate",str(itemDB.GetLocalChangeCount()))

		self.server.logger.debug("_ProcessGetItemEstimate: response document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc)

	#
	# _ProcessStatus
	#
	# INTERNAL
	#
	# Process Airsync status requests
	#

	def _ProcessStatus(self):

		req_doc = self._ReadXMLRequest()

		self.server.logger.debug("_ProcessStatus: request document is \n%s", req_doc.serialize("utf-8",1))

		for n in req_doc.getRootElement().children:
			
			# discard any nodes that are not 'elements'
			
			if n.type != "element":
				continue

			# FIXME: It is possible for a device to get into an (invalid) state in which
			# it sends both a SyncBegin and SyncEnd.  For now, we don't deal with this.  It is
			# true that this *shouldn't* happen, but it would screw up our current state enough
			# that we should deal with it.
			# UPDATE: JAG 5/4/2008 - I have not seen this condition over a lot of testing, and
			# not with WM6. Have been unable to trigger this condition at all.

			if n.name in ("SyncBegin", "SyncEnd"):

				self.server.datatype = xml2util.GetNodeAttr(n,"Datatype")
				self.server.partner = xml2util.GetNodeAttr(n,"Partner")

				if self.server.datatype == "" and self.server.partner == "":
			
					if n.name == "SyncBegin":

						# Inform the engine that sync is starting

						self.server.thread.emit("sync-begin")
						
						# Error information gets reset when a sync begins
						
						self.server.error_body = ""
						self.server.error_title = ""
						self.server.error_code = ""

						self.server.engine.StatusSyncStart()

					else:

						# Inform engine that sync has stopped

						self.server.thread.emit("sync-end")

						# Reset all state information except for error info

						self.server.progress_current = 0
						self.server.progress_max = 100
						self.server.status = ""
						self.server.status_type = ""

						self.server.engine.StatusSyncEnd()

				elif self.server.datatype == "":

					# This is to deal with the fact that not all partners have a name
					# One example of this is the exchange partnership

					_my_partner = self.server.partner 
					for p in self.server.engine.PshipManager.GetList():
						if p.info.guid == self.server.partner:
							_my_partner = p.info.name

					if n.name == "SyncBegin":
						self.server.engine.StatusSyncStartPartner(_my_partner)
					else:
						self.server.engine.StatusSyncEndPartner(_my_partner)

				else:

					_my_datatype = self.server.datatype

					if self.server.datatype in SYNC_ITEM_ID_FROM_GUID:
						_my_datatype = SYNC_ITEMS[ SYNC_ITEM_ID_FROM_GUID[ self.server.datatype ]][0]

					_my_partner = self.server.partner 
					for p in self.server.engine.PshipManager.GetList():
						if p.info.guid == self.server.partner:
							_my_partner = p.info.name

					if n.name == "SyncBegin":
						self.server.engine.StatusSyncStartDatatype(_my_partner, _my_datatype)
					else:
						self.server.engine.StatusSyncEndDatatype(_my_partner, _my_datatype)

			elif n.name == "Status" and xml2util.GetNodeAttr(n,"Partner") == self.server.partner:

				for s in n.children:
			
					if s.name == "Progress":

						self.server.progress_current = int(xml2util.GetNodeAttr(s,"value"))
						self.server.engine.StatusSetProgressValue( self.server.progress_current )

					elif s.name == "Total":

						self.server.progress_max = int(xml2util.GetNodeAttr(s,"value"))
						self.server.engine.StatusSetMaxProgressValue( self.server.progress_max )

					elif s.name == "StatusString":

						self.server.status = xml2util.GetNodeAttr(s,"value")
						self.server.status_type = xml2util.GetNodeAttr(s,"type")
						self.server.engine.StatusSetStatusString( self.server.status )

			elif n.name == "Error" and xml2util.GetNodeAttr(n,"Partner") == self.server.partner:

				self.server.error_body = xml2util.GetNodeAttr(n,"Body")
				self.server.error_title = xml2util.GetNodeAttr(n,"Title")
				self.server.error_code = xml2util.GetNodeAttr(n,"Code")

		self._SendEmptyResponse(200)

#
# class AirsyncServer
#
# The core HTTP server class, derived from BaseHTTPServer
#

class AirsyncServer(BaseHTTPServer.HTTPServer):

	#
	# __init__
	#
	# CONSTRUCTOR
	#
	
	def __init__(self, server_address, RequestHandlerClass, thread, engine):

		BaseHTTPServer.HTTPServer.__init__(self, server_address, RequestHandlerClass)

		self.logger = logging.getLogger("engine.airsync.AirsyncServer")

		self.thread = thread
		self.engine = engine

		self.stopped = False

		self.progress_current = 0
		self.progress_max = 100
		self.status = ""
		self.status_type = ""
		self.error_body = ""
		self.error_title = ""
		self.error_code = ""

	#
	# StopServer
	#
	# EXPORTED
	#
	# Shut down the server
	#

	def StopServer(self):
		
		self.logger.debug("StopServer: stopping Airsync server")
		conn = httplib.HTTPConnection("localhost:%d" % self.server_address[1])
		conn.request("QUIT", "/")
		conn.getresponse()

	#
	# StartServer
	#
	# EXPORTED
	#
	# Start the server, keep running until StopServer is called
	#

	def StartServer(self):

		while not self.stopped:
			self.handle_request()
		self.server_close()

#
# class AirsyncThread
#
# The single thread object in which the AirSync server runs
#

class AirsyncThread(gobject.GObject, threading.Thread):

	__gsignals__ = {
	                 "sync-begin": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, () ),
	                 "sync-end"  : (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE, () ),
	               }

	#
	# __init__
	#
	# CONSTRUCTOR
	#

	def __init__(self, engine):
		
		self.__gobject_init__()
		threading.Thread.__init__(self)
		
		self.logger = logging.getLogger("engine.airsync.AirsyncThread")

		self.setDaemon(True)

		self.engine = engine
		self.server = AirsyncServer(("", AIRSYNC_PORT), AirsyncHandler, self, engine)

	#
	# stop
	# 
	# OVERLOADED
	#
	# Called to stop thread
	#

	def stop(self):
		self.logger.debug("stop: stopping Airsync server")
		self.server.StopServer()

	#
	# run
	#
	# OVERLOADED
	#
	# Thread entry point
	#

	def run(self):
		self.logger.debug("run: listening for Airsync requests")
		self.server.StartServer()



