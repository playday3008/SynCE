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
 
#ifndef REMOVEPARTNERSHIPDIALOGIMPL_H
#define REMOVEPARTNERSHIPDIALOGIMPL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "removepartnershipdialog.h"
#include <qptrlist.h>

/**
@author Volker Christian,,,
*/

class QCheckListItem;

class RemovePartnershipDialogImpl : public RemovePartnershipDialog
{
public:
    static int showDialog(QString partner1, QString partner2, QWidget* parent,
            const char* name, bool modal, WFlags fl);
    

protected:    
    RemovePartnershipDialogImpl( QWidget* parent, const char* name, bool modal,
            WFlags fl );
    ~RemovePartnershipDialogImpl();

    void removePartnerships();    
    void setPartnerships(QString partner1, QString partner2);
    
    QPtrList<QCheckListItem> partnershipList;
    static int deletedItems;

};

#endif
