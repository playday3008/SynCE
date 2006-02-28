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

#include "addressbookhandler.h"
#include <kdebug.h>
#include <kapplication.h>
#include <kabc/vcardconverter.h>


namespace PocketPCCommunication
{
    AddressbookHandler::AddressbookHandler() : PimHandler()
    {
        mTypeId = 0;
    }


    bool AddressbookHandler::init()
    {
        mTypeId = m_rra->getTypeForName( RRA_SYNCMGR_TYPE_CONTACT );

        return mTypeId != 0;
    }


    AddressbookHandler::~AddressbookHandler()
    {}


    bool AddressbookHandler::retrieveAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList )
    {
        KABC::VCardConverter vCardConv;
        bool ret = false;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            incrementSteps();

            kdDebug(2120) << "Retrieving Contact from device: " << "RRA-ID-" +
                    QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            QString vCard = m_rra->getVCard( mTypeId, *it );
            if (vCard.isEmpty()) {
                goto error;
            }

            KABC::Addressee addr = vCardConv.parseVCard ( vCard );
            addr.setFormattedName(addr.formattedName().replace("\\,", ","));

            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCEAddressbook", addr.uid(), "---")) != "---") {
                addr.setUid(kdeId);
            } else {
                mUidHelper->addId("SynCEAddressbook", addr.uid(), addr.uid());
            }

            kdDebug(2120) << "    ID-Pair: KDEID: " << addr.uid() << " DeviceID: " <<
                "RRA-ID-" + QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            mAddresseeList.push_back( addr );

            KApplication::kApplication()->processEvents();
        }

        ret = true;

    error:
        return ret;
    }


    void AddressbookHandler::fakeAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList )
    {
        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            KABC::Addressee addr;

            QString konId = "RRA-ID-" + QString::number( *it, 16 ).rightJustify( 8, '0' );
            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCEAddressbook", konId, "---")) != "---") {
                addr.setUid(kdeId);
                mUidHelper->removeId("SynCEAddressbook", addr.uid());
                mAddresseeList.push_back( addr );
            }
            kdDebug(2120) << "Contact: " << konId << "  --  " << kdeId << endl;
        }
    }


    void AddressbookHandler::getIds()
    {
        m_rra->getIdsForType( mTypeId, &ids );
    }


    bool AddressbookHandler::getAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, int mRecType )
    {
        bool ret = true;

        if ( ( mRecType & CHANGED ) ) {
            setStatus("Reading changed Contacts");
            ret = retrieveAddresseeListFromDevice( mAddresseeList, ids.changedIds );
        }

        if ( ( mRecType & DELETED ) && ret) {
            setStatus("Creating dummys for deleted Contacts");
            fakeAddresseeListFromDevice( mAddresseeList, ids.deletedIds );
        }

        if ( ( mRecType & UNCHANGED ) && ret ) {
            setStatus("Reading unchanged Contacts");
            ret = retrieveAddresseeListFromDevice( mAddresseeList, ids.unchangedIds );
        }

        return ret;
    }


    void AddressbookHandler::insertIntoAddressBookSyncee(KSync::AddressBookSyncee *mAddressBookSyncee, KABC::Addressee::List &list, int state)
    {
        for(KABC::Addressee::List::Iterator it = list.begin(); it != list.end(); ++it) {
            KSync::AddressBookSyncEntry entry(*it, mAddressBookSyncee);
            entry.setState(state);
            mAddressBookSyncee->addEntry(entry.clone());
        }
    }


    bool AddressbookHandler::readSyncee(KSync::AddressBookSyncee *mAddressBookSyncee, bool firstSync)
    {
        bool ret = false;

        getIds();

        KABC::Addressee::List modifiedList;
        if (firstSync) {
            this->setMaximumSteps((ids.changedIds.size() + ids.unchangedIds.size()));
            if (!getAddresseeListFromDevice(modifiedList, PocketPCCommunication::UNCHANGED | PocketPCCommunication::CHANGED)) {
                setError("Can not retrieve unchanged Contacts from the Device");
                goto error;
            }
        } else {
            this->setMaximumSteps(ids.changedIds.size());
            if (!getAddresseeListFromDevice(modifiedList, PocketPCCommunication::CHANGED)) {
                setError("Can not retrieve changed Contacts from the Device");
                goto error;
            }

            KABC::Addressee::List removedList;
            if (!getAddresseeListFromDevice(removedList, PocketPCCommunication::DELETED)) {
                setError("Can not retrieve deleted Contacts from the Device");
                goto error;
            }
            insertIntoAddressBookSyncee(mAddressBookSyncee, removedList, KSync::SyncEntry::Removed);
        }
        insertIntoAddressBookSyncee(mAddressBookSyncee, modifiedList, KSync::SyncEntry::Modified);

        mAddressBookSyncee->setTitle("SynCEAddressbook");
        mAddressBookSyncee->setIdentifier(m_pdaName + "-Addressbook");

        ret = true;

    error:
        return ret;
    }


    void AddressbookHandler::getAddressees ( KABC::Addressee::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList )
    {
        KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin();
        for ( ; it != p_ptrList.end(); ++it ) {
            p_addressees.push_back ( ( dynamic_cast<KSync::AddressBookSyncEntry*>( *it ) ) ->addressee() );
        }
    }


    bool AddressbookHandler::addAddressees( KABC::Addressee::List& p_addresseeList )
    {
        bool ret = false;
        KABC::VCardConverter vCardConv;
        QString vCard;

        RRA_Uint32Vector* added_ids = rra_uint32vector_new();

        if ( p_addresseeList.begin() == p_addresseeList.end() ) {
            goto success;
        }

        for (KABC::Addressee::List::Iterator it = p_addresseeList.begin();
                it != p_addresseeList.end(); ++it ) {
            incrementSteps();

            kdDebug(2120) << "Adding Contact on Device: " << (*it).uid() << endl;

            vCard = vCardConv.createVCard ( ( *it ) );

            uint32_t newObjectId = m_rra->putVCard( vCard, mTypeId, 0 );
            if (newObjectId == 0) {
                goto error;
            }

            m_rra->markIdUnchanged( mTypeId, newObjectId );

            mUidHelper->addId("SynCEAddressbook",
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                (*it).uid());

            kdDebug(2120) << "    ID-Pair: KDEID: " << (*it).uid() << " DeviceID: " <<
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ) << endl;

            rra_uint32vector_add(added_ids, newObjectId);

            KApplication::kApplication()->processEvents();
        }

    success:
        ret = true;

    error:
        m_rra->registerAddedObjects(mTypeId, added_ids);
        rra_uint32vector_destroy(added_ids, true);

        return ret;
    }


    bool AddressbookHandler::updateAddressees( KABC::Addressee::List& p_addresseeList )
    {
        bool ret = false;
        KABC::Addressee::List::Iterator it = p_addresseeList.begin();
        KABC::VCardConverter vCardConv;
        QString vCard;

        if ( p_addresseeList.begin() == p_addresseeList.end() ) {
            goto success;
        }

        setStatus("Writing changed Contacts");

        for ( ; it != p_addresseeList.end(); ++it ) {
            incrementSteps();

            QString kUid = mUidHelper->konnectorId("SynCEAddressbook", (*it).uid(), "---");

            if (kUid != "---") {
                kdDebug(2120) << "Updating Contact on Device: " << "ID-Pair: KDEID: " <<
                    (*it).uid() << " DeviceId: " << kUid << endl;
                vCard = vCardConv.createVCard ( ( *it ) );
                uint32_t retId = m_rra->putVCard ( vCard, mTypeId, getOriginalId( kUid ) );
                if (retId == 0) {
                    goto error;
                }

                m_rra->markIdUnchanged( mTypeId, getOriginalId( kUid ) );
            }

            KApplication::kApplication()->processEvents();
        }

success:
        ret = true;

error:
        return ret;
    }


    bool AddressbookHandler::removeAddressees ( KABC::Addressee::List& p_addresseeList )
    {
//        int errorCount = 0;
        bool ret = false;
        RRA_Uint32Vector* deleted_ids = rra_uint32vector_new();
        KABC::Addressee::List::Iterator it = p_addresseeList.begin();

        if ( p_addresseeList.begin() == p_addresseeList.end() ) {
            goto success;
        }

        setStatus("Erasing deleted Contacts");

        for ( ; it != p_addresseeList.end(); ++it ) {
            incrementSteps();

            QString kUid = mUidHelper->konnectorId("SynCEAddressbook", (*it).uid(), "---");

            if (kUid != "---") {
                kdDebug(2120) << "Removing Contact on Device: " << "ID-Pair: KDEID: " <<
                    (*it).uid() << " DeviceId: " << kUid << endl;

                if (!m_rra->deleteObject (mTypeId, getOriginalId( kUid ))) {
//                    if (errorCount++ == -1) {
//                        goto error;
//                    }
                }

                mUidHelper->removeId("SynCEAddressbook", kUid);
                rra_uint32vector_add(deleted_ids, getOriginalId( kUid ));
            }

            KApplication::kApplication()->processEvents();
        }

    success:
        ret = true;

//    error:
        m_rra->removeDeletedObjects(mTypeId, deleted_ids);
        rra_uint32vector_destroy(deleted_ids, true);
        return ret;
    }


    bool AddressbookHandler::writeSyncee(KSync::AddressBookSyncee *mAddressBookSyncee)
    {
        bool ret = true;

        if ( mAddressBookSyncee->isValid() ) {
            KABC::Addressee::List addrAdded;
            KABC::Addressee::List addrRemoved;
            KABC::Addressee::List addrModified;

            setMaximumSteps(mAddressBookSyncee->added().count() + mAddressBookSyncee->removed().count() + mAddressBookSyncee->modified().count());
            resetSteps();
            getAddressees( addrAdded, mAddressBookSyncee->added() );
            getAddressees( addrRemoved, mAddressBookSyncee->removed() );
            getAddressees( addrModified, mAddressBookSyncee->modified() );

            setStatus( "Writing added Contacts" );
            if (ret = addAddressees( addrAdded )) {
                setStatus( "Erasing deleted Contacts" );
                if (ret = removeAddressees( addrRemoved )) {
                    setStatus( "Writing changed Contacts" );
                    if (!(ret = updateAddressees( addrModified ))) {
                        setError("Can not write back updated Contacts to the Device");
                    }
                } else {
                    setError("Can not erase deleted Contacts on the Device");
                }
            } else {
                setError("Can not added Contacts on the Device");
            }
        }

        return ret;
    }
}
