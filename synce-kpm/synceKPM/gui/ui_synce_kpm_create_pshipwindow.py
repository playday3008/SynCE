# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'synce-kpm-create-pshipwindow.ui'
#
# Created: Mon Mar 24 17:00:28 2008
#      by: PyQt4 UI code generator 4.3.3
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_synce_kpm_create_pshipwindow(object):
    def setupUi(self, synce_kpm_create_pshipwindow):
        synce_kpm_create_pshipwindow.setObjectName("synce_kpm_create_pshipwindow")
        synce_kpm_create_pshipwindow.setWindowModality(QtCore.Qt.ApplicationModal)
        synce_kpm_create_pshipwindow.resize(QtCore.QSize(QtCore.QRect(0,0,369,330).size()).expandedTo(synce_kpm_create_pshipwindow.minimumSizeHint()))

        self.viewSyncItems = QtGui.QListView(synce_kpm_create_pshipwindow)
        self.viewSyncItems.setGeometry(QtCore.QRect(20,90,331,191))
        self.viewSyncItems.setObjectName("viewSyncItems")

        self.button_create = QtGui.QPushButton(synce_kpm_create_pshipwindow)
        self.button_create.setGeometry(QtCore.QRect(270,290,81,28))
        self.button_create.setObjectName("button_create")

        self.button_cancel = QtGui.QPushButton(synce_kpm_create_pshipwindow)
        self.button_cancel.setGeometry(QtCore.QRect(170,290,81,28))
        self.button_cancel.setObjectName("button_cancel")

        self.namePartnership = QtGui.QLineEdit(synce_kpm_create_pshipwindow)
        self.namePartnership.setGeometry(QtCore.QRect(170,30,181,24))
        self.namePartnership.setMaxLength(20)
        self.namePartnership.setObjectName("namePartnership")

        self.label = QtGui.QLabel(synce_kpm_create_pshipwindow)
        self.label.setGeometry(QtCore.QRect(20,30,141,18))
        self.label.setObjectName("label")

        self.label_2 = QtGui.QLabel(synce_kpm_create_pshipwindow)
        self.label_2.setGeometry(QtCore.QRect(20,70,141,18))
        self.label_2.setObjectName("label_2")

        self.retranslateUi(synce_kpm_create_pshipwindow)
        QtCore.QMetaObject.connectSlotsByName(synce_kpm_create_pshipwindow)

    def retranslateUi(self, synce_kpm_create_pshipwindow):
        synce_kpm_create_pshipwindow.setWindowTitle(QtGui.QApplication.translate("synce_kpm_create_pshipwindow", "Create Partnership", None, QtGui.QApplication.UnicodeUTF8))
        self.button_create.setText(QtGui.QApplication.translate("synce_kpm_create_pshipwindow", "Create", None, QtGui.QApplication.UnicodeUTF8))
        self.button_cancel.setText(QtGui.QApplication.translate("synce_kpm_create_pshipwindow", "Cancel", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("synce_kpm_create_pshipwindow", "Name of partnership:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("synce_kpm_create_pshipwindow", "Sync Items:", None, QtGui.QApplication.UnicodeUTF8))

