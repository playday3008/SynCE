/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _WORKERTHREADINTERFACE_H_
#define _WORKERTHREADINTERFACE_H_

#include <qthread.h>
#include <rapi.h>

/**
 * 
 * Volker Christian,,,
 **/
class WorkerThreadInterface
{

public:
    struct sysinfo_s {
        synce::CEOSVERSIONINFO version;
        synce::SYSTEM_INFO system;
        synce::STORE_INFORMATION store;
        synce::SYSTEM_POWER_STATUS_EX power;
    };
    
    enum threadType {
        SOFTWARE_FETCHER= 1,
        SYSINFO_FETCHER,
        BATTERYSTATUS_FETCHER,
        SOFTWARE_INSTALLER,
        SOFTWARE_UNINSTALLER
    };
    
  WorkerThreadInterface();
  virtual ~WorkerThreadInterface();
  virtual void work(QThread *qThread) = 0;
  enum threadType type;
  int isRunning;
};

#endif
