# -*- coding: utf-8 -*-
#
# Copyright (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#

import gobject
from twisted.internet import defer
from twisted.web2 import http, resource, stream
from util import *
from cStringIO import StringIO
from xml.dom import minidom
import pywbxml

from contacts import contact_to_vcard

AIRSYNC_DOC_NAME = "AirSync"
AIRSYNC_PUBLIC_ID = "-//AIRSYNC//DTD AirSync//EN"
AIRSYNC_SYSTEM_ID = "http://www.microsoft.com/"

class ASResource(gobject.GObject, resource.PostableResource):
    __gsignals__ = {
            "sync-begin": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                           ()),
            "sync-end": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                         ()),

            "contact-added": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                              (object, object)),
            "contact-modified": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                                 (object, object)),
            "contact-deleted": (gobject.SIGNAL_RUN_FIRST, gobject.TYPE_NONE,
                                (object,)),
    }

    def __init__(self):
        self.__gobject_init__()
        resource.PostableResource.__init__(self)

        self.pship = None
        self.dom = minidom.getDOMImplementation()

    def set_partnership(self, pship):
        self.pship = pship

    def locateChild(self, request, segments):
        found = True

        if len(segments) < 1:
            found = False
        else:
            if segments[0] != "Microsoft-Server-ActiveSync":
                found = False

        if found:
            return (self, ())
        else:
            return (None, ())

    def render(self, request):
        return self.create_response(500)

    def create_response(self, code):
        resp = http.Response(code)
        headers = resp.headers

        headers.addRawHeader("Server", "ActiveSync/4.1")
        headers.addRawHeader("MS-Server-ActiveSync", "4.1.4841.0")

        return resp

    def create_wbxml_response(self, xml):
        resp = self.create_response(200)

        wbxml = pywbxml.xml2wbxml(xml)

        headers = resp.headers
        headers.addRawHeader("ContentType", "application/vnd.ms-sync.wbxml")

        resp.stream = stream.MemoryStream(wbxml)

        return resp

    def create_wbxml_doc(self, root_node_name):
        doc_type = self.dom.createDocumentType(
                AIRSYNC_DOC_NAME, AIRSYNC_PUBLIC_ID, AIRSYNC_SYSTEM_ID)
        return self.dom.createDocument(None, root_node_name, doc_type)

    def http_OPTIONS(self, request):
        if request.path != "/Microsoft-Server-ActiveSync":
            print "http_OPTIONS: Returning 404 for path %s" % request.path
            return http.Response(404)

        resp = self.create_response(200)
        headers = resp.headers
        headers.addRawHeader("Allow", "OPTIONS, POST")
        headers.addRawHeader("Public", "OPTIONS, POST")
        headers.addRawHeader("MS-ASProtocolVersions", "2.5")
        headers.addRawHeader("MS-ASProtocolCommands",
                "Sync,SendMail,SmartForward,SmartReply,GetAttachment,FolderSync,FolderCreate,FolderUpdate,MoveItems,GetItemEstimate,MeetingResponse")

        return resp

    def http_POST(self, request):
        buf = StringIO()

        reply_deferred = defer.Deferred()

        d = request.stream.read()
        d.addCallback(self._read_chunk, request, buf, reply_deferred)

        return reply_deferred

    def _read_chunk(self, chunk, request, buf, reply_deferred):
        if chunk is not None:
            buf.write(chunk)

            d = request.stream.read()
            d.addCallback(self._read_chunk, request, buf, reply_deferred)
        else:
            body = buf.getvalue()

            if request.path == "/Microsoft-Server-ActiveSync":
                cmd = request.args["Cmd"][0]

                print "Cmd=\"%s\"" % cmd

                if cmd == "Sync":
                    response = self.handle_sync(request, body)
                elif cmd == "FolderSync":
                    response = self.handle_foldersync(request, body)
                elif cmd == "GetItemEstimate":
                    response = self.handle_get_item_estimate(request, body)
                else:
                    response = self.create_response(500)
            elif request.path == "/Microsoft-Server-ActiveSync/SyncStat.dll":
                response = self.handle_status(request, body)
            else:
                response = self.create_response(500)

            reply_deferred.callback(response)

    def handle_sync(self, request, body):
        print "Parsing Sync request"

        xml_raw = pywbxml.wbxml2xml(body)
        req_doc = minidom.parseString(xml_raw)
        print "Which is:", req_doc.toprettyxml()
        req_sync_node = node_get_child(req_doc, "Sync")
        req_colls_node = node_get_child(req_sync_node, "Collections")

        doc = self.create_wbxml_doc("Sync")
        colls_node = node_append_child(doc.documentElement, "Collections")

        for n in req_colls_node.childNodes:
            if n.nodeType == n.ELEMENT_NODE and n.localName == "Collection":
                cls = node_get_value(node_get_child(n, "Class"))
                key = node_get_value(node_get_child(n, "SyncKey"))
                id = node_get_value(node_get_child(n, "CollectionId"))

                coll_node = node_append_child(colls_node, "Collection")
                node_append_child(coll_node, "Class", cls)
                key = "%s%d" % (id, int(key.split("}")[-1]) + 1)
                node_append_child(coll_node, "SyncKey", key)
                node_append_child(coll_node, "CollectionId", id)
                node_append_child(coll_node, "Status", 1)

                responses_node = doc.createElement("Responses")

                for sub_node in n.childNodes:
                    if sub_node.nodeType != sub_node.ELEMENT_NODE:
                        continue

                    if sub_node.localName == "Commands":
                        for req_cmd_node in sub_node.childNodes:
                            if req_cmd_node.nodeType != req_cmd_node.ELEMENT_NODE:
                                continue

                            name = "handle_sync_%s_cmd_%s" % \
                                    (cls.lower(), req_cmd_node.localName.lower())
                            if hasattr(self, name):
                                getattr(self, name)(req_cmd_node, responses_node)
                            else:
                                print "Unhandled command \"%s\" (looking for %s)" % \
                                    (req_cmd_node.localName, name)
                                print "Command node:", req_cmd_node.toprettyxml()
                    elif sub_node.localName in ("Class", "SyncKey", "CollectionId"):
                        pass
                    else:
                        print "Ignoring collection subnode \"%s\"" % \
                            sub_node.localName

                if len(responses_node.childNodes) > 0:
                    coll_node.appendChild(responses_node)

        print "Responding to Sync"
        print "With:", doc.toprettyxml()
        return self.create_wbxml_response(doc.toxml())

    def _contacts_node_to_vcard(self, sid, app_node):
        contact = {}
        for node in app_node.childNodes:
            if node.nodeType != node.ELEMENT_NODE:
                continue

            val = node_get_value(node)
            if val is None:
                continue

            contact[node.localName] = val

        return contact_to_vcard(sid, contact)

    def handle_sync_contacts_cmd_add(self, request_node, responses_node):
        response_node = node_append_child(responses_node, request_node.localName)

        cid = node_get_child(request_node, "ClientId")
        node_append_child(response_node, "ClientId", cid)

        sid = generate_guid()
        node_append_child(response_node, "ServerId", sid)

        node_append_child(response_node, "Status", 1)

        app_node = node_get_child(request_node, "ApplicationData")
        vcard = self._contacts_node_to_vcard(sid, app_node)
        self.emit("contact-added", sid, vcard)

    def handle_sync_contacts_cmd_change(self, request_node, responses_node):
        sid = node_get_child(request_node, "ServerId")

        app_node = node_get_child(request_node, "ApplicationData")
        vcard = self._contacts_node_to_vcard(sid, app_node)
        self.emit("contact-modified", sid, vcard)

    def handle_sync_contacts_cmd_delete(self, request_node, responses_node):
        sid = node_get_child(request_node, "ServerId")

        self.emit("contact-deleted", sid)

    def handle_foldersync(self, request, body):
        print "Parsing FolderSync request"

        xml_raw = pywbxml.wbxml2xml(body)
        doc = minidom.parseString(xml_raw)

        folder_node = node_get_child(doc, "FolderSync")
        key_node = node_get_child(folder_node, "SyncKey")
        key = node_get_value(key_node)
        if key != "0":
            raise ValueError("SyncKey specified is not 0")

        doc = self.create_wbxml_doc("FolderSync")
        folder_node = doc.documentElement

        node_append_child(folder_node, "Status", 1)
        node_append_child(folder_node, "SyncKey",
                "{00000000-0000-0000-0000-000000000000}1")

        changes_node = node_append_child(folder_node, "Changes")

        state = self.pship.state

        node_append_child(changes_node, "Count", len(state.folders))

        for server_id, data in state.folders.items():
            parent_id, display_name, type = data

            add_node = node_append_child(changes_node, "Add")

            node_append_child(add_node, "ServerId", server_id)
            node_append_child(add_node, "ParentId", parent_id)
            node_append_child(add_node, "DisplayName", display_name)
            node_append_child(add_node, "Type", type)

        print "Responding to FolderSync"

        return self.create_wbxml_response(doc.toxml())

    def handle_get_item_estimate(self, request, body):
        print "Parsing GetItemEstimate request"

        xml_raw = pywbxml.wbxml2xml(body)
        doc = minidom.parseString(xml_raw)

        reply_doc = self.create_wbxml_doc("GetItemEstimate")
        resp_node = node_append_child(reply_doc.documentElement, "Response")
        node_append_child(resp_node, "Status", 1)

        node = node_get_child(doc, "GetItemEstimate")
        colls_node = node_get_child(node, "Collections")
        for n in colls_node.childNodes:
            if n.nodeType == n.ELEMENT_NODE and n.localName == "Collection":
                cls = node_get_value(node_get_child(n, "Class"))
                id = node_get_value(node_get_child(n, "CollectionId"))
                filter = node_get_value(node_get_child(n, "FilterType"))
                key = node_get_value(node_get_child(n, "SyncKey"))

                coll_node = node_append_child(resp_node, "Collection")
                node_append_child(coll_node, "Class", cls)
                node_append_child(coll_node, "CollectionId", id)
                node_append_child(coll_node, "Estimate", 0)

        print "Responding to GetItemEstimate"

        return self.create_wbxml_response(reply_doc.toxml())

    def handle_status(self, request, body):
        print "Status update:"
        body = body.rstrip("\0")
        try:
            doc = minidom.parseString(body)
            print doc.documentElement.toprettyxml()

            for n in doc.documentElement.childNodes:
                if n.nodeType != n.ELEMENT_NODE:
                    continue

                if n.localName == "SyncEnd":
                    datatype = n.getAttribute("Datatype")
                    partner = n.getAttribute("Partner")
                    if datatype == "" and partner == "":
                        self.emit("sync-end")
        except Exception, e:
            print "Failed to parse status XML: %s" % e
            print body

        return self.create_response(200)

