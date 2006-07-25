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

import dbus
import dbus.glib
import gobject
import thread
import threading
from opensync import *
from xml.dom import minidom
from xml import xpath

import time

SE_SYNC_ITEM_CALENDAR  = 0
SE_SYNC_ITEM_CONTACTS  = 1
SE_SYNC_ITEM_EMAIL     = 2
SE_SYNC_ITEM_FAVORITES = 3
SE_SYNC_ITEM_FILES     = 4
SE_SYNC_ITEM_MEDIA     = 5
SE_SYNC_ITEM_NOTES     = 6
SE_SYNC_ITEM_TASKS     = 7

SE_CHANGE_ADDED        = 0
SE_CHANGE_MODIFIED     = 1
SE_CHANGE_DELETED      = 2


#
# Utility functions
#

def node_get_value(node):
    for n in node.childNodes:
        if n.nodeType == n.TEXT_NODE:
            return n.nodeValue.strip()
    return None

def node_append_child(parent, name, value=None, always_create=True):
    if not always_create:
        nodes = xpath.Evaluate(name, parent)
        if nodes:
            return nodes[0]

    doc = parent.ownerDocument
    node = doc.createElement(name)
    if value is not None:
        value_node = doc.createTextNode(value)
        node.appendChild(value_node)
    parent.appendChild(node)
    return node


#
# Conversion functions
#

def date_from_airsync(remove_dashes, as_name, value, as_node, os_contact_node):
    if value == None: return

    value = value.split("T")[0]
    if remove_dashes:
        value = value.replace("-", "")

    field_node = node_append_child(os_contact_node, as_name)
    node_append_child(field_node, "Content", value)


def addr_from_airsync(type, field, as_name, value, as_node, os_contact_node):
    if value == None: return

    nodes = xpath.Evaluate("Address[Type='%s']" % type, os_contact_node)
    if nodes:
        addr_node = nodes[0]
    else:
        addr_node = node_append_child(os_contact_node, "Address")
        node_append_child(addr_node, "Type", type)

    node_append_child(addr_node, field, value)


def im_addr_from_airsync(as_name, value, as_node, os_contact_node):
    if value == None: return

    im_node = node_append_child(os_contact_node, "IM-MSN")
    node_append_child(im_node, "Content", value)
    node_append_child(im_node, "Type", "HOME")


def email_from_airsync(as_name, value, as_node, os_contact_node):
    if value == None: return

    email_node = node_append_child(os_contact_node, "EMail")
    node_append_child(email_node, "Content", value)
    node_append_child(email_node, "Type", "HOME")


def name_from_airsync(field, as_name, value, as_node, os_contact_node):
    if value == None: return

    name_node = node_append_child(os_contact_node, "Name", always_create=False)

    node_append_child(name_node, field, value)


def phone_from_airsync(type1, type2, as_name, value, as_node, os_contact_node):
    if value == None: return

    phone_node = node_append_child(os_contact_node, "Telephone")
    node_append_child(phone_node, "Content", value)
    node_append_child(phone_node, "Type", type1)

    if type2 != None:
        node_append_child(phone_node, "Type", type2)


def organization_from_airsync(field, as_name, value, as_node, os_contact_node):
    if value == None: return

    org_node = node_append_child(os_contact_node, "Organization", always_create=False)

    node_append_child(org_node, field, value)


"""
Unhandled fields:
        Body, BodySize, BodyTruncated,
        Categories, Category,
        Children, Child,
        YomiCompanyName, YomiFirstName, YomiLastName
        CustomerId, GovernmentId
        AccountName, MMS
"""


CONTACT_AIRSYNC_TO_OPENSYNC = {
    "Anniversary" : lambda *args: date_from_airsync(False, *args),
    "AssistantName" : "Assistant",
    "AssistnamePhoneNumber" : lambda *args: phone_from_airsync("Assistant", None, *args),
    "Birthday" : lambda *args: date_from_airsync(True, *args),
    "Business2PhoneNumber" : lambda *args: phone_from_airsync("WORK", "VOICE", *args),
    "BusinessCity" : lambda *args: addr_from_airsync("WORK", "City", *args),
    "BusinessCountry" : lambda *args: addr_from_airsync("WORK", "Country", *args),
    "BusinessFaxNumber" : lambda *args: phone_from_airsync("WORK", "FAX", *args),
    "BusinessPhoneNumber" : lambda *args: phone_from_airsync("WORK", "VOICE", *args),
    "BusinessPostalCode" : lambda *args: addr_from_airsync("WORK", "PostalCode", *args),
    "BusinessState" : lambda *args: addr_from_airsync("WORK", "Region", *args),
    "BusinessStreet" : lambda *args: addr_from_airsync("WORK", "Street", *args),
    "CarPhoneNumber" : lambda *args: phone_from_airsync("CAR", None, *args),
    "CompanyMainPhone" : lambda *args: phone_from_airsync("Company", None, *args),
    "CompanyName" : lambda *args: organization_from_airsync("Name", *args),
    "Department" : lambda *args: organization_from_airsync("Department", *args),
    "Email1Address" : email_from_airsync,
    "Email2Address" : email_from_airsync,
    "Email3Address" : email_from_airsync,
    "FileAs" : "FormattedName",
    "FirstName" : lambda *args: name_from_airsync("FirstName", *args),
    "Home2PhoneNumber" : lambda *args: phone_from_airsync("HOME", "VOICE", *args),
    "HomeCity" : lambda *args: addr_from_airsync("HOME", "City", *args),
    "HomeCountry" : lambda *args: addr_from_airsync("HOME", "Country", *args),
    "HomeFaxNumber" : lambda *args: phone_from_airsync("HOME", "FAX", *args),
    "HomePhoneNumber" : lambda *args: phone_from_airsync("HOME", "VOICE", *args),
    "HomePostalCode" : lambda *args: addr_from_airsync("HOME", "PostalCode", *args),
    "HomeState" : lambda *args: addr_from_airsync("HOME", "Region", *args),
    "HomeStreet" : lambda *args: addr_from_airsync("HOME", "Street", *args),
    "IMAddress" : im_addr_from_airsync,
    "IMAddress2" : im_addr_from_airsync,
    "IMAddress3" : im_addr_from_airsync,
    "JobTitle" : "Title",
    "LastName" : lambda *args: name_from_airsync("LastName", *args),
    "ManagerName" : "Manager",
    "MiddleName" : lambda *args: name_from_airsync("Additional", *args),
    "MobilePhoneNumber" : lambda *args: phone_from_airsync("CELL", None, *args),
    "NickName" : "Nickname",
    "OfficeLocation" : lambda *args: organization_from_airsync("Unit", *args),
    "OtherCity" : lambda *args: addr_from_airsync("OTHER", "City", *args),
    "OtherCountry" : lambda *args: addr_from_airsync("OTHER", "Country", *args),
    "OtherPostalCode" : lambda *args: addr_from_airsync("OTHER", "PostalCode", *args),
    "OtherState" : lambda *args: addr_from_airsync("OTHER", "Region", *args),
    "OtherStreet" : lambda *args: addr_from_airsync("OTHER", "Street", *args),
    "PagerNumber" : lambda *args: phone_from_airsync("PAGER", None, *args),
    "Picture" : "Photo",
    "RadioPhoneNumber" : lambda *args: phone_from_airsync("Radio", None, *args),
    "Rtf" : "Note",     # FIXME: this one is probably incorrect...
    "Spouse" : "Spouse",
    "Suffix" : lambda *args: name_from_airsync("Suffix", *args),
    "Title" : lambda *args: name_from_airsync("Prefix", *args),
    "WebPage" : "Url",
}


def airsync_contact_to_opensync(as_data):
    as_doc = minidom.parseString(as_data.encode("utf-8"))

    dom = minidom.getDOMImplementation()
    doc = dom.createDocument(None, "contact", None)
    contact_node = doc.documentElement

    for n in xpath.Evaluate("/ApplicationData/*", as_doc):
        name = n.localName

        if name in CONTACT_AIRSYNC_TO_OPENSYNC:
            entry = CONTACT_AIRSYNC_TO_OPENSYNC[name]

            value = node_get_value(n)

            if isinstance(entry, basestring):
                if value == None: continue

                field_node = node_append_child(contact_node, entry)
                node_append_child(field_node, "Content", value)
            else:
                entry(name, value, n, contact_node)
        else:
            raise Exception("Unhandled: \"%s\"" % name)

    return doc.toxml()


class SyncClass:
    def __init__(self, member):
        self.__member = member
        self.engine = None

        gobject.threads_init()

        self.mainloop_exit_event = threading.Event()
        thread.start_new_thread(self._mainloop_thread_func, ())

    def _mainloop_thread_func(self):
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()
        self.mainloop_exit_event.set()

    def connect(self, ctx):
        print "SynCE::connect called"
        self.hack = []
        gobject.idle_add(self._do_connect_idle_cb, ctx)

    def _do_connect_idle_cb(self, ctx):
        try:
            bus = dbus.SessionBus()
            proxy_obj = bus.get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
            self.engine = dbus.Interface(proxy_obj, "org.synce.SyncEngine")
            self.engine.connect_to_signal("Synchronized", lambda: gobject.idle_add(self._synchronized_cb))

            ctx.report_success()
        except Exception, e:
            print "SynCE::connect: failed: %s" % e
            ctx.report_error()

    def _synchronized_cb(self):
        if self.ctx != None:
            ctx = self.ctx
            self.ctx = None

            #print "Sleeping 10 seconds...",
            #time.sleep(10)
            #print "done"

            sids = []
            changeset = self.engine.GetRemoteChanges()
            if changeset:
                print "SynCE: %d remote changes" % len(changeset)
            else:
                print "SynCE: No remote changes"

            for sid, change in changeset.items():
                chg_type, item_type, data = change

                change = OSyncChange()
                change.uid = sid.encode("utf-8")

                if chg_type == SE_CHANGE_ADDED:
                    change.changetype = CHANGE_ADDED
                elif chg_type == SE_CHANGE_MODIFIED:
                    change.changetype = CHANGE_MODIFIED
                elif chg_type == SE_CHANGE_DELETED:
                    change.changetype = CHANGE_DELETED
                else:
                    print "Unknown changetype %d" % chg_type

                if item_type == SE_SYNC_ITEM_CONTACTS:
                    change.objtype = "contact"
                    change.format = "xml-contact-string"

                    if chg_type != SE_CHANGE_DELETED:
                        xml = airsync_contact_to_opensync(data)
                        bytes = xml.encode("utf-8")
                        # this is just a temporary hack around a bug in the opensync bindings
                        # (OSyncChange.set_data() should either copy the string or
                        #  hold a reference to it)
                        self.hack.append(bytes)
                        change.set_data(bytes, TRUE)
                else:
                    raise Exception("Unhandled item_type %d" % item_type)

                change.report(ctx)

                sids.append(sid)

            if sids:
                print "SynCE: Acknowledging remote changes"
                self.engine.AcknowledgeRemoteChanges(sids)

            print "SynCE: Reporting success"
            ctx.report_success()

    def get_changeinfo(self, ctx):
        print "SynCE: get_changeinfo called"
        if self.__member.get_slow_sync("data"):
            print "SynCE: slow-sync requested"

        self.ctx = ctx
        gobject.idle_add(self._do_get_changeinfo_idle_cb)

    def _do_get_changeinfo_idle_cb(self):
        print "SynCE: Calling StartSync"
        self.engine.StartSync()

    def commit_change(self, ctx, chg):
        print "SynCE: commit_change called"
        #print "Opensync wants me to write data with size " + str(chg.datasize)
        print chg.format
        print "Data: \"%s\"" % chg.data
        ctx.report_success()

    def disconnect(self, ctx):
        print "SynCE: disconnect called"

        self.engine = None
        self.get_changeinfo_ctx = None

        ctx.report_success()

        self.hack = []

    def sync_done(self, ctx):
        print "SynCE: sync_done called"
        ctx.report_success()

    def finalize(self):
        print "SynCE: finalize called"
        gobject.idle_add(self.mainloop.quit)
        self.mainloop_exit_event.wait()

def initialize(member):
    return SyncClass(member)

def get_info(info):
    info.name = "synce-plugin"
    info.longname = "Plugin to synchronize with Windows CE device"
    info.description = "by Ole Andre Vadla Ravnaas"

    info.version = 2

    info.accept_objtype("contact")
    info.accept_objformat("contact", "xml-contact-string")
