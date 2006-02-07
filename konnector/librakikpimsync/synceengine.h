/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                    Christian Fremgen <cfremgen@users.sourceforge.net>   *
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

#ifndef KSYNC_SYNCEENGINE_H
#define KSYNC_SYNCEENGINE_H


#include <konnector.h>
#include <syncer.h>

#include <qobject.h>
#include <qptrlist.h>

#include "eventsyncee.h"
#include "todosyncee.h"


class KonnectorManager;
class KonnectorPair;

namespace KSync {

    class SyncUiKde;

/**
  This class provides the engine for the syncing process. It's responsible for
  control of the action flow through Konnectors and ActionParts. It handles
  reading and writing of Syncees by the Konnectors and triggers the actions of
  the ActionParts in the correct sequence.
*/
    class SynCEEngine : public QObject
{
  Q_OBJECT

  public:
    SynCEEngine();
    ~SynCEEngine();
    static QString progressId();

    void go( KonnectorPair *pair );
    void setResolveStrategy( int strategy );

    protected:
        void logMessage( const QString& );
        void logError( const QString& );

        void tryExecuteActions();
        void executeActions();

        void tryFinish();
        void finish();

        void disconnectDevice( Konnector *k );

    protected slots:
        void slotSynceesRead( KSync::Konnector * );
        void slotSynceeReadError( KSync::Konnector * );
        void slotSynceesWritten( KSync::Konnector * );
        void slotSynceeWriteError( KSync::Konnector * );

    signals:
        void error( const QString& );
        void doneSync();


  private:
    void doSync();
    template<class T> T  *SynCEEngine::templateSyncee(SynceeList *synceeList) const;

    Konnector::List mOpenedKonnectors;
    Konnector::List mProcessedKonnectors;
    uint mKonnectorCount;

    Konnector::List mKonnectors;
    KonnectorManager *mManager;

    Syncer mCalendarSyncer;
    Syncer mEventSyncer;
    Syncer mTodoSyncer;
    Syncer mAddressBookSyncer;

    SyncUi *mSyncUi;
};

}

#endif
