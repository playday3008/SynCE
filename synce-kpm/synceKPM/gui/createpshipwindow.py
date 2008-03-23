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

import logging


from synceKPM.gui.ui_synce_kpm_create_pshipwindow import *
import synceKPM.constants

class createpshipwindow(QtGui.QWidget, synceKPM.gui.ui_synce_kpm_create_pshipwindow.Ui_synce_kpm_create_pshipwindow):
    def __init__(self, mainwindow):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)
        self.guiDbus = mainwindow.guiDbus
        self.modelSyncItems = QStandardItemModel(0,2)
        self.viewSyncItems.setModel(self.modelSyncItems)
        self.mainwindow = mainwindow

            




        
    
    def showEvent(self,event):
        self.modelSyncItems.clear()

        #avail_sync_items, __foo = self.phoneCommunicator.getPartnerships()
        avail_sync_items = self.mainwindow.sync_items
        
        name_to_id = {}

        i=0 
        for sync_item in avail_sync_items:
            myItem = QStandardItem( avail_sync_items[ sync_item ] )
            name_to_id[ avail_sync_items[sync_item] ] = i
            i += 1
            myItem.setCheckable(True)
            myItem.setEnabled( False )
            #Set the data of the item to the id number, presented by sync-engine
            self.modelSyncItems.appendRow( myItem )
        
        #And now enable only Files,Calendar,Tasks,Contacts
        self.modelSyncItems.item( name_to_id["Files"] ).setEnabled(True)
        self.modelSyncItems.item( name_to_id["Contacts"] ).setEnabled(True)
        self.modelSyncItems.item( name_to_id["Tasks"] ).setEnabled(True)
        self.modelSyncItems.item( name_to_id["Calendar"] ).setEnabled(True)
            
    
    
    @pyqtSignature("")
    def on_button_create_clicked(self):
        #Going to try and create the partnership now
        if  self.namePartnership.text() == "":
            QMessageBox.warning(self,"Error in user input", "Please provide a name for the partnership",QMessageBox.Ok)
            return
        

        avail_sync_items = self.mainwindow.sync_items
        name_to_id = {}
        for id, name in avail_sync_items.items():
            name_to_id[name] = id

        items = []
        
        for i in range( self.modelSyncItems.rowCount() ):
            if self.modelSyncItems.item( i ).checkState():
                items.append( name_to_id[ unicode (self.modelSyncItems.item( i ).text()) ] )

        if len(items) == 0: 
            QMessageBox.warning(self,"Error in user input", "Please make sure to select at least one sync-item",QMessageBox.Ok)
            return

        #print "Name=%s, items="%self.namePartnership.text() , items
        self.guiDbus.createPartnership( unicode(self.namePartnership.text()), items) 

        self.hide()

    

    @pyqtSignature("")
    def on_button_cancel_clicked(self):
        self.hide()

