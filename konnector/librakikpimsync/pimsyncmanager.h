//
// C++ Interface: pimsyncmanager
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
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

    void subscribeTo( int type );

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

    class Private;
    Private *d;

};
#endif
