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



import dialogs.ui_synce_kpm_copycab_progresswindow


class synce_kpm_copycab_progresswindow(QtGui.QWidget, dialogs.ui_synce_kpm_copycab_progresswindow.Ui_synce_kpm_copycab_progresswindow):
    def __init__(self):
        QtGui.QWidget.__init__(self)
        self.setupUi(self)

        self.labelProgress.setText("")
        self.copyProgress.setValue(0)


        
    
    def showEvent(self,event):
        self.labelProgress.setText("")
        self.copyProgress.setValue(0)

