/***************************************************************************
 * Copyright (c) 2005 Mirko Kohns  <Mirko.Kohns@KashmirEvolution.de>       *
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

#ifndef PIMSYNCCONFIGIMPL_H
#define PIMSYNCCONFIGIMPL_H

#include "pimsyncconfig.h"

#include <kconfig.h>
#include <klistview.h>
#include <qvaluelist.h>


class PIMSyncConfigImpl : public PIMSyncConfig
{
Q_OBJECT
public:
    PIMSyncConfigImpl(KConfig *ksConfig, QWidget* parent = 0,
            const char* name = 0, bool modal = TRUE, WFlags fl = 0 );
    ~PIMSyncConfigImpl();
    void show();

private:
    KConfig *ksConfig;

    // Some Files

    QString ContactReadFilename;
    QString CalenderReadFilename;
    QString TodoReadFilename;
    QString EventReadFilename;
    QString ContactWriteFilename;
    QString CalenderWriteFilename;
    QString TodoWriteFilename;
    QString EventWriteFilename;
    QString pda_name;

public slots:
    /*$PUBLIC_SLOTS$*/

protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
   /*$PROTECTED_SLOTS$*/
   virtual void ContactTestButtonReadclicked();
   virtual void TodoTestButtonReadclicked();
   virtual void EventTestButtonReadclicked();
   virtual void CalenderTestButtonReadclicked();
   virtual void ContactTestButtonWriteclicked();
   virtual void TodoTestButtonWriteclicked();
   virtual void EventTestButtonWriteclicked();
   virtual void CalenderTestButtonWriteclicked();
   virtual void NewConfigFile();
};

#endif

