# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'synce-kpm-installwindow.ui'
#
# Created: Sun Jan 20 00:57:43 2008
#      by: PyQt4 UI code generator 4.3.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_synce_kpm_installwindow(object):
    def setupUi(self, synce_kpm_installwindow):
        synce_kpm_installwindow.setObjectName("synce_kpm_installwindow")
        synce_kpm_installwindow.setWindowModality(QtCore.Qt.ApplicationModal)
        synce_kpm_installwindow.resize(QtCore.QSize(QtCore.QRect(0,0,533,166).size()).expandedTo(synce_kpm_installwindow.minimumSizeHint()))

        self.localCabFile = QtGui.QLineEdit(synce_kpm_installwindow)
        self.localCabFile.setGeometry(QtCore.QRect(160,30,331,24))
        self.localCabFile.setReadOnly(True)
        self.localCabFile.setObjectName("localCabFile")

        self.label = QtGui.QLabel(synce_kpm_installwindow)
        self.label.setGeometry(QtCore.QRect(10,30,91,16))
        self.label.setObjectName("label")

        self.openFileChooserButton = QtGui.QToolButton(synce_kpm_installwindow)
        self.openFileChooserButton.setGeometry(QtCore.QRect(500,30,26,25))
        self.openFileChooserButton.setIcon(QtGui.QIcon("synce-kpm/data/folder.png"))
        self.openFileChooserButton.setIconSize(QtCore.QSize(16,16))
        self.openFileChooserButton.setObjectName("openFileChooserButton")

        self.deleteCAB = QtGui.QCheckBox(synce_kpm_installwindow)
        self.deleteCAB.setGeometry(QtCore.QRect(10,100,291,19))
        self.deleteCAB.setLayoutDirection(QtCore.Qt.LeftToRight)
        self.deleteCAB.setObjectName("deleteCAB")

        self.label_2 = QtGui.QLabel(synce_kpm_installwindow)
        self.label_2.setGeometry(QtCore.QRect(10,60,143,18))
        self.label_2.setObjectName("label_2")

        self.deviceList = QtGui.QComboBox(synce_kpm_installwindow)
        self.deviceList.setGeometry(QtCore.QRect(160,60,331,24))
        self.deviceList.setObjectName("deviceList")

        self.okButton = QtGui.QPushButton(synce_kpm_installwindow)
        self.okButton.setGeometry(QtCore.QRect(360,130,75,24))
        self.okButton.setObjectName("okButton")

        self.cancelButton = QtGui.QPushButton(synce_kpm_installwindow)
        self.cancelButton.setGeometry(QtCore.QRect(450,130,75,24))
        self.cancelButton.setObjectName("cancelButton")

        self.retranslateUi(synce_kpm_installwindow)
        QtCore.QMetaObject.connectSlotsByName(synce_kpm_installwindow)

    def retranslateUi(self, synce_kpm_installwindow):
        synce_kpm_installwindow.setWindowTitle(QtGui.QApplication.translate("synce_kpm_installwindow", "Install CAB File", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("synce_kpm_installwindow", "Install CAB file", None, QtGui.QApplication.UnicodeUTF8))
        self.openFileChooserButton.setText(QtGui.QApplication.translate("synce_kpm_installwindow", "...", None, QtGui.QApplication.UnicodeUTF8))
        self.deleteCAB.setText(QtGui.QApplication.translate("synce_kpm_installwindow", "Delete the CAB file after being started", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("synce_kpm_installwindow", "Temp is root of device:", None, QtGui.QApplication.UnicodeUTF8))
        self.okButton.setText(QtGui.QApplication.translate("synce_kpm_installwindow", "OK", None, QtGui.QApplication.UnicodeUTF8))
        self.cancelButton.setText(QtGui.QApplication.translate("synce_kpm_installwindow", "Cancel", None, QtGui.QApplication.UnicodeUTF8))

