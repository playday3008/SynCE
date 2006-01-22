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

#ifndef _SYNCTHREAD_H_
#define _SYNCTHREAD_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "workerthreadinterface.h"

class SyncTaskListItem;
class SyncThread;

/**
 *
 * Volker Christian,,,
 **/

#define postSyncThreadEvent(a, b) {\
    SyncThreadCustomEvent *ce = new SyncThreadCustomEvent(a, b); \
    syncThread->postEvent(ce, syncThread->sttd, WorkerThreadInterface::noBlock); \
}

#define postSyncThreadEventBlock(a, b) {\
    SyncThreadCustomEvent *ce = new SyncThreadCustomEvent(a, b); \
    syncThread->postEvent(ce, syncThread->sttd, WorkerThreadInterface::block); \
}

class SyncThreadCustomEvent : public QCustomEvent
{
    public:
        SyncThreadCustomEvent(void *(SyncThread::*userEventMethod)(void *data),
                       void *data) : QCustomEvent(QEvent::User), userEventMethod(userEventMethod), eventData(data) {};
    private:
        void *(SyncThread::*userEventMethod)(void *data);
        void *eventData;
        friend class SyncThreadThreadData;
};


#include <qcursor.h>
#include <qapplication.h>
#include <kdebug.h>

class SyncThreadThreadData : public ThreadEventObject
{
    Q_OBJECT
    public:
        SyncThreadThreadData(SyncThread *syncThread) : syncThread(syncThread) {};
    private:

        SyncThread *syncThread;
        void customEvent (QCustomEvent *customEvent) {
            SyncThreadCustomEvent *syncThreadCustomEvent = dynamic_cast<SyncThreadCustomEvent *>(customEvent);
            int *blocking = (int *) customEvent->data();
            void *(SyncThread::*userEventMethod)(void *data) = syncThreadCustomEvent->userEventMethod;

            switch (*blocking) {
                case WorkerThreadInterface::noBlock:

                    (syncThread->*userEventMethod)(syncThreadCustomEvent->eventData);
                    break;
                case WorkerThreadInterface::block:
                    QApplication::setOverrideCursor( QCursor( Qt::ArrowCursor ) );
                    (syncThread->*userEventMethod)(syncThreadCustomEvent->eventData);
                    QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
                    this->eventMutexLock();
                    this->eventMutexUnlock();
                    this->wakeUpOnEvent();
                    break;
            }
            delete blocking;
        };
};

class SyncThread : public WorkerThreadInterface
{
public:
    SyncThread();
    ~SyncThread();
    void *incTotalSteps(void *inc);
    void *decTotalSteps(void *dec);
    void *advanceProgress(void *);
    void *setTotalSteps(void *steps);
    void *setProgress(void *progress);
    void *setTask(void *task);
    void setActualSyncItem(SyncTaskListItem *actualSyncItem);
    SyncThreadThreadData *sttd;

private:
    SyncTaskListItem *actualSyncItem;
};


#endif
