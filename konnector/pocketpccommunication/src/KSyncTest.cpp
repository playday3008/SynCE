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
#include "KSyncTest.h"
//#include "syncalgo.h"

#include <kabc/resourcefile.h>

#define __USE_ABOVE_KDEPIM_3_3_0__
#ifdef __USE_ABOVE_KDEPIM_3_3_0__
#include <kitchensync/addressbooksyncee.h>
#include <kitchensync/calendarsyncee.h>
#include <kitchensync/syncer.h>
#include <kitchensync/syncuikde.h>
#else
#include <addressbooksyncee.h>
#include <calendarsyncee.h>
#include <syncer.h>
#include <syncuikde.h>
#endif
#include <kdebug.h>

#include <qfile.h>
#include <kabc/vcardconverter.h>

KSyncTest::KSyncTest()
{
}


KSyncTest::~KSyncTest()
{
}


void KSyncTest::syncAddressBook (KABC::AddressBook* p_addrBookDevice, KABC::AddressBook* p_addrBookLocal, 
                                 bool p_firstSync, bool p_overwrite, bool p_loadLog, 
                                 SyncingOption p_syncOption, const QString& p_partnerId,
                                 KABC::AddressBook* p_targetBook)
{
    KSync::SyncUiKde kde(0, true);
    //KSync::PIMSyncAlg algo (&kde);
    
    KSync::Syncer syncer; //, &algo);
    syncer.setSyncUi(&kde);
    
    KSync::AddressBookSyncee synceeDevice(p_addrBookDevice);
    KSync::AddressBookSyncee synceeLocal(p_addrBookLocal);
    /*
    KSync::AddressBookSyncEntry* entry = synceeLocal.firstEntry();
    while (entry)
    {        
        kdDebug(2120) << "KSyncTest: remote id: " << entry->addressee().custom ("POCKETPCCOMM", "REMOTE-ID-" + p_partnerId) << endl;
        kdDebug(2120) << "KSyncTest: local id: " << entry->addressee().uid() << endl;
        kdDebug(2120) << "KSyncTest: type: " << entry->type() << endl;
        synceeLocal.insertId (entry->type(), entry->addressee().custom ("POCKETPCCOMM", "REMOTE-ID-" + p_partnerId), entry->addressee().uid());
        entry = synceeLocal.nextEntry();
    }
    */
    KABC::AddressBook::Iterator it;
    
    
    it = p_addrBookLocal->begin();
    
    for (; it != p_addrBookLocal->end(); ++it)
    {
        QString localId = (*it).custom("POCKETPCCOMM", "REMOTE-ID-" + p_partnerId);
        kdDebug(2120) << "KSyncTest: POCKETPCCOMM-REMOTE-ID: " << p_partnerId << ":" <<  localId << endl;
        //synceeLocal.insertId("AddressBookSyncEntry", localId, (*it).uid());
        //synceeDevice.insertId("address", p_partnerId + ":"+ localId, (*it).uid());
        //(*it).removeCustom ("POCKETPCCOMM", "REMOTE-ID-"+p_partnerId);
        //KSync::AddressBookSyncEntry* addrEntry = new KSync::AddressBookSyncEntry (*it, &synceeLocal);
        //synceeLocal.addEntry(addrEntry);
        kdDebug(2120) << "LocalBook id: " << (*it).uid() << endl;
    }
    
    /*
    it = p_addrBookDevice->begin();
    for (; it != p_addrBookDevice->end(); ++it)
    {
        //synceeDevice.insertId("addressee", (*it).custom("POCKETPCCOMM", "REMOTE-ID-" + p_partnerId), (*it).uid());
        //(*it).removeCustom ("POCKETPCCOMM", "REMOTE-ID-"+p_partnerId);
        KSync::AddressBookSyncEntry* addrEntry = new KSync::AddressBookSyncEntry (*it, &synceeDevice);
        synceeDevice.addEntry(addrEntry);
        kdDebug(2120) << "DeviceBook id: " << (*it).uid() << endl;
    }    
    */
    
    // iterate over addressbook and set the ids on the syncee...
    // not over addressbook!!!!!! this is wrong!!! because this is not changed within the syncee!!
    /*
    KABC::AddressBook::Iterator it = p_addrBookLocal->begin();
    for (; it != p_addrBookLocal->end(); ++it)
    {
        kdDebug(2120) << "syncing: remote id: " << (*it).custom("POCKETPCCOMM", "REMOTE-ID-" + p_partnerId) << endl;
        synceeLocal.insertId ("addressee", p_partnerId + ":" + (*it).custom("POCKETPCCOMM", "REMOTE-ID-" + p_partnerId), (*it).uid());
        synceeDevice.insertId ("addressee", p_partnerId + ":" + (*it).uid(), (*it).custom("POCKETPCCOMM", "REMOTE-ID-" + p_partnerId));
        it->removeCustom ("POCKETPCCOMM", "REMOTE-ID-"+p_partnerId);
    }
    */
    
    synceeDevice.setIdentifier("device");
    synceeLocal.setIdentifier("local");

#ifdef __USE_ABOVE_KDEPIM_3_3_0__    
    synceeDevice.setTitle ("device");
    synceeLocal.setTitle ("local");
#else
    synceeDevice.setSource ("device");
    synceeLocal.setSource ("local");
#endif
    //kdDebug(2120) << "logfilenames: " << synceeDevice.statusLogName() << "    " << synceeLocal.statusLogName() << endl;
    kdDebug(2120) << "firstSync: " << p_firstSync << endl;
    
    
    // just for testing!!!!
    if (p_firstSync)
    {
        //synceeDevice.setFirstSync(true);
        //synceeLocal.setFirstSync(true);
    }
    
    if (p_loadLog)
    {
        //synceeDevice.loadLog();
        //synceeLocal.loadLog();
    }
    
    
    // what does the log in Syncer do??
    syncer.addSyncee (&synceeLocal);
    syncer.addSyncee (&synceeDevice);
    
    
    
    //syncer.sync();
    
    // what we need here is syncToTarget!!!
    KSync::AddressBookSyncee target;
    KSync::AddressBookSyncEntry* addrEntry;
    KABC::Addressee::List adrList;
    KABC::Addressee::List::Iterator adrIt;
    target.setIdentifier("target");
    
    QFile f( "/tmp/target.vcf" );
    f.open( IO_WriteOnly );
    QTextStream t( &f );
    KABC::VCardConverter vcard;
        
    
    //KABC::AddressBook::Iterator it;
    switch (p_syncOption)
    {
        case MERGE:
            syncer.syncAllToTarget(&target, p_overwrite);
            //p_addrBookLocal->clear();
            addrEntry = target.firstEntry();
            while (addrEntry)
            {
                //p_targetBook->insertAddressee (addrEntry->addressee());
                //kdDebug(2120) << "TargetBook id: " << addrEntry->addressee().uid() << endl;                
                //kdDebug(2120) << "TargetBook name: " << addrEntry->addressee().realName() << endl;                
                /*
                p_targetBook->find(addrEntry->addressee())->setUid(addrEntry->addressee().uid());
                kdDebug(2120) << "TargetBook id in target: " << p_targetBook->find(addrEntry->addressee())->uid() << endl;
                kdDebug(2120) << "TargetBook name in target: " << p_targetBook->find(addrEntry->addressee())->realName() << endl;
                */
                t << vcard.createVCard( addrEntry->addressee() );
                addrEntry = target.nextEntry();                                
            }         
            f.flush();               
            f.close();
            p_targetBook->load();
            adrList = p_targetBook->allAddressees();
            adrIt = adrList.begin();
            for (; adrIt != adrList.end(); ++adrIt)
            {
                kdDebug(2120) << "TargetDump: " << (*adrIt).uid() << endl;
            }
            
            it = p_targetBook->begin();
            
            for (; it != p_targetBook->end(); ++it)
            {
                //kdDebug(2120) << "TargetDump2: " << (*it).uid() << endl;
            }                        
            break;
        case DEVICE:
            syncer.syncToTarget(&synceeLocal, &synceeDevice, p_overwrite);    
            break;
        case LOCAL:
            syncer.syncToTarget(&synceeDevice, &synceeLocal, p_overwrite);    
            break;
    }
    
    //synceeDevice.saveLog();
    //synceeLocal.saveLog();
}


void KSyncTest::syncCalendar (KCal::CalendarLocal* p_calDevice, KCal::CalendarLocal* p_calLocal, 
                              bool p_firstSync, bool p_overwrite, bool p_loadLog, 
                              SyncingOption p_syncOption, const CalendarSyncName p_name,
                              KCal::CalendarLocal* p_targetCalendar)
{
    KSync::Syncer syncer;
    
    KCal::CalendarLocal emptyLocal;
    KCal::CalendarLocal emptyDevice;
    KSync::CalendarSyncee synceeDevice(&emptyDevice); //(p_calDevice);
    KSync::CalendarSyncee synceeLocal(&emptyLocal); //(p_calLocal);
	
    KCal::Incidence::List incidences = p_calLocal->incidences();
	KCal::Incidence::List::Iterator it = incidences.begin();
 
	for (; it != incidences.end(); ++it)
	{
        kdDebug(2120) << "syncCalendar: current id: " << (*it)->uid() << endl;
		KSync::CalendarSyncEntry* calEntry = new KSync::CalendarSyncEntry (*it, &synceeLocal);
		calEntry->setId ((*it)->uid());
        kdDebug(2120) << "syncCalendar: current calEntry->id: " << calEntry->id() << endl;     
		synceeLocal.addEntry(calEntry);
	}

    kdDebug(2120) << "and now we are at KSyncTest::210" << endl;
    
    incidences = p_calDevice->incidences();
	it = incidences.begin();
    for (; it != incidences.end(); ++it)
	{
		KSync::CalendarSyncEntry* calEntry = new KSync::CalendarSyncEntry (*it, &synceeDevice);
		calEntry->setId ((*it)->uid());
		synceeDevice.addEntry(calEntry);
	}
	
    
    switch (p_name)
    {
        case CALENDAR:            
            synceeDevice.setIdentifier("calendar_device");
            synceeLocal.setIdentifier("calendar_local");
            break;
        case EVENTS:
            synceeDevice.setIdentifier("events_device");
            synceeLocal.setIdentifier("events_local");
            break;
        case TODOS:
            synceeDevice.setIdentifier("todos_device");
            synceeLocal.setIdentifier("todos_local");
            break;
    }
    
    // just for testing!!!!
    if (p_firstSync)
    {
        //synceeDevice.setFirstSync(true);
        //synceeLocal.setFirstSync(true);
    }
    
    if (p_loadLog)    
    {
        //synceeDevice.loadLog();
        //synceeLocal.loadLog();
    }
    
    // what does the log in Syncer do??
    syncer.addSyncee (&synceeDevice);
    syncer.addSyncee (&synceeLocal);
    
    //syncer.sync();
    
    // what we need here is syncToTarget!!!
    KCal::CalendarLocal emptyTarget;
    KSync::CalendarSyncee target(&emptyTarget);
    target.setIdentifier("target");
    KSync::CalendarSyncEntry* calEntry;
    
    switch (p_syncOption)
    {
        case MERGE:
            syncer.syncAllToTarget(&target, p_overwrite);
            // now fill the target calendar
            calEntry = target.firstEntry();
            while (calEntry)
            {
                kdDebug(2120) << "CalendarSyncer: incidence->uid(): " << calEntry->incidence()->uid() << endl;
                kdDebug(2120) << "CalendarSyncer: current type: " << calEntry->incidence()->type() << endl;
                if (calEntry->incidence()->type() == "Event")
                    p_targetCalendar->addEvent (new KCal::Event(*(dynamic_cast<KCal::Event*>(calEntry->incidence()))));
                else if (calEntry->incidence()->type() == "Todo")
                    p_targetCalendar->addTodo (new KCal::Todo(*(dynamic_cast<KCal::Todo*>(calEntry->incidence()))));
                //p_targetCalendar->addIncidence (calEntry->incidence());
                //p_targetCalendar->update(calEntry->incidence());
                calEntry = target.nextEntry();
            }
            //syncer.sync();
            
            incidences = p_targetCalendar->incidences();
            it = incidences.begin();
            for (; it != incidences.end(); ++it)
                kdDebug(2120) << "id in p_targetCalendar: " << (*it)->uid() << endl;
            //p_targetCalendar.updated();
            break;
        case DEVICE:
            syncer.syncToTarget(&synceeLocal, &synceeDevice, p_overwrite);    
            break;
        case LOCAL:
            syncer.syncToTarget(&synceeDevice, &synceeLocal, p_overwrite);    
            break;
    }
}

/*
void KSyncTest::syncEvents (KCal::CalendarLocal* p_calDevice, KCal::CalendarLocal* p_calLocal, bool p_overwrite, SyncingOption p_syncOption)
{
    // seems that syncees must be filled by hand!!
    // -> nope... must be synced with calendar-syncee!!
    
    KSync::Syncer syncer;
    
    KSync::EventSyncee syncee1(p_cal1);
    KSync::EventSyncee syncee2(p_cal2);
    
}
*/

/*
void KSyncTest::syncTodos (KCal::CalendarLocal* p_calDevice, KCal::CalendarLocal* p_calLocal, bool p_overwrite, SyncingOption p_syncOption)
{
    // seems that syncees must be filled by hand!!
    // -> nope... must be synced with calendar-syncee!!
    
    KSync::Syncer syncer;
    
    KSync::TodoSyncee syncee1(p_cal1);
    KSync::TodoSyncee syncee2(p_cal2);
    
}
*/

