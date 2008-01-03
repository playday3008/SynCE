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

from threading import Thread
from threading import Timer
import time


ACTION_PHONE_DISCONNECTED         = 0
ACTION_PHONE_CONNECTED            = 1
ACTION_POWERSTATUS_CHANGED        = 2
ACTION_STORAGE_CHANGED            = 3
ACTION_INSTALLED_PROGRAMS_CHANGED = 4

TIMER_INTERVAL = 15 

class PhoneCommunicator(Observable):
    def __init__(self, observer=None):
        super(PhoneCommunicator,self).__init__()

        self.phoneConnected = False
        self.rapi_session = None

        self.checkTimer = None
        self.savedDevice= None

    

        self._listInstalledPrograms = []
        self.powerStatus = None


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
                    #The device was already connected, this means that we can 
                    #call the callback function with extra param with value True
                    self.device_connected_cb( self.mgr.GetConnectedDevices()[0] , True)
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

                    if len(mgr.GetConnectedDevices()) > 0:
                        #The device was already connected, this means that we can 
                        #call the callback function with extra param with value True
                        self.device_connected_cb( self.mgr.GetConnectedDevices()[0] , True)
                    
                    print "Waiting for device to hotplug"
                except:
                    print "Something went really wrong, odccm came online, but we can't connect..."
                    raise

    def timer_cb(self):
        #update the battery status
        #update the list of installed programs
        self.updateListInstalledPrograms()
        self.updatePowerStatus()
        self.checkTimer = Timer(TIMER_INTERVAL, self.timer_cb)
        self.checkTimer.start()



    def device_connected_cb(self, obj_path, alreadyConnected=False):
        self.device = CeDevice( self.bus, obj_path )
        self.savedDevice = CeDevice( self.bus, obj_path ) 
        try:
            self.rapi_session = RAPISession(0)
            self.phoneConnected = True
        except:
			self.phoneConnected = False
        
        if self.phoneConnected:
            if not alreadyConnected:
                self.sendMessage(ACTION_PHONE_CONNECTED)
            
            self.checkTimer = Timer(0.1, self.timer_cb)
            self.checkTimer.start()
            print "Just started timer"

    def device_disconnected_cb(self, obj_path):
        self.device = None
        self.phoneConnected = False
        self.powerStatus = None
        self.sendMessage(ACTION_PHONE_DISCONNECTED)
        #self.checkerThread = None
        if self.checkTimer is not None:
            self.checkTimer.cancel()
            self.checkTimer = None
            print "Just canceled timer"


    #This function might be used to make a threaded version of the copier in future
    def installProgram(self, localFilenameAndPath, copyToDirectory, deleteCab, progressCallback=None):
        self._installProgram(localFilenameAndPath, copyToDirectory, deleteCab, progressCallback)


    def _installProgram(self, localFilenameAndPath, copyToDirectory, deleteCab, progressCallback=None):
        fileName = os.path.basename (localFilenameAndPath)
        fileSize = os.stat( localFilenameAndPath ).st_size
        
        RAPI_BUFFER_SIZE = 65535

        fileHandle = self.rapi_session.createFile("%s%s"%(copyToDirectory,fileName), GENERIC_WRITE , 0, CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL )
        if fileHandle != 0xffffffff:
            print "Copying file to device"


            fileObject = open(localFilenameAndPath,"rb")
            read_buffer = fileObject.read(RAPI_BUFFER_SIZE)

            bytesWritten = 0 

            if progressCallback is not None:
                progressCallback( (100 * bytesWritten)/fileSize )

            while len(read_buffer) > 0:
                self.rapi_session.writeFile( fileHandle, read_buffer, len(read_buffer) )
                bytesWritten += len(read_buffer) 
                if progressCallback is not None:
                    progressCallback( (100 * bytesWritten)/fileSize )

                read_buffer = fileObject.read(RAPI_BUFFER_SIZE)

            print "Closing filehandle..."
            returnValue = self.rapi_session.closeHandle(fileHandle)
            fileObject.close()

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
        return self.powerStatus


    def updatePowerStatus(self):
        oldPowerStatus = self.powerStatus
        if self.phoneConnected:
            self.powerStatus = self.rapi_session.getSystemPowerStatus(True)
        else:
            self.powerStatus = None

        if oldPowerStatus != self.powerStatus:
            self.sendMessage( ACTION_POWERSTATUS_CHANGED )





    def stopAllThreads(self):
        if self.checkTimer is not None:
            if self.checkTimer.isAlive:
                self.checkTimer.cancel()
                self.checkTimer = None
    
    
    def getListInstalledPrograms(self):
        return self._listInstalledPrograms

    def updateListInstalledPrograms(self):
        old_listInstalledPrograms = self._listInstalledPrograms

        self._listInstalledPrograms = []
        #if no phone is connected, try to build up connection

		#if we have connection, use it :)
        if self.phoneConnected:
            try:
                for program in config_query_get(self.rapi_session, None ,   "UnInstall").children.values():
                    self._listInstalledPrograms.append( program.type )
            except:
                self.phoneConnected = False
        if old_listInstalledPrograms != self._listInstalledPrograms:
            self.sendMessage( ACTION_INSTALLED_PROGRAMS_CHANGED )
        


