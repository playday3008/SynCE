# -*- coding: utf-8 -*-
############################################################################

import gobject

import libxml2
import wbxml
import xml2util
import mailconv

import socket
import urlparse
import cgi
import httplib
import BaseHTTPServer
import email

import threading
import logging

import util
import base64

import backend

from constants import *


AIRSYNC_DOC_NAME = "AirSync"
AIRSYNC_PUBLIC_ID = "-//AIRSYNC//DTD AirSync//EN"
AIRSYNC_SYSTEM_ID = "http://www.microsoft.com/"

CHANGE_TYPE_TO_NODE_NAME = {
    OBJECT_NEW      : "Add",
    OBJECT_CHANGED  : "Change",
    OBJECT_TODEL    : "Delete"
}

CHANGE_TYPE_FROM_NODE_NAME = {
    "Add"    : OBJECT_NEW,
    "Change" : OBJECT_CHANGED,
    "Delete" : OBJECT_TODEL
}


class AirmailHandler(BaseHTTPServer.BaseHTTPRequestHandler, gobject.GObject):

	sys_version      = "" # Don't let it report that its Python. This is just to match the headers sent by ActiveSync
	server_version   = "Microsoft-IIS/6.0"
	protocol_version = "HTTP/1.1"

	def __init__(self, request, client_address, server):
		self.__gobject_init__()
		print("New Backend")
		self.backend = backend.Backend()
		self.backend.InitializeFolderList()
		BaseHTTPServer.BaseHTTPRequestHandler.__init__(self, request, client_address, server)


	def log_message(self, format, *args):
		self.server.logger.debug("HTTP Request: " + format, *args)


	def _InitHeaders(self):

		self.send_header("MS-Server-ActiveSync", "6.5.7651.44")
		self.send_header("Pragma", "no-cache")

		# Don't support persistent connections.  If the device expects
		# the connection to be closed, we don't always receivet the required
		# "Connection: close" header in the request.  Therefore, we just
		# tell the device that we're closing the connection after each request
		# it makes.
		# I think we need persistent connections here (else ping won't work)
		#self.send_header('Connection', 'close')


	def _SendEmptyResponse(self, code, headers = ()):

		self.server.logger.debug("_send_empty_response: Emitting response %d code to client", code)
		self.send_response(code)
		self.server.logger.debug("_send_empty_response: Finished emitting response %d code to client", code)
		self._InitHeaders()

		for header, value in headers:
			self.send_header(header, value)

		self.send_header("Content-Type", "text/plain")
		self.send_header("Content-Length", "0")
		self.end_headers()

	def _SendWBXMLResponse(self, xml):

		self.server.logger.debug("_SendWBXMLResponse: Emitting response %d code to client", 200)
		self.send_response(200)
		self.server.logger.debug("_SendWBXMLResponse: Finished emitting response %d code to client", 200)
		self._InitHeaders()

		wbxml = wbxml.XMLToWBXML(xml)
	
		self.send_header("Content-Type", "application/vnd.ms-sync.wbxml")
		self.send_header("Content-Length", len(wbxml))
		self.end_headers()

		self.server.logger.debug("_SendWBXMLResponse: Emitting wbxml (length = %d)", len(wbxml))
		self.wfile.write(wbxml)
		self.server.logger.debug("_SendWBXMLResponse: Finished emitting wbxml")

	def _SendRawResponse(self, contenttype, data):

		self.server.logger.debug("_SendRawResponse: Emitting response %d code to client", 200)
		self.send_response(200)
		self.server.logger.debug("_SendRawResponse: Finished emitting response %d code to client", 200)
		self._InitHeaders()
	
		self.send_header("Content-Type", contenttype)
		self.send_header("Content-Length", len(data))
		self.end_headers()

		self.server.logger.debug("_SendRawResponse: Emitting data (length = %d)", len(data))
		self.wfile.write(data)
		self.server.logger.debug("_SendRawResponse: Finished emitting data")

	def _ParsePath(self):
		p = urlparse.urlparse(self.path)
		return p[2], cgi.parse_qs(p[4], True)

	def _ReadXMLRequest(self):
		
		if self.headers.has_key("Content-Length"):
			req = self.rfile.read(int(self.headers["Content-Length"])).rstrip("\0")
			if self.headers.has_key("Content-Type") and self.headers["Content-Type"] == "application/vnd.ms-sync.wbxml":
				self.server.logger.debug("_read_xml_request: converting request from wbxml")
				req = wbxml.WBXMLToXML(req)
			return libxml2.parseDoc(req)
		else:
			raise ValueError("Request did not specify Content-Length header")

	def _ReadRawData(self):
		
		if self.headers.has_key("Content-Length"):
			return self.rfile.read(int(self.headers["Content-Length"])).rstrip("\0")
		else:
			raise ValueError("Request did not specify Content-Length header")
		

	def _CreateWBXMLDoc(self, root_node_name, namespace):

		doc = libxml2.newDoc("1.0")
		doc.createIntSubset(AIRSYNC_DOC_NAME, AIRSYNC_PUBLIC_ID, AIRSYNC_SYSTEM_ID)
		root=doc.newChild(None,root_node_name,None)
		root.setProp("xmlns", namespace)
		return doc

	def do_QUIT (self):
		
		self.server.logger.debug("do_QUIT: setting server 'stopped' flag")
		self.send_response(200)
		self.end_headers()
		self.server.stopped = True

	#
	# do_OPTIONS
	#
	# We need to respond to the request for options as we must set the protocol level
	# required for pushmail - AS2.5 as a minimum I believe (correct me if a lower one
	# will work.

	def do_OPTIONS(self):
		
		req_path, req_params = self._ParsePath()

		if req_path !=  "/Microsoft-Server-ActiveSync":
			
			self.server.logger.warning("do_OPTIONS: Returning 404 for path %s", req_path)
			self._SendEmptyResponse(404)
		else:
			self._SendEmptyResponse(200, (("Allow", "OPTIONS, POST"),
                                                      ("Public", "OPTIONS, POST"),
                                                      ("MS-ASProtocolVersions", "2.5"),
                                                      ("MS-ASProtocolCommands", "Sync,SendMail,SmartForward,SmartReply,GetAttachment,FolderSync,FolderCreate,FolderDelete,FolderUpdate,MoveItems,GetItemEstimate,MeetingResponse,Ping")))

	def do_POST(self):

		req_path, req_params = self._ParsePath()

		# authorization:
		
		print self.headers
		if self.headers.has_key("authorization"):
			auth = self.headers['Authorization']
			authtype,authpwd = str(auth).split(" ")
			self.server.logger.info("Authorization type :%s" % authtype)
			self.server.logger.info("Authorization pwd  :%s" % base64.b64decode(authpwd))
		else:
			self.server.logger.info("No authorization")
			
			# grab the content otherwise odd things seem to happen
			
			req = self.rfile.read(int(self.headers["Content-Length"])).rstrip("\0")
			
			# flag unauthorized.
			
			self._SendEmptyResponse(401)
			return

		# before we process

		if req_path == "/Microsoft-Server-ActiveSync":

			if req_params.has_key("Cmd") and len(req_params["Cmd"]) > 0:

				cmd = req_params["Cmd"][0]

				self.server.logger.info("do_POST: received %s command", cmd)

				if cmd == "Sync":
					self._ProcessSyncCommand()
					return
				elif cmd == "GetHierarchy":
					self._ProcessGetHierarchy()
					return
				elif cmd == "CreateCollection":
					self._ProcessCreateCollection(req_params)
					return
				elif cmd == "FolderCreate":
					self._ProcessCreateFolder()
					return
				elif cmd == "FolderDelete":
					self._ProcessDeleteFolder()
					return
				elif cmd == "FolderUpdate":
					self._ProcessUpdateFolder()
					return
				elif cmd == "FolderSync":
					self._ProcessFolderSync()
					return
				elif cmd == "GetItemEstimate":
					self._ProcessGetItemEstimate()
					return
				elif cmd == "SendMail":
					self._ProcessSendMail()
					return
				elif cmd == "SmartReply":
					self._ProcessSmartReply()
					return
				elif cmd == "SmartForward":
					self._ProcessSmartForward(req_params)
					return
				elif cmd == "GetAttachment":
					self._ProcessGetAttachment(req_params)
					return
				elif cmd == "MoveItems":
					self._ProcessMoveItems()
					return
				elif cmd == "Ping":
					self._ProcessKeepalive()
					return
				else:
					self.server.logger.error("do_POST: bad request in cmd(%s)" % cmd)
					self._SendEmptyResponse(400) # is this right (unsupported req)?
					return
		
		elif req_path == "/Microsoft-Server-ActiveSync/SyncStat.dll":
			
			self._ProcessStatus()
			return

		self._SendEmptyResponse(500)

	#
	# ProcessStatus
	#
	# Handle the status request command sequence

	def _ProcessStatus(self):
	    
		req_doc = self._ReadXMLRequest()

		self.server.logger.debug("_handle_status: request document is \n%s", req_doc.serialize("utf-8",1))

		for n in req_doc.getRootElement().children:
			if n.type != "element":
				continue

			# FIXME: It is possible for a device to get into an (invalid) state in which
			# it sends both a SyncBegin and SyncEnd.  For now, we don't deal with this.  It is
			# true that this *shouldn't* happen, but it would screw up our current state enough
			# that we should deal with it.
	    
			if n.name in ("SyncBegin", "SyncEnd"):
		    
				self.server.datatype = xml2util.GetNodeAttr(n,"Datatype")
				self.server.partner = xml2util.GetNodeAttr(n,"Partner")

				if self.server.datatype == "" and self.server.partner == "":

					if n.name == "SyncBegin":
			    
						# Error information gets reset when a sync begins

						self.server.error_body = ""
						self.server.error_title = ""
						self.server.error_code = ""
			
					else:

						# Reset all state information except for error info
					
						self.server.progress_current = 0
						self.server.progress_max = 100
						self.server.status = ""
						self.server.status_type = ""
			
			elif n.name == "Status" and xml2util.GetNodeAttr(n,"Partner") == self.server.partner:

				for s in n.children:
	
					if s.name == "Progress":

						self.server.progress_current = int(xml2util.GetNodeAttr(s,"value"))

					elif s.name == "Total":
					
						self.server.progress_max = int(xml2util.GetNodeAttr(s,"value"))
			
					elif s.name == "StatusString":
			    
						self.server.status = xml2util.GetNodeAttr(s,"value")
						self.server.status_type = xml2util.GetNodeAttr(s,"type")
			
			elif n.name == "Error" and xml2util.GetNodeAttr(n,"Partner") == self.server.partner:

				self.server.error_body = xml2util.GetNodeAttr(n,"Body")
				self.server.error_title = xml2util.GetNodeAttr(n,"Title")
				self.server.error_code = xml2util.GetNodeAttr(n,"Code")

		self._SendEmptyResponse(200)

	#
	# _ProcessGetHierarchy
	#
	# 
	
	def _ProcessGetHierarchy(self):
		
		try:
			req = self._ReadXMLRequest()
			self.server.logger.debug("GetHierarchy: request document is \n%s", req.serialize("utf-8",1))
		except:
			self.server.logger.debug("GetHierarchy: no request")
			self._SendEmptyResponse(500)
			return

		rsp_doc = self._CreateWBXMLDoc("Folders", "http://synce.org/formats/airsync_wm5/folderhierarchy")
		node = rsp_doc.getRootElement()
		
		folderlist = self.backend.QueryFolderList()
		
		for fldrID,parentID,displayname,type in folderlist:
		
			fldr = node.newChild(None,"Folder",None)
			fldr.newChild(None,"ServerId", fldrID)
			fldr.newChild(None,"ParentId", parentID)
			fldr.newChild(None,"DisplayName",displayname)
			fldr.newChild(None,"Type",str(type))
		
		self.server.logger.debug("GetHierarchy: reply document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# _ProcessCreateCollection
	#
	# Called when a new folder is created on the remote and passed in during the first sync
	
	def _ProcessCreateCollection(self,req):
		
		folderName     = req["CollectionName"][0]
		folderParentID = req["ParentId"][0]
		
		self.server.logger.debug("CollectionName is %s" % folderName)
		self.server.logger.debug("ParentId       is %s" % folderParentID)

		newID = self.backend.AddFolder(folderName,folderParentID)

		if newID == "":
			status = 0
			newID = None
		else:
			status = 1

		rsp_doc = self._CreateWBXMLDoc("Response", "http://synce.org/formats/airsync_wm5/folderhierarchy")
		node = rsp_doc.getRootElement()
		node.newChild(None,"Status",str(status))
		fldr = node.newChild(None,"Folder",None)
		fldr.newChild(None,"ServerId", newID)

		self.server.logger.debug("ProcessCreateCollection: reply document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# _ProcessCreateFolder
	#
	# Called when a new folder is created on the remote and passed in during the first sync
	
	def _ProcessCreateFolder(self):
		
		req_doc = self._ReadXMLRequest()

		self.server.logger.debug("ProcessSyncCommand: request document is \n%s", req_doc.serialize("utf-8",1))

		root = req_doc.getRootElement()
		SyncKey = xml2util.GetNodeValue(xml2util.FindChildNode(root,"SyncKey"))
		ParentID = xml2util.GetNodeValue(xml2util.FindChildNode(root,"ParentId"))
		DisplayName = xml2util.GetNodeValue(xml2util.FindChildNode(root,"DisplayName"))
		Type = xml2util.GetNodeValue(xml2util.FindChildNode(root,"Type"))
				
		self.server.logger.debug("New folder %s" % DisplayName)
		self.server.logger.debug("ParentId   %s" % ParentID)

		newID = self.backend.AddFolder(DisplayName,ParentID)

		if newID == "":
			status = 0
			newID = None
		else:
			status = 1

		rsp_doc = self._CreateWBXMLDoc("FolderCreate", "http://synce.org/formats/airsync_wm5/folderhierarchy")
		node = rsp_doc.getRootElement()
		node.newChild(None,"Status",str(status))
		node.newChild(None,"SyncKey",self.backend.sync_key+"1")
		node.newChild(None,"ServerId", newID)

		self.server.logger.debug("ProcessCreateFolder: reply document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# _ProcessDeleteFolder
	#
	# Called when a new folder is deleted on the remote
	#
	
	def _ProcessDeleteFolder(self):
		
		req_doc = self._ReadXMLRequest()

		self.server.logger.debug("ProcessDeleteFolder: request document is \n%s", req_doc.serialize("utf-8",1))

		root = req_doc.getRootElement()

		SyncKey = xml2util.GetNodeValue(xml2util.FindChildNode(root,"SyncKey"))
		ServerID = xml2util.GetNodeValue(xml2util.FindChildNode(root,"ServerId"))
		ParentID = xml2util.GetNodeValue(xml2util.FindChildNode(root,"ParentId"))
		DisplayName = xml2util.GetNodeValue(xml2util.FindChildNode(root,"DisplayName"))
		keyID,keyval = util.GetSyncKeyData(SyncKey)
		
		rsp_doc = self._CreateWBXMLDoc("FolderDelete", "http://synce.org/formats/airsync_wm5/folderhierarchy")
		node = rsp_doc.getRootElement()
		node.newChild(None,"Status","1")
		node.newChild(None,"SyncKey",keyID+(str(keyval+1)))

		self.server.logger.debug("ProcessDeleteFolder: reply document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# _ProcessUpdateFolder
	#
	# Called when a folder on the remote is updated
	#
	
	def _ProcessUpdateFolder(self):
		
		req_doc = self._ReadXMLRequest()
		
		self.server.logger.debug("ProcessUpdateFolder: request document is \n%s", req_doc.serialize("utf-8",1))

		root = req_doc.getRootElement()

		SyncKey = xml2util.GetNodeValue(xml2util.FindChildNode(root,"SyncKey"))
		ServerID = xml2util.GetNodeValue(xml2util.FindChildNode(root,"ServerId"))
		ParentID = xml2util.GetNodeValue(xml2util.FindChildNode(root,"ParentId"))
		DisplayName = xml2util.GetNodeValue(xml2util.FindChildNode(root,"DisplayName"))
		keyID,keyval = util.GetSyncKeyData(SyncKey)
		
		status = self.backend.DeleteFolder(ServerID)
		
		rsp_doc = self._CreateWBXMLDoc("FolderUpdate", "http://synce.org/formats/airsync_wm5/folderhierarchy")
		node = rsp_doc.getRootElement()
		node.newChild(None,"Status",str(status))
		node.newChild(None,"SyncKey",keyID+(str(keyval+1)))

		self.server.logger.debug("ProcessCreateFolder: reply document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# _ProcessSyncCommand
	#
	# Handle the process of synchronization

	def _ProcessSyncCommand(self):
 
		req_doc = self._ReadXMLRequest()

		self.server.logger.debug("ProcessSyncCommand: request document is \n%s", req_doc.serialize("utf-8",1))

		rsp_doc = self._CreateWBXMLDoc("Sync", "http://synce.org/formats/airsync_wm5/airsync")
	
		rsp_colls_node = rsp_doc.getRootElement().newChild(None,"Collections",None)

		xp = req_doc.xpathNewContext()
		xp.xpathRegisterNs("s","http://synce.org/formats/airsync_wm5/airsync")
	
		for n in xp.xpathEval("/s:Sync/s:Collections/s:Collection"):
		
			collclass  = xml2util.GetNodeValue(xml2util.FindChildNode(n,"Class"))
			keystr     = xml2util.GetNodeValue(xml2util.FindChildNode(n,"SyncKey"))
			folderID   = xml2util.GetNodeValue(xml2util.FindChildNode(n,"CollectionId"))
			delasmove  = xml2util.GetNodeValue(xml2util.FindChildNode(n,"DeletesAsMoves"))
			getchanges = xml2util.GetNodeValue(xml2util.FindChildNode(n,"GetChanges"))
			
			# get the sync key ID and value from the sync key node
			
			keyID,synckey = util.GetSyncKeyData(keystr)
			
			newKey = util.GetIncrementedSyncKey(folderID,keystr)	
			
			# capture the options for later.
			
			opt_node = xml2util.FindChildNode(n,"Options")
			options = mailconv.Options(opt_node)

			# On the very first sync, the SyncKey is "0". On subsequent collections, it
			# needs to be updated from the folder ID. Basically, if we are in a first
			# request, it means 'send everything down the wire'. This is handled 
			# slightly differently in an OpenSync/sync-engine situation.

			first_request = (synckey==0)

			# build the response block to _our_ changes

			rsp_coll_node = rsp_colls_node.newChild(None,"Collection",None)

			rsp_coll_node.newChild(None,"Class",collclass)
			rsp_coll_node.newChild(None,"SyncKey",newKey)
			rsp_coll_node.newChild(None,"CollectionId",folderID)
			rsp_coll_node.newChild(None,"Status","1")
 
 			wsnode = xml2util.FindChildNode(n,"WindowSize")
			if wsnode != None:
				window_size = int(xml2util.GetNodeValue(xml2util.FindChildNode(n,"WindowSize")))
			else:
				window_size=1000

			changes = self.backend.QueryChanges(folderID,window_size)
			changecount = len(changes)

			converter = mailconv.MailConvert(folderID)

			if changecount > 0:
				
				if changecount == window_size:
					rsp_coll_node.newChild(None,"MoreAvailable",None)

				rsp_cmd_node = rsp_coll_node.newChild(None,"Commands",None)

				for itemID,change,message in changes:
			
					rsp_change_node = rsp_cmd_node.newChild(None,CHANGE_TYPE_TO_NODE_NAME[change],None)
					combinedID = util.GenerateCombinedID(folderID,itemID)
					rsp_change_node.newChild(None,"ServerId",combinedID)
		    
					if change!=OBJECT_TODEL:
		
						xmlmail = converter.MailToApplicationNode(itemID,message)

						self.server.logger.debug("document data is \n%s", xmlmail.serialize("utf-8",1))
						
						rsp_change_node.addChild(xmlmail)

				self.backend.AcknowledgeChanges(folderID,changes)
					
					
			rsp_responses_node = rsp_doc.getRootElement().newChild(None,"Responses",None)
			cmd_count = 0
			xp.setContextNode(n)
	    
			for req_cmd_node in xp.xpathEval("s:Commands/*"):

				cmd_name = req_cmd_node.name
				objtype = CHANGE_TYPE_FROM_NODE_NAME[cmd_name]
		
				if cmd_name == "Add":
					
					# Do we need to support this for mail? We do for other class types
					
					rsp_response_node = rsp_responses_node.newChild(None,"Add",None)

					cid  = xml2util.GetNodeValue(xml2util.FindChildNode(req_cmd_node,"ClientId"))
					rsp_response_node.newChild(None,"ClientId",cid)
					remID = util.generate_guid()

					rsp_response_node.newChild(None,"ServerId",remID)
					rsp_response_node.newChild(None,"Status","1")
					
				elif cmd_name == "Fetch":
					
					# we need to fetch an item
					
					rsp_response_node = rsp_responses_node.newChild(None,"Fetch",None)
					
					remID = xml2util.GetNodeValue(xml2util.FindChildNode(req_cmd_node,"ServerId"))
					
					status = 0
					item = self.backend.FetchItem(util.MessageIDFromCombinedID(remID))
					if item:
						itemID,change,message = item
						status = 1
						
						xmlmail = converter.MailToApplicationNode(message)

						self.server.logger.debug("Fetch: document data is \n%s", xmlmail.serialize("utf-8",1))
						
						rsp_response_node.newChild(None,"Status","1")
						rsp_response_node.addChild(xmlmail)
						
				elif cmd_name == "Delete":
	
					remID = xml2util.GetNodeValue(xml2util.FindChildNode(req_cmd_node,"ServerId"))

					if self.backend.DeleteItem(folderID,util.MessageIDFromCombinedID(remID)) == False:
						self.server.logger.debug("Delete: deleting non-existent message ID %s" % remID)
					else:
						self.server.logger.info("Delete: deleted message ID %s" % remID)
						
				else:
					# TODO - We need to support 'Change' as this sets the 'Read' flag
					self.server.logger.info("Unsupported command %s" % cmd_name)


				cmd_count += 1

			if rsp_responses_node.children:
				rsp_coll_node.addChild(rsp_responses_node)

		self.server.logger.debug("_handle_sync: response document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# ProcessFolderSync
	#
	# Handle synchronization of folders
	#

	def _ProcessFolderSync(self):

		req_doc = self._ReadXMLRequest()
	
		self.server.logger.debug("_handle_foldersync: request document is\n %s", req_doc.serialize("utf-8",1))

		req_folder_node = xml2util.FindChildNode(req_doc,"FolderSync")
	
		keydata = xml2util.GetNodeValue(xml2util.FindChildNode(req_folder_node,"SyncKey"))
		keyID,keyval = util.GetSyncKeyData(keydata)
		
		initialsync = (keyval==0)
		
		if keyval == 0:
			self.server.logger.info("_ProcessFolderSync :  initial sync.")
			
		if keyID == "":
			keyID = self.backend.sync_key
			
		newKey = keyID + str(keyval+1)

		changes = self.backend.QueryFolderChanges(initialsync)

		rsp_doc = self._CreateWBXMLDoc("FolderSync", "http://synce.org/formats/airsync_wm5/folderhierarchy")
	
		rsp_folder_node = rsp_doc.getRootElement()
		
		rsp_folder_node.newChild(None,"Status","1")
		rsp_folder_node.newChild(None,"SyncKey",newKey)

		rsp_changes_node = rsp_folder_node.newChild(None,"Changes",None)
		 
		rsp_changes_node.newChild(None,"Count",str(len(changes)))
	
		for chg in changes:
		
			folderID,parentID,change,displayname,foldertype = chg
			
			if change==OBJECT_NEW:

				add_node = rsp_changes_node.newChild(None,"Add",None)

				add_node.newChild(None,"ServerId", folderID)
				add_node.newChild(None,"ParentId", parentID)
				add_node.newChild(None,"DisplayName",displayname)
				add_node.newChild(None,"Type",str(foldertype))
				
			elif change==OBJECT_TODEL:
				
				add_node = rsp_changes_node.newChild(None,"Delete",None)
				add_node.newChild(None,"ServerId", folderID)
			
			elif change==OBJECT_CHANGED:
				
				add_node = rsp_changes_node.newChild(None,"Update",None)

				add_node.newChild(None,"ServerId", folderID)
				add_node.newChild(None,"ParentId", parentID)
				add_node.newChild(None,"DisplayName",displayname)
				add_node.newChild(None,"Type",str(foldertype))
			else:
				self.server.logger.debug("ProcessFolderSync: unsupported change type %s" % str(change))
				
		self.backend.AcknowledgeFolderChanges(changes)

        	self.server.logger.debug("handle_foldersync: response document is \n%s", rsp_doc.serialize("utf-8",1))
        
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# ProcessGetItemEstimate
	#
	# ItemEstimate requests are processed here
	#

	def _ProcessGetItemEstimate(self):
	    
		req_doc = self._ReadXMLRequest()
        
		self.server.logger.debug("_handle_get_item_estimate: request document is \n%s", req_doc.serialize("utf-8",1))

		rsp_doc = self._CreateWBXMLDoc("GetItemEstimate", "http://synce.org/formats/airsync_wm5/getitemestimate")

		xp=req_doc.xpathNewContext()
		xp.xpathRegisterNs("e","http://synce.org/formats/airsync_wm5/getitemestimate")
	
		for n in xp.xpathEval("/e:GetItemEstimate/e:Collections/e:Collection"):

			rsp_node = rsp_doc.getRootElement().newChild(None,"Response",None)

			rsp_node.newChild(None,"Status", "1")

			collclass = xml2util.GetNodeValue(xml2util.FindChildNode(n,"Class"))
			folderID  = xml2util.GetNodeValue(xml2util.FindChildNode(n,"CollectionId"))
			filter    = xml2util.GetNodeValue(xml2util.FindChildNode(n,"FilterType"))
			keystr    = xml2util.GetNodeValue(xml2util.FindChildNode(n,"SyncKey"))

			keyID,synckey = util.GetSyncKeyData(keystr)
			
			firstrequest = (synckey==0)

			coll_node = rsp_node.newChild(None,"Collection",None)
			coll_node.newChild(None,"Class",collclass)
			coll_node.newChild(None,"CollectionId",folderID)

			# get change count here. TODO: Support filter-type and sync-key
			
			chcount = self.backend.QueryChangeCount(folderID,firstrequest)

			coll_node.newChild(None,"Estimate",str(chcount))

		self.server.logger.debug("_handle_get_item_estimate: response document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# ProcessSendMail
	#
	# At the moment, just dump the mail to the screen.
	
	def _ProcessSendMail(self):
		
		req_doc = self._ReadRawData()
		self.server.logger.info("ProcessSendMail: request is \n%s",req_doc)
		
		# First unpack the message, and identify us in the mailer path
		
		message = email.message_from_string(req_doc)
		message.add_header("X-Mailer", "AirMail - open mobile mail solution")

		# send it to the backend

		try:
			self.backend.IncomingMail(message)
			self._SendEmptyResponse(200)
		except:
			self._SendEmptyResponse(500)

	#
	# ProcessMoveItems
	#
	# Move an item, usually between folders
	
	def _ProcessMoveItems(self):
		
		req_doc = self._ReadXMLRequest()
		self.server.logger.info("ProcessMoveItems: request is \n%s",req_doc.serialize("utf-8",1))

		rsp_doc = self._CreateWBXMLDoc("Moves", "http://synce.org/formats/airsync_wm5/move")

		xp=req_doc.xpathNewContext()
		xp.xpathRegisterNs("m","http://synce.org/formats/airsync_wm5/move")
	
		for n in xp.xpathEval("/m:Moves/m:Move"):

			respnode = rsp_doc.getRootElement().newChild(None,"Response",None)
			
			src_msgID = xml2util.GetNodeValue(xml2util.FindChildNode(n,"SrcMsgId"))
			src_fldID = xml2util.GetNodeValue(xml2util.FindChildNode(n,"SrcFldId"))
			dst_fldID = xml2util.GetNodeValue(xml2util.FindChildNode(n,"DstFldId"))
			
			src_msgfldr, src_msg = util.SplitCombinedID(src_msgID)
			
			self.server.logger.info("ProcessMoveItems: src_msgfldr is %s" % src_msgfldr)
			self.server.logger.info("ProcessMoveItems: src_fldID is %s" % src_fldID)
			
			respnode.newChild(None,"SrcMsgId",src_msgID)
		
			status = 0
			if src_msgfldr == src_fldID:
				rc = self.backend.MoveItem(src_fldID,dst_fldID,src_msg)
				if rc:
					status=3
				else:
					status=2
			else:
				status=0
				
			respnode.newChild(None,"Status",str(status))
			respnode.newChild(None,"DstMsgId",util.GenerateCombinedID(dst_fldID,src_msg))
			
		self.server.logger.debug("_ProcessMoveItems: response document is \n%s", rsp_doc.serialize("utf-8",1))
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))

	#
	# ProcessSmartReply
	#
	# To be implemented
	
	def _ProcessSmartReply(self):
		req_doc = self._ReadRawData()
		self.server.logger.info("ProcessSmartReply: request is \n%s",req_doc)

		self._SendEmptyResponse(200)
		
	#
	# ProcessSmartForward
	#
	# SmartForward instructs the server to forward an existing mail item:
	# Commands are in header:
	#   ItemId=<full item ID of mail to forward>
	#   CollectionId = folder ID of mail to forward
	#   Message data consists of a valid email message.
	
	def _ProcessSmartForward(self,cmds):
		
		req_doc = self._ReadRawData()
		self.server.logger.info("ProcessSmartForward: request is \n%s",req_doc)
		itemID = cmds["ItemId"]
		folderID = cmds["CollectionId"]
		
		folderID_s, messageID = util.SplitCombinedID(itemID)
		 
		if folderID == folderID_s:
			msg = self.backend.FetchItem(folderID,messageID)
			if msg != None:
				itemID,changes,message = msg
				newmsg = email.message_from_string(req_doc)
				newmsg.attach(message)
				try:
					self.server.logger.info("sending forwarded message")
					self.backend.IncomingMail(message)
					self._SendEmptyResponse(200)
				except:
					self.server.logger.info("unable to send forwarded message")
					self._SendEmptyResponse(500)
			else:
				self.server.logger.info("could not find message")
				self._SendEmptyResponse(500)
		else:
			self.server.logger.info("Folder ID error")
			self._SendEmptyResponse(500)

	#
	# ProcessGetAttachment
	#
	# 
	
	def _ProcessGetAttachment(self,arglist):
		
		if arglist.has_key("AttachmentName"):
			
			attname = arglist["AttachmentName"][0]
			
			# grab attachment here - then just send it down the wire as is.
			
			self.server.logger.info("ProcessGetAttachment: requested attachment %s" % attname)
			
			folderID,itemID,filename,index = mailconv.ParseAttachmentName(attname)
		
			index=int(index)
			self.server.logger.info("ProcessGetAttachment: folderID %s" % folderID)
			self.server.logger.info("ProcessGetAttachment: itemID   %s" % itemID)
			self.server.logger.info("ProcessGetAttachment: filename %s" % filename)
			self.server.logger.info("ProcessGetAttachment: index    %s" % index)
			
			
			contenttype, attdata = self.backend.GetAttachment(folderID,itemID,filename,index)
			
			# send it back
			
			if attdata != None:
				self._SendRawResponse(contenttype,attdata)	# for now only
			else:
				self._SendEmptyResponse(400)
			
		else:
			self._SendEmptyResponse(400)
			
			
	#
	# ProcessKeepalive
	#
	# The keepalive seems to be a request for a delayed response, returning sooner if
	# there is data available to exchange.
	
	def _ProcessKeepalive(self):
		
		req_doc = self._ReadXMLRequest()
		self.server.logger.info("ProcessKeepAlive: request is \n%s",req_doc.serialize("utf-8",1))
		
		req_node = req_doc.getRootElement()
		
		interval = xml2util.GetNodeValue(xml2util.FindChildNode(req_node,"HeartbeatInterval"))
		
		self.server.logger.info("ProcessKeepAlive: requested interval -%s-" % interval)
		
		# we now know how long we have!
		
		if interval != "":
			ivalue = int(interval)
		else:
			ivalue = 60  # get this from config later?
			
		self.server.logger.info("ProcessKeepAlive: actual interval -%d-" % ivalue)
		
		gobject.timeout_add(1000*ivalue,self._ProcessEndKeepalive)
		
		return
		
	#
	# ProcessEndKeepalive
	#
	# Return the response at the end of the keepalive. We can return sooner than this if
	# messages have arrived that need to be sent back.
	
	def _ProcessEndKeepalive(self):
		
		# if we have changes, status = 2 otherwise 1
		status = 1
		folderIDs = []
		
		rsp_doc = self._CreateWBXMLDoc("Ping", "http://synce.org/formats/airsync_wm5/ping")
		rsp_node = rsp_doc.getRootElement()
		statusnode = rsp_node.newChild(None,"PingStatus",str(status))
		
		for folderID in folderIDs:
			pf=rsp_node.newChild(None,"PingFolders",None)
			pf.newChild(None,"PingFolderID",str(folderID))
			
		self.server.logger.info("ProcessEndKeepalive: response \n%s" % rsp_doc.serialize("utf-8",1))
			
		self._SendWBXMLResponse(rsp_doc.serialize("utf-8",0))
		
		
#
#
# AirmailServer is the main server class
#


class AirmailServer(BaseHTTPServer.HTTPServer):

	def __init__(self, server_address, RequestHandlerClass, thread):

		BaseHTTPServer.HTTPServer.__init__(self, server_address, RequestHandlerClass)
		self.logger = logging.getLogger("engine.airsync.AirsyncServer")

		self.thread = thread

		self.stopped = False

		self.progress_current = 0
		self.progress_max = 100
		self.status = ""
		self.status_type = ""
		self.error_body = ""
		self.error_title = ""
		self.error_code = ""

	def stop_server(self):

		self.logger.debug("stop_server: stopping AirMail server")
		conn = httplib.HTTPConnection("localhost:%d" % self.server_address[1])
		conn.request("QUIT", "/")
		conn.getresponse()

	def serve_forever(self):

		while not self.stopped:
			self.handle_request()
		self.server_close()

#
# Thread running server
#


class AirmailThread(gobject.GObject, threading.Thread):

	def __init__(self):
 
		threading.Thread.__init__(self)

		self.__gobject_init__()
		self.logger = logging.getLogger("AirmailThread")

		self.setDaemon(True)

		self.server = AirmailServer(("", 80), AirmailHandler, self)

	def stop(self):
		self.logger.debug("stopping AirMail server")
		self.server.stop_server()

	def run(self):
		self.logger.debug("listening for requests")
		self.server.serve_forever()
