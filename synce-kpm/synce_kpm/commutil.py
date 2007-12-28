import sys

from pyrapi2 import *
from rapiutil import *
import libxml2
import xml2util
import characteristics
import logging
from Observable import *
from CeDevice import *

import dbus
import dbus.glib





class PhoneCommunicator(Observable):
    def __init__(self):
        super(PhoneCommunicator,self).__init__()
        self.phoneConnected = False
        self.rapi_session = None
        self.checkConnection()
        
        bus = dbus.SystemBus()
        self.bus = bus

        
        try:
            proxy_obj_dbus = bus.get_object("org.freedesktop.DBus", "/org/freedesktop/DBus")
        except dbus.DBusException:
            print "Error: Could not connect to dbus. Is it started?"
            print "This program relies on DBus, please make sure it is running"
            sys.exit(1) 


        mgrOdccm = dbus.Interface(proxy_obj_dbus, "org.freedesktop.DBus")
        mgrOdccm.connect_to_signal("NameOwnerChanged", self.odccm_status_changed_cb)

        try:
            proxy_obj = bus.get_object("org.synce.odccm", "/org/synce/odccm/DeviceManager")
            self.odccmRunning = True
        except dbus.DBusException:
            self.odccmRunning = False

        if self.odccmRunning:
            try:
                self.mgr = dbus.Interface(proxy_obj, "org.synce.odccm.DeviceManager")

                self.mgr.connect_to_signal("DeviceConnected", self.device_connected_cb)
                self.mgr.connect_to_signal("DeviceDisconnected", self.device_disconnected_cb)

                if len(self.mgr.GetConnectedDevices()) > 0:
                    self.device = CeDevice( self.bus, self.mgr.GetConnectedDevices()[0] )
                    self.phoneConnected = True
                    self.sendMessage("model changed")
            except:
                print "Odccm seems to be running, but there are problems with connecting to it"
                raise


        if self.odccmRunning == False:
            print "Waiting for odccm to start"
        else:
            print "Waiting for device to hotplug"



    def odccm_status_changed_cb(self, obj_path, param2, param3):
        if obj_path == "org.synce.odccm":
            #If this parameter is empty, the odccm just came online 
            if param2 == "":
                self.odccmRunning = True

            #If this parameter is empty, the odccm just went offline
            if param3 == "":
                self.odccmRunning = False
                print "Waiting for odccm to start"
            
            
            if self.odccmRunning:
                try:
                    proxy_obj = self.bus.get_object("org.synce.odccm", "/org/synce/odccm/DeviceManager")
                    mgr = dbus.Interface(proxy_obj, "org.synce.odccm.DeviceManager")

                    mgr.connect_to_signal("DeviceConnected", self.device_connected_cb)
                    mgr.connect_to_signal("DeviceDisconnected", self.device_disconnected_cb)

                    for obj_path in mgr.GetConnectedDevices():
                        self._add_device(obj_path, False)
                    
                    print "Waiting for device to hotplug"
                except:
                    print "Something went really wrong, odccm came online, but we can't connect..."
                    raise




    def device_connected_cb(self, obj_path):
        self.device = CeDevice( self.bus, obj_path )
        self.phoneConnected = True
        self.sendMessage("model changed")

    def device_disconnected_cb(self, obj_path):
        self.device = None
        self.phoneConnected = False
        self.sendMessage("model changed")


    def checkConnection(self):
        before = self.phoneConnected
        try:
            self.rapi_session = RAPISession(0)
            self.phoneConnected = True
        except:
			self.phoneConnected = False
        

    def getPowerStatus(self):
        self.checkConnection() 

        if self.phoneConnected:
            powerStatus = self.rapi_session.getSystemPowerStatus(True)
            return ( powerStatus["BatteryLifePercent"], powerStatus["BatteryFlag"] , powerStatus["ACLineStatus"] )

        return (0,0,0)
        



    def getListInstalledPrograms(self):
        result = []
        #if no phone is connected, try to build up connection
        self.checkConnection()

		#if we have connection, use it :)
        if self.phoneConnected:
            try:
                for program in config_query_get(self.rapi_session, None ,   "UnInstall").children.values():
                    result.append( program.type )
                    #print "",program.type
            except:
                self.phoneConnected = False
        
        return result	
