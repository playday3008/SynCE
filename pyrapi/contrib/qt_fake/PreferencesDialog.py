# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'PreferencesDialog.ui'
#
# Created: Sam Mai 1 11:01:00 2004
#      by: The PyQt User Interface Compiler (pyuic) 3.11
#
# WARNING! All changes made in this file will be lost!


from qt import *
# Needed for Qtopia / Zaurus
Qt.WState_Polished      = 8192


class Preferences(QDialog):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QDialog.__init__(self,parent,name,modal,fl)

        if not name:
            self.setName("Preferences")


        PreferencesLayout = QGridLayout(self,1,1,11,6,"PreferencesLayout")

        self.textLabelCvsInfo = QLabel(self,"textLabelCvsInfo")
        self.textLabelCvsInfo.setAlignment(QLabel.AlignBottom | QLabel.AlignRight)

        PreferencesLayout.addWidget(self.textLabelCvsInfo,5,0)

        self.checkBoxDebugRename = QCheckBox(self,"checkBoxDebugRename")

        PreferencesLayout.addWidget(self.checkBoxDebugRename,0,0)

        self.checkBoxDebugCopy = QCheckBox(self,"checkBoxDebugCopy")

        PreferencesLayout.addWidget(self.checkBoxDebugCopy,1,0)

        layout18 = QHBoxLayout(None,0,6,"layout18")

        self.pushButtonCancel = QPushButton(self,"pushButtonCancel")
        layout18.addWidget(self.pushButtonCancel)

        self.pushButtonOK = QPushButton(self,"pushButtonOK")
        layout18.addWidget(self.pushButtonOK)

        PreferencesLayout.addLayout(layout18,4,0)

        self.checkBoxDebugMouse = QCheckBox(self,"checkBoxDebugMouse")

        PreferencesLayout.addWidget(self.checkBoxDebugMouse,2,0)

        self.checkBoxShowSymLinks = QCheckBox(self,"checkBoxShowSymLinks")

        PreferencesLayout.addWidget(self.checkBoxShowSymLinks,3,0)

        self.languageChange()

        self.resize(QSize(287,216).expandedTo(self.minimumSizeHint()))
        self.clearWState(Qt.WState_Polished)

        self.connect(self.pushButtonCancel,SIGNAL("clicked()"),self,SLOT("reject()"))
        self.connect(self.pushButtonOK,SIGNAL("clicked()"),self,SLOT("accept()"))


    def languageChange(self):
        self.setCaption(self.__tr("Preferences"))
        self.textLabelCvsInfo.setText(self.__tr("$Revision$, $Date$"))
        self.checkBoxDebugRename.setText(self.__tr("Debug Rename"))
        self.checkBoxDebugCopy.setText(self.__tr("Debug Copy"))
        self.pushButtonCancel.setText(self.__tr("Cancel"))
        self.pushButtonOK.setText(self.__tr("OK"))
        self.checkBoxDebugMouse.setText(self.__tr("Debug Mouse"))
        self.checkBoxShowSymLinks.setText(self.__tr("Show Symbolic Links"))


    def __tr(self,s,c = None):
        return qApp.translate("Preferences",s,c)
