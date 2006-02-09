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
    EventHandler ();

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
    QString sCurrentTimeZone;
};

}

#endif
