/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/

#include "rakiworkerthread.h"

#include <qapplication.h>
#include <qcursor.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

RakiWorkerThread *RakiWorkerThread::rakiWorkerThread = new RakiWorkerThread();


RakiWorkerThread::RakiWorkerThread() : QThread()
{
    wti = NULL;
}


RakiWorkerThread::~RakiWorkerThread()
{}


void RakiWorkerThread::start(WorkerThreadInterface *wti, void (WorkerThreadInterface::*userRun)(QThread *thread, void *data = NULL), void *data)
{
    threadMutex.lock();
    this->wait();
    this->userRun = userRun;
    this->wti = wti;
    this->wti->setRunning(true);
    this->data = data;
    QThread::start();
    threadMutex.unlock();
}


bool RakiWorkerThread::running()
{
    return rakiWorkerThread->isRunning();
}


bool RakiWorkerThread::isRunning()
{
    return QThread::running();
}


bool RakiWorkerThread::finished()
{
    return rakiWorkerThread->isFinished();
}


bool RakiWorkerThread::isFinished()
{
    return QThread::finished();
}


void RakiWorkerThread::stop()
{
    if (wti) {
        wti->lock();
        wti->setRunning(false);
        wti->unlock();
        this->wait();
    }
}


WorkerThreadInterface *RakiWorkerThread::getCurrentInterface()
{
    return wti;
}


void RakiWorkerThread::run()
{
    QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    if (userRun) {
        (wti->*userRun)(this, data);
    }
    wti->lock();
    wti->setRunning(false);
    wti->unlock();
    wti = NULL;
    QApplication::setOverrideCursor( QCursor(Qt::ArrowCursor) );
    waitCondition.wakeOne();
}
