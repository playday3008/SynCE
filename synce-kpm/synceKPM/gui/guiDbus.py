import dbus
import dbus.service
import dbus.mainloop.glib

import synceKPM.constants

import time 
from PyQt4 import QtGui, QtCore, uic
from PyQt4.QtCore import *

class GuiDbus(dbus.service.Object):
    def __init__(self, busConn, mainwindow):
        dbus.service.Object.__init__(self, busConn, synceKPM.constants.DBUS_SYNCEKPM_GUI_OBJPATH)
        self.mainwindow = mainwindow 
        self.busConn = busConn

        self.deviceName = ""
        
        
        self.name = dbus.service.BusName(synceKPM.constants.DBUS_SYNCEKPM_GUI_BUSNAME, self.busConn)
        
        
        busConn.add_signal_receiver(self.handleNameOwnerChange, dbus_interface = "org.freedesktop.DBus", signal_name = "NameOwnerChanged")
        
        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")
        dataServerIface = dbus.Interface( dataServer, "org.synce.kpm.DataServerInterface")
        
        dataServerIface.connect_to_signal("deviceConnected", self.handleDeviceConnected )
        dataServerIface.connect_to_signal("deviceAuthorized", self.handleDeviceAuthorized )
        dataServerIface.connect_to_signal("deviceDisconnected", self.handleDeviceDisconnected )
        dataServerIface.connect_to_signal("ListInstalledPrograms", self.handleListInstalledPrograms )
        dataServerIface.connect_to_signal("PowerStatus", self.handlePowerStatus )
        dataServerIface.connect_to_signal("SyncEngineStatusChange", self.handleSyncEngineStatusChange )
        dataServerIface.connect_to_signal("DevicePartnerships", self.handleDevicePartnerships )
        dataServerIface.connect_to_signal("StorageInformation", self.handleStorageInformation )
        dataServerIface.connect_to_signal("DeviceOwner", self.handleDeviceOwner )
        dataServerIface.connect_to_signal("DeviceModel", self.handleDeviceModel )
        dataServerIface.connect_to_signal("CabInstallStarted", self.handleCabInstallStarted )
        dataServerIface.connect_to_signal("deviceOsVersion", self.handleDeviceOsVersion )
        
        dataServerIface.connect_to_signal("UnlockDeviceViaHost", self.handleUnlockDeviceViaHost )
        dataServerIface.connect_to_signal("UnlockDeviceViaDevice", self.handleUnlockDeviceViaDevice )




        dataServerIface.connect_to_signal( "SyncEngineStatus_SetMaxValue"      , self.handleSyncEngineStatus_SetMaxValue)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SetValue"         , self.handleSyncEngineStatus_SetValue)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SetStatusString"  , self.handleSyncEngineStatus_SetStatusString)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SyncStart"        , self.handleSyncEngineStatus_SyncStart)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SyncEnd"          , self.handleSyncEngineStatus_SyncEnd)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SyncStartPartner" , self.handleSyncEngineStatus_SyncStartPartner)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SyncEndPartner"   , self.handleSyncEngineStatus_SyncEndPartner)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SyncStartDatatype", self.handleSyncEngineStatus_SyncStartDatatype)
        dataServerIface.connect_to_signal( "SyncEngineStatus_SyncEndDatatype"  , self.handleSyncEngineStatus_SyncEndDatatype)





    def handleSyncEngineStatus_SetMaxValue(self , maxValue):
        pass

    def handleSyncEngineStatus_SetValue(self , value):
        pass

    def handleSyncEngineStatus_SetStatusString(self , statusString):
        pass

    def handleSyncEngineStatus_SyncStart(self , ):
        self.mainwindow.startSyncAnimation()


    def handleSyncEngineStatus_SyncEnd(self , ):
        self.mainwindow.stopSyncAnimation()
    pass

    def handleSyncEngineStatus_SyncStartPartner(self , parter):
        pass

    def handleSyncEngineStatus_SyncEndPartner(self , parter):
        pass

    def handleSyncEngineStatus_SyncStartDatatype(self , parter,datatype):
        pass

    def handleSyncEngineStatus_SyncEndDatatype(self , parter,datatype):
        pass














    def handleUnlockDeviceViaDevice(self):
        QTimer.singleShot(250, self.mainwindow.handleUnlockDeviceViaDevice)



    def handleUnlockDeviceViaHost(self):
        QTimer.singleShot(250, self.mainwindow.handleUnlockDeviceViaHost)




    def handleDeviceOsVersion(self, osVersion):
        self.mainwindow.OsVersion = osVersion
        pass
    
    def handleCabInstallStarted(self):
        #For some reason I must delay this a bit.... Otherwise computer locks up ?!?
        QTimer.singleShot(250, self.mainwindow.handleCabInstallStarted)


    def handleDeviceModel(self, model):
        self.mainwindow.updateDeviceModel(model)
    
    def handleDeviceOwner(self, owner):
        self.mainwindow.updateDeviceOwner(owner)
    
    def handleDevicePartnerships(self, sync_items, partnerships):
        self.__sync_items = sync_items 
        self.__partnerships = partnerships
        self.mainwindow.updateDevicePartnerships(sync_items, partnerships)
        
        QTimer.singleShot(5000, self.checkActivePartnership )


    def handleStorageInformation(self, storageInformation):
        self.mainwindow.updateStorageInformation(storageInformation)
    

    def handlePowerStatus(self, powerStatus):
        battPercent = powerStatus["BatteryLifePercent"]
        self.mainwindow.batteryStatus.setValue( battPercent )
      
    def triggerListInstalledPrograms(self):
        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")
        dataServer.updateListInstalledPrograms(dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE,
                                        reply_handler=self.handle_update_listInstalledPrograms,
                                        error_handler=self.handle_update_listInstalledPrograms_error)
   
    def handle_update_listInstalledPrograms(self):
        pass
    
    def handle_update_listInstalledPrograms_error(self, error):
        print error

            

    def handleListInstalledPrograms(self, programsList):
        self.mainwindow.listInstalledPrograms.clear()
        for program in programsList:
            self.mainwindow.listInstalledPrograms.addItem( program ) 
    
    def handleSyncEngineStatusChange(self, isOnline):
        self.mainwindow.syncEngineStatusChange(isOnline)


    def checkActivePartnership(self):
        if len(self.__partnerships) == 0:
            self.mainwindow.tray.showMessage("No partnerships detected on device","Please go to the partnership tab to create a partnership")



    def handleDeviceConnected(self,deviceName, alreadyConnected):
        self.deviceName = deviceName
        self.mainwindow.updateTray(True)
        self.mainwindow.updateDeviceName(deviceName)
        self.mainwindow.updateStatusBar(True, deviceName)
        if not alreadyConnected:
            self.mainwindow.tray.showMessage("Device connected", "The device %s just connected"%deviceName,QtGui.QSystemTrayIcon.Information, 5000)



    def handleDeviceAuthorized(self):
        self.mainwindow.tabWidget.setEnabled(True)
        self.mainwindow.labelDeviceIsLocked.setVisible(False)
        self.mainwindow.toolButtonDeviceIsLocked.setVisible(False)


        #gobject.timeout_add(2000, self.updateDeviceInformation)

        self.mainwindow.toolButtonDeviceIsLocked.setVisible(False)
        self.mainwindow.labelDeviceIsLocked.setVisible(False)

            
        

        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")
        dataServer.updatePartnerships(dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE,
                                        reply_handler=self.handle_update_partnerships,
                                        error_handler=self.handle_update_partnerships_error)




    def handle_update_partnerships(self):
        pass

    def handle_update_partnerships_error(self, _error):
        print _error







    def handleDeviceDisconnected(self,deviceName):
        self.mainwindow.updateTray(False)
        self.mainwindow.tray.showMessage("Device disconnected", "The device %s just disconnected"%deviceName, QtGui.QSystemTrayIcon.Information,5000)

        self.mainwindow.tabWidget.setEnabled(False)

        self.mainwindow.modelPartnerships.clear()
        self.mainwindow.listInstalledPrograms.clear()
       
        self.mainwindow.storageSelector.clear() 

        self.mainwindow.updateStatusBar(False)
        
        self.mainwindow.updateDeviceName()
        self.mainwindow.updateDeviceOwner()
        self.mainwindow.updateDeviceModel()
        
        #self.labelStorageTotal.setText("")
        #self.labelStorageUsed.setText("")
        #self.labelStorageFree.setText("")

        #if self._showingUnlockMessage:
        #     QApplication.activeWindow().close()
        #     self._showingUnlockMessage = False
        # 
        # if self.installWindow.isVisible:
        #     self.installWindow.hide()
        # self.tabWidget.setEnabled(False)
        # self.updateTray()
        # self.tray.showMessage("Phone Disconnected", "The phone %s just disconnected"%self.phoneCommunicator.deviceName)
        # self.updateStatusBar()
        # self.updatePowerInformation()
        # self.updateInstalledProgramsList()
        # self.updateDeviceStatus()
        # self.updateStorageInformation()
        # self.labelStorageTotal.setText("")
        # self.labelStorageUsed.setText("")
        # self.labelStorageFree.setText("")
        # 
        # self.labelDeviceOwner.setText("")
        # self.labelModelName.setText("")
        # 
        # self.toolButtonDeviceIsLocked.setVisible(False)
        # self.labelDeviceIsLocked.setVisible(False)

        # self.modelPartnerships.clear()
        # return







    
    
    def handleNameOwnerChange(self, obj_path, param2, param3):
        if obj_path == "org.synce.kpm.dataserver" and param3 == "":
            print "We have got a problem.... The dataserver went offline...."
            print "We must notifiy the user and Quit"
        

    @dbus.service.method(synceKPM.constants.DBUS_SYNCEKPM_GUI_IFACE, in_signature='', out_signature='')
    def ShowScreen(self):
        self.mainwindow.show()

    
    @dbus.service.method(synceKPM.constants.DBUS_SYNCEKPM_GUI_IFACE, in_signature='', out_signature='')
    def ToggleAnimation(self):
        print "Toggling animation"
        self.mainwindow.on_activesyncStatusIcon_clicked()


    

    @dbus.service.method(synceKPM.constants.DBUS_SYNCEKPM_GUI_IFACE, in_signature='', out_signature='')
    def Quit(self):
        print "Quiting SynCE-KPM"
        self.mainwindow.quitApplication()


    
    def handle_init(self):
        print "Finished with init"
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)
    
    def handle_init_error(self, error):
        print "Eself.rror during init!"
        print "Error is ", error
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)

    def InitializeDataServer(self):
        #Make async call to the dataserver
        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")

        self.mainwindow.setCursor(QtCore.Qt.BusyCursor)
        dataServer.InitializeDataServer(dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE,
                                        reply_handler=self.handle_init,
                                        error_handler=self.handle_init_error)


    def handle_uninstallProgram(self):
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)
        pass

    def handle_uninstallProgram_error(self,error):
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)
        print "Error happened during uninstall..."
        print error

    def uninstallProgram(self, programName):
        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")

        self.mainwindow.setCursor(QtCore.Qt.BusyCursor)
        dataServer.uninstallProgram(programName, dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE,
                                        reply_handler=self.handle_uninstallProgram,
                                        error_handler=self.handle_uninstallProgram_error,
                                        timeout=600)


    def handle_deletePartnership(self):
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)
        print "Finished deleting partnership"
        pass


    def handle_deletePartnership_error(self,error):
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)
        print "Error happened during deletion of partnership..."
        print error

    def deletePartnership(self, id, guid):
        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")

        self.mainwindow.setCursor(QtCore.Qt.BusyCursor)
        dataServer.deletePartnership(id, guid, dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE,
                                        reply_handler=self.handle_deletePartnership,
                                        error_handler=self.handle_deletePartnership_error,
                                        timeout=600)



    def handle_createPartnership(self):
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)
        print "Finished creating partnership"
        pass

    def handle_createPartnership_error(self,error):
        self.mainwindow.setCursor(QtCore.Qt.ArrowCursor)
        print "Error happened during creation of partnership..."
        print error

    def createPartnership(self, id, guid):
        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")

        self.mainwindow.setCursor(QtCore.Qt.BusyCursor)
        dataServer.createPartnership(id, guid, dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE,
                                        reply_handler=self.handle_createPartnership,
                                        error_handler=self.handle_createPartnership_error,
                                        timeout=600)


    def provideAuthorization(self,password):
        dataServer = self.busConn.get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")
        dataServer.processAuthorization(password, dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE)
