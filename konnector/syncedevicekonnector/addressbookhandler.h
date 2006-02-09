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

#ifndef POCKETPCCOMMUNICATIONADDRESSBOOKHANDLER_H
#define POCKETPCCOMMUNICATIONADDRESSBOOKHANDLER_H

#include "pimhandler.h"
#include "recordtype.h"
#include <kitchensync/addressbooksyncee.h>
#include <kitchensync/idhelper.h>



namespace KPIM {
    class ProgressItem;
}

namespace PocketPCCommunication {

/**
This class handles an AddressBook which can be read from and written to a Windows CE device.

@author Christian Fremgen cfremgen@users.sourceforge.net, Volker Christian voc@users.sourceforge.net
*/
class AddressbookHandler : public PimHandler
{
public:
    AddressbookHandler ();

    bool init();

    virtual ~AddressbookHandler();

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
};

}

#endif
