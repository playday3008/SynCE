/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include <qcheckbox.h>
#include <klineedit.h>
#include <kpushbutton.h>

#include "configdialogimpl.h"
#include "raki.h"


ConfigDialogImpl::ConfigDialogImpl(QWidget* parent, const char* name, bool modal, WFlags fl)
  : ConfigDialog(parent, name, modal, fl)
{    
    startDccm = true;
    usePassword = false;
    dccmPath = "/usr/local/bin/dccm";
    password = "";
    readConfig();
    updateFields();
}


ConfigDialogImpl::~ConfigDialogImpl()
{
}


void ConfigDialogImpl::updateFields()
{
    startDccmCheckbox->setChecked(startDccm);
    passwordCheckbox->setChecked(usePassword);
    passwordEdit->setText(password);
    buttonApply->setDisabled(true);
    dccmPathInput->setText(dccmPath);
}


bool ConfigDialogImpl::getUsePassword()
{
    return usePassword;
}


bool ConfigDialogImpl::getStartDccm()
{
    return startDccm;
}


QString ConfigDialogImpl::getPassword()
{
    return password;
}


QString ConfigDialogImpl::getDccmPath()
{
    return dccmPath;
}


void ConfigDialogImpl::writeConfig()
{
    ksConfig->setGroup("DCCM");
    ksConfig->writeEntry("StartDccm", startDccm);
    ksConfig->writeEntry("UsePassword", usePassword);
    ksConfig->writeEntry("Password", password);
    ksConfig->writeEntry("DccmPath", dccmPath);
    ksConfig->sync();
}


void ConfigDialogImpl::readConfig()
{
    ksConfig=kapp->config();
    ksConfig->setGroup("DCCM");
    startDccm = ksConfig->readBoolEntry("StartDccm");
    usePassword = ksConfig->readBoolEntry("UsePassword");
    password = ksConfig->readEntry("Password");
    dccmPath = ksConfig->readEntry("DccmPath");
    if (dccmPath.isEmpty())
        dccmPath = "dccm";
}


void ConfigDialogImpl::applySlot()
{
    if (buttonApply->isEnabled()) {
        this->password = passwordEdit->text();
        this->dccmPath = dccmPathInput->text();
        startDccm = startDccmCheckbox->isChecked();
        usePassword = passwordCheckbox->isChecked();
        writeConfig();
        buttonApply->setDisabled(true);
        ((Raki *) parent())->restartDccm();
    }
}
