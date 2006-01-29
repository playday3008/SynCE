//
// C++ Interface: syncekonnectorbase
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SYNCEKONNECTORBASE_H
#define SYNCEKONNECTORBASE_H

#include <konnector.h>
#include <rra.h>

/**
@author Christian Fremgen; Volker Christian
*/

#define CONTACTS    0x01
#define EVENTS      0x02
#define TODOS       0x04

namespace KSync {
class SynCEKonnectorBase : public Konnector
{
public:
    SynCEKonnectorBase(const KConfig* p_config);

    ~SynCEKonnectorBase();

    virtual void actualSyncType(int type ) = 0;

    virtual void subscribeTo( int type ) = 0;

    virtual void unsubscribeFrom( int type ) = 0;

    virtual void setPdaName(const QString &pdaName) = 0;
};
}
#endif
