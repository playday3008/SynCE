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
    readConfig();
    updateFields();
    buttonApply->setEnabled(false);
    dccmChanged = false;
}


ConfigDialogImpl::~ConfigDialogImpl()
{
}


void ConfigDialogImpl::updateFields()
{
    startDccmCheckbox->setChecked(startDccm);
    passwordCheckbox->setChecked(usePassword);
    passwordEdit->setText(password);
    dccmPathInput->setText(dccmPath);
    synceStartInput->setText(synceStart);
    synceStopInput->setText(synceStop);
    masqEnabledCheckbox->setChecked(masqEnabled);
    ipTablesInput->setText(ipTables);
    buttonApply->setDisabled(true);
    dccmChanged = false;
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


QString ConfigDialogImpl::getSynceStart()
{
    return synceStart;
}


QString ConfigDialogImpl::getSynceStop()
{
    return synceStop;
}


QString ConfigDialogImpl::getIpTables()
{
    return ipTables;
}


bool ConfigDialogImpl::getMasqueradeEnabled()
{
    return masqEnabled;
}


void ConfigDialogImpl::writeConfig()
{
    ksConfig->setGroup("DCCM");
    ksConfig->writeEntry("StartDccm", startDccm);
    ksConfig->writeEntry("UsePassword", usePassword);
    ksConfig->writeEntry("Password", password);
    ksConfig->writeEntry("DccmPath", dccmPath);
    ksConfig->writeEntry("SynCEStart", synceStart);
    ksConfig->writeEntry("SynCEStop", synceStop);
    ksConfig->writeEntry("Masquerade", masqEnabled);
    ksConfig->writeEntry("IpTables", ipTables);
    ksConfig->sync();
}


void ConfigDialogImpl::readConfig()
{
    ksConfig=kapp->config();
    ksConfig->setGroup("DCCM");
    startDccm = ksConfig->readBoolEntry("StartDccm");
    usePassword = ksConfig->readBoolEntry("UsePassword");
    masqEnabled = ksConfig->readBoolEntry("Masquerade");
    password = ksConfig->readEntry("Password");
    dccmPath = ksConfig->readEntry("DccmPath");
    ipTables = ksConfig->readEntry("IpTables");
    if (dccmPath.isEmpty()) {
        dccmPath = "dccm";
    }
    if (synceStart.isEmpty()) {
        synceStart = "synce-serial-start";
    }
    if (synceStop.isEmpty()) {
        synceStop = "synce-serial-abort";
    }
    if (ipTables.isEmpty()) {
        ipTables = "/usr/sbin/iptables";
    }
}


void ConfigDialogImpl::applySlot()
{
    if (buttonApply->isEnabled()) {
        this->password = passwordEdit->text();
        this->dccmPath = dccmPathInput->text();
        startDccm = startDccmCheckbox->isChecked();
        usePassword = passwordCheckbox->isChecked();
        synceStart = synceStartInput->text();
        synceStop = synceStopInput->text();
        masqEnabled = masqEnabledCheckbox->isChecked();
        ipTables = ipTablesInput->text();
        writeConfig();
        buttonApply->setDisabled(true);
        if (dccmChanged) {
            ((Raki *) parent())->restartDccm();
            dccmChanged = false;
        }
    }
}


void ConfigDialogImpl::changedSlot()
{
    dccmChanged = true;
    buttonApply->setEnabled(true);
}


void ConfigDialogImpl::pathChangedSlot()
{
    buttonApply->setEnabled(true);
}


void ConfigDialogImpl::masqChangedSlot()
{
    buttonApply->setEnabled(true);
}


void ConfigDialogImpl::disableApply()
{
    updateFields();
}
