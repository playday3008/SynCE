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
from threading import *
import time


ACTION_PHONE_DISCONNECTED         = 0
ACTION_PHONE_CONNECTED            = 1
ACTION_PASSWORD_NEEDED			  = 2
ACTION_PASSWORD_NEEDED_ON_DEVICE  = 3
ACTION_PHONE_AUTHORIZED           = 4
ACTION_POWERSTATUS_CHANGED        = 5
ACTION_STORAGE_CHANGED            = 6
ACTION_INSTALLED_PROGRAMS_CHANGED = 7
ACTION_DEVICE_OWNER_CHANGED       = 8

TIMER_INTERVAL = 15 

ODCCM_DEVICE_PASSWORD_FLAG_SET               = 1
ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE           = 2
ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE = 4

class PhoneCommunicator(Observable):
    def __init__(self, observer=None):
        super(PhoneCommunicator,self).__init__()

        self._stopTimer = False
        self.phoneConnected = False
        self.rapi_session = None

        self._sm_phone_connected        = None
        self._sm_phone_disconnected     = None
        self._sm_password_flags_changed = None

        self.checkTimer = None
        self.deviceName= None
        self._listInstalledPrograms = []
        self.powerStatus = None
        self.deviceOwner = ""
        self.storageInformation = []



        if observer is not None:
            self.addListener( observer )

        
        
        ##Make the program DBus aware. Furthermore, make sure that the 
        ##program just keeps working whenever odccm is not running
        bus = dbus.SystemBus()
        self.bus = bus

        try:
            proxy_obj_dbus = bus.get_object("org.freedesktop.DBus", 
                                            "/org/freedesktop/DBus")
        except dbus.DBusException:
            print "Error: Could not connect to dbus. Is it started?"
            print "This program relies on DBus, please make sure it is running"
            sys.exit(1) 


        mgrOdccm = dbus.Interface(proxy_obj_dbus, "org.freedesktop.DBus")
        mgrOdccm.connect_to_signal("NameOwnerChanged", 
                                    self.odccm_status_changed_cb)

        try:
            proxy_obj = bus.get_object("org.synce.odccm", 
                                        "/org/synce/odccm/DeviceManager")
            self.odccmRunning = True
        except dbus.DBusException:
            self.odccmRunning = False

        if self.odccmRunning:
            try:
                self.mgr = dbus.Interface(proxy_obj, 
                                          "org.synce.odccm.DeviceManager")

                self._sm_phone_connected    = self.mgr.connect_to_signal(
                                                    "DeviceConnected", 
                                                    self.device_connected_cb)
                self._sm_phone_disconnected = self.mgr.connect_to_signal(
                                                    "DeviceDisconnected", 
                                                    self.device_disconnected_cb)

                if len(self.mgr.GetConnectedDevices()) > 0:
                    #The device was already connected, this means that we can 
                    #call the callback function with extra param with value True
                    self.device_connected_cb( self.mgr.GetConnectedDevices()[0] )
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
                    proxy_obj = self.bus.get_object("org.synce.odccm", 
                                                    "/org/synce/odccm/DeviceManager")
                    mgr = dbus.Interface(proxy_obj, "org.synce.odccm.DeviceManager")

                    self._sm_phone_connected = mgr.connect_to_signal("DeviceConnected", 
                                                                    self.device_connected_cb)
                    self._sm_phone_disconnected = mgr.connect_to_signal("DeviceDisconnected", 
                                                                        self.device_disconnected_cb)

                    if len(mgr.GetConnectedDevices()) > 0:
                        #The device was already connected, this means that we can 
                        #call the callback function with extra param with value True
                        self.device_connected_cb( self.mgr.GetConnectedDevices()[0] )
                    
                    print "Waiting for device to hotplug"
                except:
                    print "Something went really wrong, odccm came online, but we can't connect..."
                    raise
            else:
                self._sm_phone_disconnected.remove() 
                self._sm_phone_connected.remove() 

    def timer_cb(self):
        #update the battery status
        #update the list of installed programs
        self.updateListInstalledPrograms()
        self.updatePowerStatus()
        self.updateDeviceOwner()
        self.updateStorageInformation()

        if not self._stopTimer:
            self.checkTimer = Timer(TIMER_INTERVAL, self.timer_cb)
            self.checkTimer.start()

    def processAuthorization(self, password):
        self.device.ProvidePassword( password )
        #authenticated = self.dev_iface.ProvidePassword(dlg.get_text())


    #As soon as a phone connects, notify the listeners about this
    #NB: This is only connect, there is not yet a rapi connection to the
    #    phone, this will be available as soon after onAuthorized is
    #    called
    def onConnect(self):
        self.deviceName = self.device.GetName()
        self.sendMessage(ACTION_PHONE_CONNECTED)

    #This means that the phone has been authorized. When a device
    #was not locked, this happens right away, if a device was locked
    #then this function will be called after the user has unlocked 
    #the device
    #Only after a device is authorized, the connections can be started
    def onAuthorized(self):
        try:
            self.rapi_session = RAPISession(0)
            self.phoneConnected = True
        except:
            self.phoneConnected = False

        if self.phoneConnected:
            self.sendMessage(ACTION_PHONE_AUTHORIZED)
            self.checkTimer = Timer(0.1, self.timer_cb)
            self.checkTimer.start()


    def password_flags_changed_cb( self, added, removed ):
        if removed & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
            self.onAuthorized()
        if removed & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
            self.onAuthorized()


    def device_connected_cb(self, obj_path):
        deviceObject = dbus.SystemBus().get_object("org.synce.odccm",obj_path)
        self.device = dbus.Interface(deviceObject,"org.synce.odccm.Device")

        #Start listening to dbus for changes in the status of authorization
        self._sm_password_flags_changed = self.device.connect_to_signal("PasswordFlagsChanged", self.password_flags_changed_cb)

        self.onConnect()

        flags = self.device.GetPasswordFlags()
        
        if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
            #This means the WM5 style
            self.sendMessage(ACTION_PASSWORD_NEEDED)
            return 

        if flags & ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
            #print "Dealing with a WM6 phone, user must unlock device on device itself"
            self.sendMessage(ACTION_PASSWORD_NEEDED_ON_DEVICE)
            return 

        #If the device is not locked at all, then we can build up rapi connections
        #and notify all listeners. This is done by the onAuthorized method.
        self.onAuthorized()


    def device_disconnected_cb(self, obj_path):
        self.device = None
        self.phoneConnected = False
        self.powerStatus = None
        self._listInstalledPrograms = []

        #Cancel the callback signal match
        self._sm_password_flags_changed.remove() ; 
       
        #Notify the listeners 
        self.sendMessage(ACTION_PHONE_DISCONNECTED)
        
        if self.checkTimer is not None:
            self.checkTimer.cancel()
            self.checkTimer = None


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

        self.sendMessage( ACTION_INSTALLED_PROGRAMS_CHANGED )


    
    
    def getPowerStatus(self):
        return self.powerStatus


    def updatePowerStatus(self):
        oldPowerStatus = self.powerStatus

        self.powerStatus = None
        if self.phoneConnected:
            self.powerStatus = self.rapi_session.getSystemPowerStatus(True)

        if oldPowerStatus != self.powerStatus:
            self.sendMessage( ACTION_POWERSTATUS_CHANGED )





    def stopAllThreads(self):
        if self.checkTimer is not None:
            self._stopTimer = True
            a = self.checkTimer.cancel()
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

    def getDeviceOwner(self):
        return self.deviceOwner

    def updateDeviceOwner(self):
        old_deviceOnwer = self.deviceOwner 
        
        self.deviceOwner = ""

        if self.phoneConnected:
            try:
                hkcu = self.rapi_session.HKEY_CURRENT_USER
                owner_key = hkcu.open_sub_key(r"ControlPanel\Owner")
                self.deviceOwner = owner_key.query_value("Name")
            except:
                print "Something went wrong with fetching the info from the registry"
        if self.deviceOwner != old_deviceOnwer:
            self.sendMessage( ACTION_DEVICE_OWNER_CHANGED )


    def getIpAddress(self):
        return self.device.GetIpAddress()

    def getModelName(self):
        return self.device.GetModelName()


    def getStorageInformation(self):
        return self.storageInformation
    
    def updateStorageInformation(self):
        old_storageInformation = self.storageInformation 
        
        self.storageInformation = [] 

        if self.phoneConnected:
            #Amount free on mainMemory:
            freeDisk,totalDisk,freeDiskTotal = self.rapi_session.getDiskFreeSpaceEx("\\")
            self.storageInformation.append( ("MainMemory","\\" ,freeDisk,totalDisk,freeDiskTotal) )

            myFiles = self.rapi_session.findAllFiles("\\*.*",FAF_FOLDERS_ONLY|FAF_ATTRIBUTES| FAF_NAME)
            
            for folder in myFiles:
                if folder["Attributes"] & FILE_ATTRIBUTE_TEMPORARY:
                    freeDisk,totalDisk,freeDiskTotal = self.rapi_session.getDiskFreeSpaceEx("\\"+folder["Name"]+"\\")
                    self.storageInformation.append( ("/%s"%folder["Name"],"\\%s\\"%folder["Name"], freeDisk,totalDisk,freeDiskTotal) )

        if self.storageInformation != old_storageInformation:
            self.sendMessage( ACTION_STORAGE_CHANGED )


