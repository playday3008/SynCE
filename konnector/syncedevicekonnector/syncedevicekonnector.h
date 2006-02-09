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

#ifndef KSYNCPOCKETPCKONNECTOR_H
#define KSYNCPOCKETPCKONNECTOR_H

#include <ksharedptr.h>

#include "syncekonnectorbase.h"
#include <kitchensync/synceelist.h>
#include <kitchensync/idhelper.h>
#include <libkcal/calendarlocal.h>
#include <kabc/addressbook.h>

#include "rra.h"
#include "eventsyncee.h"
#include "todosyncee.h"

namespace KCal
{
    class Event;
}
namespace KABC
{
    class ResourceFile;
}

namespace PocketPCCommunication {
    class AddressbookHandler;
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
class SynCEDeviceKonnector : public KSync::SynCEKonnectorBase
{
public:
    /** Just the overloaded standard constructor. */
    SynCEDeviceKonnector(const KConfig* p_config);

    /** The destructor. Tries to disconnect nicely! */
    ~SynCEDeviceKonnector();

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

    void subscribeTo( int type );

    void unsubscribeFrom( int type );

    void actualSyncType(int type);

    void setPairUid(const QString &pairUid);

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
    KABC::AddressBook mAddressBook;

    KSync::AddressBookSyncee *mAddressBookSyncee;
    KSync::EventSyncee *mEventSyncee;
    KSync::TodoSyncee *mTodoSyncee;

    PocketPCCommunication::AddressbookHandler *mAddrHandler;
    PocketPCCommunication::TodoHandler *mTodoHandler;
    PocketPCCommunication::EventHandler *mEventHandler;

    KABC::ResourceFile *mAddressBookResourceFile;

    SynceeList mSyncees;

    QString    mBaseDir;

    QString pairUid;

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
