############################################################################## 
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

import gobject
import time

import os 

import dbus
import dbus.service
import dbus.mainloop.glib

from pyrapi2 import *
from synceKPM.dataserver.rapiutil import *

import synceKPM.constants


class DataServer(dbus.service.Object):
    def __init__(self, busConn, dataServerEventLoop):
        dbus.service.Object.__init__(self, busConn, synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_OBJPATH)
        self.dataServerEventLoop = dataServerEventLoop 
        self.busConn = busConn
        
        self.deviceIsConnected = False 

        self.odccm_device = None
        self.hal_device = None
        self.syncEngineRunning = False


        #
        # These are for saving the signal matches for the D-Bus callbacks
        #Whenever the phone disconnects, we must disconnect the callbacks
        #also
        self._sm_odccm_phone_connected        = None
        self._sm_odccm_phone_disconnected     = None
        self._sm_odccm_password_flags_changed = None

        
        self._sm_syncengine_status_set_max_value         = None
        self._sm_syncengine_status_set_value             = None
        self._sm_syncengine_status_set_status_string     = None

        self._sm_syncengine_status_sync_start            = None
        self._sm_syncengine_status_sync_end              = None

        self._sm_syncengine_status_sync_start_partner    = None
        self._sm_syncengine_status_sync_end_partner      = None
        
        self._sm_syncengine_status_sync_start_datatype   = None
        self._sm_syncengine_status_sync_end_datatype     = None

        self._sm_syncengine_partnerships_changed		 = None
        
        self._programList = None
        self._powerStatus = None
        self._storageInformation = None

        self.deviceName   = ""
        self.deviceHalObjPath = ""




    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def deviceConnected(self, deviceName, alreadyConnected):
        pass
        #print "Device %s got connected"

    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def deviceAuthorized(self):
        pass

    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def deviceDisconnected(self, deviceName):
        pass
        #print "Device %s just got disconnected"




    def odccm_password_flags_changed_cb( self, added, removed ):
        if removed & synceKPM.constants.ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
            self.onAuthorized()
            pass
        if removed & synceKPM.constants.ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
            pass
            self.onAuthorized()


    def hal_password_flags_changed_cb( self, num_changes, properties ):
        for property in properties:
            property_name, added, removed = property
            if property_name == "pda.pocketpc.password":
                flag = self.hal_device.GetPropertyString("pda.pocketpc.password")

                if flag == synceKPM.constants.SYNCE_DEVICE_PASSWORD_FLAG_UNSET:
                    self.onAuthorized()
                if flag == synceKPM.constants.SYNCE_DEVICE_PASSWORD_FLAG_UNLOCKED:
                    self.onAuthorized()

    
    
    def odccm_device_disconnected_cb(self, obj_path ):
        self.deviceDisconnected( self.deviceName )
        self.odccm_device = None
        self.deviceName   = ""
        self.deviceIsConnected = False

        self._programList = []
        pass


    def odccm_device_connected_cb(self, obj_path, alreadyConnected=False ):
        deviceObject = dbus.SystemBus().get_object("org.synce.odccm",obj_path)
        self.odccm_device = dbus.Interface(deviceObject,"org.synce.odccm.Device")
        self.deviceName = self.odccm_device.GetName()
        self.deviceModelName = self.odccm_device.GetModelName()

        self.deviceConnected(self.deviceName,alreadyConnected)

        __deviceOsVersion=self.odccm_device.GetOsVersion()
        self.deviceOsVersion( __deviceOsVersion )


        #Start listening to dbus for changes in the status of authorization
        self._sm_odccm_password_flags_changed = self.odccm_device.connect_to_signal("PasswordFlagsChanged", self.odccm_password_flags_changed_cb)
        

        #self.onConnect()

        flags = self.odccm_device.GetPasswordFlags()
        
        if flags & synceKPM.constants.ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE:
            #This means the WM5 style
            #self.sendMessage(ACTION_PASSWORD_NEEDED)
            self.UnlockDeviceViaHost()
            return 

        if flags & synceKPM.constants.ODCCM_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
            #print "Dealing with a WM6 phone, user must unlock device on device itself"
            #self.sendMessage(ACTION_PASSWORD_NEEDED_ON_DEVICE)
            self.UnlockDeviceViaDevice()
            return 

        #If the device is not locked at all, then we can build up rapi connections
        #and notify all listeners. This is done by the onAuthorized method.
        self.onAuthorized()

        #pass



    def hal_device_connected_cb(self, obj_path, alreadyConnected=False ):
        deviceObject = dbus.SystemBus().get_object("org.freedesktop.Hal",obj_path)
        device = dbus.Interface(deviceObject,"org.freedesktop.Hal.Device")

        if not device.PropertyExists("pda.pocketpc.name"):
            device = None
            return
        self.hal_device = device
        self.deviceHalObjPath = obj_path

        self.hal_device_synce = dbus.Interface(deviceObject,"org.freedesktop.Hal.Device.Synce")

        self.deviceName = self.hal_device.GetPropertyString("pda.pocketpc.name")
        self.deviceModelName = self.hal_device.GetPropertyString("pda.pocketpc.model")

        self.deviceConnected(self.deviceName,alreadyConnected)

        os_major = self.hal_device.GetPropertyInteger("pda.pocketpc.os_major")
        os_minor = self.hal_device.GetPropertyInteger("pda.pocketpc.os_minor")
        __deviceOsVersion=[os_major,os_minor]
        self.deviceOsVersion( __deviceOsVersion )

        #Start listening to Hal for changes in the status of authorization
        self._sm_hal_password_flags_changed = self.hal_device.connect_to_signal("PropertyModified", self.hal_password_flags_changed_cb)

        #self.onConnect()

        flags = self.hal_device.GetPropertyString("pda.pocketpc.password")
        
        if flags == synceKPM.constants.SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE:
            #This means the WM5 style
            #self.sendMessage(ACTION_PASSWORD_NEEDED)
            self.UnlockDeviceViaHost()
            return 

        if flags == synceKPM.constants.SYNCE_DEVICE_PASSWORD_FLAG_PROVIDE_ON_DEVICE:
            #print "Dealing with a WM6 phone, user must unlock device on device itself"
            #self.sendMessage(ACTION_PASSWORD_NEEDED_ON_DEVICE)
            self.UnlockDeviceViaDevice()
            return 

        #If the device is not locked at all, then we can build up rapi connections
        #and notify all listeners. This is done by the onAuthorized method.
        self.onAuthorized()

        #pass


    def hal_device_disconnected_cb(self, obj_path):
        if obj_path == self.deviceHalObjPath:
            print "disconnecting device ", self.deviceName , " obj_path = ", obj_path
            self.deviceHalObjPath = ""
            self.deviceDisconnected( self.deviceName )
            self.hal_device = None
            self.hal_device_synce = None
            self.deviceName   = ""
            self.deviceIsConnected = False
            self._programList = []
        pass



    def onAuthorized(self):
        #Only now the device is actually connected!!
        self.deviceAuthorized()
        self.deviceIsConnected = True
        
        self.DeviceOwner( self.getDeviceOwner() )

        if self.hal_device != None:
            self.DeviceModel( self.hal_device.GetPropertyString("pda.pocketpc.model") )
        else:
            self.DeviceModel( self.odccm_device.GetModelName() )
        
        self.updateDeviceInformation()


        try:
            self.busConn.get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
            self.handleSyncEngineStatusChange( True, True ) 
        except dbus.DBusException:
            self.handleSyncEngineStatusChange( False, False) 
            print "SyncEngine is NOT running!!"



        gobject.timeout_add(45000, self.updateDeviceInformation)
        
        pass

    def updateDeviceInformation(self):
        if not self.deviceIsConnected:
            return False


        self.updatePowerStatus()
        self.updateListInstalledPrograms()
        self.updateStorageInformation()
        #self.updatePartnerships()
        
        return True




    @dbus.service.signal('org.synce.kpm.DataServerInterface', signature='a(ssttt)')
    def StorageInformation(self, storageInformation):
        pass

    def updateStorageInformation(self):
        old_storageInformation = self._storageInformation 
        
        self._storageInformation = [] 

        if self.deviceIsConnected:
            #Amount free on mainMemory:
            rapi_session = RAPISession(0)
            freeDisk,totalDisk,freeDiskTotal = rapi_session.getDiskFreeSpaceEx("\\")
            self._storageInformation.append( ("MainMemory","\\" ,freeDisk,totalDisk,freeDiskTotal) )

            myFiles = rapi_session.findAllFiles("\\*.*",FAF_FOLDERS_ONLY|FAF_ATTRIBUTES| FAF_NAME)
            
            for folder in myFiles:
                if folder["Attributes"] & FILE_ATTRIBUTE_TEMPORARY:
                    try:
                        freeDisk,totalDisk,freeDiskTotal = rapi_session.getDiskFreeSpaceEx("\\"+folder["Name"]+"\\")
                        self._storageInformation.append( ("/%s"%folder["Name"],"\\%s\\"%folder["Name"], freeDisk,totalDisk,freeDiskTotal) )
                    except pyrapi2.RapiError, e:
                        print "Failed to get disk space for %s, skipping: %d: %s" % (folder["Name"], e.err_code, e)


        if self._storageInformation != old_storageInformation:
            self.StorageInformation( self._storageInformation )



    
    @dbus.service.signal('org.synce.kpm.DataServerInterface', signature='a{us}a(ussssau)')
    def DevicePartnerships(self, sync_items, partnerships):
        pass


    @dbus.service.method(synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE, in_signature='', out_signature='')
    def updatePartnerships(self):
        if self.syncEngineRunning and self.deviceIsConnected:

            syncEngineProxy = self.busConn.get_object("org.synce.SyncEngine","/org/synce/SyncEngine")
            syncEngineObj   = dbus.Interface(syncEngineProxy,"org.synce.SyncEngine")


            item_types = syncEngineObj.GetItemTypes()
            
            partnerships = []

            try:
                partnerships = syncEngineObj.GetPartnerships()
            except Exception,e:
                print "Error while getting partnerships..."
                return
                
            oldAPI = False

            try:
                for pship in partnerships:
                    id,guid,name,hostname,devicename,storetype,items = pship
            except Exception,e:
                oldAPI = True


            __updatedPartnerships = []
            if oldAPI:
                print "Using old partnership api of sync-engine"
                __updatedPartnerships = partnerships

            else:
                print "Using new partnership api of sync-engine"
                for pship in partnerships:
                    id,guid,name,hostname,devicename,storetype,items = pship
                    __updatedPartnerships.append( (id,guid,name,hostname,devicename,items) )
                 
        

            self.DevicePartnerships(item_types, __updatedPartnerships)
            

    def handleSyncEngineStatusChange(self, isOnline, wasAlreadyOnline):
        self.syncEngineRunning = isOnline 
        self.updateStatusListeners()
        
        self.SyncEngineStatusChange(isOnline, wasAlreadyOnline)

        
        #if isOnline:
        #    self.updatePartnerships()


    def updateStatusListeners(self):

        if self.syncEngineRunning:
            dbusSyncEngine = self.busConn.get_object( "org.synce.SyncEngine",
                                                "/org/synce/SyncEngine",
                                                "org.synce.SyncEngine")
            self._sm_syncengine_status_set_max_value     = dbusSyncEngine.connect_to_signal("StatusSetMaxProgressValue", self.handle_syncengine_status_set_max_value) 
            self._sm_syncengine_status_set_value         = dbusSyncEngine.connect_to_signal("StatusSetProgressValue", self.handle_syncengine_status_set_value) 
            self._sm_syncengine_status_set_status_string = dbusSyncEngine.connect_to_signal("StatusSetStatusString", self.handle_syncengine_status_set_status_string) 

            self._sm_syncengine_status_sync_start            =   dbusSyncEngine.connect_to_signal("StatusSyncStart"        ,self.handle_syncengine_status_sync_start)
            self._sm_syncengine_status_sync_end              =   dbusSyncEngine.connect_to_signal("StatusSyncEnd"          ,self.handle_syncengine_status_sync_end)
            self._sm_syncengine_status_sync_start_partner    =   dbusSyncEngine.connect_to_signal("StatusSyncStartPartner" ,self.handle_syncengine_status_sync_start_partner)
            self._sm_syncengine_status_sync_end_partner      =   dbusSyncEngine.connect_to_signal("StatusSyncEndPartner"   ,self.handle_syncengine_status_sync_end_partner)
            self._sm_syncengine_status_sync_start_datatype   =   dbusSyncEngine.connect_to_signal("StatusSyncStartDatatype",self.handle_syncengine_status_sync_start_datatype)
            self._sm_syncengine_status_sync_end_datatype     =   dbusSyncEngine.connect_to_signal("StatusSyncEndDatatype"  ,self.handle_syncengine_status_sync_end_datatype)
            self._sm_syncengine_partnerships_changed		 = dbusSyncEngine.connect_to_signal("PartnershipsChanged"  ,self.handleSyncEnginePartnershipsChanged )

            
        else:
            self._sm_syncengine_status_set_max_value         = None
            self._sm_syncengine_status_set_value             = None
            self._sm_syncengine_status_set_status_string     = None

            self._sm_syncengine_status_sync_start            = None
            self._sm_syncengine_status_sync_end              = None

            self._sm_syncengine_status_sync_start_partner    = None
            self._sm_syncengine_status_sync_end_partner      = None

            self._sm_syncengine_status_sync_start_datatype   = None
            self._sm_syncengine_status_sync_end_datatype     = None
            self._sm_syncengine_partnerships_changed		 = None


    def handleSyncEnginePartnershipsChanged(self):
        self.updatePartnerships()

    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatusChange(self, isOnline, wasAlreadyOnline):
        pass
    

    
    def handleOdccmStatusChange(self, isOnline):
        if isOnline:
            odccmProxy      = dbus.SystemBus().get_object("org.synce.odccm", "/org/synce/odccm/DeviceManager")
            odccmDevManager = dbus.Interface( odccmProxy, "org.synce.odccm.DeviceManager"  )

            self._sm_odccm_phone_connected   = odccmDevManager.connect_to_signal( "DeviceConnected", self.odccm_device_connected_cb )
            self._sm_odccm_phone_disconnected= odccmDevManager.connect_to_signal( "DeviceDisconnected", self.odccm_device_disconnected_cb )

            if len(odccmDevManager.GetConnectedDevices()) > 0:
                #The device was already connected, this means that we can 
                #call the callback function with extra param with value True
                self.odccm_device_connected_cb( odccmDevManager.GetConnectedDevices()[0], True )
        else:
            self._sm_odccm_device_connected    = None
            self._sm_odccm_device_disconnected = None



    def handleNameOwnerChange(self, obj_path, param2, param3):
        if obj_path == "org.synce.kpm.gui" and param3 == "":
            print "We have got a problem.... The GUI went offline...."
            print "We must close down the dataserver"
            self.ShutdownDataServer() 

        if obj_path == "org.synce.kpm.gui" and param2 == "":
            print "Aparently we were a bit too quick...."
        
        if obj_path == "org.synce.odccm":
            isOnline = True
            if param3 == "":
                isOnline = False
            self.handleOdccmStatusChange( isOnline )


        if obj_path == "org.synce.SyncEngine":
            print "something happened to syncengine!!"
            isOnline = True
            if param3 == "":
                isOnline = False
            self.handleSyncEngineStatusChange( isOnline, False )

    @dbus.service.method(synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE, in_signature='', out_signature='')
    def ShutdownDataServer(self):
        print "Shutting down DataServer"
        self.dataServerEventLoop.quit()


    @dbus.service.method(synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE, in_signature='', out_signature='')
    def InitializeDataServer(self):
        print "Initializing DataServer"
        #Start listening for changes regarding Sync-engine or the GUI on the sessionbus
        self.busConn.add_signal_receiver(self.handleNameOwnerChange, dbus_interface = "org.freedesktop.DBus", signal_name = "NameOwnerChanged")

        #Start listening for changes regarding odccm on the systembus
        dbus.SystemBus().add_signal_receiver(self.handleNameOwnerChange, dbus_interface = "org.freedesktop.DBus", signal_name = "NameOwnerChanged") 


        try:
            dbus.SystemBus().get_object("org.synce.odccm", "/org/synce/odccm/DeviceManager")
            self.handleOdccmStatusChange( True ) 
        except dbus.DBusException:
            print "odccm is NOT running!!"


        # connect to Hal device manager

        hal_manager = None
        try:
            hal_manager = dbus.Interface(dbus.SystemBus().get_object("org.freedesktop.Hal", "/org/freedesktop/Hal/Manager"), "org.freedesktop.Hal.Manager")
            self._sm_hal_device_connected = hal_manager.connect_to_signal("DeviceAdded", self.hal_device_connected_cb)
            self._sm_hal_device_disconnected = hal_manager.connect_to_signal("DeviceRemoved", self.hal_device_disconnected_cb)

        except:
            print "Problems connecting to Hal, is it running ?"

        if hal_manager != True:
            obj_paths = hal_manager.FindDeviceStringMatch("pda.platform", "pocketpc")
            if len(obj_paths) > 0:
                self.hal_device_connected_cb(obj_paths[0], True)


        try:
            self.busConn.get_object("org.synce.SyncEngine", "/org/synce/SyncEngine")
            self.handleSyncEngineStatusChange( True, True ) 
        except dbus.DBusException:
            self.handleSyncEngineStatusChange( False, False) 
            print "SyncEngine is NOT running!!"

     
    @dbus.service.method('org.synce.kpm.DataServerInterface')
    def updateListInstalledPrograms(self):
        old_programList = self._programList

        self._programList = []
        #if no phone is connected, try to build up connection

        if self.deviceIsConnected:
            try:
                rapi_session = RAPISession(0)
                _programs = config_query_get(rapi_session, None ,   "UnInstall").children.values()
                for program in _programs:
                    self._programList.append( program.type )
            except Exception,e:
                print "Error while retrieving list of installed programs:"
                print e
                self.deviceIsConnected = False

        
        if old_programList != self._programList:
            self.ListInstalledPrograms( self._programList )
    
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface', signature="as")
    def ListInstalledPrograms(self, programsList):
        pass





    
    def updatePowerStatus(self):
        oldPowerStatus = self._powerStatus

        
        try:
            rapi_session = RAPISession(0)
            self._powerStatus = rapi_session.getSystemPowerStatus(True)
        except Exception,e:
            print "Error while retrieving battery information:"
            print e
            self.deviceIsConnected = False

        self.PowerStatus( self._powerStatus )
        #if oldPowerStatus != self._powerStatus:
        #    self.PowerStatus( self._powerStatus )

    @dbus.service.signal('org.synce.kpm.DataServerInterface', signature="a{sv}")
    def PowerStatus(self, _status ):
        pass

    @dbus.service.signal('org.synce.kpm.DataServerInterface', signature="s")
    def DeviceModel(self, deviceModel):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface', signature="s")
    def DeviceOwner(self, _deviceOwner ):
        pass

    def getDeviceOwner(self):
        owner = ""
        

        if self.deviceIsConnected:
            try:
                rapi_session = RAPISession(0)
                hkcu = rapi_session.HKEY_CURRENT_USER
                owner_key = hkcu.open_sub_key(r"ControlPanel\Owner")
                owner = owner_key.query_value("Name")
            except:
                print "Something went wrong with fetching the info from the registry"
        
        return owner









    @dbus.service.method('org.synce.kpm.DataServerInterface')
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
        rapi_session = RAPISession(0)
        reply = rapi_session.process_config(doc_node.serialize("utf-8",0), 1)
        reply_doc = libxml2.parseDoc(reply)
        print "_config_query: CeProcessConfig response is \n", reply_doc.serialize("utf-8",1)

        reply_node = xml2util.GetNodeOnLevel(reply_doc, 2 )
        
        self.updateListInstalledPrograms()


    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def installationCopyProgress(self, progress):
        pass

    @dbus.service.method('org.synce.kpm.DataServerInterface')
    def installProgram(self, localFilenameAndPath, copyToDirectory, deleteCab):
        fileName = os.path.basename (localFilenameAndPath)
        fileSize = os.stat( localFilenameAndPath ).st_size
        
        RAPI_BUFFER_SIZE = 65535
        
        rapi_session = RAPISession(0)
        
        fileHandle = rapi_session.createFile("%s%s"%(copyToDirectory,fileName), GENERIC_WRITE , 0, CREATE_ALWAYS , FILE_ATTRIBUTE_NORMAL )
        
        if fileHandle != 0xffffffff:
            print "Copying file to device"

            fileObject = open(localFilenameAndPath,"rb")
            read_buffer = fileObject.read(RAPI_BUFFER_SIZE)

            bytesWritten = 0 

            self.installationCopyProgress( (100*bytesWritten) / fileSize )

            while len(read_buffer) > 0:
                rapi_session.writeFile( fileHandle, read_buffer, len(read_buffer) )
                bytesWritten += len(read_buffer) 
                self.installationCopyProgress( (100*bytesWritten) / fileSize )

                read_buffer = fileObject.read(RAPI_BUFFER_SIZE)


            print "Closing filehandle..."
            returnValue = rapi_session.closeHandle(fileHandle)

            fileObject.close()

            applicationName = "wceload.exe"
            #Add the quotes to make sure paths with spaces are no problem
            applicationParms = "\"%s%s\""%(copyToDirectory,fileName)
            if not deleteCab:
                applicationParms += " /nodelete"
            result = rapi_session.createProcess( applicationName, applicationParms )
            self.CabInstallStarted()
         
    
    @dbus.service.method('org.synce.kpm.DataServerInterface', in_signature="sau")
    def createPartnership(self, name, items):
        print "DS :: Create partnership"
        dbusSyncEngine = self.busConn.get_object( "org.synce.SyncEngine",
                                                "/org/synce/SyncEngine",
                                                "org.synce.SyncEngine")
        try:
            itypes = dbusSyncEngine.GetItemTypes().items()

            dbusSyncEngine.CreatePartnership(name,items)
        except dbus.DBusException,e:
            raise e





    @dbus.service.method('org.synce.kpm.DataServerInterface', in_signature="us")
    def deletePartnership(self, id, guid):
        dbusSyncEngine = self.busConn.get_object( "org.synce.SyncEngine",
                                                "/org/synce/SyncEngine",
                                                "org.synce.SyncEngine")

        dbusSyncEngine.DeletePartnership(id,guid)
        #self.updatePartnerships()

    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def CabInstallStarted(self):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface',signature="(ii)")
    def deviceOsVersion(self, osversion):
        pass



    @dbus.service.method('org.synce.kpm.DataServerInterface', in_signature="s")
    def processAuthorization(self, password):
        if self.hal_device != None:
            self.hal_device_synce.ProvidePassword( password )
            return

        self.odccm_device.ProvidePassword( password )
        #authenticated = self.dev_iface.ProvidePassword(dlg.get_text())




    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def UnlockDeviceViaHost(self):
        pass

    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def UnlockDeviceViaDevice(self):
        pass



    def handle_syncengine_status_set_max_value         (self, maxValue):
        self.SyncEngineStatus_SetMaxValue(maxValue)

    def handle_syncengine_status_set_value             (self, value):
        self.SyncEngineStatus_SetValue(value)

    def handle_syncengine_status_set_status_string     (self, statusString):
        self.SyncEngineStatus_SetStatusString(statusString)

    def handle_syncengine_status_sync_start            (self):
        self.SyncEngineStatus_SyncStart()

    def handle_syncengine_status_sync_end              (self):
        self.SyncEngineStatus_SyncEnd()

    def handle_syncengine_status_sync_start_partner    (self, partner):
        self.SyncEngineStatus_SyncStartPartner(partner)
    
    def handle_syncengine_status_sync_end_partner      (self, partner):
        self.SyncEngineStatus_SyncEndPartner(partner)
    
    def handle_syncengine_status_sync_start_datatype   (self, partner, datatype):
        self.SyncEngineStatus_SyncStartDatatype(partner,datatype)
    
    def handle_syncengine_status_sync_end_datatype     (self, partner, datatype):
        self.SyncEngineStatus_SyncEndDatatype(partner,datatype)


    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SetMaxValue(self , maxValue):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SetValue(self , value):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SetStatusString(self , statusString):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SyncStart(self ):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SyncEnd(self ):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SyncStartPartner(self , parter):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SyncEndPartner(self , parter):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SyncStartDatatype(self , parter,datatype):
        pass
    
    @dbus.service.signal('org.synce.kpm.DataServerInterface')
    def SyncEngineStatus_SyncEndDatatype(self , parter,datatype):
        pass



    @dbus.service.method(synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE, in_signature='', out_signature='u')
    def GetDeviceBindingState(self):
        if self.syncEngineRunning and self.deviceIsConnected:
            syncEngineProxy = self.busConn.get_object("org.synce.SyncEngine","/org/synce/SyncEngine")
            syncEngineObj   = dbus.Interface(syncEngineProxy,"org.synce.SyncEngine")
            deviceBindingState = syncEngineObj.GetDeviceBindingState()

            return deviceBindingState
        return 0




