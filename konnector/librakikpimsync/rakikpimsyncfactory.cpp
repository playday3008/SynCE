//
// C++ Implementation: rakikpimsyncfactory
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "rakikpimsyncfactory.h"

RakiKPimSyncFactory::RakiKPimSyncFactory(QObject* parent, const char* name): RakiSyncFactory(parent, name)
{
}


RakiKPimSyncFactory::~RakiKPimSyncFactory()
{
}


QObject *RakiKPimSyncFactory::createObject (QObject */*parent*/, const char */*name*/,
                    const char */*classname*/, const QStringList &/*args*/)
{
    return new RakiKPimSync();
}


extern "C" {
    void *init_librakikpimsync()
    {
        return new RakiKPimSyncFactory();
    }
}


#include "rakikpimsyncfactory.moc"
