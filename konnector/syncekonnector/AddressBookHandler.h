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
#ifndef POCKETPCCOMMUNICATIONADDRESSBOOKHANDLER_H
#define POCKETPCCOMMUNICATIONADDRESSBOOKHANDLER_H

#include "PimHandler.h"
#include <kitchensync/addressbooksyncee.h>
#include <kitchensync/idhelper.h>


namespace pocketPCCommunication {

/**
This class handles an AddressBook which can be read from and written to a Windows CE device.

@author Christian Fremgen cfremgen@users.sourceforge.net, Volker Christian voc@users.sourceforge.net
*/
class AddressBookHandler : public PimHandler
{
public:
    AddressBookHandler (KSharedPtr<Rra> p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper);

    bool init();

    virtual ~AddressBookHandler();

    int retrieveAddresseeListFromDevice(KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList);
    int fakeAddresseeListFromDevice(KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList);
    bool getIds();
    int getAddresseeListFromDevice(KABC::Addressee::List &mAddresseeList, int mRecType);
    bool readSyncee(KSync::AddressBookSyncee *mAddressBookSyncee, bool firstSync);
    void getAddressees ( KABC::Addressee::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList );
    bool writeSyncee(KSync::AddressBookSyncee *mAddressBookSyncee);
    void insertIntoAddressBookSyncee(KSync::AddressBookSyncee *mAddressBookSyncee, KABC::Addressee::List &list, int state);

    void addAddressees    (KABC::Addressee::List& p_addresseeList);
    void updateAddressees (KABC::Addressee::List& p_addresseeList);
    void removeAddressees (KABC::Addressee::List& p_addresseeList);

private:
    KSync::KonnectorUIDHelper *mUidHelper;
    QString mBaseDir;
};

}

#endif
