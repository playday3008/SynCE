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
#ifndef KSYNCTEST_H
#define KSYNCTEST_H

#include "Preferences.h"

#include <libkcal/calendarlocal.h>
#include <kabc/addressbook.h>


enum CalendarSyncName {CALENDAR = 0, EVENTS = 1, TODOS = 2};

/**
This is just a little class to test the various Syncees of libksync.

@author Christian Fremgen
*/
class KSyncTest
{
public:
    KSyncTest();

    ~KSyncTest();
    
    void syncAddressBook (KABC::AddressBook* p_addrBookDevice, KABC::AddressBook* p_addrBookLocal, 
                          bool p_firstSync, bool p_overwrite, bool p_loadLog, SyncingOption p_syncOption,
                          const QString& p_partnerId, KABC::AddressBook* p_targetBook);
    void syncCalendar    (KCal::CalendarLocal* p_calDevice, KCal::CalendarLocal* p_calLocal, 
                          bool p_firstSync, bool p_overwrite, bool p_loadLog, SyncingOption p_syncOption, const CalendarSyncName p_name,
                          KCal::CalendarLocal* p_targetCalendar);
    //void syncEvents      (KCal::CalendarLocal* p_calDevice, KCal::CalendarLocal* p_calLocal, bool p_overwrite, SyncingOption p_syncOption = MERGE);
    //void syncTodos       (KCal::CalendarLocal* p_calDevice, KCal::CalendarLocal* p_calLocal, bool p_overwrite, SyncingOption p_syncOption = MERGE);

private:
//    void doSyncing (KSync::Syncee& p_syncee1, KSync::Syncee& p_syncee2, bool p_overwrite, SyncingOption p_syncOption);
};

#endif
