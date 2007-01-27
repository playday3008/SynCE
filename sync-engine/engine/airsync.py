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

import gobject

from xml.dom import minidom
from xml import xpath

import socket
import urlparse
import cgi
import httplib
import BaseHTTPServer
import threading
import logging

import pywbxml

import formats
from xmlutil import *
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

class AirsyncHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    sys_version      = "" # Don't let it report that its Python. This is just to match the headers sent by ActiveSync
    server_version   = "ActiveSync/4.1 Microsoft-HTTPAPI/1.0"
    protocol_version = "HTTP/1.1"

    def __init__(self, request, client_address, server):
        BaseHTTPServer.BaseHTTPRequestHandler.__init__(self, request, client_address, server)

    def log_message(self, format, *args):
        self.server.logger.debug("HTTP Request: " + format, *args)

    def _init_server_headers(self):
        self.send_header("MS-Server-ActiveSync", "4.2.4876.0")
        self.send_header("Pragma", "no-cache")

        # Don't support persistent connections.  If the device expects
        # the connection to be closed, we don't always receivet the required
        # "Connection: close" header in the request.  Therefore, we just
        # tell the device that we're closing the connection after each request
        # it makes.
        self.send_header('Connection', 'close')

    def _send_empty_response(self, code, headers = ()):
        self.server.logger.debug("_send_empty_response: Emitting response %d code to client", code)
        self.send_response(code)
        self.server.logger.debug("_send_empty_response: Finished emitting response %d code to client", code)
        self._init_server_headers()

        for header, value in headers:
            self.send_header(header, value)

        self.send_header("Content-Type", "text/plain")
        self.send_header("Content-Length", "0")
        self.end_headers()

    def _send_wbxml_response(self, xml):
        self.server.logger.debug("_send_wbxml_response: Emitting response %d code to client", 200)
        self.send_response(200)
        self.server.logger.debug("_send_wbxml_response: Finished emitting response %d code to client", 200)
        self._init_server_headers()

        wbxml = pywbxml.xml2wbxml(xml)

        self.send_header("Content-Type", "application/vnd.ms-sync.wbxml")
        self.send_header("Content-Length", len(wbxml))
        self.end_headers()

        self.server.logger.debug("_send_wbxml_response: Emitting wbxml (length = %d)", len(wbxml))
        self.wfile.write(wbxml)
        self.server.logger.debug("_send_wbxml_response: Finished emitting wbxml")

    def _parse_path(self):
        p = urlparse.urlparse(self.path)
        return p[2], cgi.parse_qs(p[4], True)

    def _read_xml_request(self):
        if self.headers.has_key("Content-Length"):
            req = self.rfile.read(int(self.headers["Content-Length"])).rstrip("\0")
            if self.headers.has_key("Content-Type") and self.headers["Content-Type"] == "application/vnd.ms-sync.wbxml":
                self.server.logger.debug("_read_xml_request: converting request from wbxml")
                req = pywbxml.wbxml2xml(req)
            return minidom.parseString(req)
        else:
            raise ValueError("Request did not specify Content-Length header")

    def _create_wbxml_doc(self, root_node_name, namespace):
        dom = minidom.getDOMImplementation()
        doc = dom.createDocument(None, root_node_name, dom.createDocumentType(AIRSYNC_DOC_NAME, AIRSYNC_PUBLIC_ID, AIRSYNC_SYSTEM_ID))
        doc.documentElement.setAttribute("xmlns", namespace)
        return doc

    def do_QUIT (self):
        self.server.logger.debug("do_QUIT: setting server 'stopped' flag")
        self.send_response(200)
        self.end_headers()
        self.server.stopped = True

    def do_OPTIONS(self):
        req_path, req_params = self._parse_path()

        if req_path !=  "/Microsoft-Server-ActiveSync":
            self.server.logger.warning("do_OPTIONS: Returning 404 for path %s", req_path)
            self._send_empty_response(404)
        else:
            self._send_empty_response(200, (("Allow", "OPTIONS, POST"),
                                            ("Public", "OPTIONS, POST"),
                                            ("MS-ASProtocolVersions", "2.5"),
                                            ("MS-ASProtocolCommands", "Sync,SendMail,SmartForward,SmartReply,GetAttachment,FolderSync,FolderCreate,FolderUpdate,MoveItems,GetItemEstimate,MeetingResponse")))

    def do_POST(self):
        req_path, req_params = self._parse_path()

        if req_path == "/Microsoft-Server-ActiveSync":

            if req_params.has_key("Cmd") and len(req_params["Cmd"]) > 0:
                cmd = req_params["Cmd"][0]

                self.server.logger.info("do_POST: received %s command", cmd)

                if cmd == "Sync":
                    self._handle_sync()
                    return
                elif cmd == "FolderSync":
                    self._handle_foldersync()
                    return
                elif cmd == "GetItemEstimate":
                    self._handle_get_item_estimate()
                    return
        elif req_path == "/Microsoft-Server-ActiveSync/SyncStat.dll":
            self.server.logger.info("do_POST: received Status command")
            self._handle_status()
            return

        self._send_empty_response(500)

    def _handle_sync(self):
        req_doc = self._read_xml_request()
        self.server.logger.debug("_handle_sync: request document is \n%s", req_doc.toprettyxml())

        rsp_doc = self._create_wbxml_doc("Sync", "AirSync:")
        rsp_colls_node = node_append_child(rsp_doc.documentElement, "Collections")

        state = self.server.engine.partnerships.get_current().state

        for n in xpath.Evaluate("/Sync/Collections/Collection", req_doc):
            coll_cls = node_get_value(node_get_child(n, "Class"))
            coll_key = node_get_value(node_get_child(n, "SyncKey"))
            coll_id = node_get_value(node_get_child(n, "CollectionId"))

            first_request = (coll_key == "0")

            coll_key = "%s%d" % (coll_id, int(coll_key.split("}")[-1]) + 1)

            rsp_coll_node = node_append_child(rsp_colls_node, "Collection")
            node_append_child(rsp_coll_node, "Class", coll_cls)
            node_append_child(rsp_coll_node, "SyncKey", coll_key)
            node_append_child(rsp_coll_node, "CollectionId", coll_id)
            node_append_child(rsp_coll_node, "Status", 1)

            item = state.items[SYNC_ITEM_CLASS_TO_ID[coll_cls]]
            if not first_request and item.get_local_change_count() > 0:
                window_size = int(node_get_value(node_get_child(n, "WindowSize")))
                changes = item.extract_local_changes(window_size)

                if item.get_local_change_count() > 0:
                    node_append_child(rsp_coll_node, "MoreAvailable")

                rsp_cmd_node = node_append_child(rsp_coll_node, "Commands")

                for guid, change in changes.items():
                    change_type, data = change

                    if change_type == CHANGE_ADDED:
                        luid, guid = state.register_guid(guid)
                    else:
                        luid = state.get_luid_from_guid(guid)

                    rsp_change_node = node_append_child(rsp_cmd_node, CHANGE_TYPE_TO_NODE_NAME[change_type])
                    node_append_child(rsp_change_node, "ServerId", luid)

                    if change_type != CHANGE_DELETED:
                        os_doc = minidom.parseString(data)
                        as_doc = None

                        if item.type == SYNC_ITEM_CONTACTS:
                            as_doc = formats.contact.to_airsync(os_doc)
                        elif item.type == SYNC_ITEM_CALENDAR:
                            as_doc = formats.event.to_airsync(os_doc)
                        else:
                            raise Exception("Can't convert data of item_type %d" % item.type)

                        rsp_change_node.appendChild(as_doc.documentElement)

            rsp_responses_node = rsp_doc.createElement("Responses")

            cmd_count = 0
            for req_cmd_node in xpath.Evaluate("Commands/*", n):
                cmd_name = req_cmd_node.localName
                chg_type = CHANGE_TYPE_FROM_NODE_NAME[cmd_name]

                if chg_type == CHANGE_ADDED:
                    rsp_response_node = node_append_child(rsp_responses_node, cmd_name)

                    cid = node_get_value(node_get_child(req_cmd_node, "ClientId"))
                    node_append_child(rsp_response_node, "ClientId", cid)

                    luid, guid = state.register_luid()

                    node_append_child(rsp_response_node, "ServerId", luid)
                    node_append_child(rsp_response_node, "Status", 1)
                else:
                    luid = node_get_value(node_get_child(req_cmd_node, "ServerId"))
                    guid = state.get_guid_from_luid(luid)

                xml = u""
                if chg_type in (CHANGE_ADDED, CHANGE_MODIFIED):
                    app_node = node_get_child(req_cmd_node, "ApplicationData")

                    if item.type == SYNC_ITEM_CONTACTS:
                        os_doc = formats.contact.from_airsync(guid, app_node)
                    elif item.type == SYNC_ITEM_CALENDAR:
                        os_doc = formats.event.from_airsync(guid, app_node)
                    else:
                        raise Exception("Can't convert data of item_type %d" % item.type)

                    xml = os_doc.documentElement.toxml()

                item.add_remote_change(guid, chg_type, xml)
                cmd_count += 1

            if rsp_responses_node.childNodes:
                rsp_coll_node.appendChild(rsp_responses_node)

        self.server.logger.debug("_handle_sync: response document is \n%s", rsp_doc.toprettyxml())
        self._send_wbxml_response(rsp_doc.toxml())

    def _handle_foldersync(self):
        req_doc = self._read_xml_request()
        self.server.logger.debug("_handle_foldersync: request document is\n %s", req_doc.toprettyxml())

        req_folder_node = node_get_child(req_doc, "FolderSync")
        req_key = node_get_value(node_get_child(req_folder_node, "SyncKey"))
        if req_key != "0":
            raise ValueError("SyncKey in FolderSync request is not 0")

        rsp_doc = self._create_wbxml_doc("FolderSync", "FolderHierarchy:")
        rsp_folder_node = rsp_doc.documentElement

        node_append_child(rsp_folder_node, "Status", 1)
        node_append_child(rsp_folder_node, "SyncKey", "{00000000-0000-0000-0000-000000000000}1")

        rsp_changes_node = node_append_child(rsp_folder_node, "Changes")

        state = self.server.engine.partnerships.get_current().state

        node_append_child(rsp_changes_node, "Count", len(state.folders))

        for server_id, data in state.folders.items():
            parent_id, display_name, type = data

            add_node = node_append_child(rsp_changes_node, "Add")

            node_append_child(add_node, "ServerId", server_id)
            node_append_child(add_node, "ParentId", parent_id)
            node_append_child(add_node, "DisplayName", display_name)
            node_append_child(add_node, "Type", type)

        self.server.logger.debug("handle_foldersync: response document is \n%s", rsp_doc.toprettyxml())
        self._send_wbxml_response(rsp_doc.toxml())

    def _handle_get_item_estimate(self):
        req_doc = self._read_xml_request()
        self.server.logger.debug("_handle_get_item_estimate: request document is \n%s", req_doc.toprettyxml())

        rsp_doc = self._create_wbxml_doc("GetItemEstimate", "GetItemEstimate:")

        state = self.server.engine.partnerships.get_current().state

        for n in xpath.Evaluate("/GetItemEstimate/Collections/Collection", req_doc):
            rsp_node = node_append_child(rsp_doc.documentElement, "Response")
            node_append_child(rsp_node, "Status", 1)

            coll_cls = node_get_value(node_get_child(n, "Class"))
            coll_id = node_get_value(node_get_child(n, "CollectionId"))
            coll_filter = node_get_value(node_get_child(n, "FilterType"))
            coll_key = node_get_value(node_get_child(n, "SyncKey"))

            coll_node = node_append_child(rsp_node, "Collection")
            node_append_child(coll_node, "Class", coll_cls)
            node_append_child(coll_node, "CollectionId", coll_id)

            item = state.items[SYNC_ITEM_CLASS_TO_ID[coll_cls]]
            node_append_child(coll_node, "Estimate", item.get_local_change_count())

        self.server.logger.debug("_handle_get_item_estimate: response document is \n%s", rsp_doc.toprettyxml())
        self._send_wbxml_response(rsp_doc.toxml())

    def _handle_status(self):
        req_doc = self._read_xml_request()
        self.server.logger.debug("_handle_status: request document is \n%s", req_doc.toprettyxml())

        for n in req_doc.documentElement.childNodes:
            if n.nodeType != n.ELEMENT_NODE:
                continue

            # FIXME: It is possible for a device to get into an (invalid) state in which
            # it sends both a SyncBegin and SyncEnd.  For now, we don't deal with this.  It is
            # true that this *shouldn't* happen, but it would screw up our current state enough
            # that we should deal with it.
            if n.localName in ("SyncBegin", "SyncEnd"):
                self.server.datatype = n.getAttribute("Datatype")
                self.server.partner = n.getAttribute("Partner")

                if self.server.datatype == "" and self.server.partner == "":
                    if n.localName == "SyncBegin":
                        self.server.thread.emit("sync-begin")
                        # Error information gets reset when a sync begins
                        self.server.error_body = ""
                        self.server.error_title = ""
                        self.server.error_code = ""
                    else:
                        self.server.thread.emit("sync-end")
                        # Reset all state information except for error info
                        self.server.progress_current = 0
                        self.server.progress_max = 100
                        self.server.status = ""
                        self.server.status_type = ""
            elif n.localName == "Status" and n.getAttribute("Partner") == self.server.partner:
                for s in n.childNodes:
                    if s.localName == "Progress":
                        self.server.progress_current = int(s.getAttribute("value"))
                    elif s.localName == "Total":
                        self.server.progress_max = int(s.getAttribute("value"))
                    elif s.localName == "StatusString":
                        self.server.status = s.getAttribute("value")
                        self.server.status_type = s.getAttribute("type")
            elif n.localName == "Error" and n.getAttribute("Partner") == self.server.partner:
                self.server.error_body = n.getAttribute("Body")
                self.server.error_title = n.getAttribute("Title")
                self.server.error_code = n.getAttribute("Code")

        self._send_empty_response(200)


class AirsyncServer(BaseHTTPServer.HTTPServer):
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

    def stop_server(self):
        self.logger.debug("stop_server: stopping Airsync server")
        conn = httplib.HTTPConnection("localhost:%d" % self.server_address[1])
        conn.request("QUIT", "/")
        conn.getresponse()

    def serve_forever(self):
        while not self.stopped:
            self.handle_request()
        self.server_close()

class AirsyncThread(gobject.GObject, threading.Thread):
    __gsignals__ = {
            "sync-begin": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                           ()),
            "sync-end"  : (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                           ()),
    }

    def __init__(self, engine):
        self.__gobject_init__()
        threading.Thread.__init__(self)
        self.logger = logging.getLogger("engine.airsync.AirsyncThread")

        self.setDaemon(True)

        self.engine = engine
        self.server = AirsyncServer(("", AIRSYNC_PORT), AirsyncHandler, self, engine)

    def stop(self):
        self.logger.debug("stop: stopping Airsync server")
        self.server.stop_server()

    def run(self):
        self.logger.debug("run: listening for Airsync requests")
        self.server.serve_forever()
