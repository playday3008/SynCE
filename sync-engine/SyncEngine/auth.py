# -*- coding: utf-8 -*-
############################################################################
# auth.py
#
# Authorization handler for sync-engine
#
# Copyright (C) 2007 Dr J A Gow 
#
############################################################################

import dbus.service
import dbus
import os.path
import config
import re


ODCCM_DEVICE_PASSWORD_FLAG_SET     = 1
ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE = 2
ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = 4

UDEV_DEVICE_PASSWORD_FLAG_UNSET             = "unset"
UDEV_DEVICE_PASSWORD_FLAG_PROVIDE           = "provide"
UDEV_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = "provide-on-device"
UDEV_DEVICE_PASSWORD_FLAG_CHECKING          = "checking"
UDEV_DEVICE_PASSWORD_FLAG_UNLOCKED          = "unlocked"

	
AUTH_TOOLS_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),"../tools")
CLI_TOOL = os.path.join(AUTH_TOOLS_PATH,"authcli.py")
GUI_TOOL = os.path.join(AUTH_TOOLS_PATH,"authgui.py")
	
###############################################################################
# IsAuthRequired
#
# Is authorization required to use this device?
	
def IsAuthRequired(device):
	if re.compile('/org/synce/dccm/Device/').match(device.object_path) != None:
		flags = device.GetPasswordFlags()
		if flags == "unset" or flags == "unlocked":
			return False
		return True

	flags = device.GetPasswordFlags()
        if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE or flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
		return True
	return False

############################################################################
# Authorize
#
# Obtain an authorization token to continue device connection. 
# If we are set to allow an external program to handle auth, or
#   if an auth attempt is pending, return 0 and remain disconnected
# Success (the device is unlocked) will return 1 and allow the
#   device to proceed with connection
# A password entry required on the device (WM6) will return 2,
#   the device will remain disconnected for now
# A failed authorisation attempt over the wire will return 3,
#   the device will remain disconnected

def Authorize(devpath,device,config):
	
        if IsAuthRequired(device) == False:
            return 1

	if re.compile('/org/synce/dccm/Device/').match(device.object_path) != None:
		flags = device.GetPasswordFlags()
		if flags == "provide-on-device":
			return 2
		if flags == "pending":
			return 0
	else:
		flags = device.GetPasswordFlags()
		if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
			return 2

	rc=1
	# get the program we need

	prog = config.cfg["AuthMethod"]

	if prog == "EXTERNAL":
            return 0

	if prog == "INTERNAL_GUI":
		clist = [GUI_TOOL,devpath]
	elif prog == "INTERNAL_CLI":
		clist = [CLI_TOOL,devpath]
	else:
		clist = [prog,devpath]
	
	# check that we have it
	
	if os.path.exists(clist[0]):
		rc = os.spawnvp(os.P_WAIT,clist[0],clist)
		if rc<0:
			rc = 3
		if not IsAuthRequired(device):
			rc = 1
		else:
			rc = 3
	else:
		print "auth: auth prog %s does not exist" % clist[0]
		rc = 3

	return rc
