/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _RakiEvent_H_
#define _RakiEvent_H_

#include <qevent.h>
#include <qapplication.h>
#include <qlistbox.h>
#include "workerthreadinterface.h"

/**
 * 
 * Volker Christian,,,
 **/
class RakiEvent : public QCustomEvent
{
public:
  enum eventTypes {
    ERROR = 1,
    BEGIN,
    SYSINFO,
    BATINFO,
    PROGRESS,
    FINISHED,
    UNINSTALLED,
    INSTALLED,
    END
  };
  
public:
  RakiEvent(enum eventTypes lt);
  RakiEvent(enum eventTypes lt, struct WorkerThreadInterface::sysinfo_s sysinfo);
  RakiEvent(enum eventTypes lt, QString msg);
  RakiEvent(enum eventTypes lt, QListBoxItem *item);
  
  ~RakiEvent();
  enum eventTypes eventType();
  struct WorkerThreadInterface::sysinfo_s getSysinfo();
  QString getMessage();
  QListBoxItem *getItem();

private:
    enum eventTypes lt;
    struct WorkerThreadInterface::sysinfo_s sysinfo;
    QString msg;
    QListBoxItem *item;
};

#endif
