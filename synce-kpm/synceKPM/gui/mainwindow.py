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


import sys

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import QtGui, QtCore, uic

from pkg_resources import resource_filename

import logging


from synceKPM.gui.ui_synce_kpm_mainwindow import *
import synceKPM.gui.installwindow 
import synceKPM.gui.createpshipwindow 


import dbus
import dbus.service
import synceKPM.constants
import synceKPM.gui.guiDbus


class mainwindow(QtGui.QMainWindow, synceKPM.gui.ui_synce_kpm_mainwindow.Ui_synce_kpm_mainwindow):
    


    def __init__(self):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)
        dbus.mainloop.qt.DBusQtMainLoop

        self.session_bus = dbus.SessionBus()
        self.guiDbus = synceKPM.gui.guiDbus.GuiDbus( self.session_bus, self )


        self.showCreatePShipMessage = False 

        self.toolButtonDeviceIsLocked.setVisible(False)
        self.labelDeviceIsLocked.setVisible(False)
        self.iconLocked = QtGui.QIcon( "%s/lock.svg"%resource_filename('synceKPM', 'data'))
        self.toolButtonDeviceIsLocked.setIcon( self.iconLocked )
        self.viewPartnerships.setVisible(False)
        self.button_add_pship.setVisible(False)
        self.button_delete_pship.setVisible(False)

        self._showingUnlockMessage = False

        self.sync_progressbar.setVisible(False)

        self.storageInformation = []


        self.iconDisconnected = QtGui.QIcon("%s/blue_scalable.svg"%resource_filename('synceKPM','data'))
        self.iconConnected = QtGui.QIcon("%s/green_scalable.svg"%resource_filename('synceKPM','data'))
        
        self.animIcons = [] 

        for i in range(0,10):
            self.animIcons.append( QtGui.QIcon("%s/anim/synce-green-anim-%i.svg"%(resource_filename('synceKPM','data'),i))) 
        
        self.activesyncStatusIcon.setIconSize( QSize( self.activesyncStatusIcon.width(),self.activesyncStatusIcon.height()) )
        #self.activesyncStatusIcon.setIcon( self.animIcons[0] ) 
        self.activesyncStatusIcon.setIcon( self.iconDisconnected ) 
        

        self.animating = False
        self.animationIndex = 0 
        self.animationThread = None
        
        self.setWindowIcon(self.iconConnected)

        self.tray = QtGui.QSystemTrayIcon(self)
        self.trayMenu = QtGui.QMenu()
        self.action_about = QtGui.QAction( u'About', self)
        self.action_show = QtGui.QAction( u'Show mainscreen', self)
        self.action_quit = QtGui.QAction( u'Quit', self)

        self.connect(self.action_quit, SIGNAL("triggered()"), self.quitApplication)
        self.connect(self.action_about, SIGNAL("triggered()"), self.helpAbout)
        self.connect(self.action_show, SIGNAL("triggered()"), self.show)

        self.trayMenu.addAction(self.action_about)
        self.trayMenu.addAction(self.action_show)
        self.trayMenu.addAction(self.action_quit)
        self.tray.setContextMenu(self.trayMenu)
        self.tray.setIcon(self.iconDisconnected)
        self.tray.setToolTip(u"SynCE KDE PDA Manager")
        self.tray.show()
        
        self.tray.connect(self.tray, SIGNAL("activated(QSystemTrayIcon::ActivationReason)"),
                self.cb_systray_activated)



        self.connect(self.storageSelector, SIGNAL("currentIndexChanged(int)"), self.updateFreeUsedTotalDiskSpace)
         
        
        self.installWindow = synceKPM.gui.installwindow.installwindow(self.guiDbus)

        self.createPshipWindow = synceKPM.gui.createpshipwindow.createpshipwindow(self)



        #
        # Setup the model for the treeView, representing the partnerships
        self.modelPartnerships = QStandardItemModel(0,1)
        self.viewPartnerships.setModel(self.modelPartnerships)
        self.viewPartnerships.drawBranches = self.myDrawNoBranches
        self.modelPartnerships.setHeaderData(0, QtCore.Qt.Horizontal, QtCore.QVariant("Partnerships"))
        
        self.partnerships = None  
        self.sync_items   = None
        self.OsVersion    = None

        self.deviceName = ""
        self.guiDbus.InitializeDataServer()


        self.array_async_status_pships_label = []
        self.array_async_status_pships_text  = []

        self.array_async_status_pships_label.append( self.label_partnership_1)
        self.array_async_status_pships_label.append( self.label_partnership_2)
        self.array_async_status_pships_label.append( self.label_partnership_3)
            
        self.array_async_status_pships_text.append( self.label_partnership_1_text)
        self.array_async_status_pships_text.append( self.label_partnership_2_text)
        self.array_async_status_pships_text.append( self.label_partnership_3_text)

        self.currentActiveSyncStatusPartner = ""
        self.currentActiveSyncStatusDatatype = ""
        self.mapping_pship_to_label_idx = {}

    def getASLabelIdx(self, pshipName):
        if pshipName in self.mapping_pship_to_label_idx:
            return self.mapping_pship_to_label_idx[ pshipName ]
        else:
            #This is currently for Exchange partnerships, till jagow updates sync-engine
            self.array_async_status_pships_label[ len(self.partnerships) ].setText( pshipName )
            return len(self.partnerships)


    def setActiveSyncStatusString(self, statusString):
        __currentIdx = self.getASLabelIdx( self.currentActiveSyncStatusPartner )
        if self.currentActiveSyncStatusDatatype == "":
            self.array_async_status_pships_text[ __currentIdx ].setText("%s"%statusString)
        else:
            self.array_async_status_pships_text[ __currentIdx ].setText("%s: %s"%(self.currentActiveSyncStatusDatatype, statusString))


    def cb_systray_activated(self, reason):

		if reason != QtGui.QSystemTrayIcon.Context:
			if not self.isVisible():
				self.show()
			else:
				self.hide()


    def myDrawNoBranches(self,  painter, rect, index):
        pass


    
    def syncEngineStatusChange(self, isOnline):
        if isOnline:
            self.activesyncStatusIcon.setIcon(self.animIcons[0])
            self.labelSyncEngineNotRunning_2.setVisible(False)
            
            self.label_partnership_1.setVisible(True)
            self.label_partnership_2.setVisible(True)
            self.label_partnership_3.setVisible(True)
            
            self.label_partnership_1_text.setVisible(True)
            self.label_partnership_2_text.setVisible(True)
            self.label_partnership_3_text.setVisible(True)


            self.label_partnership_1.setText("")
            self.label_partnership_2.setText("")
            self.label_partnership_3.setText("")
            
            self.label_partnership_1_text.setText("")
            self.label_partnership_2_text.setText("")
            self.label_partnership_3_text.setText("")
            
            self.label_sync_stat.setVisible(True)
            self.sync_progressbar.setValue(0)
            #self.sync_progressbar.setVisible(True)
            
            self.labelSyncEngineNotRunning.setVisible(False)
            self.viewPartnerships.setVisible(True)
            self.button_add_pship.setVisible(True)
            self.button_delete_pship.setVisible(True)



    
        else:
            self.activesyncStatusIcon.setIcon(self.iconDisconnected)
            self.modelPartnerships.clear()
            
            self.label_partnership_1.setVisible(False)
            self.label_partnership_2.setVisible(False)
            self.label_partnership_3.setVisible(False)
            
            self.label_partnership_1_text.setVisible(False)
            self.label_partnership_2_text.setVisible(False)
            self.label_partnership_3_text.setVisible(False)


            self.labelSyncEngineNotRunning_2.setVisible(True)

            self.label_sync_stat.setVisible(False)
            self.sync_progressbar.setVisible(False)

            self.labelSyncEngineNotRunning.setVisible(True)
            self.viewPartnerships.setVisible(False)
            self.button_add_pship.setVisible(False)
            self.button_delete_pship.setVisible(False)
            



    def helpAbout(self):
        QMessageBox.about(self, "About SynCE-KPM",
            """<b>SynCE KDE PDA Manager</b> v %s
            <p>SynCE-KPM allows you to manage the basic features of your 
            WM5/WM6 device under Linux like the ActiveSync application does  
            under MS-Windows.
            <p>Copyright (c) 2008 Guido Diepen &lt;guido@guidodiepen.nl&gt;
            <p>SynCE-KPM is licensed under GPL
            """%"0.11.1")


    def updateTray(self, deviceConnected):
        if deviceConnected:
            self.tray.setIcon(self.iconConnected)
        else:
            self.tray.setIcon(self.iconDisconnected)

    def updateDeviceName(self,deviceName=""):
        self.deviceName = deviceName
        self.labelDeviceName.setText(deviceName)

    def updateDeviceOwner(self, deviceOwner="" ):
        self.labelDeviceOwner.setText( deviceOwner )

    def updateDeviceModel(self, deviceModel="" ):
        self.labelModelName.setText( deviceModel )


    def updateStatusBar(self, isConnected, deviceName=""):
        if isConnected:
            self.statusBar().showMessage("Connected to phone %s"%deviceName)
        else:
            self.statusBar().showMessage("No phone connected")



    def updatePowerInformation(self):
        powerStatus = self.phoneCommunicator.getPowerStatus()
        if powerStatus is not None:
            battPercent = powerStatus["BatteryLifePercent"]
            self.batteryStatus.setValue( battPercent )
        else:
            self.batteryStatus.setValue( 0 ) 

    @pyqtSignature("")
    def on_toolButtonDeviceIsLocked_clicked(self):
        qs_text, pressedOK = QInputDialog.getText(self,"Locked device", "Please provide password:", QLineEdit.Password, "")
        u_text = unicode(qs_text)
        if pressedOK:
            self.guiDbus.provideAuthorization( u_text )


    @pyqtSignature("")
    def on_pushButton_InstallCAB_clicked(self):
        self.installWindow.move(self.x() + self.width()/2 -  self.installWindow.width()/2 , self.y() + self.height()/2 - self.installWindow.height()/2)
        self.installWindow.progressWindow.move(self.x() + self.width()/2 -  self.installWindow.progressWindow.width()/2 , self.y() + self.height()/2 - self.installWindow.progressWindow.height()/2)
        self.installWindow.show()
        



        
    @pyqtSignature("")
    def on_pushButton_Uninstall_clicked(self):
        #Determine the program
        currentItem = self.listInstalledPrograms.currentItem()

        if currentItem == None:
            return

        programName = unicode(currentItem.text())
        
        reply = QMessageBox.question(self,"Confirm uninstall", "Are you really sure you want to uninstall %s"%programName, QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) 

        if reply == QMessageBox.Yes:
            self.guiDbus.uninstallProgram(programName)

    def showErrorMessage(self, title, message):
        QMessageBox.warning(self,title,message)

    def showCriticalError(self,title, message):
        QMessageBox.critical(self,title,message)



    def updateFreeUsedTotalDiskSpace(self):
        if self.storageSelector.currentIndex() == -1:
            self.labelStorageTotal.setText("")
            self.labelStorageUsed.setText("")
            self.labelStorageFree.setText("")

        else:
            storageName, storageLocation, freeDisk,totalDisk,freeDiskTotal = self.storageInformation[ self.storageSelector.currentIndex() ]
            
            usedDisk = totalDisk - freeDisk
            self.labelStorageTotal.setText("%.2fMB"%(totalDisk/(1024.0*1024.0)))
            self.labelStorageUsed.setText("%.2fMB"%(usedDisk/(1024.0*1024.0)))
            self.labelStorageFree.setText("%.2fMB"%(freeDisk/(1024.0*1024.0)))
    
    def updateStorageInformation(self, storageInformation):
        self.storageSelector.clear()

        self.installWindow.deviceList.clear()
        self.installWindow.deviceListRoot = []

        self.storageInformation = storageInformation
        
        for storageItem in storageInformation:
            storageName, storageLocation, freeDisk,totalDisk,freeDiskTotal = storageItem
            self.storageSelector.addItem("%s"%storageName)
            
            self.installWindow.deviceList.addItem("%s [Free: %.2fMB , Total: %.2fMB]" % (storageName,freeDisk/(1024.0*1024.0),totalDisk/(1024.0*1024.0)) )
            self.installWindow.deviceListRoot.append(storageLocation)







    @pyqtSignature("")
    def on_pushButton_Refresh_clicked(self):
        self.guiDbus.triggerListInstalledPrograms()
       
    def quitApplication(self):
        QApplication.quit()



    @pyqtSignature("")
    def on_pushButton_Quit_clicked(self):
        self.quitApplication()



    def updateDevicePartnerships(self, sync_items, partnerships):
        self.modelPartnerships.clear() 
        self.partnerships = partnerships 
        self.sync_items   = sync_items

        self.array_async_status_pships_label[0].setText( "" )
        self.array_async_status_pships_label[1].setText( "" )
        self.array_async_status_pships_label[2].setText( "" )
        
        self.mapping_pship_to_label_idx = {}
        i=0
        for pship in partnerships:
            id,guid,name,hostname,devicename,items = pship

            #print id,guid,name
            
            self.mapping_pship_to_label_idx[ name ] = i

            self.array_async_status_pships_label[i].setText( name )
            
            rootItem = QStandardItem( name )
            rootItem.setIcon(self.iconConnected)
            self.modelPartnerships.appendRow(rootItem)
            self.formatDeviceSyncItems( rootItem, sync_items, items )

            i+=1
        
        self.modelPartnerships.setHeaderData(0, QtCore.Qt.Horizontal, QtCore.QVariant("Partnerships"))
        self.viewPartnerships.expandAll()




    def formatDeviceSyncItems(self, parent, item_types, selected_item_types):
        for item_type in item_types:
            childItem = QStandardItem(item_types[item_type])
            childItem.setCheckable(True)
            childItem.setEnabled(False)
            if item_type in selected_item_types:
                childItem.setCheckState(QtCore.Qt.Checked )

            parent.appendRow( childItem )


        
    
    
    @pyqtSignature("")
    def on_button_add_pship_clicked(self):
        self.createPshipWindow.move(self.x() + self.width()/2 -  self.createPshipWindow.width()/2 , self.y() + self.height()/2 - self.createPshipWindow.height()/2)
        self.createPshipWindow.show()

       
    @pyqtSignature("")
    def on_button_delete_pship_clicked(self):

        #Only delete things if user selected something
        if len( self.viewPartnerships.selectedIndexes() ) > 0:
            #Determine which partnership the user wants to delete
            
            #First option: the user selected one of the subitems
            pshipIdx = self.viewPartnerships.selectedIndexes()[0].parent().row()

            #The parent of the selected item is -1, this means that the
            #user selected the partnership line itself.
            if pshipIdx == -1:
                pshipIdx = self.viewPartnerships.selectedIndexes()[0].row()
            
            #Ask the user if he is sure ;)
            #sync_items, partnerships = self.phoneCommunicator.getPartnerships()
            selected_partnership = self.partnerships[pshipIdx]
            id,guid,name,hostname,devicename,items = selected_partnership


            reply = QMessageBox.question(self,"Confirm partnership deletion", "Are you really sure you want to delete the partnership %s"%name, QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) 

            if reply != QMessageBox.Yes:
                return

           
            #This is for checking whether the user is having a WM6 device
            if self.OsVersion[0] == 5 and self.OsVersion[1] == 2:
                showExtraWarning = False
                orphanedItems=[]

                
                pshipConnections = dict()
                for _item in self.sync_items:
                    pshipConnections[ _item ] = 0



                #Check whether the removal of this particular partnership would result
                #in orphaned itemtypes, that would be deleted by wm6
                for pship in self.partnerships:
                    id2,guid2,name2,hostname2,devicename2,items2 = pship
                    for _item2 in items2:
                        pshipConnections[ _item2 ] = pshipConnections[ _item2 ] + 1

                for _item in items:
                    #If this partnership connection was the last one for this particular
                    #type of syncitem, we must notify the user
                    if pshipConnections[ _item ] == 1:
                        showExtraWarning = True 
                        orphanedItems.append( self.sync_items[ _item ] ) 

                    

                if showExtraWarning:
                    _orphanedItemsString=""
                    for orphanedItem in orphanedItems:
                        _orphanedItemsString = "%s,%s"%(_orphanedItemsString, orphanedItem)
                    _orphanedItemsString = _orphanedItemsString[1:]
                    reply = QMessageBox.warning(self,"Possible deletion of 'orphaned' items", "It appears you are using a Windows Mobile 6 device.\nDeleting the partnership might result in all items of [%s] to be deleted from device because no other partnership is connected with these.\nAre you really sure you want to delete this partnership?"%_orphanedItemsString, QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) 

                    if reply != QMessageBox.Yes:
                        return
                
            #If we got here, then the user is 100% sure the partnership
            #should be deleted. Then we do this ;)
            self.guiDbus.deletePartnership( id,guid )
   
    def startActiveSyncSync(self):
        self.startSyncAnimation()
        self.sync_progressbar.setValue(0)
        self.sync_progressbar.setMaximum(100)
        self.sync_progressbar.setVisible(True)


    def stopActiveSyncSync(self):
        self.sync_progressbar.setVisible(False)
        self.stopSyncAnimation()


    def myAnimationThread_cb(self):
        self.animationIndex = (self.animationIndex + 1)%10
        self.activesyncStatusIcon.setIcon( self.animIcons[self.animationIndex] ) 
        self.tray.setIcon(self.animIcons[self.animationIndex] )
    
    def startSyncAnimation(self):
        self.animationIndex = 0
        self.activesyncStatusIcon.setIcon( self.animIcons[self.animationIndex] ) 
        self.tray.setIcon(self.animIcons[self.animationIndex] )
        
        if self.animationThread is None:
            self.animationThread = QTimer(self)
            self.animationThread.connect(self.animationThread, SIGNAL("timeout()"), self.myAnimationThread_cb)
        
        self.animationThread.start(150)

    def stopSyncAnimation(self):
        self.animationIndex = 0
        self.activesyncStatusIcon.setIcon( self.animIcons[self.animationIndex] ) 
        self.tray.setIcon(self.animIcons[self.animationIndex] )
        self.animationThread.stop()




    def handleCabInstallStarted(self):
        QMessageBox.information(self,"Install CAB file", "Please follow instructions on the device now to install the program.", QMessageBox.Ok) 
        return False



    def handleUnlockDeviceViaHost(self):
        self.labelDeviceIsLocked.setVisible(True)
        self.toolButtonDeviceIsLocked.setVisible(True)
        qs_text, pressedOK = QInputDialog.getText(self,"Locked device", "Please provide password:", QLineEdit.Password, "")
        u_text = unicode(qs_text)
        if pressedOK:
            self.guiDbus.provideAuthorization( u_text )


    def handleUnlockDeviceViaDevice(self):
        self.labelDeviceIsLocked.setVisible(True)
        QMessageBox.information( self, "Locked device", "The device %s is locked. Please unlock it by following instructions on the device"%self.deviceName)
    

    def showEvent(self, event):
        if self.showCreatePShipMessage:
            self.showCreatePShipMessage = False
            QTimer.singleShot(0,self.askUserForPshipCreation )



    def askUserForPshipCreation(self):
        self.showCreatePShipMessage = False
        reply = QMessageBox.information(self,"No active partnerships found", "No active partnerships have been found with the device. Would you like to create a partnership right now?", QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) 
        if reply == QMessageBox.Yes:
            self.tabWidget.setCurrentIndex(2)
            self.on_button_add_pship_clicked()


