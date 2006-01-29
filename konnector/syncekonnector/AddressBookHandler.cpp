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
#include "AddressBookHandler.h"
#include <kdebug.h>
#include <kapplication.h>
#include <kabc/vcardconverter.h>


namespace pocketPCCommunication
{
    AddressBookHandler::AddressBookHandler( Rra *p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper)
            : PimHandler( p_rra, mUidHelper )
    {
        initialized = false;
        mTypeId = 0;
        this->mBaseDir = mBaseDir;
        this->mUidHelper = mUidHelper;
    }


    bool AddressBookHandler::init()
    {
        mTypeId = m_rra->getTypeForName( RRA_SYNCMGR_TYPE_CONTACT );

        return initialized = mTypeId != 0;
    }


    AddressBookHandler::~AddressBookHandler()
    {}


    int AddressBookHandler::retrieveAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList )
    {
        int count = 0;
        KABC::VCardConverter vCardConv;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            incrementSteps();
            count++;

            kdDebug(2120) << "Retrieving Contact from device: " << "RRA-ID-" +
                    QString::number ( *it, 16 ).rightJustify( 8, '0' ) << endl;

            QString vCard = m_rra->getVCard( mTypeId, *it );

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

        return count;
    }


    int AddressBookHandler::fakeAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList )
    {
        int count = 0;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;

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

        return count;
    }


    bool AddressBookHandler::getIds()
    {
        m_rra->getIdsForType( mTypeId, &ids );

        return true;
    }


    int AddressBookHandler::getAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, int mRecType )
    {
        int count = 0;
        int ret = 0;

        if ( ( mRecType & CHANGED ) && ( ret >= 0 ) ) {
            setStatus("Reading changed Contacts");
            kdDebug(2120) << "Retrieving changed Contacts" << endl;
            ret = retrieveAddresseeListFromDevice( mAddresseeList, ids.changedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & DELETED ) && ( ret >= 0 ) ) {
            setStatus("Creating dummys for deleted Contacts");
            kdDebug(2120) << "Faking deleted Contacts" << endl;
            ret = fakeAddresseeListFromDevice( mAddresseeList, ids.deletedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & UNCHANGED ) && ( ret >= 0 ) ) {
            setStatus("Reading unchanged Contacts");
            kdDebug(2120) << "Retrieving unchanged Contacts" << endl;
            ret = retrieveAddresseeListFromDevice( mAddresseeList, ids.unchangedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ret < 0 ) {
            return -count - 1;
        }

        return count;
    }


    void AddressBookHandler::insertIntoAddressBookSyncee(KSync::AddressBookSyncee *mAddressBookSyncee, KABC::Addressee::List &list, int state)
    {
        for(KABC::Addressee::List::Iterator it = list.begin(); it != list.end(); ++it) {
            KSync::AddressBookSyncEntry entry(*it, mAddressBookSyncee);
            entry.setState(state);
            mAddressBookSyncee->addEntry(entry.clone());
        }
    }


    bool AddressBookHandler::readSyncee(KSync::AddressBookSyncee *mAddressBookSyncee, bool firstSync)
    {
        mAddressBookSyncee->reset();

        getIds();

        KABC::Addressee::List modifiedList;
        if (firstSync) {
            this->setMaximumSteps((ids.changedIds.size() + ids.unchangedIds.size()));
            if (getAddresseeListFromDevice(modifiedList, pocketPCCommunication::UNCHANGED | pocketPCCommunication::CHANGED) < 0) {
                return false;
            }
        } else {
            this->setMaximumSteps(ids.changedIds.size());
            if (getAddresseeListFromDevice(modifiedList, pocketPCCommunication::CHANGED) < 0) {
                return false;
            }

            KABC::Addressee::List removedList;
            if (getAddresseeListFromDevice(removedList, pocketPCCommunication::DELETED) < 0) {
                return false;
            }
            insertIntoAddressBookSyncee(mAddressBookSyncee, removedList, KSync::SyncEntry::Removed);
        }
        insertIntoAddressBookSyncee(mAddressBookSyncee, modifiedList, KSync::SyncEntry::Modified);

        mAddressBookSyncee->setTitle("SynCEAddressbook");
        mAddressBookSyncee->setIdentifier(m_pdaName + "-Addressbook");

        return true;
    }


    void AddressBookHandler::getAddressees ( KABC::Addressee::List& p_addressees, KSync::SyncEntry::PtrList p_ptrList )
    {
        KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin();
        for ( ; it != p_ptrList.end(); ++it ) {
            p_addressees.push_back ( ( dynamic_cast<KSync::AddressBookSyncEntry*>( *it ) ) ->addressee() );
        }
    }


    void AddressBookHandler::addAddressees( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        setStatus("Writing new Contacts");

        KABC::VCardConverter vCardConv;
        QString vCard;
        RRA_Uint32Vector* added_ids = rra_uint32vector_new();

        for (KABC::Addressee::List::Iterator it = p_addresseeList.begin();
                it != p_addresseeList.end(); ++it ) {
            incrementSteps();

            kdDebug(2120) << "Adding Contact on Device: " << (*it).uid() << endl;

            vCard = vCardConv.createVCard ( ( *it ) );

            uint32_t newObjectId = m_rra->putVCard( vCard, mTypeId, 0 );
            m_rra->markIdUnchanged( mTypeId, newObjectId );

            mUidHelper->addId("SynCEAddressbook",
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                (*it).uid());

            kdDebug(2120) << "    ID-Pair: KDEID: " << (*it).uid() << " DeviceID: " <<
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ) << endl;

            rra_uint32vector_add(added_ids, newObjectId);

            KApplication::kApplication()->processEvents();
        }
        m_rra->registerAddedObjects(mTypeId, added_ids);

        rra_uint32vector_destroy(added_ids, true);
    }


    void AddressBookHandler::updateAddressees( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        setStatus("Writing changed Contacts");

        KABC::Addressee::List::Iterator it = p_addresseeList.begin();
        KABC::VCardConverter vCardConv;
        QString vCard;

        for ( ; it != p_addresseeList.end(); ++it ) {
            incrementSteps();

            QString kUid = mUidHelper->konnectorId("SynCEAddressbook", (*it).uid(), "---");

            if (kUid != "---") {
                kdDebug(2120) << "Updating Contact on Device: " << "ID-Pair: KDEID: " <<
                    (*it).uid() << " DeviceId: " << kUid << endl;
                vCard = vCardConv.createVCard ( ( *it ) );
                m_rra->putVCard ( vCard, mTypeId, getOriginalId( kUid ) );
                m_rra->markIdUnchanged( mTypeId, getOriginalId( kUid ) );
            }

            KApplication::kApplication()->processEvents();
        }
    }


    void AddressBookHandler::removeAddressees ( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        setStatus("Erasing deleted Contacts");

        RRA_Uint32Vector* deleted_ids = rra_uint32vector_new();
        KABC::Addressee::List::Iterator it = p_addresseeList.begin();

        for ( ; it != p_addresseeList.end(); ++it ) {
            incrementSteps();

            QString kUid = mUidHelper->konnectorId("SynCEAddressbook", (*it).uid(), "---");

            if (kUid != "---") {
                kdDebug(2120) << "Removing Contact on Device: " << "ID-Pair: KDEID: " <<
                    (*it).uid() << " DeviceId: " << kUid << endl;
                deleteSingleEntry ( mTypeId, getOriginalId( kUid ) );
                mUidHelper->removeId("SynCEAddressbook", kUid);
                rra_uint32vector_add(deleted_ids, getOriginalId( kUid ));
            }

            KApplication::kApplication()->processEvents();
        }

        m_rra->removeDeletedObjects(mTypeId, deleted_ids);

        rra_uint32vector_destroy(deleted_ids, true);
    }


    bool AddressBookHandler::writeSyncee(KSync::AddressBookSyncee *mAddressBookSyncee)
    {
        if ( mAddressBookSyncee->isValid() ) {
            KABC::Addressee::List addrAdded;
            KABC::Addressee::List addrRemoved;
            KABC::Addressee::List addrModified;

            setMaximumSteps(mAddressBookSyncee->added().count() + mAddressBookSyncee->removed().count() + mAddressBookSyncee->modified().count());
            resetSteps();
            getAddressees( addrAdded, mAddressBookSyncee->added() );
            getAddressees( addrRemoved, mAddressBookSyncee->removed() );
            getAddressees( addrModified, mAddressBookSyncee->modified() );

            addAddressees( addrAdded );
            removeAddressees( addrRemoved );
            updateAddressees( addrModified );
        }

        return true;
    }
}
