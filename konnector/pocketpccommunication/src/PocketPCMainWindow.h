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
#ifndef POCKETPCMAINWINDOW_H
#define POCKETPCMAINWINDOW_H

#include <rra.h> // for uint32_t

#include "PocketPCMainWindowUI.h"
#include "KSyncTest.h"

#include <libkcal/calendarlocal.h>

class QProgressBar;
class Preferences;

/**
This is the main window for testing libkpocketpccomm

@author Christian Fremgen
*/
class PocketPCMainWindow : public PocketPCMainWindowUI
{
    
public:
    PocketPCMainWindow();

    ~PocketPCMainWindow();

protected slots:
    void slotGetTypesAction ();
    void slotGetObjectIds (QListViewItem* p_item);
    //void slotSetPdaName ();
    void slotGetAddresses ();
    void slotPushAddresses ();
    void slotGetCalendar();
    void slotPutCalendar();
    
    void slotGetEvents();
    void slotGetTodos();
    void slotPushEvents();
    void slotPushTodos();
    
    void slotProgress (int p_progress);
    
    void slotBackup ();
    void slotRestore ();
    
    void slotDeleteCalendar ();    
    void slotDeleteAddressBook ();
    void slotDeleteEverything ();    
    
    void slotShowPreferences ();
    
    void slotSyncKonnector ();
    void slotSyncAddressBooks();
    void slotSyncCalendars();
    void slotSyncEvents();
    void slotSyncTodos();
    
    
private:
    void appendRedText (const QString& p_text);
    void appendBlueText (const QString& p_text);
    void appendColoredText (const QColor& p_col, const QString& p_text);
    void printObjectIds (const QValueList<uint32_t>& p_valList, const QString& p_text, const QColor& p_col = QColor(0, 0, 0));
    
    void syncCalendars (const QString& p_fileDevice, const QString& p_fileLocal, const QString& p_fileOutput, 
                        bool p_firstSync, bool p_overwrite, bool p_loadLog, 
                        SyncingOption p_syncOption, const CalendarSyncName p_name);
    
    void saveAddressBook (KABC::AddressBook& p_addrBook, const QString& p_fileName);
    
    void syncAddressBookNoKonnector ();
    void syncAddressBookKonnector ();
    
    //QString                m_pdaName;    
    KCal::CalendarLocal    m_calendar;    
    QProgressBar*          m_progressBar;
    Preferences*           m_preferences;
    
    QString                m_partnerId;
};

#endif
