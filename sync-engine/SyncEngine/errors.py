# -*- coding: utf-8 -*-
############################################################################
# ERRORS.py
#
# Error and exception classes for sync-engine. Designed to make error
# handling a little cleaner than using bare exceptions. It also contains
# the dbus exception classes generated from dbus.exception from the original
# errors.py
#
# This module will be expanded as error handling is slowly cleaned up.
#
# Original errors.py (C) 2006  Ole André Vadla Ravnås <oleavr@gmail.com>
#
# Dr J A Gow 24/02/08
#
############################################################################

import dbus

############################################################################
# SynceError
#
# API error exception base class
#
############################################################################

class SynceError(dbus.DBusException):
	_dbus_error_name = "org.synce.SyncEngine.Error"

############################################################################
# Disconnected
#
# Device is not currently connected.
#
############################################################################

class Disconnected(SynceError):
	_dbus_error_name = SynceError._dbus_error_name + ".Disconnected"

############################################################################
# InvalidArgument
#
# One of the arguments specified is invalid.
#
############################################################################

class InvalidArgument(SynceError):
    _dbus_error_name = SynceError._dbus_error_name + ".InvalidArgument"

############################################################################
# NotAvailable
#
# The requested operation is not available.
#
############################################################################

class NotAvailable(dbus.DBusException):
    _dbus_error_name = SynceError._dbus_error_name + ".NotAvailable"

############################################################################
# NoFreeSlots
#
# No free slots on the device. Delete an existing partnership and
# try again.
#
############################################################################

class NoFreeSlots(dbus.DBusException):
    _dbus_error_name = SynceError._dbus_error_name + ".NoFreeSlots"

############################################################################
# ProtocolError
#
# An unexpected protocol error occured. This usually means that there's
# a bug in the implementation.
#
############################################################################

class ProtocolError(dbus.DBusException):
    _dbus_error_name = SynceError._dbus_error_name + ".ProtocolError"

############################################################################
# SyncRunning
#
# A sync is running, and the requested action must wait until the
# sync is complete
#
############################################################################

class SyncRunning(dbus.DBusException):
    _dbus_error_name = SynceError._dbus_error_name + ".SyncRunning"

############################################################################
# NoBoundPartnership
#
# There is no bound partnership in the device and the current action
# can not continue
#
############################################################################

class NoBoundPartnership(dbus.DBusException):
    _dbus_error_name = SynceError._dbus_error_name + ".NoBoundPartnership"

