/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _CONFIGDIALOGIMPL_H_
#define _CONFIGDIALOGIMPL_H_


#include <kconfig.h>
#include <configdialog.h>


/**
 * 
 * Volker Christian,,,
 **/
class ConfigDialogImpl : public ConfigDialog
{

public:
    ConfigDialogImpl(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);
    ~ConfigDialogImpl();
    QString getPassword();
    QString getDccmPath();
    QString getSynceStart();
    QString getSynceStop();
    QString getIpTables();
    bool getUsePassword();
    bool getStartDccm();
    bool getMasqueradeEnabled();
    
private slots:
    void applySlot();
    void changedSlot();
    void pathChangedSlot();
    void masqChangedSlot();
    void disableApply();
    
private:
    void readConfig();
    void updateFields();
    
    KConfig *ksConfig;
    QString password;
    QString dccmPath;
    QString synceStart;
    QString synceStop;
    QString ipTables;
    bool startDccm;
    bool usePassword;
    bool masqEnabled;
    void writeConfig();
    bool dccmChanged;
};

#endif
