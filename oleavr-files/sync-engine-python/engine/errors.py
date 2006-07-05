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

_SYNC_ENGINE_ERROR_IFACE = "org.synce.SyncEngine.Error"

class Disconnected(dbus.DBusException):
    """
    Device is not currently connected.
    """
    _dbus_error_name = _SYNC_ENGINE_ERROR_IFACE + ".Disconnected"

class InvalidArgument(dbus.DBusException):
    """
    One of the arguments specified is invalid.
    """
    _dbus_error_name = _SYNC_ENGINE_ERROR_IFACE + ".InvalidArgument"

class NoFreeSlots(dbus.DBusException):
    """
    No free slots on the device. Delete an existing partnership and
    try again.
    """
    _dbus_error_name = _SYNC_ENGINE_ERROR_IFACE + ".NoFreeSlots"

class ProtocolError(dbus.DBusException):
    """
    An unexpected protocol error occured. This usually means that there's
    a bug in the implementation.
    """
    _dbus_error_name = _SYNC_ENGINE_ERROR_IFACE + ".ProtocolError"
