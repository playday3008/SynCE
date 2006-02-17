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

#ifndef PAIREDITORWIDGET_H
#define PAIREDITORWIDGET_H

#include <qwidget.h>
#include <qpair.h>
#include <qvaluelist.h>
#include <kresources/configwidget.h>
#include <konnector.h>

class KLineEdit;

class KonnectorPair;
class PluginEditorWidget;
class QRadioButton;

class PairEditorWidget : public QWidget
{
  Q_OBJECT

  public:
    PairEditorWidget( QWidget *parent = 0, const char *name = 0, QString pdaName = "" );
    ~PairEditorWidget();

    void setPair( KonnectorPair *pair );
    KonnectorPair *pair() const;

    void initGUI();

    void save();

  private:
    QWidget* createPluginTab();
    QWidget* createSyncOptionTab();
    QWidget* createFilterTab();

//    KLineEdit *mPairNameEdit;

    QRadioButton *mResolveManually;
    QRadioButton *mResolveFirst;
    QRadioButton *mResolveSecond;
    QRadioButton *mResolveBoth;

//    QValueList<PluginEditorWidget*> mEditorWidgets;

    KonnectorPair *mPair;
    QString pdaName;

    KSync::Konnector *konnector[2];

    QValueList<QPair<KRES::ConfigWidget *, KSync::Konnector *> > configWidgets;

};

#endif
