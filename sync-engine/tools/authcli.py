#!/usr/bin/env python
# -*- coding: utf-8 -*-
###############################################################################
# authgui.py
#
# Helper tool for authorization in sync-engine. Run as a stand-alone unit - it
# is not bound to sync-engine in case gtk is not available
#
###############################################################################

import dbus
import dbus.glib
import sys
import getpass
import re

ODCCM_DEVICE_PASSWORD_FLAG_SET     = 1
ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE = 2

UDEV_DEVICE_PASSWORD_FLAG_UNSET             = "unset"
UDEV_DEVICE_PASSWORD_FLAG_PROVIDE           = "provide"
UDEV_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = "provide-on-device"
UDEV_DEVICE_PASSWORD_FLAG_CHECKING          = "checking"
UDEV_DEVICE_PASSWORD_FLAG_UNLOCKED          = "unlocked"

# 
# AuthCli
#
# Application class.

class AuthCli:
	
	def __init__(self,objpath):

		bus = dbus.SystemBus()

		if re.compile('/org/synce/dccm/Device/').match(objpath) != None:
			self.deviceObject = bus.get_object("org.synce.dccm", objpath)
			self.device = dbus.Interface(self.deviceObject, "org.synce.dccm.Device")
			self.deviceName = self.device.GetName()
			return

		self.deviceObject = bus.get_object("org.synce.odccm", objpath)
		self.device = dbus.Interface(self.deviceObject, "org.synce.odccm.Device")
		self.deviceName = self.device.GetName()
	
	def Authorize(self):
		
		# no need to run if for some reason we are called on a 
		# device that is not blocked
		
		if re.compile('/org/synce/dccm/Devices/').match(self.device.object_path) != None:
			flags = self.device.GetPasswordFlags()
			rc=1
			if flags == "provide":
				print
				print "Authorization required for device %s." % self.deviceName
				
				rc = 0
				cnt = 3
				while not rc and cnt:
					rc = self.device.ProvidePassword(getpass.getpass("Password:"))
					cnt -= 1
			return rc

		flags = self.device.GetPasswordFlags()
		rc=1
		if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
			
			print
			print "Authorization required for device %s." % self.deviceName
			
			rc = 0
			cnt = 3
			while not rc and cnt:
				rc = self.device.ProvidePassword(getpass.getpass("Password:"))
				cnt -= 1
		return rc

#
# main
#
# Get the objpath from the command line, then authorize

if len(sys.argv) > 1:
	devobjpath = sys.argv[1]
	app = AuthCli(devobjpath)
	sys.exit(app.Authorize())
else:
	sys.exit(0)
