/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
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

#ifndef SYNCDIALOGIMPL_H
#define SYNCDIALOGIMPL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "syncdialog.h"

#include <kconfig.h>
#include <qptrlist.h>
#include <qstring.h>
#include <qthread.h>

class KProgressDialog;
class SyncTaskListItem;
class Rra;
class PdaConfigDialogImpl;
class SyncDialogImpl;

/**
@author Volker Christian,,,
*/

class SyncDialogImpl : public SyncDialog
{
Q_OBJECT

public:
    SyncDialogImpl(Rra *rra, QString& pdaName, QWidget* parent,
            const char* name = 0, bool modal = FALSE, WFlags fl = 0);
    ~SyncDialogImpl();
    void show(QPtrList<SyncTaskListItem>& syncItems);
    void work(QThread *qt = NULL, void *data = NULL);
    void reject(bool forced = false);
    bool isRunning();
    bool stopRequested();

signals:
    void finished();

private:
    Rra *rra;
    QString pdaName;
    QPtrList<SyncTaskListItem> syncItems;
    void *finishedSynchronization(void *);
    bool running;
    bool end;
};

#endif
