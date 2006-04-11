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

#ifndef MANAGERIMPL_H
#define MANAGERIMPL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "management.h"
#include "workerthreadinterface.h"

#include <rapi.h>

class PDA;
class ManagerImpl;

/**
@author Volker Christian,,,
*/
#define postManagerImplEvent(a, b, c) { \
    ManagerImplCustomEvent *ce = new ManagerImplCustomEvent(a, b); \
    postEvent(ce, mtd, c); \
}


class ManagerImplCustomEvent : public QCustomEvent
{
    public:
        ManagerImplCustomEvent(void *(ManagerImpl::*userEventMethod)(void *data),
                       void *data) : QCustomEvent(QEvent::User), userEventMethod(userEventMethod), eventData(data) {};
    private:
        void *(ManagerImpl::*userEventMethod)(void *data);
        void *eventData;
        friend class ManagerImplThreadData;
};


#include <qcursor.h>
#include <qapplication.h>
#include <kdebug.h>

class ManagerImplThreadData : public ThreadEventObject
{
    Q_OBJECT
    public:
        ManagerImplThreadData(ManagerImpl *managerImpl) : managerImpl(managerImpl) {};
    private:

        ManagerImpl *managerImpl;
        void customEvent (QCustomEvent *customEvent) {
            ManagerImplCustomEvent *managerImplEvent = dynamic_cast<ManagerImplCustomEvent *>(customEvent);
            int *blocking = (int *) customEvent->data();
            void *(ManagerImpl::*userEventMethod)(void *data) = managerImplEvent->userEventMethod;

            switch (*blocking) {
                case WorkerThreadInterface::noBlock:

                    (managerImpl->*userEventMethod)(managerImplEvent->eventData);
                    break;
                case WorkerThreadInterface::block:
                    QApplication::setOverrideCursor( QCursor( Qt::ArrowCursor ) );
                    (managerImpl->*userEventMethod)(managerImplEvent->eventData);
                    QApplication::setOverrideCursor( QCursor( Qt::WaitCursor ) );
                    this->eventMutexLock();
                    this->eventMutexUnlock();
                    this->wakeUpOnEvent();
                    break;
            }
            delete blocking;
        };
};

class ManagerImpl : public Manager, public WorkerThreadInterface
{
    Q_OBJECT
public:
    ManagerImpl(QString pdaName, QWidget *parent, const char* name = 0,
            bool modal = FALSE, WFlags fl = 0);
    ~ManagerImpl();
    void closeEvent(QCloseEvent *e);

private slots:
    void uninstallSoftwareSlot();
    void refreshSystemInfoSlot();
    void refreshSoftwareSlot();
    void refreshBatteryStatusSlot();

private:
    struct sysinfo_s {
        synce::CEOSVERSIONINFO version;
        synce::SYSTEM_INFO system;
        synce::STORE_INFORMATION store;
        synce::SYSTEM_POWER_STATUS_EX power;
    };

    void uninstallSoftware(QThread *qt = NULL, void *data = NULL);
    void fetchSystemInfo(QThread *qt = NULL, void *data = NULL);
    void fetchBatteryStatus(QThread *qt = NULL, void *data = NULL);
    void fetchSoftwareList(QThread *qt = NULL, void *data = NULL);
    void *beginEvent(void *data = NULL);
    void *endEvent(void *data = NULL);
    void *insertInstalledItemEvent(void *data);
    void *systemInfoEvent(void *data);
    void *batInfoEvent(void *data);
    void *uninstalledEvent(void *data);
    QString pdaName;
    QString msg;
    ManagerImplThreadData *mtd;
};

#endif
