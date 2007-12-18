#!/usr/bin/env python
import sys

from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import QtGui, QtCore, uic

from pyrapi2 import *
from rapiutil import *
import libxml2
import xml2util
import characteristics
import logging








#import ui_programUninstaller

rapi_session = RAPISession(SYNCE_LOG_LEVEL_DEFAULT)




class MainWindowsSynceKDM(QtGui.QWidget):
    def __init__(self, *args):
        QtGui.QWidget.__init__(self, *args)
        uic.loadUi("synce-kdm.ui", self)

    @pyqtSignature("")
    def on_pushButton_Refresh_clicked(self):
        self.listInstalledPrograms.clear()
        powerStatus = rapi_session.getSystemPowerStatus(True)
        self.batteryStatus.setValue( powerStatus["BatteryLifePercent"] ) 
        for program in config_query_get(rapi_session, None ,   "UnInstall").children.values():
            self.listInstalledPrograms.addItem( program.type ) 


		

    
    @pyqtSignature("")
    def on_pushButton_Quit_clicked(self):
        QApplication.quit()


app = QtGui.QApplication(sys.argv)
mainWindowsSynceKDM = MainWindowsSynceKDM()
mainWindowsSynceKDM.show()
app.exec_()

        
