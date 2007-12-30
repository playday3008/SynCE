#    Copyright (C) 2007 Guido Diepen
#    Email: Guido Diepen <guido@guidodiepen.nl>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
############################################################################## 


import sys
import os
import re

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


ACTION_PHONE_DISCONNECTED = 0
ACTION_PHONE_CONNECTED    = 1
ACTION_BATTERY_CHANGED    = 2
ACTION_STORAGE_CHANGED    = 3


class PhoneCommunicator(Observable):
    def __init__(self, observer=None):
        super(PhoneCommunicator,self).__init__()
        self.phoneConnected = False
        self.rapi_session = None

##Make the program DBus aware. Furthermore, make sure that the program just 
##keeps working whenever odccm is not running
        bus = dbus.SystemBus()
        self.bus = bus

        if observer is not None:
            self.addListener( observer )

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
           

            #odccm just came online, make sure that we are going to listen to
            #the messages
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
        self.sendMessage(ACTION_PHONE_CONNECTED)

    def device_disconnected_cb(self, obj_path):
        self.device = None
        self.phoneConnected = False
        self.sendMessage(ACTION_PHONE_DISCONNECTED)


    def checkConnection(self):
        before = self.phoneConnected
        try:
            self.rapi_session = RAPISession(0)
            self.phoneConnected = True
        except:
			self.phoneConnected = False

    def installProgram(self, localFilenameAndPath, copyToDirectory, deleteCab):
        fileName = os.path.basename (localFilenameAndPath)
        fileHandle = self.rapi_session.createFile("%s%s"%(copyToDirectory,fileName), GENERIC_WRITE , 0, CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL )
        if fileHandle != 0xffffffff:
            print "Copying file to device"
            fileObject = open(localFilenameAndPath,"rb")
            read_buffer = fileObject.read(65535)
            while len(read_buffer) > 0:
                self.rapi_session.writeFile( fileHandle, read_buffer, len(read_buffer) )
                read_buffer = fileObject.read(65535)

            print "Closing filehandle..."
            returnValue = self.rapi_session.closeHandle(fileHandle)
            fileObject.close()

#            applicationName = "wceload.exe"
#            applicationParms = "%s%s"%(copyToDirectory,programName)
#            if not deleteCab:
#                applicationParms += " /nodelete"
#            
#            print "%s %s"%(applicationName,applicationParms)
#            print "Programname: %s"%programNameAndPath

            applicationName = "wceload.exe"
            applicationParms = "%s%s"%(copyToDirectory,fileName)
            if not deleteCab:
                applicationParms += " /nodelete"
            result = self.rapi_session.createProcess( applicationName, applicationParms )

    def uninstallProgram(self, programName):
        doc = libxml2.newDoc("1.0")
        doc_node = doc.newChild(None,"wap-provisioningdoc",None)
        parent=doc_node

        node = parent.newChild(None,"characteristic",None)
        node.setProp("type", "UnInstall") ; 

        parent = node ; 
        node = parent.newChild(None,"characteristic",None)
        node.setProp("type", unicode(programName)) ; 

        parent = node ;
        node = parent.newChild(None,"parm",None)
        node.setProp("name", "uninstall") ; 
        node.setProp("value", "1") ; 

        print "_config_query: CeProcessConfig request is \n", doc_node.serialize("utf-8",1)
        reply = self.rapi_session.process_config(doc_node.serialize("utf-8",0), 1)
        reply_doc = libxml2.parseDoc(reply)

        print "_config_query: CeProcessConfig response is \n", reply_doc.serialize("utf-8",1)

        reply_node = xml2util.GetNodeOnLevel(reply_doc, 2 )


    

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
