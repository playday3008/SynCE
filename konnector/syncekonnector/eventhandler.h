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


#include "pimhandler.h"
#include "recordtype.h"
#include <kitchensync/idhelper.h>
#include "eventsyncee.h"
#include <libkdepim/progressmanager.h>


namespace KPIM {
    class ProgressItem;
}

namespace PocketPCCommunication {

/**
@author Christian Fremgen cfremgen@users.sourceforge.net, Volker Christian voc@users.sourceforge.net
*/
class EventHandler : public PimHandler
{
public:
    EventHandler (Rra* p_rra, KSync::KonnectorUIDHelper *mUidHelper);

    bool init();

    virtual ~EventHandler();

    int retrieveEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList);
    int fakeEventListFromDevice(KCal::Event::List &mEventList, QValueList<uint32_t> &idList);
    bool getIds();
    int getEventListFromDevice(KCal::Event::List &mEventList, int mRecType);
    bool readSyncee(KSync::EventSyncee *mCalendarSyncee, bool firstSync);
    void getEvents (KCal::Event::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList );
    void getTodosAsFakedEvents(KCal::Event::List& p_events, KSync::SyncEntry::PtrList p_ptrList );
    bool writeSyncee(KSync::EventSyncee *mCalendarSyncee);
    void insertIntoCalendarSyncee(KSync::EventSyncee *mCalendarSyncee, KCal::Event::List &list, int state);

    void addEvents    (KCal::Event::List& p_eventList);
    void updateEvents (KCal::Event::List& p_eventList);
    void removeEvents (KCal::Event::List& p_eventList);

private:
    KSync::KonnectorUIDHelper *mUidHelper;
    QString sCurrentTimeZone;
};

}

#endif
