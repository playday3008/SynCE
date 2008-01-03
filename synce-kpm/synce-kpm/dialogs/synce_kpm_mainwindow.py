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
        print "called"
        return QEvent.User


class synce_kpm_mainwindow(QtGui.QMainWindow, dialogs.ui_synce_kpm_mainwindow.Ui_synce_kpm_mainwindow):
    def __init__(self, *args):
        QtGui.QWidget.__init__(self, *args)
        self.setupUi(self)
        
        self.iconDisconnected = QtGui.QIcon("synce-kpm/data/blue_22x22.png")
        self.iconConnected  = QtGui.QIcon("synce-kpm/data/green_22x22.png")
        

        self.setWindowIcon(self.iconConnected)

        self.tray = QtGui.QSystemTrayIcon(self)
        self.trayMenu = QtGui.QMenu()
        self.action_quit = QtGui.QAction(QtGui.QIcon("images/quit.png"), u'Quit', self)

        self.trayMenu.addAction(self.action_quit)
        self.tray.setContextMenu(self.trayMenu)
        self.tray.setIcon(self.iconDisconnected)
        self.tray.setToolTip(u"SynCE KDE PDA Manager")
        self.tray.show()

         
       
        self.phoneCommunicator = PhoneCommunicator(self.updateView)
        self.installWindow = dialogs.synce_kpm_installwindow.synce_kpm_installwindow( self.phoneCommunicator )

        self.updateView() 



    """
    @pyqtSignature("")
    def hideEvent(self,event):
        print "abc"
        self.hide()
        event.accept()

    @pyqtSignature("")
    def closeEvent(self,event):
        event.ignore()
        self.hide()
    """

    def updateTray(self):
        if self.phoneCommunicator.phoneConnected:
            self.tray.setIcon(self.iconConnected)
        else:
            self.tray.setIcon(self.iconDisconnected)

    def updateDeviceStatus(self):
        #print "updateStatus()"
        if self.phoneCommunicator.phoneConnected:
            self.labelDeviceName.setText(self.phoneCommunicator.device.name)
        else:
            self.labelDeviceName.setText("")



    #Use the param later to distinguish what the reason was
    #for the model update 
    def updateView(self,reason=-1):
        callbackEvent = PhoneCommunicatorCallbackEvent(reason)
        QCoreApplication.postEvent(self, callbackEvent)
         
    @pyqtSignature("")
    def customEvent(self,event):
        if event.reason == ACTION_PHONE_DISCONNECTED:
            if self.installWindow.isVisible:
                self.installWindow.hide()
            self.tabWidget.setEnabled(False)
            self.updateTray()
            self.tray.showMessage("Phone Disconnected", "The phone %s just disconnected"%self.phoneCommunicator.savedDevice.name)
            self.updateStatusBar()
            self.updatePowerInformation()
            self.updateInstalledProgramsList()
            self.updateDeviceStatus()
            return


        if event.reason == ACTION_PHONE_CONNECTED:
            self.tabWidget.setEnabled(True)
            self.updateTray()
            self.tray.showMessage("Phone Connected", "The phone %s just connected"%self.phoneCommunicator.device.name)
            self.updateStatusBar()
            self.updateDeviceStatus()
            return


        if event.reason == ACTION_POWERSTATUS_CHANGED:
            self.updatePowerInformation()
            return
        
        if event.reason == ACTION_STORAGE_CHANGED:
            print "Storage chagned..."
            return

        if event.reason == ACTION_INSTALLED_PROGRAMS_CHANGED:
            self.updateInstalledProgramsList()
            return

        #if we got here, it is just the initial update
        #If we update everyhing, they will automatically become disabled 
        #when needed
        self.updateInstalledProgramsList()
        self.updatePowerInformation()
        self.updateStatusBar()
        self.tabWidget.setEnabled(self.phoneCommunicator.phoneConnected)
        self.updateTray()
        self.updateDeviceStatus()


    def updateStatusBar(self):
        if self.phoneCommunicator.phoneConnected:
            self.statusBar().showMessage("Connected to phone %s"%self.phoneCommunicator.device.name )
        else:
            self.statusBar().showMessage("No phone connected")


    def updatePowerInformation(self):
        powerStatus = self.phoneCommunicator.getPowerStatus()
        if powerStatus is not None:
            battPercent = self.phoneCommunicator.getPowerStatus()["BatteryLifePercent"]
            self.batteryStatus.setValue( battPercent )
        else:
            self.batteryStatus.setValue( 0 ) 


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
            return

        programName = currentItem.text()
        
        reply = QMessageBox.question(self, "Confirm uninstall", "Are you really sure you want to uninstall \""+programName+"\"", QMessageBox.Yes, QMessageBox.No|QMessageBox.Default|QMessageBox.Escape) 

        if reply == QMessageBox.Yes:

            #print "Going to uninstall:",programName
            self.phoneCommunicator.uninstallProgram(programName)

            self.updateInstalledProgramsList()










    def updateInstalledProgramsList(self):
        self.listInstalledPrograms.clear()
        if self.phoneCommunicator.phoneConnected:
            programs = self.phoneCommunicator.getListInstalledPrograms()
            for program in programs:
                self.listInstalledPrograms.addItem( program ) 



    @pyqtSignature("")
    def on_pushButton_Refresh_clicked(self):
        self.updateInstalledProgramsList() 
        self.updatePowerInformation()
        

    @pyqtSignature("")
    def on_pushButton_Quit_clicked(self):
        self.phoneCommunicator.stopAllThreads()
        QApplication.quit()


