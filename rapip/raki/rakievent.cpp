/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include "rakievent.h"


RakiEvent::RakiEvent(enum eventTypes lt)
  : QCustomEvent(QEvent::User)
{
    this->lt = lt;
}


RakiEvent::RakiEvent(enum eventTypes lt, struct WorkerThreadInterface::sysinfo_s sysinfo)
  : QCustomEvent(QEvent::User)
{
    this->lt = lt;
    this->sysinfo = sysinfo;
}


RakiEvent::RakiEvent(enum eventTypes lt, QString msg)
  : QCustomEvent(QEvent::User)
{
    this->lt = lt;
    this->msg = msg;
}


RakiEvent::RakiEvent(enum eventTypes lt, QListBoxItem *item)
  : QCustomEvent(QEvent::User)
{
    this->lt = lt;
    this->item = item;
}


RakiEvent::~RakiEvent()
{
}


enum RakiEvent::eventTypes RakiEvent::eventType()
{
    return lt;
}



struct WorkerThreadInterface::sysinfo_s RakiEvent::getSysinfo()
{
    return sysinfo;
}


QString RakiEvent::getMessage()
{
    return msg;
}


/*
int RakiEvent::getProgress()
{
    return progressValue;
}*/


QListBoxItem *RakiEvent::getItem()
{
    return item;
}
