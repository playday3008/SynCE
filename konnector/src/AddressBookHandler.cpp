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
#include <kabc/vcardconverter.h>


namespace pocketPCCommunication
{
    AddressBookHandler::AddressBookHandler( KSharedPtr<Rra> p_rra, QString mBaseDir, KSync::KonnectorUIDHelper *mUidHelper )
            : PimHandler( p_rra )
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
    {
        mUidHelper->save();
    }


    int AddressBookHandler::retrieveAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList )
    {
        int count = 0;
        KABC::VCardConverter vCardConv;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;
            kdDebug( 2120 ) << " ||| " << endl;
            QString vCard = m_rra->getVCard( mTypeId, *it );

            m_rra->markIdUnchanged( mTypeId, *it );

            kdDebug( 2120 ) << vCard << endl;
            KABC::Addressee addr = vCardConv.parseVCard ( vCard );
            addr.setFormattedName(addr.formattedName().replace("\\,", ","));

            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCEAddressbook", addr.uid(), "---")) != "---") {
                addr.setUid(kdeId);
            } else {
                mUidHelper->addId("SynCEAddressbook", addr.uid(), addr.uid());
            }

            mAddresseeList.push_back( addr );
        }

        return count;
    }


    int AddressBookHandler::fakeAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, QValueList<uint32_t> &idList )
    {
        int count = 0;

        for ( QValueList<uint32_t>::const_iterator it = idList.begin(); it != idList.end(); ++it ) {
            count++;
            kdDebug( 2120 ) << " &&& " << endl;
            KABC::Addressee addr;

            QString konId = "RRA-ID-" + QString::number( *it, 16 ).rightJustify( 8, '0' );
            QString kdeId;

            if ((kdeId = mUidHelper->kdeId("SynCEAddressbook", konId, "---")) != "---") {
                addr.setUid(kdeId);
                mUidHelper->removeId("SynCEAddressbook", addr.uid());
            }

            mAddresseeList.push_back( addr );
        }

        return count;
    }


    bool AddressBookHandler::getIds()
    {
        if ( !m_rra->getIds( mTypeId, &ids ) ) {
            kdDebug( 2120 ) << "AddressBookHandler::getIds: could not get the ids.. :(" << endl;
            return false;
        }

        return true;
    }


    int AddressBookHandler::getAddresseeListFromDevice( KABC::Addressee::List &mAddresseeList, int mRecType )
    {
        kdDebug( 2120 ) << "[AddressBookHandler]: got ids.. fetching information" << endl;

        QValueList<uint32_t>::const_iterator begin;
        QValueList<uint32_t>::const_iterator end;

        int count = 0;
        int ret = 0;

        if ( ( mRecType & CHANGED ) && ( ret >= 0 ) ) {
            ret = retrieveAddresseeListFromDevice( mAddresseeList, ids.changedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & DELETED ) && ( ret >= 0 ) ) {
            ret = retrieveAddresseeListFromDevice( mAddresseeList, ids.deletedIds );
            if ( ret >= 0 ) {
                count += 0;
            }
        }

        if ( ( mRecType & UNCHANGED ) && ( ret >= 0 ) ) {
            ret = fakeAddresseeListFromDevice( mAddresseeList, ids.unchangedIds );
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
        kdDebug(2120) << "Begin Inserting into AddressBookSyncee State: " << state << endl;
        for(KABC::Addressee::List::Iterator it = list.begin(); it != list.end(); ++it) {
            KSync::AddressBookSyncEntry entry(*it, mAddressBookSyncee);
            entry.setState(state);
            mAddressBookSyncee->addEntry(entry.clone());
        }
        kdDebug(2120) << "End Inserting into AddressBookSyncee" << endl;
    }


    bool AddressBookHandler::readSyncee(KSync::AddressBookSyncee *mAddressBookSyncee, bool firstSync)
    {
        if (!initialized) {
            if (!init()) {
                kdDebug(2120) << "Could not initialize AddressBookHandler" << endl;
//              emit synceeReadError(this);
                return false;
            }
        }

        if (!getIds()) {
            kdDebug(2120) << "Could not retriev Address-IDs" << endl;
//            emit synceeReadError(this);
            return false;
        }

        mAddressBookSyncee->reset();

        KABC::Addressee::List modifiedList;
        if (firstSync) {
            if (getAddresseeListFromDevice(modifiedList, pocketPCCommunication::UNCHANGED | pocketPCCommunication::CHANGED) < 0) {
//                emit synceeReadError(this);
                return false;
            }
        } else {
            if (getAddresseeListFromDevice(modifiedList, pocketPCCommunication::CHANGED) < 0) {
//                emit synceeReadError(this);
                return false;
            }

            KABC::Addressee::List removedList;
            if (getAddresseeListFromDevice(removedList, pocketPCCommunication::DELETED) < 0) {
//                emit synceeReadError(this);
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
        kdDebug( 2120 ) << "getAddressees: " << endl;
        KSync::SyncEntry::PtrList::Iterator it = p_ptrList.begin();
        for ( ; it != p_ptrList.end(); ++it ) {
            p_addressees.push_back ( ( dynamic_cast<KSync::AddressBookSyncEntry*>( *it ) ) ->addressee() );
            kdDebug( 2120 ) << "     " << ( dynamic_cast<KSync::AddressBookSyncEntry*>( *it ) ) ->id() << endl;
        }
    }


    void AddressBookHandler::addAddressees( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        KABC::VCardConverter vCardConv;
        QString vCard;

        for (KABC::Addressee::List::Iterator it = p_addresseeList.begin();
                it != p_addresseeList.end(); ++it ) {

            vCard = vCardConv.createVCard ( ( *it ) );

            uint32_t newObjectId = m_rra->putVCard( vCard, mTypeId, 0 );

            mUidHelper->addId("SynCEAddressbook",
                "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ),
                (*it).uid());
        }
    }


    void AddressBookHandler::updateAddressees( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        KABC::Addressee::List::Iterator it = p_addresseeList.begin();
        KABC::VCardConverter vCardConv;
        QString vCard;

        for ( ; it != p_addresseeList.end(); ++it ) {
            QString kUid = mUidHelper->konnectorId("SynCEAddressbook", (*it).uid(), "---");

            if (kUid != "---") {
                vCard = vCardConv.createVCard ( ( *it ) );
                m_rra->putVCard ( vCard, mTypeId, getOriginalId( kUid ) );
            }
        }
    }


    void AddressBookHandler::removeAddressees ( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        KABC::Addressee::List::Iterator it = p_addresseeList.begin();

        for ( ; it != p_addresseeList.end(); ++it ) {
            QString kUid = mUidHelper->konnectorId("SynCEAddressbook", (*it).uid(), "---");

            if (kUid != "---") {
                deleteSingleEntry ( mTypeId, getOriginalId( kUid ) );
                mUidHelper->removeId("SynCEAddressbook", kUid);
            }
        }
    }


    bool AddressBookHandler::writeSyncee(KSync::AddressBookSyncee *mAddressBookSyncee)
    {
        if (!initialized) {
            if (!init()) {
                kdDebug(2120) << "Could not initialize AddressBookHandler" << endl;
//              emit synceeReadError(this);
                return false;
            }
        }

        if ( mAddressBookSyncee->isValid() ) {
            KABC::Addressee::List addrAdded;
            KABC::Addressee::List addrRemoved;
            KABC::Addressee::List addrModified;

            getAddressees( addrAdded, mAddressBookSyncee->added() );
            getAddressees( addrRemoved, mAddressBookSyncee->removed() );
            getAddressees( addrModified, mAddressBookSyncee->modified() );

            addAddressees( addrAdded );
            removeAddressees( addrRemoved );
            updateAddressees( addrModified );
        }

        return true;
    }


    bool AddressBookHandler::connectDevice()
    {
        if ( !m_rra->connect()) {
            kdDebug( 2120 ) << "PocketPCKonnector: could not connect to device!" << endl;
            return false;
        } else {
            kdDebug( 2120 ) << "PocketPCKonnector: connected to device!" << endl;;
        }

        return true;
    }


    /** Disconnect the device.
     * @see KSync::Konnector::disconnectDevice()
     * @return true if device can be disconnect. false otherwise
     */
    bool AddressBookHandler::disconnectDevice()
    {
        m_rra->disconnect();

        mUidHelper->save();

        return true;
    }
};
