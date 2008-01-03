# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'synce-kpm-mainwindow.ui'
#
# Created: Thu Jan  3 22:16:35 2008
#      by: PyQt4 UI code generator 4.3.1
#
# WARNING! All changes made in this file will be lost!

from PyQt4 import QtCore, QtGui

class Ui_synce_kpm_mainwindow(object):
    def setupUi(self, synce_kpm_mainwindow):
        synce_kpm_mainwindow.setObjectName("synce_kpm_mainwindow")
        synce_kpm_mainwindow.resize(QtCore.QSize(QtCore.QRect(0,0,582,494).size()).expandedTo(synce_kpm_mainwindow.minimumSizeHint()))

        self.groupBox = QtGui.QGroupBox(synce_kpm_mainwindow)
        self.groupBox.setGeometry(QtCore.QRect(30,10,521,101))
        self.groupBox.setObjectName("groupBox")

        self.label = QtGui.QLabel(self.groupBox)
        self.label.setGeometry(QtCore.QRect(10,20,71,16))
        self.label.setObjectName("label")

        self.label_2 = QtGui.QLabel(self.groupBox)
        self.label_2.setGeometry(QtCore.QRect(10,40,71,16))
        self.label_2.setObjectName("label_2")

        self.batteryStatus = QtGui.QProgressBar(self.groupBox)
        self.batteryStatus.setGeometry(QtCore.QRect(100,40,118,16))

        font = QtGui.QFont()
        font.setPointSize(9)
        self.batteryStatus.setFont(font)
        self.batteryStatus.setProperty("value",QtCore.QVariant(0))
        self.batteryStatus.setInvertedAppearance(False)
        self.batteryStatus.setObjectName("batteryStatus")

        self.label_devicename = QtGui.QLabel(self.groupBox)
        self.label_devicename.setGeometry(QtCore.QRect(100,20,44,14))
        self.label_devicename.setObjectName("label_devicename")

        self.labelDeviceName = QtGui.QLabel(self.groupBox)
        self.labelDeviceName.setGeometry(QtCore.QRect(100,20,71,16))
        self.labelDeviceName.setObjectName("labelDeviceName")

        self.pushButton_Quit = QtGui.QPushButton(synce_kpm_mainwindow)
        self.pushButton_Quit.setGeometry(QtCore.QRect(470,440,75,24))
        self.pushButton_Quit.setObjectName("pushButton_Quit")

        self.tabWidget = QtGui.QTabWidget(synce_kpm_mainwindow)
        self.tabWidget.setGeometry(QtCore.QRect(30,120,521,311))
        self.tabWidget.setObjectName("tabWidget")

        self.tab = QtGui.QWidget()
        self.tab.setObjectName("tab")
        self.tabWidget.addTab(self.tab,"")

        self.tab_2 = QtGui.QWidget()
        self.tab_2.setObjectName("tab_2")

        self.listInstalledPrograms = QtGui.QListWidget(self.tab_2)
        self.listInstalledPrograms.setGeometry(QtCore.QRect(10,30,411,211))
        self.listInstalledPrograms.setObjectName("listInstalledPrograms")

        self.pushButton_Refresh = QtGui.QPushButton(self.tab_2)
        self.pushButton_Refresh.setGeometry(QtCore.QRect(430,30,75,24))
        self.pushButton_Refresh.setObjectName("pushButton_Refresh")

        self.pushButton_Uninstall = QtGui.QPushButton(self.tab_2)
        self.pushButton_Uninstall.setGeometry(QtCore.QRect(430,60,75,24))
        self.pushButton_Uninstall.setObjectName("pushButton_Uninstall")

        self.pushButton_InstallCAB = QtGui.QPushButton(self.tab_2)
        self.pushButton_InstallCAB.setGeometry(QtCore.QRect(10,250,75,24))
        self.pushButton_InstallCAB.setObjectName("pushButton_InstallCAB")
        self.tabWidget.addTab(self.tab_2,"")

        self.retranslateUi(synce_kpm_mainwindow)
        self.tabWidget.setCurrentIndex(0)
        QtCore.QMetaObject.connectSlotsByName(synce_kpm_mainwindow)

    def retranslateUi(self, synce_kpm_mainwindow):
        synce_kpm_mainwindow.setWindowTitle(QtGui.QApplication.translate("synce_kpm_mainwindow", "SynCE KDE PDA Manager", None, QtGui.QApplication.UnicodeUTF8))
        self.groupBox.setTitle(QtGui.QApplication.translate("synce_kpm_mainwindow", "Device Status", None, QtGui.QApplication.UnicodeUTF8))
        self.label.setText(QtGui.QApplication.translate("synce_kpm_mainwindow", "Device:", None, QtGui.QApplication.UnicodeUTF8))
        self.label_2.setText(QtGui.QApplication.translate("synce_kpm_mainwindow", "Battery status:", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_Quit.setText(QtGui.QApplication.translate("synce_kpm_mainwindow", "&Quit", None, QtGui.QApplication.UnicodeUTF8))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab), QtGui.QApplication.translate("synce_kpm_mainwindow", "System Information", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_Refresh.setText(QtGui.QApplication.translate("synce_kpm_mainwindow", "&Refresh", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_Uninstall.setText(QtGui.QApplication.translate("synce_kpm_mainwindow", "&Uninstall", None, QtGui.QApplication.UnicodeUTF8))
        self.pushButton_InstallCAB.setText(QtGui.QApplication.translate("synce_kpm_mainwindow", "&Install CAB", None, QtGui.QApplication.UnicodeUTF8))
        self.tabWidget.setTabText(self.tabWidget.indexOf(self.tab_2), QtGui.QApplication.translate("synce_kpm_mainwindow", "Software Manager", None, QtGui.QApplication.UnicodeUTF8))

