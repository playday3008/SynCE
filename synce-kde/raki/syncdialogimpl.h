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

#ifndef SYNCDIALOGIMPL_H
#define SYNCDIALOGIMPL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "syncdialog.h"
#include "syncthread.h"

#include <kconfig.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qthread.h>

class KProgressDialog;
class SyncTaskListItem;
class Rra;
class PdaConfigDialogImpl;
class SyncDialogImpl;

/**
@author Volker Christian,,,
*/

#define postSyncDialogImplEvent(a, b, c) { \
    SyncDialogImplCustomEvent *ce = new SyncDialogImplCustomEvent(a, b); \
    postEvent(ce, sdtd, c); \
}

class SyncDialogImplCustomEvent : public QCustomEvent
{
    public:
        SyncDialogImplCustomEvent(void *(SyncDialogImpl::*userEventMethod)(void *data),
                       void *data) : QCustomEvent(QEvent::User), userEventMethod(userEventMethod), eventData(data) {};
    private:
        void *(SyncDialogImpl::*userEventMethod)(void *data);
        void *eventData;
        friend class SyncDialogImplThreadData;
};


#include <qcursor.h>
#include <qapplication.h>
#include <kdebug.h>

class SyncDialogImplThreadData : public ThreadEventObject
{
    Q_OBJECT
    public:
        SyncDialogImplThreadData(SyncDialogImpl *pda) : pda(pda) {};
    private:

        SyncDialogImpl *pda;
        void customEvent (QCustomEvent *customEvent) {
            SyncDialogImplCustomEvent *pdaCustomEvent = dynamic_cast<SyncDialogImplCustomEvent *>(customEvent);
            int *blocking = (int *) customEvent->data();
            void *(SyncDialogImpl::*userEventMethod)(void *data) = pdaCustomEvent->userEventMethod;

            switch (*blocking) {
                case WorkerThreadInterface::noBlock:

                    (pda->*userEventMethod)(pdaCustomEvent->eventData);
                    break;
                case WorkerThreadInterface::block:
                    QApplication::setOverrideCursor( QCursor( Qt::ArrowCursor ) );
                    (pda->*userEventMethod)(pdaCustomEvent->eventData);
                    QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
                    this->eventMutexLock();
                    this->eventMutexUnlock();
                    this->wakeUpOnEvent();
                    break;
            }
            delete blocking;
        };
};

class SyncDialogImpl : public SyncDialog, public SyncThread
{
Q_OBJECT

public:
    SyncDialogImpl(Rra *rra, QString& pdaName, QWidget* parent,
            const char* name = 0, bool modal = FALSE, WFlags fl = 0);
    ~SyncDialogImpl();
    void show(QPtrList<SyncTaskListItem>& syncItems);
    void work(QThread *qt = NULL, void *data = NULL);
    void reject(bool forced = false);

signals:
    void finished();

private:
    Rra *rra;
    QString pdaName;
    QPtrList<SyncTaskListItem> syncItems;
    void *finishedSynchronization(void *);
    void *preSync(void *v_item);
    void *postSync(void *v_item);
    SyncDialogImplThreadData *sdtd;
};

#endif
