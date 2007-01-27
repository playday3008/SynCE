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

"""
    Unhandled fields:
        Body (subnodes/attrs: Body{Size,Truncated}), Children (subnodes: Child),
        Yomi{CompanyName,FirstName,LastName}, {Customer,Government}Id,
        AccountName, MMS and Rtf
"""

import parser
from engine.xmlutil import *
from engine.util import *


def categories_to_airsync(doc, src_doc, src_node, value):
    app_node = xpath.Evaluate("/ApplicationData", doc)[0]
    # We need to find an existing 'Categories' node in the target
    # and add the converted data to that instead of simply returning
    # the converted data

    nodes = xpath.Evaluate("Categories", app_node)
    if nodes:
        parent = nodes[0]
    else:
        parent = node_append_child(app_node, "Categories")

    for cat_node in xpath.Evaluate("Category", src_node):
        node_append_child(parent, "Category", node_get_value(cat_node))

    return ()


ADDR_FIELDS = ("City", "Country", "PostalCode", "Region", "Street")

FROM_AIRSYNC_SPEC = (
 ("FileAs",                                                                                     "FileAs"),
 ("FormattedName",                                                                              "FormattedName"),
 (("FirstName", "LastName", "MiddleName", "Suffix", "Title"),                                   ("Name", ("FirstName", "LastName", "Additional", "Suffix", "Prefix"))),
 ("NickName",                                                                                   "Nickname"),
 (("HomeCity", "HomeCountry", "HomePostalCode", "HomeState", "HomeStreet"),                     ("Address", (("Type", "HOME"),) + ADDR_FIELDS)),
 (("BusinessCity", "BusinessCountry", "BusinessPostalCode", "BusinessState", "BusinessStreet"), ("Address", (("Type", "WORK"),) + ADDR_FIELDS)),
 (("OtherCity", "OtherCountry", "OtherPostalCode", "OtherState", "OtherStreet"),                ("Address", (("Type", "OTHER"),) + ADDR_FIELDS)),
 ("Picture",                                                                                    "Photo"),
 ("JobTitle",                                                                                   "Title"),
 ("ManagerName",                                                                                "Manager"),
 ("Spouse",                                                                                     "Spouse"),
 ("WebPage",                                                                                    "Url"),
 ("AssistantName",                                                                              "Assistant"),
 ("Categories",                                                                                 None),
 ("Anniversary",                                                                                lambda d, sd, sn, v: v.split("T")[0]),
 ("Birthday",                                                                                   lambda d, sd, sn, v: v.split("T")[0].replace("-", "")),
 (("HomePhoneNumber",),                                                                         ("Telephone", (("Type", "HOME"), "Content"))),
 (("Home2PhoneNumber",),                                                                        ("Telephone", (("Type", "HOME"), "Content"))),
 (("HomeFaxNumber",),                                                                           ("Telephone", (("Type", "HOME"), ("Type", "FAX"), "Content"))),
 (("MobilePhoneNumber",),                                                                       ("Telephone", (("Type", "CELL"), "Content"))),
 (("PagerNumber",),                                                                             ("Telephone", (("Type", "PAGER"), "Content"))),
 (("CarPhoneNumber",),                                                                          ("Telephone", (("Type", "CAR"), "Content"))),
 (("RadioPhoneNumber",),                                                                        ("Telephone", (("Type", "Radio"), "Content"))),
 (("BusinessPhoneNumber",),                                                                     ("Telephone", (("Type", "WORK"), "Content"))),
 (("Business2PhoneNumber",),                                                                    ("Telephone", (("Type", "WORK"), "Content"))),
 (("BusinessFaxNumber",),                                                                       ("Telephone", (("Type", "WORK"), ("Type", "FAX"), "Content"))),
 (("AssistnamePhoneNumber",),                                                                   ("Telephone", (("Type", "Assistant"), "Content"))),
 (("CompanyMainPhone",),                                                                        ("Telephone", (("Type", "Company"), "Content"))),
 (("CompanyName", "Department", "OfficeLocation"),                                              ("Organization", ("Name", "Department", "Unit"))),
 (("Email1Address",),                                                                           ("EMail", ("Content",))),
 (("Email2Address",),                                                                           ("EMail", ("Content",))),
 (("Email3Address",),                                                                           ("EMail", ("Content",))),
 (("IMAddress",),                                                                               ("IM-MSN", ("Content",))),
 (("IMAddress2",),                                                                              ("IM-MSN", ("Content",))),
 (("IMAddress3",),                                                                              ("IM-MSN", ("Content",))),
)

FROM_AIRSYNC_UNMAPPED = [ "Children", "Rtf", "CustomerId", "GovernmentId", "AccountName" ]

TO_AIRSYNC_SPEC = (
 ("FileAs",                             ("FileAs",)),
 ("Name",                               ({"FirstName"  : "FirstName",
                                          "LastName"   : "LastName",
                                          "Additional" : "MiddleName",
                                          "Suffix"     : "Suffix",
                                          "Prefix"     : "Title"},)),
 ("Nickname",                           ("NickName",)),
 ("Photo",                              ("Picture",)),
 ("Address[Type='HOME']",               ({"City"       : "HomeCity",
                                          "Country"    : "HomeCountry",
                                          "PostalCode" : "HomePostalCode",
                                          "Region"     : "HomeState",
                                          "Street"     : "HomeStreet" },)),
 ("Address[Type='WORK']",               ({"City"       : "BusinessCity",
                                          "Country"    : "BusinessCountry",
                                          "PostalCode" : "BusinessPostalCode",
                                          "Region"     : "BusinessState",
                                          "Street"     : "BusinessStreet" },)),
 ("Address[Type='OTHER']",              ({"City"       : "OtherCity",
                                          "Country"    : "OtherCountry",
                                          "PostalCode" : "OtherPostalCode",
                                          "Region"     : "OtherState",
                                          "Street"     : "OtherStreet" },)),
 ("Address",                            ({"City"       : "HomeCity",
                                          "Country"    : "HomeCountry",
                                          "PostalCode" : "HomePostalCode",
                                          "Region"     : "HomeState",
                                          "Street"     : "HomeStreet" },)),
 ("Categories",                         categories_to_airsync),
 ("Assistant",                          ("AssistantName",)),
 ("EMail",                              ("Email1Address",
                                         "Email2Address",
                                         "Email3Address",)),
 ("IM-MSN",                             ("IMAddress",
                                         "IMAddress2",
                                         "IMAddress3",)),
 ("Manager",                            ("ManagerName",)),
 ("Organization",                       ({"Name"       : "CompanyName",
                                          "Department" : "Department",
                                          "Unit"       : "OfficeLocation", },)),
 ("Spouse",                             ("Spouse",)),
 ("Telephone[Type='HOME'][Type='FAX']", ("HomeFaxNumber",)),
 ("Telephone[Type='HOME']",             ("HomePhoneNumber",
                                         "Home2PhoneNumber",)),
 ("Telephone[Type='WORK'][Type='FAX']", ("BusinessFaxNumber",)),
 ("Telephone[Type='WORK']",             ("BusinessPhoneNumber",
                                         "Business2PhoneNumber",)),
 ("Telephone[Type='CAR']",              ("CarPhoneNumber",)),
 ("Telephone[Type='CELL']",             ("MobilePhoneNumber",)),
 ("Telephone[Type='PAGER']",            ("PagerNumber",)),
 ("Telephone[Type='Assistant']",        ("AssistnamePhoneNumber",)),
 ("Telephone[Type='Company']",          ("CompanyMainPhone",)),
 ("Telephone[Type='Radio']",            ("RadioPhoneNumber",)),
 ("Title",                              ("JobTitle",)),
 ("Url",                                ("WebPage",)),
 ("Anniversary",                        ({"Content" : lambda d, sd, sn, v: v + "T22:00:00.000Z" },)),
 ("Birthday",                           ({"Content" : lambda d, sd, sn, v: "%s-%s-%sT22:00:00.000Z" % (v[0:4], v[4:6], v[6:8]) },)),
)

def from_airsync(guid, app_node):
    title = node_find_child(app_node, "Title")
    first = node_find_child(app_node, "FirstName")
    middle = node_find_child(app_node, "MiddleName")
    last = node_find_child(app_node, "LastName")
    suffix = node_find_child(app_node, "Suffix")

    fn = ""
    for node in (title, first, middle, last, suffix):
        if node == None:
            continue
        if fn:
            fn += " "
        fn += node_get_value(node)

    doc = parser.from_airsync(guid, app_node, "contact", FROM_AIRSYNC_SPEC, FROM_AIRSYNC_UNMAPPED)

    node = node_append_child(doc.documentElement, "FormattedName")
    node_append_child(node, "Content", fn)

    return doc

def to_airsync(os_doc):

    # Add a FileAs node if it isn't there already
    if node_find_child(os_doc.documentElement, "FileAs") == None:
        fa_node = node_append_child(os_doc.documentElement, "FileAs")

        fn_node = node_find_child(os_doc.documentElement, "FormattedName")
        if fn_node != None:
            fn_content_node = node_find_child(fn_node, "Content")
            node_append_child(fa_node, "Content", node_get_value(fn_content_node))
        else:
            nm_node = node_find_child(os_doc.documentElement, "Name")
            title = node_find_child(nm_node, "Title")
            first = node_find_child(nm_node, "FirstName")
            middle = node_find_child(nm_node, "MiddleName")
            last = node_find_child(nm_node, "LastName")
            suffix = node_find_child(nm_node, "Suffix")

            fn = ""
            for node in (title, first, middle, last, suffix):
                if node == None:
                    continue
                if fn:
                    fn += " "
                fn += node_get_value(node)

            node_append_child(fa_node, "Content", fn)

    return parser.to_airsync(os_doc, "contact", TO_AIRSYNC_SPEC, "POOMCONTACTS:")
