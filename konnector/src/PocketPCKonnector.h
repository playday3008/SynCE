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
#ifndef KSYNCPOCKETPCKONNECTOR_H
#define KSYNCPOCKETPCKONNECTOR_H

//#define __USE_ABOVE_KDEPIM_3_3_0__
//#ifdef __USE_ABOVE_KDEPIM_3_3_0__
#include <kitchensync/konnector.h>
#include <kitchensync/konnectorinfo.h>
//#include <kitchensync/kapabilities.h>
#include <kitchensync/synceelist.h>
//#else
/*
#include <konnector.h>
#include <konnectorinfo.h>
#include <kapabilities.h>
#include <synceelist.h>
*/
//#endif
#include <ksharedptr.h>
#include <libkcal/calendarlocal.h>
#include <kabc/addressbook.h>

#include "rra.h"
#include "RecordType.h"

namespace KCal
{
    class Event;
};

class KABC::Addressee;

namespace KSync {

class AddressBookSyncEntry;
class CalendarSyncEntry;
class KonnectorUIDHelper;

/**
 * This is the Konnector to a Windows CE 3.0 driven device. This class massivly depends on external libraries to fullfill its purpose!
 * When reading the syncees this class checks, whether this is the first sync or not. Only if it is the first sync the complete data is
 * loaded from the device. Further loading only gets the necessary data (added and modified entries). Also only added and modified entries
 * are sent to the device. And of course, removed entries will be deleted.
 *  @author Christian Fremgen
*/
class PocketPCKonnector : public KSync::Konnector
{
public:
    /** Just the overloaded standard constructor. */
    PocketPCKonnector(const KConfig* p_config);

    /** The destructor. Tries to disconnect nicely! */
    ~PocketPCKonnector();
        
    /** Get the stored syncees. 
     * @see KSync::Konnector::syncees() 
    */
    virtual SynceeList syncees();    
    /** Read the syncess from the devices.
     * @see KSync::Konnector::readSyncees()
     * @return true if datasets can be loaded. false otherwise
    */    
    virtual bool readSyncees();      
    /** Write the synced syncees to the device. Store necessary ids and data locally.
     * @see KSync::Konnector::writeSyncees()
     * @return true if datasets can be written. false otherwise
    */
    virtual bool writeSyncees();
    /** Get capabilities of this Konnector.
     * @see KSync::Konnector::capabilities()
     * @return the Kapabilities
     */
//    virtual Kapabilities capabilities();
    /** Connect the device.
     * @see KSync::Konnector::connectDevice()
     * @return true if device can be connected. false otherwise
     */
    virtual bool connectDevice();
    /** Disconnect the device.
     * @see KSync::Konnector::disconnectDevice()
     * @return true if device can be disconnect. false otherwise
     */
    virtual bool disconnectDevice();
    /** Get info about the Konnector.
     * @see KSync::Konnector::info()
     * @return just some information
     */
    virtual KonnectorInfo info() const;       
    /** Store the configuration for a specific instance.
     * @see KSync::Konnector::writeConfig()
     */
    virtual void writeConfig (KConfig* p_config); 
    
    /** Set the name of the pda with which we want to sync.
     * @param p_pdaName name of the device. @see synce documentation. active-connection-file. vdccm!!
     */
    void setPdaName (const QString& p_pdaName);
    /** Get the currently used PDA-name.
     * @return pda name
     */
    const QString getPdaName () const;
    
private:    
    /** Modify the syncees in case of first sync.
     * @param p_syncee the syncee we want to modify
     */
    void firstSyncModifications (KSync::Syncee* p_syncee);
    
    /** Set the status of an entry and get or create a new Konnector-id.
     * @param p_entry the entry which we want to change
     * @param p_status the state we want to give this entry
     */
    void setSyncEntry    (KSync::SyncEntry* p_entry, KSync::SyncEntry::Status p_status);
    
    /** When writing back to the device we want to have the original ids. This method reassigns them.
     * @param p_syncee the syncee we want to change
     * @param p_name name of the entries within the KonnectorUIDHelper
     */
    void setKonnectorId  (KSync::Syncee* p_syncee, const QString& p_name);
    
    /** Save ids of added entries in the KonnectorUIDHelper.
     * @param p_syncee the syncee with the ids
     * @param p_name name of the entries within KonnectorUIDHelper
     */
    void saveIds         (KSync::Syncee* p_syncee, const QString& p_name);
    
    /** Just dump the ids of a syncee
     * @param p_syncee syncee to dump
     */
    void dumpIds         (KSync::Syncee* p_syncee);
       
    /** Add new ids to the KonnectorUIDHelper which are generated when writing to the device.
     * @param p_syncee to have proper ids in the meta-file we need to adjust the ids in this syncee
     * @param p_name name of the entries within KonnectorUIDHelper
     * @param p_newIds just the new ids (in conjunction with the local ones!)
     * @return a list of all the RRA-IDs which must be appended to the ids-File
     */
    QStringList addNewIds       (KSync::Syncee* p_syncee, const QString& p_name, QValueList<QPair<QString, QString> > p_newIds);
    /** The other way round to the above method (@see addNewIds())
     * @param p_name name of the entries within KonnectorUIDHelper
     * @param p_oldIds removed ids which are no longer usefull
     */
    void removeOldIds    (const QString& p_name, KSync::SyncEntry::PtrList p_oldIds);
    
    /** Just clear the internal data structures like m_addressBook.
     */
    void clearDataStructures ();
    
    /** Get a specified list of addressees to write to the device. e.g. all modified or added entries.
     * @param p_addressees addressees are stored here
     * @param p_ptrList a list of SyncEntries in which we are interestd (e.g. syncee->added() or syncee->modified())
     */
    void getAddressees (KABC::Addressee::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList);
    /** Get a specified list of events and todos to write to the device. e.g. all modified or added entries.
     * @param p_events events are stored here
     * @param p_todos todos are stored here
     * @param p_ptrList a list of SyncEntries in which we are interested (e.g. syncee->added() or syncee->modified())
     */
    void getEvents     (KCal::Event::List& p_events, KCal::Todo::List& p_todos, KSync::SyncEntry::PtrList p_ptrList);
    
    /** Load the meta data. This is the remote addressBook- and calendar-data which is stored locally.
     * @param p_dir directory where this data can be found
     */
    void loadMetaData  (const QString& p_dir);
    /** Save the meta data. This is the remote addressBook- and calendar-data which is stored locally.
     * @param p_dir directory where this data can be found
     */
    void saveMetaData  (const QString& p_dir);
    
    /** Update the addressBook after loading the data from the device after a first sync. This sets the correct state of the SyncEntries 
     * and does the necessary id-conversion.
     * @param p_added added addressees
     * @param p_modified modified addressees
     * @param p_removed ids for the removed addressees. These entries do exist locally within the meta-data!
     */
    void updateAddressBookSyncee (KABC::Addressee::List& p_added, KABC::Addressee::List& p_modified, QStringList& p_removed);
    /** Update the calendar after loading the data from the device after a first sync. This sets the correct state of the SyncEntries
     * and does the necessary id-conversion.
     * @param p_addedEvents new events
     * @param p_modifiedEvents modified events
     * @param p_addedTodos new todos
     * @param p_modifiedTodos modified todos
     * @param p_removedIds ids for the removed events and todos. These entries do exist locally within the meta-data!
     */
    void updateCalendarSyncee    (KCal::Event::List& p_addedEvents, KCal::Event::List& p_modifiedEvents,
                                  KCal::Todo::List& p_addedTodos, KCal::Todo::List& p_modifiedTodos, QStringList& p_removedIds);
    
    
    QString    m_pdaName;
    QString    m_baseDir;
    
    KSync::SynceeList m_syncees;        
        
    KSharedPtr<pocketPCCommunication::Rra>    m_rra;
    bool                                      m_rraExists;
    
    KSync::KonnectorUIDHelper*                m_uidHelper;
    
    KABC::AddressBook*                        m_addressBook;
    KCal::CalendarLocal*                      m_calendar;
};

};

#endif
