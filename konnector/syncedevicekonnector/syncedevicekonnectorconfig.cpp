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

#include "syncedevicekonnectorconfig.h"
#include "syncedevicekonnector.h"

#include <kdebug.h>
#include <kdialog.h>

namespace KSync {

SynCEDeviceKonnectorConfig::SynCEDeviceKonnectorConfig(QWidget *parent, const char *name)
 : SynCEKonnectorConfigBase(parent, name)
{

    QGridLayout*   m_layout;
    QGridLayout*   m_layout1;
    QGridLayout*   m_layoutMain;



    m_layoutMain = new QGridLayout (this, 2, 1);
    m_layoutMain->setSpacing (KDialog::spacingHint());

    m_layout = new QGridLayout (/*m_layoutMain*/ 0, 1, 2);
    m_layout->setSpacing (KDialog::spacingHint());

    m_layout1 = new QGridLayout (/*m_layoutMain*/ 0, 4, 3);
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
    m_EventsLabel->setText("Appointments");
    m_EventsEnabled = new QCheckBox(this, "EventsEnabled");
    m_EventsFirstSync = new QCheckBox(this, "EventsFirstSync");

    m_TodosLabel = new QLabel(this, "TodosLabel");
    m_TodosLabel->setText("Tasks");
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
}


SynCEDeviceKonnectorConfig::~SynCEDeviceKonnectorConfig()
{
}

void SynCEDeviceKonnectorConfig::loadSettings (KRES::Resource* p_res)
{
    kdDebug(2120) << "PocketPCConnectorConfig::loadSettings" << endl;

    KSync::SynCEDeviceKonnector* k = dynamic_cast<KSync::SynCEDeviceKonnector*>(p_res);

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


void SynCEDeviceKonnectorConfig::saveSettings (KRES::Resource* p_res)
{
    kdDebug(2120) << "PocketPCConnectorConfig::saveSettings" << endl;

    KSync::SynCEDeviceKonnector* k = dynamic_cast<KSync::SynCEDeviceKonnector*>(p_res);

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

void SynCEDeviceKonnectorConfig::enableRaki()
{
    m_textPdaName->setDisabled(true);
    m_ContactsEnabled->setDisabled(true);
    m_EventsEnabled->setDisabled(true);
    m_TodosEnabled->setDisabled(true);
}

}
#include "syncedevicekonnectorconfig.moc"
