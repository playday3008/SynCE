/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/


#include "rakiworkerthread.h"
 
RakiWorkerThread *RakiWorkerThread::rakiWorkerThread = new RakiWorkerThread();


RakiWorkerThread::RakiWorkerThread() : QThread()
{
    wti = NULL;
}


RakiWorkerThread::~RakiWorkerThread()
{}


void RakiWorkerThread::start(WorkerThreadInterface *wti, enum WorkerThreadInterface::threadType type)
{
    this->wti = wti;
    this->wti->type = type;
    this->wti->isRunning = true;
    QThread::start();
}


void RakiWorkerThread::stop()
{
    if (wti) {
        wti->isRunning = false;
        wait();
    }
}


void RakiWorkerThread::run()
{
    wti->work(this);
}
