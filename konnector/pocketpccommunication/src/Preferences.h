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
#ifndef PREFERENCES_H
#define PREFERENCES_H

#include "PreferencesUI.h"

#include <kconfig.h>

class QLineEdit;
class QButtonGroup;

/** This enum is used to specify the desired syncing-option.
  */
enum SyncingOption {MERGE = 0, DEVICE = 1, LOCAL = 2};


/**
This class implements a simple preferences dialog.

@author Christian Fremgen
*/
class Preferences : public PreferencesUI
{
public:
    Preferences(QWidget* p_parent = 0, const char* p_name = 0, bool p_modal = TRUE, WFlags p_fl = 0);

    ~Preferences();
    
    const QString getPdaName () const;
    const QString getAddressBookFileName () const;
    const QString getEventsFileName () const;
    const QString getTodosFileName () const;
    const QString getCalendarFileName () const;
    const bool    useStdAddressBook() const;
    
    const QString getAddressBookLocal() const;
    const QString getAddressBookOutput() const;
    const QString getCalendarLocal() const;
    const QString getCalendarOutput() const;
    const QString getEventsLocal() const;
    const QString getEventsOutput() const;
    const QString getTodosLocal() const;
    const QString getTodosOutput() const;
    
    //const bool getUseKonnector () const;
    
    const SyncingOption getAddressBookSyncing () const;
    const SyncingOption getCalendarSyncing () const;
    const SyncingOption getEventsSyncing () const;
    const SyncingOption getTodosSyncing () const;
    
    const bool getAddressBookOverwrite () const;
    const bool getCalendarOverwrite () const;
    const bool getEventsOverwrite () const;
    const bool getTodosOverwrite () const;

    const bool getAddressBookFirstSync() const;
    const bool getCalendarFirstSync() const;
    const bool getEventsFirstSync() const;
    const bool getTodosFirstSync() const;
    
    const bool getAddressBookLoadLog() const;
    const bool getCalendarLoadLog() const;
    const bool getEventsLoadLog() const;
    const bool getTodosLoadLog() const;
        
    bool isInitialized();
    
public slots:
    virtual void show();    
    
protected slots:      
    void accept();
    void reject();
    
    void slotGetAddressBookFile ();
    void slotGetEventsFile ();
    void slotGetTodosFile ();
    void slotGetPdaName ();
    void slotGetCalendarFile ();
    
    void slotGetAddressBookLocal();
    void slotGetAddressBookOutput();
    void slotGetCalendarLocal();
    void slotGetCalendarOutput();
    void slotGetEventsLocal();
    void slotGetEventsOutput();
    void slotGetTodosLocal();
    void slotGetTodosOutput();
    
private:
    void writeConfiguration ();
    void loadConfiguration ();
    
    //const QString getFileName(const QString& p_path);        
    void getFileName(QLineEdit* p_lineEdit, bool p_open = false);        
    
    const SyncingOption getSyncingOption (const QButtonGroup* p_buttonBox) const;
    
private:
    QString        m_pdaName;
    QString        m_addressBookFileName;
    QString        m_eventsFileName;
    QString        m_todosFileName;
    
    KConfig*       m_config;
};

#endif
