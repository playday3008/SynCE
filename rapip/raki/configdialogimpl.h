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

    void updateFields();
    bool getUsePassword();
    bool getStartDccm();
    QString getPassword();
    QString getDccmPath();

public slots:
    void applySlot();
    
protected:
    QString password;
    QString dccmPath;
    bool startDccm;
    bool usePassword;
    void readConfig();

private:
    KConfig *ksConfig;
    void writeConfig();
};

#endif
