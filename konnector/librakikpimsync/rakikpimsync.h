//
// C++ Interface: rakikpimsync
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAKIKPIMSYNC_H
#define RAKIKPIMSYNC_H

#include <rakisyncplugin.h>
#include <libkdepim/progressmanager.h>
#include "pimsyncmanager.h"

class SyncTaskListItem;
/**
@author Christian Fremgen; Volker Christian
*/


class RakiKPimSync : public RakiSyncPlugin
{
    Q_OBJECT
public:
    RakiKPimSync();

    ~RakiKPimSync();

    virtual bool postSync( QWidget* parent, bool firstSynchronize, uint32_t partnerId );
    virtual bool preSync( QWidget* parent, bool firstSynchronize, uint32_t partnerId );
    virtual void init(Rra* rra, SyncTaskListItem *item, QString pdaName, QWidget *parent,
                       QString serviceName );
    virtual void unInit();
    virtual void configure();
    virtual void subscribeTo();
    virtual void unsubscribeFrom();
    virtual void createConfigureObject( KConfig* ksConfig );
    virtual bool sync();

private slots:
    void progressItemAdded( KPIM::ProgressItem* );
    void progressItemStatus( KPIM::ProgressItem*, const QString& );
    void progressItemProgress( KPIM::ProgressItem*, unsigned int );

private:
    static int refcount;
    int type;
};

#endif
