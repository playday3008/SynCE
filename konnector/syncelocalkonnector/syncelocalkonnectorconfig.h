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

#ifndef KSYNC_LOCALKONNECTORCONFIG_H
#define KSYNC_LOCALKONNECTORCONFIG_H

#include <kurlrequester.h>
#include <syncekonnectorconfigbase.h>
#include <kresources/configwidget.h>

#include <qwidget.h>

namespace KSync {

class SynCELocalKonnectorConfig : public SynCEKonnectorConfigBase
{
    Q_OBJECT
  public:
      SynCELocalKonnectorConfig( QWidget *parent, const char *name  );
      ~SynCELocalKonnectorConfig();

    void loadSettings( KRES::Resource *resource );
    void saveSettings( KRES::Resource *resource );

    void enableRaki();

  protected slots:
    void selectAddressBookResource();
    void selectCalendarResource();

  private:
    KURLRequester *mCalendarFile;
    KURLRequester *mAddressBookFile;
    KURLRequester *mBookmarkFile;
};

}

#endif
