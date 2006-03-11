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

#ifndef PIMSYNCMANAGER_H
#define PIMSYNCMANAGER_H

#include <rra.h>
#include <kconfig.h>

#include <qobject.h>
#include <qstring.h>

#include <qwidget.h>
#include <qmap.h>

#include <syncer.h>
#include <kstaticdeleter.h>
#include <kresources/manager.h>

#include <syncee.h>
#include <synceelist.h>

#include <konnector.h>
#include <filter.h>

/**
@author Christian Fremgen; Volker Christian
*/
class KAboutData;
class KAction;
class KActionCollection;
class KonnectorPairView;
class KonnectorPair;
class KXMLGUIClient;
class LogDialog;

namespace KSync {
class SynCEEngine;
}

class PimSyncManager : QObject
{
Q_OBJECT
public:
    ~PimSyncManager();

    static PimSyncManager* self(QString pdaName);

    bool loadKonnectors( KConfig* ksConfig);

    void subscribeTo(Rra* rra, int type );

    void unsubscribeFrom( int type );

    void setActualSyncType(int type);

    void startSync();
    void configure(QWidget *parent, KConfig* ksConfig);

protected slots:
    void syncDone();


private:
    PimSyncManager(QString pdaName);
    static QMap<QString, PimSyncManager *> pimSyncMap;
    bool konnectorsLoaded;

    KonnectorPairView *mView;
    KonnectorPair *pair;
    KSync::SynCEEngine *mEngine;
    QString pdaName;
    int refCount;

    class Private;
    Private *d;

};
#endif
