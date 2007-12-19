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

DBUS_SYNCENGINE_BUSNAME = "org.synce.SyncEngine"
DBUS_SYNCENGINE_IFACE   = "org.synce.SyncEngine"
DBUS_SYNCENGINE_OBJPATH = "/org/synce/SyncEngine"

DBUS_ODCCM_BUSNAME      = "org.synce.odccm"
DBUS_ODCCM_IFACE        = "org.synce.odccm.DeviceManager"
DBUS_ODCCM_OBJPATH      = "/org/synce/odccm/DeviceManager"

SYNC_ITEM_CALENDAR  = 0
SYNC_ITEM_CONTACTS  = 1
SYNC_ITEM_EMAIL     = 2
SYNC_ITEM_FAVORITES = 3
SYNC_ITEM_FILES     = 4
SYNC_ITEM_MEDIA     = 5
SYNC_ITEM_NOTES     = 6
SYNC_ITEM_TASKS     = 7
SYNC_ITEM_AVANTGO   = 8
SYNC_ITEM_WORLDMATE = 9

CHANGE_ADDED        = 1
CHANGE_MODIFIED     = 4
CHANGE_DELETED      = 3

SYNC_ITEMS = {
    SYNC_ITEM_CALENDAR  : ("Calendar", False),
    SYNC_ITEM_CONTACTS  : ("Contacts", False),
    SYNC_ITEM_EMAIL     : ("E-mail", False),
    SYNC_ITEM_FAVORITES : ("Favorites", True),
    SYNC_ITEM_FILES     : ("Files", False),
    SYNC_ITEM_MEDIA     : ("Media", True),
    SYNC_ITEM_NOTES     : ("Notes", True),
    SYNC_ITEM_TASKS     : ("Tasks", False),
    SYNC_ITEM_AVANTGO   : ("AvantGo", True),
    SYNC_ITEM_WORLDMATE : ("WorldMate", False)
}

SYNC_ITEM_CLASS_TO_ID = {
    "Calendar" : SYNC_ITEM_CALENDAR,
    "Contacts" : SYNC_ITEM_CONTACTS,
    "E-mail"   : SYNC_ITEM_EMAIL,
    "Tasks"    : SYNC_ITEM_TASKS
}

SYNC_ITEM_PSHIPID_TO_RRANAME = {
    SYNC_ITEM_FILES : "File",
    SYNC_ITEM_MEDIA : "Media",
    SYNC_ITEM_FAVORITES : "Favorite"
}

GUID_WM5_ACTIVESYNC_ENGINE = "{176F4FFD-F20C-4BD4-BDD7-01D0726C567B}"
GUID_WM5_EXCHANGE_ENGINE = "{22C7DA12-F3FD-4875-8344-7786454F6534}"

GUID_WM5_ITEM_CALENDAR  = "{4A5D9FE0-F139-4A63-A5A4-4F31CEEA02AD}"
GUID_WM5_ITEM_CONTACTS  = "{0DD8685C-E272-4FCB-9ECF-2EAD7EA2497B}"
GUID_WM5_ITEM_EMAIL     = "{C6D47067-6E92-480E-B0FC-4BA82182FAC7}"
GUID_WM5_ITEM_FAVORITES = "{7E29B5F7-C686-4B0C-9892-FD8BAD8E0D08}"
GUID_WM5_ITEM_FILES     = "{B7B6ACB2-AF1D-43F5-BF9A-586111B263EF}"
GUID_WM5_ITEM_MEDIA     = "{A38DEEBF-C535-D8E2-AE24-8AE61845CF82}"
GUID_WM5_ITEM_NOTES     = "{8E98CB51-85A4-4777-8DEB-A0298DF8899F}"
GUID_WM5_ITEM_TASKS     = "{783AE4F6-4C12-4423-8270-66361260D4F1}"
GUID_WM5_ITEM_AVANTGO   = "{BEA275EB-E3B7-641F-3D77-752349BAAA63}"
GUID_WM5_ITEM_WORLDMATE = "{89768ABD-D173-18E3-16EC-98034F67B775}"


SYNC_ITEM_ID_FROM_GUID = {
    GUID_WM5_ITEM_CALENDAR  : SYNC_ITEM_CALENDAR,
    GUID_WM5_ITEM_CONTACTS  : SYNC_ITEM_CONTACTS,
    GUID_WM5_ITEM_EMAIL     : SYNC_ITEM_EMAIL,
    GUID_WM5_ITEM_FAVORITES : SYNC_ITEM_FAVORITES,
    GUID_WM5_ITEM_FILES     : SYNC_ITEM_FILES,
    GUID_WM5_ITEM_MEDIA     : SYNC_ITEM_MEDIA,
    GUID_WM5_ITEM_NOTES     : SYNC_ITEM_NOTES,
    GUID_WM5_ITEM_TASKS     : SYNC_ITEM_TASKS,
    GUID_WM5_ITEM_AVANTGO   : SYNC_ITEM_AVANTGO,
    GUID_WM5_ITEM_WORLDMATE : SYNC_ITEM_WORLDMATE
}

SYNC_ITEM_ID_TO_GUID = {
    SYNC_ITEM_CALENDAR  : GUID_WM5_ITEM_CALENDAR,
    SYNC_ITEM_CONTACTS  : GUID_WM5_ITEM_CONTACTS,
    SYNC_ITEM_EMAIL     : GUID_WM5_ITEM_EMAIL,
    SYNC_ITEM_FAVORITES : GUID_WM5_ITEM_FAVORITES,
    SYNC_ITEM_FILES     : GUID_WM5_ITEM_FILES,
    SYNC_ITEM_MEDIA     : GUID_WM5_ITEM_MEDIA,
    SYNC_ITEM_NOTES     : GUID_WM5_ITEM_NOTES,
    SYNC_ITEM_TASKS     : GUID_WM5_ITEM_TASKS,
    SYNC_ITEM_AVANTGO   : GUID_WM5_ITEM_AVANTGO,
    SYNC_ITEM_WORLDMATE : GUID_WM5_ITEM_WORLDMATE
}

#
# Direction flags for the formatter function

DIR_TO_AIRSYNC = 0
DIR_FROM_AIRSYNC = 1

REMSYNC_PORT = 999
RRA_PORT     = 5678
AIRSYNC_PORT = 26675