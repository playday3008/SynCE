# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'synce-kpm-copycab-progresswindow.ui'
#
# Created: Sun Jan 20 00:52:55 2008
#      by: PyQt4 UI code generator 4.3.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_synce_kpm_copycab_progresswindow(object):
    def setupUi(self, synce_kpm_copycab_progresswindow):
        synce_kpm_copycab_progresswindow.setObjectName("synce_kpm_copycab_progresswindow")
        synce_kpm_copycab_progresswindow.resize(QtCore.QSize(QtCore.QRect(0,0,400,84).size()).expandedTo(synce_kpm_copycab_progresswindow.minimumSizeHint()))

        self.labelProgress = QtGui.QLabel(synce_kpm_copycab_progresswindow)
        self.labelProgress.setGeometry(QtCore.QRect(10,10,351,18))
        self.labelProgress.setObjectName("labelProgress")

        self.copyProgress = QtGui.QProgressBar(synce_kpm_copycab_progresswindow)
        self.copyProgress.setGeometry(QtCore.QRect(10,40,381,23))
        self.copyProgress.setProperty("value",QtCore.QVariant(24))
        self.copyProgress.setObjectName("copyProgress")

        self.retranslateUi(synce_kpm_copycab_progresswindow)
        QtCore.QMetaObject.connectSlotsByName(synce_kpm_copycab_progresswindow)

    def retranslateUi(self, synce_kpm_copycab_progresswindow):
        synce_kpm_copycab_progresswindow.setWindowTitle(QtGui.QApplication.translate("synce_kpm_copycab_progresswindow", "Progress copy file", None, QtGui.QApplication.UnicodeUTF8))
        self.labelProgress.setText(QtGui.QApplication.translate("synce_kpm_copycab_progresswindow", "TextLabel", None, QtGui.QApplication.UnicodeUTF8))

