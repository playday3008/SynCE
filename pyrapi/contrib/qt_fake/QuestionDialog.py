# -*- coding: utf-8 -*-

# Form implementation generated from reading ui file 'QuestionDialog.ui'
#
# Created: Mon Apr 26 19:26:00 2004
#      by: The PyQt User Interface Compiler (pyuic) 3.11
#
# WARNING! All changes made in this file will be lost!


from qt import *
# Needed for Qtopia / Zaurus
Qt.WState_Polished      = 8192


class Question(QDialog):
    def __init__(self,parent = None,name = None,modal = 0,fl = 0):
        QDialog.__init__(self,parent,name,modal,fl)

        if not name:
            self.setName("Question")


        QuestionLayout = QGridLayout(self,1,1,11,6,"QuestionLayout")

        self.textLabelQuestion = QLabel(self,"textLabelQuestion")
        self.textLabelQuestion.setAlignment(QLabel.WordBreak | QLabel.AlignTop)

        QuestionLayout.addWidget(self.textLabelQuestion,0,0)

        layout2 = QHBoxLayout(None,0,6,"layout2")

        self.pushButtonNo = QPushButton(self,"pushButtonNo")
        layout2.addWidget(self.pushButtonNo)

        self.pushButtonYes = QPushButton(self,"pushButtonYes")
        layout2.addWidget(self.pushButtonYes)

        QuestionLayout.addLayout(layout2,1,0)

        self.textLabelCvsInfo = QLabel(self,"textLabelCvsInfo")
        self.textLabelCvsInfo.setAlignment(QLabel.WordBreak | QLabel.AlignBottom | QLabel.AlignRight)

        QuestionLayout.addWidget(self.textLabelCvsInfo,2,0)

        self.languageChange()

        self.resize(QSize(190,157).expandedTo(self.minimumSizeHint()))
        self.clearWState(Qt.WState_Polished)

        self.connect(self.pushButtonNo,SIGNAL("clicked()"),self,SLOT("reject()"))
        self.connect(self.pushButtonYes,SIGNAL("clicked()"),self,SLOT("accept()"))


    def languageChange(self):
        self.setCaption(self.__tr("Question"))
        self.textLabelQuestion.setText(self.__tr("TheQuestion"))
        self.pushButtonNo.setText(self.__tr("No"))
        self.pushButtonYes.setText(self.__tr("Yes"))
        self.textLabelCvsInfo.setText(self.__tr("$Revision$, $Date$"))


    def __tr(self,s,c = None):
        return qApp.translate("Question",s,c)
