/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _RakiWorkerThread_H_
#define _RakiWorkerThread_H_

#include <qthread.h>
#include <klistbox.h>
#include <rapi.h>


/**
 * 
 * Volker Christian,,,
 **/


class Manager;
class Progress;

class RakiWorkerThread : public QThread
{

protected:
    RakiWorkerThread();
    ~RakiWorkerThread();
    
public:
    enum threadType {
        SOFTWARE_FETCHER= 1,
        SYSINFO_FETCHER,
        BATTERYSTATUS_FETCHER,
        SOFTWARE_INSTALLER,
        SOFTWARE_UNINSTALLER
    };
    
    struct sysinfo_s {
        CEOSVERSIONINFO version;
        SYSTEM_INFO system;
        STORE_INFORMATION store;
        SYSTEM_POWER_STATUS_EX power;
    };
    
    void run();
    void start(Manager *manager, enum threadType type);
    void start(Progress *progress, QString fileName, enum threadType type);
    void start(Manager *manager, QListBoxItem *item, enum RakiWorkerThread::threadType type);
    void stop();
    
private:
    void fetchSoftwareList();
    void fetchSystemInfo();
    void fetchBatteryStatus();
    void installSoftware();
    void uninstallSoftware();
    
    Manager *manager;
    Progress *progress;
    enum threadType type;
    bool isRunning;
    QString fileName;
    QListBoxItem *item;

public:
    static RakiWorkerThread *rakiWorkerThread;
};

#include "management.h"
#include "progressbar.h"

#endif
