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

#include "workerthreadinterface.h"
#include "rakiworkerthread.h"

#include <kdebug.h>
#include <kapplication.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

WorkerThreadInterface::WorkerThreadInterface()
{
    isRunning = false;
    isStopRequested = false;
}


WorkerThreadInterface::~WorkerThreadInterface()
{
    if (this == RakiWorkerThread::rakiWorkerThread->getCurrentInterface()) {
        RakiWorkerThread::rakiWorkerThread->stop();
    }
    kapp->processEvents();
}


bool WorkerThreadInterface::running()
{
    return isRunning;
}


void WorkerThreadInterface::setRunning(bool running)
{
    this->isRunning = running;
}


bool WorkerThreadInterface::stopRequested()
{
    return isStopRequested;
}


void WorkerThreadInterface::setStopRequested(bool isStopRequested)
{
    this->isStopRequested = isStopRequested;
}


void WorkerThreadInterface::setDelayedDelete(bool isDelayedDelete)
{
    this->isDelayedDelete = isDelayedDelete;
}


bool WorkerThreadInterface::delayedDelete()
{
    return isDelayedDelete;
}


void *WorkerThreadInterface::postEvent(
        void *(WorkerThreadInterface::*userEventMethode)(void *data),
        void *data, int blocking)
{
    ThreadEvent *threadEvent = new ThreadEvent(this, userEventMethode, data);
    int *pBlocking = new int;
    *pBlocking = blocking;
    threadEvent->setData(pBlocking);
    if (blocking == block && running()) {
// Inhibit finishing of the event before we are not waiting for it to finish
        threadEventObject.eventMutexLock();
    }
    QApplication::postEvent(&threadEventObject, threadEvent);
    if (blocking == block && running()) {
// Allow the event to finish and wait for an wake up event from it
        threadEventObject.waitOnEvent();
// Unlock the event mutex correctly
        threadEventObject.eventMutexUnlock();
    }

    return eventReturnValue();
}


void WorkerThreadInterface::setEventReturnValue(void *value)
{
    eventReturn = value;
}


void *WorkerThreadInterface::eventReturnValue()
{
    return eventReturn;
}


void *WorkerThreadInterface::guiSynchronizator(void */*data*/)
{
    syncMutex.lock();
    syncMutex.unlock();
    syncWaitCondition.wakeAll();
    return NULL;
}


void *WorkerThreadInterface::synchronizeGui()
{
    if (!isStopRequested) {
        syncMutex.lock();
        postThreadEvent(&WorkerThreadInterface::guiSynchronizator, NULL,
                noBlock);
        syncWaitCondition.wait(&syncMutex);
        syncMutex.unlock();
    }
    
    return NULL;
}
