#!/usr/bin/env python

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

import libxml2
import util.xml2util
import util.characteristics
import logging

from util.commutil import * 

import dialogs.ui_synce_kpm_mainwindow
import dialogs.synce_kpm_installwindow

class PhoneCommunicatorCallbackEvent(QEvent):
    def __init__(self, _reason=-1):
        self.reason = _reason
        QEvent.__init__(self, QEvent.User)

    def type(self):
        return QEvent.User


class synce_kpm_mainwindow(QtGui.QMainWindow, dialogs.ui_synce_kpm_mainwindow.Ui_synce_kpm_mainwindow):
    def __init__(self, *args):
        QtGui.QWidget.__init__(self, *args)
        self.setupUi(self)
        
        self.toolButtonDeviceIsLocked.setVisible(False)
        self.labelDeviceIsLocked.setVisible(False)
        self.toolButtonDeviceIsLocked.setIcon(QtGui.QIcon("synce-kpm/data/lock.svg"))
        self.viewPartnerships.setVisible(False)
        self.button_add_pship.setVisible(False)
        self.button_delete_pship.setVisible(False)

        self._showingUnlockMessage = False

        self.storageInformation = []


        self.iconDisconnected = QtGui.QIcon("synce-kpm/data/blue_22x22.png")
        self.iconConnected  = QtGui.QIcon("synce-kpm/data/green_22x22.png")
        
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

        self.connect(self.storageSelector, SIGNAL("currentIndexChanged(int)"), self.updateFreeUsedTotalDiskSpace)
         
        self.phoneCommunicator = PhoneCommunicator(self.updateView_cb)
        
        self.installWindow = dialogs.synce_kpm_installwindow.synce_kpm_installwindow( self.phoneCommunicator )


        #
        # Setup the model for the treeView, representing the partnerships
        self.modelPartnerships = QStandardItemModel(0,1)
        self.viewPartnerships.setModel(self.modelPartnerships)
        self.viewPartnerships.drawBranches = self.myTest
        self.modelPartnerships.setHeaderData(0, QtCore.Qt.Horizontal, QtCore.QVariant("Partnerships"))


    """
    @pyqtSignature("")
    def hideEvent(self,event):
        print "abc"
        self.hide()
        event.accept()
    """


    def helpAbout(self):
        QMessageBox.about(self, "About SynCE-KPM",
                """<b>SynCE KDE PDA Manager</b> v %s
                <p>This application can be used to manage a WM5/WM6 PDA
                connected to the computer with SynCE.
                <p>Author: Guido Diepen <guido@guidodiepen.nl>
                """%"0.0.1")

    @pyqtSignature("")
    def closeEvent(self,event):
        event.ignore()
        self.hide()

    def updateTray(self):
        if self.phoneCommunicator.phoneConnected:
            self.tray.setIcon(self.iconConnected)
        else:
            self.tray.setIcon(self.iconDisconnected)

    def updateDeviceStatus(self):
        if self.phoneCommunicator.phoneConnected:
            self.labelDeviceName.setText(self.phoneCommunicator.device.GetName())
        else:
            self.labelDeviceName.setText("")



    #Use the param later to distinguish what the reason was
    #for the model update 
    def updateView_cb(self,reason=-1):
        callbackEvent = PhoneCommunicatorCallbackEvent(reason)
        QCoreApplication.postEvent(self, callbackEvent)
        

    @pyqtSignature("")
    def customEvent(self,event):
        if event.reason == ACTION_PHONE_DISCONNECTED:
            if self._showingUnlockMessage:
                QApplication.activeWindow().close()
                self._showingUnlockMessage = False
            
            if self.installWindow.isVisible:
                self.installWindow.hide()
            self.tabWidget.setEnabled(False)
            self.updateTray()
            self.tray.showMessage("Phone Disconnected", "The phone %s just disconnected"%self.phoneCommunicator.deviceName)
            self.updateStatusBar()
            self.updatePowerInformation()
            self.updateInstalledProgramsList()
            self.updateDeviceStatus()
            self.updateStorageInformation()
            self.labelStorageTotal.setText("")
            self.labelStorageUsed.setText("")
            self.labelStorageFree.setText("")
            
            self.labelDeviceOwner.setText("")
            self.labelModelName.setText("")
            
            self.toolButtonDeviceIsLocked.setVisible(False)
            self.labelDeviceIsLocked.setVisible(False)

            self.modelPartnerships.clear()
            return


        if event.reason == ACTION_PHONE_AUTHORIZED:
            self.toolButtonDeviceIsLocked.setVisible(False)
            self.labelDeviceIsLocked.setVisible(False)


            if self._showingUnlockMessage: 
                QApplication.activeWindow().close() 
                self._showingUnlockMessage = False
            
            self.tabWidget.setEnabled(True)
            self.labelModelName.setText( self.phoneCommunicator.getModelName() )
            
            self.updateStatusBar()
            self.updateDeviceStatus()
            self.updatePartnerships()
            return
            


        if event.reason == ACTION_PHONE_CONNECTED:
            self.updateTray()
            self.tray.showMessage("Phone Connected", "The phone %s just connected"%self.phoneCommunicator.deviceName)
            self.toolButtonDeviceIsLocked.setVisible(True)
            self.labelDeviceIsLocked.setVisible(True)
            return

        if event.reason == ACTION_PASSWORD_NEEDED_ON_DEVICE:
            self.show()
            self.labelDeviceIsLocked.setVisible(True)
            self._showingUnlockMessage = True
            QMessageBox.information( self, "Locked device", "The device %s is locked. Please unlock it by following instructions on the device"%self.phoneCommunicator.deviceName)
            self._showingUnlockMessage = False
            

        if event.reason == ACTION_PASSWORD_NEEDED:
            self.show()
            self._showingUnlockMessage = True
            qs_text, pressedOK = QInputDialog.getText(self,"Locked device", "Please provide password:", QLineEdit.Password, "")
            u_text = unicode(qs_text)
            self._showingUnlockMessage = False
          
            if pressedOK:
                self.phoneCommunicator.processAuthorization( u_text )

        if event.reason == ACTION_POWERSTATUS_CHANGED:
            self.updatePowerInformation()
            return
        
        if event.reason == ACTION_STORAGE_CHANGED:
            self.updateStorageInformation()
            return
        
        if event.reason == ACTION_DEVICE_OWNER_CHANGED:
            self.updateDeviceOwner()
            return

        if event.reason == ACTION_INSTALLED_PROGRAMS_CHANGED:
            self.updateInstalledProgramsList()
            return

        if event.reason == ACTION_SYNCENGINE_CHANGED_ONLINE:
            self.labelSyncEngineNotRunning.setVisible(False)
            self.viewPartnerships.setVisible(True)
            self.button_add_pship.setVisible(False)
            self.button_delete_pship.setVisible(False)
            self.updatePartnerships()
            return
        
        if event.reason == ACTION_SYNCENGINE_CHANGED_OFFLINE:
            self.labelSyncEngineNotRunning.setVisible(True)
            self.viewPartnerships.setVisible(False)
            self.button_add_pship.setVisible(False)
            self.button_delete_pship.setVisible(False)
            self.updatePartnerships()
            return

        print "Unhandled event: %i"%event.reason

    def updateDeviceOwner(self):
        self.labelDeviceOwner.setText( self.phoneCommunicator.getDeviceOwner() )


    def updateStatusBar(self):
        if self.phoneCommunicator.phoneConnected:
            self.statusBar().showMessage("Connected to phone %s"%self.phoneCommunicator.deviceName )
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
        callbackEvent = PhoneCommunicatorCallbackEvent(ACTION_PASSWORD_NEEDED)
        QCoreApplication.postEvent(self, callbackEvent)


    @pyqtSignature("")
    def on_pushButton_InstallCAB_clicked(self):
        self.installWindow.show()

        
    @pyqtSignature("")
    def on_pushButton_Uninstall_clicked(self):
        #First do an update of the power information, this will make sure the phone is connected
        self.updatePowerInformation() 
       
        #in case no phone was connected, reset everything
        if self.phoneCommunicator.phoneConnected == False:
            self.updateInstalledProgramsList()
            return
        

        #Determine the program
        currentItem = self.listInstalledPrograms.currentItem()

        if currentItem == None:
            self.updateStatusBar()
            self.updateDeviceStatus()
            return

        programName = currentItem.text()
        
        reply = QMessageBox.question(self,"Confirm uninstall", "Are you really sure you want to uninstall %s"%programName, QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) 

        if reply == QMessageBox.Yes:

            #print "Going to uninstall:",programName
            self.phoneCommunicator.uninstallProgram(programName)

            self.updateInstalledProgramsList()





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
    
    def updateStorageInformation(self):
        self.storageInformation = []

        if self.phoneCommunicator.phoneConnected:
            self.storageInformation = self.phoneCommunicator.getStorageInformation()

        self.storageSelector.clear()
        for storageItem in self.storageInformation:
            storageName, storageLocation, freeDisk,totalDisk,freeDiskTotal = storageItem
            self.storageSelector.addItem("%s"%storageName)
            #self.deviceList.addItem("%s [Free: %.2fMB , Total: %.2fMB]" % (storageName,freeDisk/(1024.0*1024.0),totalDisk/(1024.0*1024.0)) )
            #self.deviceListRoot.append(storageLocation)




    def updateInstalledProgramsList(self):
        self.listInstalledPrograms.clear()
        programs = self.phoneCommunicator.getListInstalledPrograms()
        for program in programs:
            self.listInstalledPrograms.addItem( program ) 



    @pyqtSignature("")
    def on_pushButton_Refresh_clicked(self):
        self.updateInstalledProgramsList() 
       
    def quitApplication(self):
        self.phoneCommunicator.stopAllThreads()
        QApplication.quit()

    @pyqtSignature("")
    def on_pushButton_Quit_clicked(self):
        self.quitApplication()


    def myTest(self,  painter, rect, index):
        pass


    def myBBB(self, group, item):
        childItem = QStandardItem(item)
        childItem.setCheckable(True)
        childItem.setEnabled(False)
        group.appendRow( childItem )

    def myAAA(self, parent):
        self.myBBB(parent, "Contacts")
        self.myBBB(parent, "Agenda")
        self.myBBB(parent, "Email")
        self.myBBB(parent, "Tasks")
        self.myBBB(parent, "Files")


    def myCCC(self, parent, item_types, selected_item_types):
        for item_type in item_types:
            #print item_types[item_type]

            childItem = QStandardItem(item_types[item_type])
            childItem.setCheckable(True)
            childItem.setEnabled(False)
            if item_type in selected_item_types:
                childItem.setCheckState(QtCore.Qt.Checked )

            parent.appendRow( childItem )

    def updatePartnerships(self):
        self.modelPartnerships.clear()
        
        
        #Only if the sync-engine is running
        if self.phoneCommunicator.syncEngineRunning:
            sync_items, partnerships = self.phoneCommunicator.getPartnerships()

            for pship in partnerships:
                id,guid,name,hostname,devicename,items = pship 
                
                rootItem = QStandardItem( name )
                rootItem.setIcon(self.iconConnected)
                self.modelPartnerships.appendRow(rootItem)
                
                self.myCCC( rootItem, sync_items, items )



        self.modelPartnerships.setHeaderData(0, QtCore.Qt.Horizontal, QtCore.QVariant("Partnerships"))
        """
        rootItem = QStandardItem( "Microsoft Exchange Server" )
        rootItem.setIcon(self.iconConnected)
        self.modelPartnerships.appendRow( rootItem)


        rootItem2 = QStandardItem( "Linux PC" )
        rootItem2.setIcon(self.iconDisconnected)
        self.modelPartnerships.appendRow(rootItem2)

        
        self.myAAA(rootItem)
        self.myAAA(rootItem2)
        #self.childItem_1_Files = QStandardItem("Files")
        #self.childItem_1_Files.setCheckable(True)
        #rootItem.appendRow( self.childItem_1_Files )
        """

        self.viewPartnerships.expandAll()
        
        print "aaa"

       
    @pyqtSignature("")
    def on_button_delete_pship_clicked(self):
        if len( self.viewPartnerships.selectedIndexes() ) > 0:
            print "something was selected"
            print self.viewPartnerships.selectedIndexes()[0].row()
            print self.viewPartnerships.selectedIndexes()[0].parent()
        else:
            print "something was selected"

    @pyqtSignature("")
    def on_BUTTON_TEST_clicked(self):
        pass

