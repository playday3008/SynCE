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

#include <klocale.h>

#include <qlayout.h>
#include <kdebug.h>

#include "paireditorwidget.h"

#include "paireditordialog.h"

PairEditorDialog::PairEditorDialog( QWidget *parent, const char *name, QString pdaName )
  : KDialogBase( Plain, i18n( "PIM-Resource Editor" ), Ok | Cancel, Ok,
                 parent, name, true, true )
{
  kdDebug(2120) << " Creating PairEditorDialog" << endl;
  QWidget *page = plainPage();

  layout = new QVBoxLayout( page );
  mPairEditorWidget = new PairEditorWidget( page, "PairEditorWidget", pdaName );
}

PairEditorDialog::~PairEditorDialog()
{
}

void PairEditorDialog::setPair( KonnectorPair *pair )
{
  mPairEditorWidget->setPair( pair );

  initGUI();
  setInitialSize( QSize( 300, 200 ) );
}

KonnectorPair *PairEditorDialog::pair() const
{
  return mPairEditorWidget->pair();
}

void PairEditorDialog::initGUI()
{
  mPairEditorWidget->initGUI();
  layout->addWidget( mPairEditorWidget );
}


void PairEditorDialog::accept()
{
    mPairEditorWidget->save();
    KDialog::accept();
}

#include "paireditordialog.moc"
