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

#ifndef PDACONFIGDIALOGIMPL_H
#define PDACONFIGDIALOGIMPL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "pdaconfigdialog.h"

#include <rapi.h>

#include <kconfig.h>
#include <klistview.h>
#include <kpopupmenu.h>
#include <qptrlist.h>
#include <qdatetime.h>

class PDA;
class Rra;
class SyncTaskListItem;
class RemovePartnershipDialogImpl;

/**
@author Volker Christian,,,
*/ 

class PdaConfigDialogImpl : public PdaConfigDialog
{
public:
    PdaConfigDialogImpl(PDA *pda, QString pdaName, QWidget* parent, const char* name = 0, bool modal = FALSE, WFlags fl = 0);
    ~PdaConfigDialogImpl();
    QString getPassword();
    bool getMasqueradeEnabled();
    bool getSyncAtConnect();
    void show();
    QString getDeviceIp();
    bool isNewPda();
    void setPartner(QString partnerName, uint32_t partnerId);
    QString getPartnerName();
    uint32_t getPartnerId();
    QPtrList<SyncTaskListItem>& syncronizationTasks();
    void setNewPartner(QString partnerName, uint32_t partnerId);

public slots:
    void writeConfig();
    void applySlot();
    void changedSlot();
    
private slots:
    void masqChangedSlot();
    void disableApply();
    void objectTypeList_rightButtonClicked(QListViewItem *, const QPoint &, int);
    
private:
    void readConfig();
    void updateFields();

    Rra *rra;
    KConfig *ksConfig;
    QString password;
    bool masqEnabled;
    bool typesRead;
    QString pdaName;
    QDateTime partnershipCreated;
    uint32_t partnerId;
    QString partnerName;
    QPtrList<SyncTaskListItem> syncTaskItemList;
    bool syncAtConnect;
    bool newPda;
    PDA *pda;
};

#endif
