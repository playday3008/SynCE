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

#ifndef PDA_H
#define PDA_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "workerthreadinterface.h"

extern "C" {
#include <rra/syncmgr.h>
}
#include <qobject.h>
#include <qmap.h>
#include <qptrdict.h>
#include <qthread.h>
#include <kpopupmenu.h>
#include <kconfig.h>
#include <kprogress.h>
#include <kprocess.h>
#include <kio/job.h>

class RunWindowImpl;
class ManagerImpl;
class PasswordDialogImpl;
class Raki;
class PdaConfigDialogImpl;
class SyncDialogImpl;
class InitProgress;
class Rra;
class SyncTaskListItem;
class MatchMaker;

/**
@author Volker Christian,,,
*/
#include <qcursor.h>
#include <qapplication.h>
#include <kdebug.h>

#define postThreadEvent(a, b, c) { \
    PDACustomEvent *ce = new PDACustomEvent(a, b); \
    postEvent(ce, ptd, c); \
}

class PDA;

class PDACustomEvent : public QCustomEvent
{
    public:
        PDACustomEvent(void *(PDA::*userEventMethod)(void *data),
                       void *data) : QCustomEvent(QEvent::User), userEventMethod(userEventMethod), eventData(data) {};
    private:
        void *(PDA::*userEventMethod)(void *data);
        void *eventData;
        friend class PDAThreadData;
};

class PDAThreadData : public ThreadEventObject
{
Q_OBJECT
    public:
        PDAThreadData(PDA *pda) : pda(pda) {};
    private:

        PDA *pda;
        void customEvent (QCustomEvent *customEvent) {
            PDACustomEvent *pdaCustomEvent = dynamic_cast<PDACustomEvent *>(customEvent);
            int *blocking = (int *) customEvent->data();
            void *(PDA::*userEventMethod)(void *data) = pdaCustomEvent->userEventMethod;

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


class PDA : public QObject, public WorkerThreadInterface
{
     Q_OBJECT

public:
    PDA(Raki *raki, QString pdaName);
    ~PDA();
    void setMenuIndex(int menuIndex);
    int getMenuIndex();
    const char *getName();
    KPopupMenu *getMenu();
    void requestPassword();
    void passwordInvalid();
    void registerCopyJob(KIO::CopyJob *copyJob);
    void addURLByCopyJob(KIO::CopyJob *copyJob, KURL& url);
    void unregisterCopyJob(KIO::CopyJob *copyJob);
    unsigned int getNumberOfCopyJobs();
    KURL::List getURLListByCopyJob(KIO::CopyJob *copyJob);
    bool getSynchronizationTypes(QValueList<uint32_t> *);
    void init();
    bool isPartner();
    bool running();
    void setStopRequested(bool);
    bool isMasqueradeEnabled();
    QString getDeviceIp();
    void setMasqueradeStarted();
    bool masqueradeStarted();

signals:
    void resolvedPassword(QString pdaName, QString passwd);
    void initialized(PDA *pda, int initialized);
    void disconnectPda(QString pdaName);

private:
    void setPartnership(QThread *thread, void *data);
    void *removePartnershipDialog(void *data);
    void *alreadyTwoPartnershipsDialog(void *data);
    void *removeLocalPartnershipDialog(void *data);
    void *progressDialogCancel(void *data);
    void *advanceProgressEvent(void *data);
    void *advanceTotalStepsEvent(void *data);
    void *rraConnectionError(void *data);
    void *synchronizationTasks(void *data);

    InitProgress *initProgress;
    KProgress *progressBar;
    QString pdaName;
    bool needPasswd;
    int menuIndex;
    KPopupMenu *associatedMenu;
    RunWindowImpl *runWindow;
    ManagerImpl *managerWindow;
    PasswordDialogImpl *passwordDialog;
    PdaConfigDialogImpl *configDialog;
    SyncDialogImpl *syncDialog;
    bool _masqueradeStarted;
    Raki *raki;
    QPtrDict<KURL::List> slaveDict;
    uint32_t partnerId;
    bool partnerOk;
    Rra *rra;
    int syncItem;
    int pdaMirrorItem;
    bool typesRead;
    KProcess pdaMirror;
    PDAThreadData *ptd;
    bool alreadySynced;

private slots:
    void execute();
    void manage();
    void openFs();
    void configurePda();
    void setPassword(QString password);
    void startPdaMirror();
    void pdaMirrorExited(KProcess* );
    void disconnectPda();

public slots:
    void synchronize(bool forced = true);
    void syncFinished();
};

#endif
