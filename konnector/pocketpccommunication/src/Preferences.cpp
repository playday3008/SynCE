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
#include "Preferences.h"

#include <qlineedit.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <qfiledialog.h>
#include <qregexp.h>


#include <kapplication.h>
#include <kdebug.h>

Preferences::Preferences(QWidget* p_parent, const char* p_name, bool p_modal, WFlags p_fl)
 : PreferencesUI(p_parent, p_name, p_modal, p_fl)
{
    m_config = kapp->config();
    
    loadConfiguration();
}


Preferences::~Preferences()
{
}


const QString Preferences::getPdaName () const
{
    return m_textPdaName->text();
}


const QString Preferences::getAddressBookFileName () const
{
    return m_textAddrBookFile->text();
}


const QString Preferences::getEventsFileName () const
{
    return m_textEventFile->text();
}


const QString Preferences::getTodosFileName () const
{
    return m_textTodoFile->text();
}


const bool Preferences::useStdAddressBook() const
{
    return m_checkUseStdAddrBook->isChecked();
}


const QString Preferences::getCalendarFileName () const
{
    return m_textCalendarFile->text();
}


const QString Preferences::getAddressBookLocal() const
{
    return m_textLocalAddressBook->text();
}


const QString Preferences::getAddressBookOutput() const
{
    return m_textOutputAddressBook->text();
}


const QString Preferences::getCalendarLocal() const
{
    return m_textLocalCalendar->text();
}


const QString Preferences::getCalendarOutput() const
{
    return m_textOutputCalendar->text();
}


const QString Preferences::getEventsLocal() const
{
    return m_textLocalEvents->text();
}


const QString Preferences::getEventsOutput() const
{
    return m_textOutputEvents->text();
}


const QString Preferences::getTodosLocal() const
{
    return m_textLocalTodos->text();
}


const QString Preferences::getTodosOutput() const
{
    return m_textOutputTodos->text();
}


/*
const bool Preferences::getUseKonnector() const
{
    return m_checkUseKonnector->isChecked();            
}
*/

const SyncingOption Preferences::getAddressBookSyncing () const
{
            
    return getSyncingOption (m_radioGroupAddressBook);
}


const SyncingOption Preferences::getCalendarSyncing () const
{
    return getSyncingOption (m_radioGroupCalendar);
}


const SyncingOption Preferences::getEventsSyncing () const
{
    return getSyncingOption (m_radioGroupEvents);
}


const SyncingOption Preferences::getTodosSyncing () const
{
    return getSyncingOption (m_radioGroupTodos);
}


const SyncingOption Preferences::getSyncingOption(const QButtonGroup* p_buttonBox) const
{
    switch (p_buttonBox->selectedId())
    {
        case 0:
            return MERGE;
            break;
        case 1:
            return DEVICE;
            break;
        case 2:
            return LOCAL;
            break;
        default:
            return MERGE;        
    }    
}    


const bool Preferences::getAddressBookOverwrite () const
{
    return m_checkAddressBookOverwrite->isChecked();
}


const bool Preferences::getCalendarOverwrite () const
{
    return m_checkCalendarOverwrite->isChecked(); 
}


const bool Preferences::getEventsOverwrite () const
{
    return m_checkEventsOverwrite->isChecked();
}


const bool Preferences::getTodosOverwrite () const
{
    return m_checkTodosOverwrite->isChecked();
}


const bool Preferences::getAddressBookFirstSync() const
{
    return m_checkFirstSyncAddressBook->isChecked();
}


const bool Preferences::getCalendarFirstSync() const
{
    return m_checkFirstSyncCalendar->isChecked();
}


const bool Preferences::getEventsFirstSync() const
{
    return m_checkFirstSyncEvents->isChecked();
}


const bool Preferences::getTodosFirstSync() const
{
    return m_checkFirstSyncTodos->isChecked();
}


const bool Preferences::getAddressBookLoadLog() const
{
    return m_checkLogAddressBook->isChecked();
}


const bool Preferences::getCalendarLoadLog() const
{
    return m_checkLogCalendar->isChecked();
}


const bool Preferences::getEventsLoadLog() const
{
    return m_checkLogEvents->isChecked();
}


const bool Preferences::getTodosLoadLog() const
{
    return m_checkLogTodos->isChecked();
}
    
    
void Preferences::accept()
{
    // save configuration after hiding!
    hide();
    writeConfiguration();
}


void Preferences::reject()
{
    // reset configuration after hiding!!

    hide();
    loadConfiguration();
}


void Preferences::slotGetAddressBookFile ()
{
    getFileName (m_textAddrBookFile);
}


void Preferences::slotGetEventsFile ()
{
    getFileName (m_textEventFile);
}


void Preferences::slotGetTodosFile ()
{
    getFileName (m_textTodoFile);
}


void Preferences::slotGetCalendarFile ()
{
    getFileName(m_textCalendarFile);
}


void Preferences::slotGetPdaName ()
{
}


void Preferences::slotGetAddressBookLocal()
{
    getFileName(m_textLocalAddressBook, true);
}


void Preferences::slotGetAddressBookOutput()
{
    getFileName (m_textOutputAddressBook);
}


void Preferences::slotGetCalendarLocal()
{
    getFileName (m_textLocalCalendar, true);
}


void Preferences::slotGetCalendarOutput()
{
    getFileName (m_textOutputCalendar);
}


void Preferences::slotGetEventsLocal()
{
    getFileName (m_textLocalEvents, true);
}

void Preferences::slotGetEventsOutput()
{
    getFileName (m_textOutputEvents);
}


void Preferences::slotGetTodosLocal()
{
    getFileName (m_textLocalTodos, true);
}

void Preferences::slotGetTodosOutput()
{
    getFileName (m_textOutputTodos);    
}


void Preferences::show()
{
    loadConfiguration();
    PreferencesUI::show();
}


void Preferences::writeConfiguration ()
{
    m_config->setGroup ("PDA Name");
    m_config->writeEntry ("Name", m_textPdaName->text());
    
    m_config->setGroup ("AddressBook");
    m_config->writeEntry ("StdAddrBook", m_checkUseStdAddrBook->isChecked());
    m_config->writeEntry ("File Name", m_textAddrBookFile->text());
    
    m_config->setGroup ("Calendar");
    m_config->writeEntry ("Calendar", m_textCalendarFile->text());
    m_config->writeEntry ("Events", m_textEventFile->text());
    m_config->writeEntry ("Todos", m_textTodoFile->text());
    
    m_config->setGroup ("SyncingFiles");
    m_config->writeEntry ("AddressBookLocal", m_textLocalAddressBook->text());
    m_config->writeEntry ("AddressBookOutput", m_textOutputAddressBook->text());
    m_config->writeEntry ("CalendarLocal", m_textLocalCalendar->text());
    m_config->writeEntry ("CalendarOutput", m_textOutputCalendar->text());
    m_config->writeEntry ("EventsLocal", m_textLocalEvents->text());
    m_config->writeEntry ("EventsOutput", m_textOutputEvents->text());
    m_config->writeEntry ("TodosLocal", m_textLocalTodos->text());
    m_config->writeEntry ("TodosOutput", m_textOutputTodos->text());
    
    m_config->setGroup ("SyncingOptions");
    //m_config->writeEntry ("UseKonnector", m_checkUseKonnector->isChecked());
    
    m_config->writeEntry ("AddressSync", m_radioGroupAddressBook->selectedId());
    m_config->writeEntry ("AddressSyncOverwrite", m_checkAddressBookOverwrite->isChecked());
    m_config->writeEntry ("AddressFirstSync", m_checkFirstSyncAddressBook->isChecked());
    m_config->writeEntry ("AddressLoadLog", m_checkLogAddressBook->isChecked());
    
    m_config->writeEntry ("CalendarSync", m_radioGroupCalendar->selectedId());
    m_config->writeEntry ("CalendarSyncOverwrite", m_checkCalendarOverwrite->isChecked());
    m_config->writeEntry ("CalendarFirstSync", m_checkFirstSyncCalendar->isChecked());
    m_config->writeEntry ("CalendarLoadLog", m_checkLogCalendar->isChecked());
    
    m_config->writeEntry ("EventsSync", m_radioGroupEvents->selectedId());
    m_config->writeEntry ("EventsSyncOverwrite", m_checkEventsOverwrite->isChecked());
    m_config->writeEntry ("EventsFirstSync", m_checkFirstSyncEvents->isChecked());
    m_config->writeEntry ("EventsLoadLog", m_checkLogEvents->isChecked());
    
    m_config->writeEntry ("TodosSync", m_radioGroupTodos->selectedId());
    m_config->writeEntry ("TodosSyncOverwrite", m_checkTodosOverwrite->isChecked());
    m_config->writeEntry ("TodosFirstSync", m_checkFirstSyncTodos->isChecked());
    m_config->writeEntry ("TodosLoadLog", m_checkLogTodos->isChecked());
    
    m_config->sync();
}


void Preferences::loadConfiguration ()
{
    m_config->setGroup ("PDA Name");
    m_textPdaName->setText(m_config->readEntry ("Name"));
    
    m_config->setGroup ("AddressBook");
    m_checkUseStdAddrBook->setChecked (m_config->readBoolEntry ("StdAddrBook"));
    m_textAddrBookFile->setText (m_config->readEntry ("File Name"));
    
    m_config->setGroup ("Calendar");
    m_textCalendarFile->setText (m_config->readEntry ("Calendar"));
    m_textEventFile->setText (m_config->readEntry ("Events"));
    m_textTodoFile->setText (m_config->readEntry ("Todos"));
    
    m_config->setGroup ("SyncingFiles");
    m_textLocalAddressBook->setText (m_config->readEntry("AddressBookLocal"));
    m_textOutputAddressBook->setText (m_config->readEntry("AddressBookOutput"));
    m_textLocalCalendar->setText (m_config->readEntry("CalendarLocal"));
    m_textOutputCalendar->setText (m_config->readEntry("CalendarOutput"));
    m_textLocalEvents->setText (m_config->readEntry("EventsLocal"));
    m_textOutputEvents->setText (m_config->readEntry("EventsOutput"));
    m_textLocalTodos->setText (m_config->readEntry("TodosLocal"));
    m_textOutputTodos->setText (m_config->readEntry("TodosOutput"));
    
    m_config->setGroup ("SyncingOptions");
    //m_checkUseKonnector->setChecked(m_config->readBoolEntry ("UseKonnector"));
    
    m_radioGroupAddressBook->setButton (m_config->readNumEntry ("AddressSync"));
    m_checkAddressBookOverwrite->setChecked (m_config->readBoolEntry ("AddressSyncOverwrite"));
    m_checkFirstSyncAddressBook->setChecked (m_config->readBoolEntry ("AddressFirstSync"));
    m_checkLogAddressBook->setChecked (m_config->readBoolEntry ("AddressLoadLog"));
    
    m_radioGroupCalendar->setButton (m_config->readNumEntry ("CalendarSync"));
    m_checkCalendarOverwrite->setChecked (m_config->readBoolEntry ("CalendarSyncOverwrite"));
    m_checkFirstSyncCalendar->setChecked (m_config->readBoolEntry ("CalendarFirstSync"));
    m_checkLogCalendar->setChecked (m_config->readBoolEntry ("CalendarLoadLog"));
    
    m_radioGroupEvents->setButton (m_config->readNumEntry ("EventsSync"));
    m_checkEventsOverwrite->setChecked (m_config->readBoolEntry ("EventsSyncOverwrite"));
    m_checkFirstSyncEvents->setChecked (m_config->readBoolEntry ("EventsFirstSync"));
    m_checkLogEvents->setChecked (m_config->readBoolEntry ("EventsLoadLog"));
    
    m_radioGroupTodos->setButton (m_config->readNumEntry ("TodosSync"));
    m_checkTodosOverwrite->setChecked (m_config->readBoolEntry ("TodosSyncOverwrite"));
    m_checkFirstSyncTodos->setChecked (m_config->readBoolEntry ("TodosFirstSync"));
    m_checkLogTodos->setChecked (m_config->readBoolEntry ("TodosLoadLog"));
}


void Preferences::getFileName(QLineEdit* p_lineEdit, bool p_open)
{
    kdDebug(2120) << "[Preferences::getFileName]: p_path: " << p_lineEdit->text() << endl;
    QString path = p_lineEdit->text();
    if (!p_lineEdit->text().isEmpty() && QFile::exists(p_lineEdit->text()))
        path = path.remove (QRegExp (QString("$")+QDir::separator()+".*"));
    else
        path = QDir::homeDirPath();
    
    QString s;
    if (p_open)
    {
        s = QFileDialog::getOpenFileName(
                    path,
                    "vCard or iCalendar (*.vcf;*.ics)",
                    this,
                    "get filename dialog"
                    "Choose a filename" );
    }
    else
    {
        s = QFileDialog::getSaveFileName(
                    path,
                    "vCard or iCalendar (*.vcf;*.ics)",
                    this,
                    "get filename dialog"
                    "Choose a filename to save under" );
    }
    
    if (!s.isEmpty())
        p_lineEdit->setText (s);        
}


bool Preferences::isInitialized()
{
    return ((m_checkUseStdAddrBook->isChecked() || (!m_checkUseStdAddrBook->isChecked() && !m_textAddrBookFile->text().isEmpty())) &&
            !m_textCalendarFile->text().isEmpty() &&!m_textEventFile->text().isEmpty() && !m_textTodoFile->text().isEmpty() && 
            !m_textPdaName->text().isEmpty()); 
}
