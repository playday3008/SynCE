//
// C++ Interface: eventhandler
//
// Description:
//
//
// Author: Christian Fremgen; Volker Christian <cfremgen@users.sourceforge.net>, (C) 2005
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef POCKETPCCOMMUNICATIONEVENTHANDLER_H
#define POCKETPCCOMMUNICATIONEVENTHANDLER_H


#include "PimHandler.h"
#include "RecordType.h"
#include <kitchensync/idhelper.h>
#include <kitchensync/calendarsyncee.h>

namespace pocketPCCommunication {

/**
@author Christian Fremgen cfremgen@users.sourceforge.net, Volker Christian voc@users.sourceforge.net
*/
class EventHandler : public PimHandler
{
public:
    EventHandler (KSharedPtr<Rra> p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper);

    bool init();

    virtual ~EventHandler();

    int retrieveEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList);
    int fakeEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList);
    bool getIds();
    int getEventListFromDevice(KCal::Event::List &mEventList, int mRecType);
    bool readSyncee(KSync::CalendarSyncee *mCalendarSyncee, bool firstSync);
    void getEvents (KCal::Event::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList );
    void getTodosAsFakedEvents(KCal::Event::List& p_events, KSync::SyncEntry::PtrList p_ptrList );
    bool writeSyncee(KSync::CalendarSyncee *mCalendarSyncee);
    void insertIntoCalendarSyncee(KSync::CalendarSyncee *mCalendarSyncee, KCal::Event::List &list, int state);

    void addEvents    (KCal::Event::List& p_eventList);
    void updateEvents (KCal::Event::List& p_eventList);
    void removeEvents (KCal::Event::List& p_eventList);

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

private:
    uint32_t    mTypeId;   /**< This static member stores the typeId belonging to "Contact" */

    bool initialized;

    struct Rra::ids ids;
    KSync::KonnectorUIDHelper *mUidHelper;
    QString mBaseDir;
    QString sCurrentTimeZone;
};

}

#endif
