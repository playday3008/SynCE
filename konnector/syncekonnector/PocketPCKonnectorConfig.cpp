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

    QGridLayout*   m_layout;
    QGridLayout*   m_layout1;
    QGridLayout*   m_layoutMain;



    m_layoutMain = new QGridLayout (this, 2, 1);
    m_layoutMain->setSpacing (KDialog::spacingHint());

    m_layout = new QGridLayout (m_layoutMain, 1, 2);
    m_layout->setSpacing (KDialog::spacingHint());

    m_layout1 = new QGridLayout (m_layoutMain, 4, 3);
    m_layout->setSpacing (KDialog::spacingHint());

    m_layoutMain->addLayout( m_layout, 0, 0);
    m_layoutMain->addLayout(m_layout1, 1, 0);


    m_label = new QLabel (this, "PDA Name");
    m_label->setText ("PDA Name");

    m_textPdaName = new QLineEdit(this, "pda name");


    m_Synctarget = new QLabel(this,  "SyncTarget");
    m_Synctarget->setText("Target");

    m_ActiveLabel = new QLabel(this, "Active");
    m_ActiveLabel->setText("Active");

    m_FirstSyncLabel = new QLabel(this, "FirstSync");
    m_FirstSyncLabel->setText("Fresh Sync");



    m_ContactsLabel = new QLabel(this, "ContactsLabel");
    m_ContactsLabel->setText("Contacts");
    m_ContactsEnabled = new QCheckBox(this, "ContactsEnabled");
    m_ContactsFirstSync = new QCheckBox(this, "ContactsFirstSync");

    m_EventsLabel = new QLabel(this, "EventsLabel");
    m_EventsLabel->setText("Events");
    m_EventsEnabled = new QCheckBox(this, "EventsEnabled");
    m_EventsFirstSync = new QCheckBox(this, "EventsFirstSync");

    m_TodosLabel = new QLabel(this, "TodosLabel");
    m_TodosLabel->setText("Todos");
    m_TodosEnabled = new QCheckBox(this, "TodosEnabled");
    m_TodosFirstSync = new QCheckBox(this, "TodosFirstSync");

    m_layout1->addWidget (m_label, 0, 0);
    m_layout1->addWidget (m_textPdaName, 0, 1);

    m_layout1->addWidget (m_Synctarget, 1, 0);
    m_layout1->addWidget (m_ActiveLabel, 1, 1);
    m_layout1->addWidget (m_FirstSyncLabel, 1, 2);

    m_layout1->addWidget (m_ContactsLabel, 2, 0);
    m_layout1->addWidget (m_ContactsEnabled, 2, 1);
    m_layout1->addWidget (m_ContactsFirstSync, 2, 2);

    m_layout1->addWidget (m_EventsLabel, 3, 0);
    m_layout1->addWidget (m_EventsEnabled, 3, 1);
    m_layout1->addWidget (m_EventsFirstSync, 3, 2);

    m_layout1->addWidget (m_TodosLabel, 4, 0);
    m_layout1->addWidget (m_TodosEnabled, 4, 1);
    m_layout1->addWidget (m_TodosFirstSync, 4, 2);

    m_ContactsEnabled->setDisabled(true);
    m_EventsEnabled->setDisabled(true);
    m_TodosEnabled->setDisabled(true);

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

    m_ContactsEnabled->setChecked(k->getContactsEnabled());
    m_TodosEnabled->setChecked(k->getTodosEnabled());
    m_EventsEnabled->setChecked(k->getEventsEnabled());

    m_ContactsFirstSync->setChecked(k->getContactsFirstSync());
    m_TodosFirstSync->setChecked(k->getTodosFirstSync());
    m_EventsFirstSync->setChecked(k->getEventsFirstSync());

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
    k->setContactsState(m_ContactsEnabled->isChecked(), m_ContactsFirstSync->isChecked());
    k->setEventsState(m_EventsEnabled->isChecked(), m_EventsFirstSync->isChecked());
    k->setTodosState(m_TodosEnabled->isChecked(), m_TodosFirstSync->isChecked());
}


}
#include "PocketPCKonnectorConfig.moc"
