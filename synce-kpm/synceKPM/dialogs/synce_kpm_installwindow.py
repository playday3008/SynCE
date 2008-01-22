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


import dialogs.ui_synce_kpm_installwindow
import dialogs.synce_kpm_copycab_progresswindow
import threading

class InstallerThread(threading.Thread):
    def __init__ (self,phoneCommunicator, localCabFilePath, destinationPath,deleteCab, copyProgress_cb):
        Thread.__init__(self)
        self.phoneCommunicator= phoneCommunicator
        self.localCabFilePath = localCabFilePath
        self.destinationPath  = destinationPath
        self.deleteCab        = deleteCab
        self.copyProgress_cb = copyProgress_cb
    
    def run(self):
        self.phoneCommunicator.installProgram( self.localCabFilePath , self.destinationPath, self.deleteCab, self.copyProgress_cb )

        #We are finished with copying
        if self.copyProgress_cb is not None:
            self.copyProgress_cb(-1)




class synce_kpm_installwindow(QtGui.QWidget, dialogs.ui_synce_kpm_installwindow.Ui_synce_kpm_installwindow):
    def __init__(self, _phoneCommunicator):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)
        self.phoneCommunicator = _phoneCommunicator
        self.deviceListRoot = [] 

        self.progressWindow = dialogs.synce_kpm_copycab_progresswindow.synce_kpm_copycab_progresswindow()

    
    def showEvent(self,event):
        self.deviceList.clear()
        self.deviceListRoot = []

        self.localCabFile.setText("")
        self.deleteCAB.setChecked(False)

        if self.phoneCommunicator.phoneConnected:
            for storageItem in self.phoneCommunicator.getStorageInformation():
                storageName, storageLocation, freeDisk,totalDisk,freeDiskTotal = storageItem
                self.deviceList.addItem("%s [Free: %.2fMB , Total: %.2fMB]" % (storageName,freeDisk/(1024.0*1024.0),totalDisk/(1024.0*1024.0)) )
                self.deviceListRoot.append(storageLocation)


    
    
    @pyqtSignature("")
    def on_openFileChooserButton_clicked(self):
        dir = "."
        formats = [ "*.cab"] 
        fname = unicode(QFileDialog.getOpenFileName(self, "SynCE-KPM - Choose CAB", dir, "Microsoft Cabinet files (%s)"%" ".join(formats)))

        if not fname:
            return
        
        self.localCabFile.setText( fname ) 


    @pyqtSignature("")
    def on_cancelButton_clicked(self):
        self.hide()

    def copy_progress_cb( self, progress ):
        print "%i percent"%progress
    
    @pyqtSignature("")
    def on_okButton_clicked(self):

        self.progressWindow.show()
        self.progressWindow.labelProgress.setText("Copying <b>%s</b> to the device:"%os.path.basename(unicode(self.localCabFile.text())))
        #Create a new thread
        installerThread = InstallerThread( self.phoneCommunicator, unicode(self.localCabFile.text()), self.deviceListRoot[self.deviceList.currentIndex()], self.deleteCAB.isChecked() , self.progressWindow.updateProgress_cb)

        installerThread.start()
        
        self.hide()

