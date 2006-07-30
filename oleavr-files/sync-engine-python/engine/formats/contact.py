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

"""
    Unhandled fields:
        Body (subnodes/attrs: Body{Size,Truncated}), Children (subnodes: Child),
        Yomi{CompanyName,FirstName,LastName}, {Customer,Government}Id,
        AccountName, MMS and Rtf
"""

import parser

ADDR_FIELDS = ("City", "Country", "PostalCode", "Region", "Street")
HOME_PHONE_FIELDS = (("Type", "HOME"), ("Type", "VOICE"))
WORK_PHONE_FIELDS = (("Type", "WORK"), ("Type", "VOICE"))
EMAIL_FIELDS = ("EMail", (("Type", "HOME"), "Content"))
IM_FIELDS = ("IM-MSN", (("Type", "HOME"), "Content"))

FROM_AIRSYNC_SPEC = \
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
 ("Anniversary", lambda d, sd, sn, v: v.split("T")[0]),
 ("Birthday", lambda d, sd, sn, v: v.split("T")[0].replace("-", "")),
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

FROM_AIRSYNC_UNMAPPED = [ "Children", "Rtf", "CustomerId", "GovernmentId", "AccountName" ]

TO_AIRSYNC_SPEC = (
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
 ("Anniversary", ({ "Content" : lambda d, sd, sn, v: v + "T22:00:00.000Z" },)),
 ("Birthday", ({ "Content" : lambda d, sd, sn, v: "%s-%s-%sT22:00:00.000Z" % (v[0:4], v[4:6], v[6:8]) },)),
)

def from_airsync(guid, app_node):
    return parser.from_airsync(guid, app_node, "contact", FROM_AIRSYNC_SPEC,
                               FROM_AIRSYNC_UNMAPPED)

def to_airsync(os_doc):
    return parser.to_airsync(os_doc, "contact", TO_AIRSYNC_SPEC)
