# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'PreferencesDialog.ui'
#
# Created: Mit Apr 21 08:48:57 2004
#      by: The PyQt User Interface Compiler (pyuic) 3.8.1
#
# WARNING! All changes made in this file will be lost!


from qt import *


class Preferences(QDialog):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QDialog.__init__(self,parent,name,modal,fl)

        if not name:
            self.setName("Preferences")


        PreferencesLayout = QGridLayout(self,1,1,11,6,"PreferencesLayout")

        layout17 = QVBoxLayout(None,0,6,"layout17")

        self.checkBoxDebugRename = QCheckBox(self,"checkBoxDebugRename")
        layout17.addWidget(self.checkBoxDebugRename)

        self.checkBoxDebugCopy = QCheckBox(self,"checkBoxDebugCopy")
        layout17.addWidget(self.checkBoxDebugCopy)

        PreferencesLayout.addLayout(layout17,0,0)

        self.textLabel1 = QLabel(self,"textLabel1")
        self.textLabel1.setAlignment(QLabel.AlignBottom | QLabel.AlignRight)

        PreferencesLayout.addWidget(self.textLabel1,2,0)

        layout18 = QHBoxLayout(None,0,6,"layout18")

        self.pushButtonCancel = QPushButton(self,"pushButtonCancel")
        layout18.addWidget(self.pushButtonCancel)

        self.pushButtonOK = QPushButton(self,"pushButtonOK")
        layout18.addWidget(self.pushButtonOK)

        PreferencesLayout.addLayout(layout18,1,0)

        self.languageChange()

        self.resize(QSize(223,208).expandedTo(self.minimumSizeHint()))
        self.clearWState(Qt.WState_Polished)

        self.connect(self.pushButtonCancel,SIGNAL("clicked()"),self,SLOT("reject()"))
        self.connect(self.pushButtonOK,SIGNAL("clicked()"),self,SLOT("accept()"))


    def languageChange(self):
        self.setCaption(self.__tr("Preferences"))
        self.checkBoxDebugRename.setText(self.__tr("Debug Rename"))
        self.checkBoxDebugCopy.setText(self.__tr("Debug Copy"))
        self.textLabel1.setText(self.__tr("2004-04-21 1.0"))
        self.pushButtonCancel.setText(self.__tr("Cancel"))
        self.pushButtonOK.setText(self.__tr("OK"))


    def __tr(self,s,c = None):
        return qApp.translate("Preferences",s,c)
