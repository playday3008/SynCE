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

HAL_DEVICE_PASSWORD_FLAG_UNSET             = "unset"
HAL_DEVICE_PASSWORD_FLAG_PROVIDE           = "provide"
HAL_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = "provide-on-device"
HAL_DEVICE_PASSWORD_FLAG_CHECKING          = "checking"
HAL_DEVICE_PASSWORD_FLAG_UNLOCKED          = "unlocked"

	
AUTH_TOOLS_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)),"../tools")
CLI_TOOL = os.path.join(AUTH_TOOLS_PATH,"authcli.py")
GUI_TOOL = os.path.join(AUTH_TOOLS_PATH,"authgui.py")
	
###############################################################################
# IsAuthRequired
#
# Is authorization required to use this device?
	
def IsAuthRequired(device):
	if re.compile('/org/freedesktop/Hal/devices/').match(device.object_path) != None:
		flags = device.GetPropertyString("pda.pocketpc.password")
		if flags == "provide":
			return True
		return False

	flags = device.GetPasswordFlags()
        if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
		return True
	return False

############################################################################
# Authorize
#
# Obtain an authorization token to continue device connection. If this
# fails, we return False and the device will remain disconnected. Success
# will return True and allow the device to proceed with connection

def Authorize(devpath,device,config):
	
	rc=1
        if IsAuthRequired(device):

		# get the program we need
		
		prog = config.cfg["AuthMethod"]
		
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
				rc = 0
			if not IsAuthRequired(device):
				rc = 1
			else:
				rc = 0
		else:
			print "auth: auth prog %s does not exist" % prog
			rc = 0

	return rc
