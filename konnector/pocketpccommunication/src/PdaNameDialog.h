/***************************************************************************
 *   Copyright (C) 2004 by Christian Fremgen                               *
 *   cfremgen@users.sourceforge.net                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef PDANAMEDIALOG_H
#define PDANAMEDIALOG_H

#include <PdaNameDialogUI.h>

/**
@author Christian Fremgen
*/
class PdaNameDialog : public PdaNameDialogUI
{
public:
    PdaNameDialog();
    PdaNameDialog (QWidget* p_parent = 0, const char* p_name = 0, WFlags p_f = 0);

    ~PdaNameDialog();

    const QString getText ();   
    void setText (const QString& p_text);
    
protected slots:
    void slotCheckForText (const QString& p_text);    
    
};

#endif
