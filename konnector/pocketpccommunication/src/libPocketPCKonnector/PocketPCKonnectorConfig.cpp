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
#include "PocketPCKonnectorConfig.h"
#include "PocketPCKonnector.h"

#include <kdebug.h>
#include <kdialog.h>

namespace pocketPCHelper {

PocketPCKonnectorConfig::PocketPCKonnectorConfig(QWidget *parent, const char *name)
 : KRES::ConfigWidget(parent, name)
{
    m_layout = new QGridLayout (this, 4, 5);
    m_layout->setSpacing (KDialog::spacingHint());
            
    m_label = new QLabel (this, "PDA Name");
    m_label->setText ("PDA Name");
    
    m_textPdaName = new QLineEdit(this, "pda name");
    
    m_layout->addWidget (m_label, 0, 0);
    m_layout->addWidget (m_textPdaName, 0, 1);
}


PocketPCKonnectorConfig::~PocketPCKonnectorConfig()
{
}

void PocketPCKonnectorConfig::loadSettings (KRES::Resource* p_res)
{
    kdDebug(2120) << "PocketPCConnectorConfig::loadSettings" << endl;
    
    KSync::PocketPCKonnector* k = dynamic_cast<KSync::PocketPCKonnector*>(p_res);
    
    if (!k)
    {
        kdError() << "PocketPCKonnectorConfig::loadSettings(): Wrong Konnector type." << endl;
        return;
    }
    
    m_textPdaName->setText (k->getPdaName());
}


void PocketPCKonnectorConfig::saveSettings (KRES::Resource* p_res)
{
    kdDebug(2120) << "PocketPCConnectorConfig::saveSettings" << endl;
    
    KSync::PocketPCKonnector* k = dynamic_cast<KSync::PocketPCKonnector*>(p_res);
    
    if (!k)
    {
        kdError() << "PocketPCKonnectorConfig::saveSettings(): Wrong Konnector type." << endl;
        return;
    }
    
    k->setPdaName(m_textPdaName->text());
}


};
#include "PocketPCKonnectorConfig.moc"
