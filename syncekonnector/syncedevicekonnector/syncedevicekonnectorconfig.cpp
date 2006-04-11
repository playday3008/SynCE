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

    m_layout = new QGridLayout (this, 2, 2);
    m_layout->setSpacing (KDialog::spacingHint());

    m_ContactsLabel = new QLabel(this, "ContactsLabel");
    m_ContactsLabel->setText("Contacts");
    m_ContactsEnabled = new QCheckBox(this, "ContactsEnabled");

    m_EventsLabel = new QLabel(this, "EventsLabel");
    m_EventsLabel->setText("Appointments");
    m_EventsEnabled = new QCheckBox(this, "EventsEnabled");

    m_TodosLabel = new QLabel(this, "TodosLabel");
    m_TodosLabel->setText("Tasks");
    m_TodosEnabled = new QCheckBox(this, "TodosEnabled");

    m_layout->addWidget (m_ContactsEnabled, 0, 0);
    m_layout->addWidget (m_ContactsLabel, 0, 1);

    m_layout->addWidget (m_EventsEnabled, 1, 0);
    m_layout->addWidget (m_EventsLabel, 1, 1);

    m_layout->addWidget (m_TodosEnabled, 2, 0);
    m_layout->addWidget (m_TodosLabel, 2, 1);
}


SynCEDeviceKonnectorConfig::~SynCEDeviceKonnectorConfig()
{
}

void SynCEDeviceKonnectorConfig::loadSettings (KRES::Resource* p_res)
{
    kdDebug(2120) << "SynCEDeviceKonnectorConfig::loadSettings" << endl;

    KSync::SynCEDeviceKonnector* k = dynamic_cast<KSync::SynCEDeviceKonnector*>(p_res);

    if (!k)
    {
        kdError() << "SynCEDeviceKonnectorConfig::loadSettings(): Wrong Konnector type." << endl;
        return;
    }

    m_ContactsEnabled->setChecked(k->getContactsEnabled());
    m_TodosEnabled->setChecked(k->getTodosEnabled());
    m_EventsEnabled->setChecked(k->getEventsEnabled());
}


void SynCEDeviceKonnectorConfig::saveSettings (KRES::Resource* p_res)
{
    kdDebug(2120) << "SynCEDeviceKonnectorConfig::saveSettings" << endl;

    KSync::SynCEDeviceKonnector* k = dynamic_cast<KSync::SynCEDeviceKonnector*>(p_res);

    if (!k)
    {
        kdError() << "SynCEDeviceKonnectorConfig::saveSettings(): Wrong Konnector type." << endl;
        return;
    }

    k->setContactsState(m_ContactsEnabled->isChecked());
    k->setEventsState(m_EventsEnabled->isChecked());
    k->setTodosState(m_TodosEnabled->isChecked());
}

void SynCEDeviceKonnectorConfig::enableRaki()
{
    m_ContactsEnabled->setDisabled(true);
    m_EventsEnabled->setDisabled(true);
    m_TodosEnabled->setDisabled(true);
}

}
#include "syncedevicekonnectorconfig.moc"
