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
import gtk
import sys
import re

ODCCM_DEVICE_PASSWORD_FLAG_SET     = 1
ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE = 2

HAL_DEVICE_PASSWORD_FLAG_UNSET             = "unset"
HAL_DEVICE_PASSWORD_FLAG_PROVIDE           = "provide"
HAL_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = "provide-on-device"
HAL_DEVICE_PASSWORD_FLAG_CHECKING          = "checking"
HAL_DEVICE_PASSWORD_FLAG_UNLOCKED          = "unlocked"

# EntryDialog
#
# Password entry dialog for GUI entry of password
#

class EntryDialog(gtk.Dialog):
    def __init__(self, parent, title, text, password=False):
        gtk.Dialog.__init__(self, title, parent,
                            gtk.DIALOG_MODAL | gtk.DIALOG_DESTROY_WITH_PARENT | gtk.DIALOG_NO_SEPARATOR,
                            (gtk.STOCK_CANCEL, gtk.RESPONSE_REJECT,
                            gtk.STOCK_OK, gtk.RESPONSE_ACCEPT | gtk.CAN_DEFAULT))

        self.set_default_response(gtk.RESPONSE_ACCEPT)
        label = gtk.Label(text)
        label.set_alignment(0.0, 0.5)
        self.vbox.pack_start(label, False)
        self._label = label

        entry = gtk.Entry()
        entry.set_visibility(not password)
	entry.set_activates_default(True)
        self.vbox.pack_start(entry, False, True, 5)
        self._entry = entry

        self.show_all()

    def get_text(self):
        return self._entry.get_text()

# 
# AuthGui
#
# Application class.

class AuthGui:
	
	def __init__(self,objpath):

		bus = dbus.SystemBus()

		if re.compile('/org/freedesktop/Hal/devices/').match(objpath) != None:
			self.deviceObject = bus.get_object("org.freedesktop.Hal", objpath)
			self.device = dbus.Interface(self.deviceObject, "org.freedesktop.Hal.Device")
			self.deviceName = self.device.GetPropertyString("pda.pocketpc.name")
			return

		self.deviceObject = bus.get_object("org.synce.odccm", objpath)
		self.device = dbus.Interface(self.deviceObject, "org.synce.odccm.Device")
		self.deviceName = self.device.GetName()
	
	def Authorize(self):
		
		# no need to run if for some reason we are called on a 
		# device that is not blocked

		if re.compile('/org/freedesktop/Hal/devices/').match(self.device.object_path) != None:
			flags = self.device.GetPropertyString("pda.pocketpc.password")
			rc=1
			if flags == "provide":
				stopAsking = False
				while not stopAsking:
					dlg = EntryDialog(None,	"SynCE: Password required to synchronize device",
								"Enter password for device '%s'" % self.deviceName,
								True)

					if dlg.run() == gtk.RESPONSE_ACCEPT:
						stopAsking = self.deviceObject.ProvidePassword(dlg.get_text(), dbus_interface='org.freedesktop.Hal.Device.Synce')
						if stopAsking:
							rc=1
					else:
		    				stopAsking = True
						rc=0
        	        		dlg.destroy()
			return rc

		
		flags = self.device.GetPasswordFlags()
		rc=1
		if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
			stopAsking = False
			while not stopAsking:
				dlg = EntryDialog(None,	"SynCE: Password required to synchronize device",
							"Enter password for device '%s'" % self.deviceName,
							True)

				if dlg.run() == gtk.RESPONSE_ACCEPT:
					stopAsking = self.device.ProvidePassword(dlg.get_text())
					if stopAsking:
						rc=1
				else:
		    			stopAsking = True
					rc=0
                		dlg.destroy()
		return rc

#
# main
#
# Get the objpath from the command line, then authorize

if len(sys.argv) > 1:
	devobjpath = sys.argv[1]
	app = AuthGui(devobjpath)
	sys.exit(app.Authorize())
else:
	sys.exit(0)
