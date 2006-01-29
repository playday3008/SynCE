//
// C++ Interface: syncekonnectorconfigbase
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SYNCEKONNECTORCONFIGBASE_H
#define SYNCEKONNECTORCONFIGBASE_H

#include <kresources/configwidget.h>

namespace KSync {

/**
@author Christian Fremgen; Volker Christian
*/
class SynCEKonnectorConfigBase : public KRES::ConfigWidget
{
Q_OBJECT
public:
    SynCEKonnectorConfigBase(QWidget *parent = 0, const char *name = 0);

    virtual ~SynCEKonnectorConfigBase();

    virtual void enableRaki() = 0;
};
}
#endif
