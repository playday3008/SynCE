/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _MANAGERIMPL_H_
#define _MANAGERIMPL_H_


#include "management.h"
#include "workerthreadinterface.h"
#include "rakievent.h"

/**
 * 
 * Volker Christian,,,
 **/
class ManagerImpl : public Manager, WorkerThreadInterface
{
public:    
    ManagerImpl(QWidget* parent = 0, const char* name = 0, bool modal = FALSE, WFlags fl = 0);
    ~ManagerImpl();

    void closeEvent(QCloseEvent *e);
    void customEvent (QCustomEvent *e);
    void work(QThread *qt);
    
private slots:
    void uninstallSoftwareSlot();
    void refreshSystemInfoSlot();
    void refreshSoftwareSlot();
    void refreshBatteryStatusSlot();

private:
    void uninstallSoftware();
    void fetchSystemInfo();
    void fetchBatteryStatus();
    void fetchSoftwareList();
};

#endif
