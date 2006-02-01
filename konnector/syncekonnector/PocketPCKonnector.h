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

#include <ksharedptr.h>

#include "syncekonnectorbase.h"
#include <kitchensync/synceelist.h>
#include <kitchensync/idhelper.h>
#include <libkcal/calendarlocal.h>

#include "rra.h"
#include "RecordType.h"
#include "eventsyncee.h"
#include "todosyncee.h"

namespace KCal
{
    class Event;
}

namespace pocketPCCommunication {
    class AddressBookHandler;
    class TodoHandler;
    class EventHandler;
}

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
class PocketPCKonnector : public KSync::SynCEKonnectorBase
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

    virtual void subscribeTo( int type );

    virtual void unsubscribeFrom( int type );

    virtual void actualSyncType(int type);

    bool getContactsEnabled();
    bool getContactsFirstSync();
    bool getEventsEnabled();
    bool getEventsFirstSync();
    bool getTodosEnabled();
    bool getTodosFirstSync();

    void setContactsState(bool enabled, bool firstSync);
    void setEventsState(bool enabled, bool firstSync);
    void setTodosState(bool enabled, bool firstSync);

    QStringList supportedFilterTypes() const {
        QStringList types;
        types << "addressbook" << "calendar";

        return types;
    };

    void init();

private:

    KCal::CalendarLocal mEventCalendar;
    KCal::CalendarLocal mTodoCalendar;

    KSync::AddressBookSyncee *mAddressBookSyncee;
    KSync::EventSyncee *mEventSyncee;
    KSync::TodoSyncee *mTodoSyncee;

    pocketPCCommunication::AddressBookHandler *mAddrHandler;
    pocketPCCommunication::TodoHandler *mTodoHandler;
    pocketPCCommunication::EventHandler *mEventHandler;

    SynceeList mSyncees;

    QString    mBaseDir;

    bool contactsEnabled;
    bool contactsFirstSync;

    bool eventsEnabled;
    bool eventsFirstSync;

    bool todosEnabled;
    bool todosFirstSync;

    bool initialized;

    /** Just clear the internal data structures like m_addressBook.
     */
    void clearDataStructures ();

    QString    m_pdaName;
    Rra*   m_rra;

    KSync::KonnectorUIDHelper *mUidHelper;

    KPIM::ProgressItem *mProgressItem;

    int subscribtions;

    int _actualSyncType;

    bool idsRead;

    int subscribtionCount;
};

}

#endif
