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

from xml.dom import minidom
from xml import xpath
from util import *

"""
    Unhandled fields:
        Body (subnodes/attrs: Body{Size,Truncated}), Children (subnodes: Child),
        Yomi{CompanyName,FirstName,LastName}, {Customer,Government}Id,
        AccountName, MMS and Rtf
"""

ADDR_FIELDS = ("City", "Country", "PostalCode", "Region", "Street")
HOME_PHONE_FIELDS = (("Type", "HOME"), ("Type", "VOICE"))
WORK_PHONE_FIELDS = (("Type", "WORK"), ("Type", "VOICE"))
EMAIL_FIELDS = ("EMail", (("Type", "HOME"), "Content"))
IM_FIELDS = ("IM-MSN", (("Type", "HOME"), "Content"))

AIRSYNC_TO_OPENSYNC_MAPPINGS = \
(
 ("FileAs", "FormattedName"),
 (("FirstName", "LastName", "MiddleName", "Suffix", "Title"),
  ("Name", ("FirstName", "LastName", "Additional", "Suffix", "Prefix"))),
 ("NickName", "Nickname"),
 (("HomeCity", "HomeCountry", "HomePostalCode", "HomeState", "HomeStreet"),
  ("Address", (("Type", "HOME"),) + ADDR_FIELDS)),
 ("Picture", "Photo"),
 ("JobTitle", "Title"),
 ("ManagerName", "Manager"),
 ("Spouse", "Spouse"),
 ("WebPage", "Url"),
 ("AssistantName", "Assistant"),
 ("Categories", None),
 ("Anniversary", lambda v: v.split("T")[0]),
 ("Birthday", lambda v: v.split("T")[0].replace("-", "")),
 (("BusinessCity", "BusinessCountry", "BusinessPostalCode", "BusinessState", "BusinessStreet"),
  ("Address", (("Type", "WORK"),) + ADDR_FIELDS)),
 (("OtherCity", "OtherCountry", "OtherPostalCode", "OtherState", "OtherStreet"),
  ("Address", (("Type", "OTHER"),) + ADDR_FIELDS)),
 (("HomePhoneNumber",), ("Telephone", HOME_PHONE_FIELDS + ("Content",))),
 (("Home2PhoneNumber",),("Telephone", HOME_PHONE_FIELDS + ("Content",))),
 (("HomeFaxNumber",), ("Telephone", (("Type", "HOME"), ("Type", "FAX"), "Content"))),
 (("MobilePhoneNumber",), ("Telephone", (("Type", "CELL"), "Content"))),
 (("PagerNumber",), ("Telephone", (("Type", "PAGER"), "Content"))),
 (("CarPhoneNumber",), ("Telephone", (("Type", "CAR"), "Content"))),
 (("RadioPhoneNumber",), ("Telephone", (("Type", "Radio"), "Content"))),
 (("BusinessPhoneNumber",), ("Telephone", WORK_PHONE_FIELDS + ("Content",))),
 (("Business2PhoneNumber",), ("Telephone", WORK_PHONE_FIELDS + ("Content",))),
 (("BusinessFaxNumber",), ("Telephone", (("Type", "WORK"), ("Type", "FAX"), "Content"))),
 (("AssistnamePhoneNumber",), ("Telephone", (("Type", "Assistant"), "Content"))),
 (("CompanyMainPhone",), ("Telephone", (("Type", "Company"), "Content"))),
 (("CompanyName", "Department", "OfficeLocation"), ("Organization", ("Name", "Department", "Unit"))),
 (("Email1Address",), EMAIL_FIELDS), (("Email2Address",), EMAIL_FIELDS), (("Email3Address",), EMAIL_FIELDS),
 (("IMAddress",), IM_FIELDS), (("IMAddress2",), IM_FIELDS), (("IMAddress3",), IM_FIELDS),
)

AIRSYNC_UNMAPPED_ENTRIES = [ "Children", "Rtf", "CustomerId", "GovernmentId", "AccountName" ]

def contact_from_airsync(app_node):
    dom = minidom.getDOMImplementation()
    doc = dom.createDocument(None, "contact", None)
    contact_node = doc.documentElement

    for src_spec, dst_spec in AIRSYNC_TO_OPENSYNC_MAPPINGS:
        if isinstance(src_spec, basestring):
            matches = xpath.Evaluate(src_spec, app_node)
            if matches:
                node = matches[0]

                if dst_spec != None:
                    value = node_get_value(node)
                    if callable(dst_spec):
                        value = dst_spec(value)
                        dst_spec = node.localName

                    val_node = node_append_child(contact_node, dst_spec)
                    node_append_child(val_node, "Content", value)
                else:
                    contact_node.appendChild(node.cloneNode(True))

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
                dst_parent_node = node_append_child(contact_node, dst_spec[0])
                for dst_desc in dst_spec[1]:
                    if isinstance(dst_desc, basestring):
                        if src_index in src_values:
                            node_append_child(dst_parent_node, dst_desc, src_values[src_index])
                        src_index += 1
                    else:
                        node_append_child(dst_parent_node, dst_desc[0], dst_desc[1])

    for entry in AIRSYNC_UNMAPPED_ENTRIES:
        nodes = xpath.Evaluate(entry, app_node)
        for node in nodes:
            app_node.removeChild(node)

    nodes = xpath.Evaluate("*", app_node)
    if nodes:
        print "contact_from_airsync: Unparsed XML:"
        print app_node.toprettyxml()
        print

    return doc

OPENSYNC_TO_AIRSYNC_MAPPINGS = (
    ("FormattedName", ("FileAs",)),
    ("Name", ({"FirstName" : "FirstName",
               "LastName" : "LastName",
               "Additional" : "MiddleName",
               "Suffix" : "Suffix",
               "Prefix" : "Title"},)),
    ("Nickname", ("NickName",)),
    ("Photo", ("Picture",)),
    ("Address[Type='HOME']", ({ "City"       : "HomeCity",
                                "Country"    : "HomeCountry",
                                "PostalCode" : "HomePostalCode",
                                "Region"     : "HomeState",
                                "Street"     : "HomeStreet" },)),
    ("Address[Type='WORK']", ({ "City"       : "BusinessCity",
                                "Country"    : "BusinessCountry",
                                "PostalCode" : "BusinessPostalCode",
                                "Region"     : "BusinessState",
                                "Street"     : "BusinessStreet" },)),
    ("Address[Type='OTHER']", ({ "City"       : "OtherCity",
                                 "Country"    : "OtherCountry",
                                 "PostalCode" : "OtherPostalCode",
                                 "Region"     : "OtherState",
                                 "Street"     : "OtherStreet" },)),
    ("Categories", None),
    ("Assistant", ("AssistantName",)),
    ("EMail[Type='HOME']", ("Email1Address", "Email2Address", "Email3Address",)),
    ("IM-MSN[Type='HOME']", ("IMAddress", "IMAddress2", "IMAddress3",)),
    ("Manager", ("ManagerName",)),
    ("Organization", ({ "Name" : "CompanyName",
                        "Department" : "Department",
                        "Unit" : "OfficeLocation", },)),
    ("Spouse", ("Spouse",)),
    ("Telephone[Type='HOME'][Type='VOICE']", ("HomePhoneNumber", "Home2PhoneNumber",)),
    ("Telephone[Type='HOME'][Type='FAX']", ("HomeFaxNumber",)),
    ("Telephone[Type='WORK'][Type='VOICE']", ("BusinessPhoneNumber", "Business2PhoneNumber",)),
    ("Telephone[Type='WORK'][Type='FAX']", ("BusinessFaxNumber",)),
    ("Telephone[Type='CAR']", ("CarPhoneNumber",)),
    ("Telephone[Type='CELL']", ("MobilePhoneNumber",)),
    ("Telephone[Type='PAGER']", ("PagerNumber",)),
    ("Telephone[Type='Assistant']", ("AssistnamePhoneNumber",)),
    ("Telephone[Type='Company']", ("CompanyMainPhone",)),
    ("Telephone[Type='Radio']", ("RadioPhoneNumber",)),
    ("Title", ("JobTitle",)),
    ("Url", ("WebPage",)),
    ("Anniversary", (lambda v: v + "T22:00:00.000Z",)),
    ("Birthday", (lambda v: "%s-%s-%sT22:00:00.000Z" % (v[0:4], v[4:6], v[6:8]),)),
)

def contact_to_airsync(contact_doc):
    dom = minidom.getDOMImplementation()
    doc = dom.createDocument(None, "ApplicationData", None)
    app_node = doc.documentElement

    contact_node = xpath.Evaluate("/contact", contact_doc)[0]
    for expr, mappings in OPENSYNC_TO_AIRSYNC_MAPPINGS:
        i = 0

        for node in xpath.Evaluate(expr, contact_node):
            if mappings != None:
                mapping = mappings[i]

                if not isinstance(mapping, dict):
                    mapping = { "Content" : mapping }

                for node_name, as_name in mapping.items():
                    nodes = xpath.Evaluate(node_name, node)
                    if nodes:
                        value = node_get_value(nodes[0])
                        if callable(as_name):
                            value = as_name(value)
                            as_name = node.localName
                        node_append_child(app_node, as_name, value)
            else:
                app_node.appendChild(node.cloneNode(True))

            contact_node.removeChild(node)

            i += 1
            if mappings == None or i >= len(mappings):
                break

    nodes = xpath.Evaluate("*", contact_node)
    if nodes:
        print "contact_to_airsync: Unparsed XML:"
        print contact_node.toprettyxml()
        print

    return doc
