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

#include "removepartnershipdialogimpl.h"

#include <klistview.h>
#include <kdebug.h>

RemovePartnershipDialogImpl::RemovePartnershipDialogImpl( QWidget* parent, const char* name, bool modal, WFlags fl )
 : RemovePartnershipDialog(parent, name, modal, fl )
{   
    partnershipListView->setFullWidth(true);
    
}


RemovePartnershipDialogImpl::~RemovePartnershipDialogImpl()
{
}


void RemovePartnershipDialogImpl::setPartnerships(QString partner1, QString partner2)
{
    partnershipList.clear();
    partnershipListView->clear();
    QCheckListItem *partnerItem1 = new QCheckListItem(partnershipListView, partner1, QCheckListItem::CheckBox);
    QCheckListItem *partnerItem2 = new QCheckListItem(partnershipListView, partner2, QCheckListItem::CheckBox);

    partnershipListView->insertItem(partnerItem1);
    partnershipList.append(partnerItem1);
    partnershipListView->insertItem(partnerItem2);
    partnershipList.append(partnerItem2);
}


void RemovePartnershipDialogImpl::removePartnerships()
{
    kdDebug(2120) << "Removing partnerships" << endl;
    
    QCheckListItem *first = partnershipList.first();
    QCheckListItem *second = partnershipList.next();
    
    if (first->isOn()) {
        partnershipListView->takeItem(first);
        deletedItems |= 1;
    }
    
    if (second->isOn()) {
        partnershipListView->takeItem(second);
        deletedItems |= 2;
    }
}

int RemovePartnershipDialogImpl::showDialog(QString partner1, QString partner2, QWidget* parent, const char* name, bool modal, WFlags fl)
{
    RemovePartnershipDialogImpl *dialog = new RemovePartnershipDialogImpl(parent, name, modal, fl);
    dialog->setPartnerships(partner1, partner2);
    
    deletedItems = 0;
    
    dialog->exec();
    
    delete dialog;
    
    return deletedItems;
}

int RemovePartnershipDialogImpl::deletedItems = 0;


