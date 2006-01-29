//
// C++ Interface: rakikpimsyncfactory
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef RAKIKPIMSYNCFACTORY_H
#define RAKIKPIMSYNCFACTORY_H

#include <rakisyncfactory.h>
#include "rakikpimsync.h"

/**
@author Christian Fremgen; Volker Christian
*/
class RakiKPimSyncFactory : public RakiSyncFactory
{
Q_OBJECT
public:
    RakiKPimSyncFactory(QObject *parent = 0, const char *name = 0);

    ~RakiKPimSyncFactory();

    QObject *createObject (QObject */*parent*/, const char */*name*/,
            const char */*classname*/, const QStringList &/*args*/);
};

#endif
