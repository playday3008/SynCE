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

import libxml2
import logging


import os

from synceKPM.gui.ui_synce_kpm_installwindow import *
import synceKPM.gui.installprogresswindow 
import synceKPM.constants

import dbus

class installwindow(QtGui.QWidget, Ui_synce_kpm_installwindow):
    def __init__(self, dbusGui):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)
        self.deviceListRoot = [] 
        self.dbusGui = dbusGui
        self.iconFolder = QtGui.QIcon( "%s/folder.png"%resource_filename('synceKPM', 'data'))

        self.openFileChooserButton.setIcon( self.iconFolder )
        self.progressWindow = synceKPM.gui.installprogresswindow.installprogresswindow()

    
    def showEvent(self,event):
        self.localCabFile.setText("")
        self.deleteCAB.setChecked(False)


    
    
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

    
    @pyqtSignature("")
    def on_okButton_clicked(self):
        self.hide()
        self.progressWindow.show()
        self.progressWindow.labelProgress.setText("Copying <b>%s</b> to the device:"%os.path.basename(unicode(self.localCabFile.text())))
        
        dataServer = dbus.SessionBus().get_object("org.synce.kpm.dataserver","/org/synce/kpm/DataServer")
        dataServer.installProgram(  unicode(self.localCabFile.text()), 
                                    unicode(self.deviceListRoot[self.deviceList.currentIndex()]), self.deleteCAB.isChecked() , 
                                    dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_DATASERVER_IFACE,
                                    reply_handler=self.handle_installProgram,
                                    error_handler=self.handle_installProgram_error,
                                    timeout=600)


    def handle_installProgram(self):
        self.progressWindow.hide()
        self.hide()
        
    def handle_installProgram_error(self,error):
        print "Something went wrong with installing"
        print error
        self.hide()

