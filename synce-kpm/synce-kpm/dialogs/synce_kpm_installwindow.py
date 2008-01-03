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


class synce_kpm_installwindow(QtGui.QWidget, dialogs.ui_synce_kpm_installwindow.Ui_synce_kpm_installwindow):
    def __init__(self, _phoneCommunicator):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)
        self.phoneCommunicator = _phoneCommunicator

        self.deviceListRoot = [] 
        
    
    def showEvent(self,event):
        self.deviceList.clear()
        self.deviceListRoot = []

        self.localCabFile.setText("")
        self.deleteCAB.setChecked(False)

        if self.phoneCommunicator.phoneConnected:
			
            rapi_session = RAPISession(SYNCE_LOG_LEVEL_DEFAULT)
            
            #Amount free on mainMemory:
            freeDisk,totalDisk,freeDiskTotal = rapi_session.getDiskFreeSpaceEx("\\")
            self.deviceList.addItem("MainMemory [Free: %.2fMB , Total: %.2fMB]" % (freeDisk/(1024.0*1024.0),totalDisk/(1024.0*1024.0)) )
            self.deviceListRoot.append("\\")

            

            myFiles = rapi_session.findAllFiles("\\*.*",FAF_FOLDERS_ONLY|FAF_ATTRIBUTES| FAF_NAME)
            
            for folder in myFiles:
                if folder["Attributes"] & FILE_ATTRIBUTE_TEMPORARY:
                    freeDisk,totalDisk,freeDiskTotal = rapi_session.getDiskFreeSpaceEx("\\"+folder["Name"]+"\\")
                    
                    self.deviceList.addItem("/%s [Free: %.2fMB , Total: %.2fMB]" % (folder["Name"],freeDisk/(1024.0*1024.0),totalDisk/(1024.0*1024.0)) )
                    self.deviceListRoot.append("\\%s\\"%folder["Name"])

    
    
    @pyqtSignature("")
    def on_openFileChooserButton_clicked(self):
        dir = "."
        formats = [ "*.cab"] 
        fname = unicode(QFileDialog.getOpenFileName(self, "SynCE-KPM - Choose CAB", dir, "Microsoft Cabinet files (%s)"%" ".join(formats)))

        if not fname:
            return
        
        self.localCabFile.setText( fname ) 
        #print "Selected the file %s for opening" % fname


    @pyqtSignature("")
    def on_cancelButton_clicked(self):
        self.hide()

    def copy_progress_cb( self, progress ):
        print "%i percent"%progress
    
    @pyqtSignature("")
    def on_okButton_clicked(self):
        #Install the file through rapiutil

        self.phoneCommunicator.installProgram( unicode(self.localCabFile.text()), self.deviceListRoot[self.deviceList.currentIndex()], self.deleteCAB.isChecked() , self.copy_progress_cb )
        self.hide()

