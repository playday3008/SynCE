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
#include <iostream>
#include <kdebug.h>
#include <kabc/vcardconverter.h>

#include <qapplication.h>

#include "matchmaker.h"


namespace pocketPCCommunication
{

    uint32_t AddressBookHandler::s_typeId = 0;

    AddressBookHandler::AddressBookHandler( const QString p_pdaName )
            : PimHandler( p_pdaName )
    {}


    AddressBookHandler::AddressBookHandler( KSharedPtr<Rra> p_rra )
            : PimHandler( p_rra )
    {}


    AddressBookHandler::~AddressBookHandler()
    {}


    bool AddressBookHandler::getAddressBook ( KABC::AddressBook& p_addressBook, RecordType p_recType )
    {
        //Rra rra(m_pdaName);
        m_rra->connect();
        //rra.connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            //rra.disconnect();
            return false;
        }

        struct Rra::ids ids;

        if ( !m_rra->getIds ( s_typeId, &ids ) )
            //if (!rra.getIds (s_typeId, &ids))
        {
            m_rra->disconnect();
            //rra.disconnect();
            kdDebug( 2120 ) << "AddressBookHandler::getAddressBook: could not get the ids.. :(" << endl;
            return false;
        }

        /*
        kdDebug(2120) << "now getting partnership-info: " << endl;
        uint32_t id;
        m_rra->getMatchMaker()->get_partner_id(1, &id);
        char* name;
        m_rra->getMatchMaker()->get_partner_name(1, &name); // or mabybe 2??
        kdDebug(2120) << "now getting partnership-info: " << "0x" << QString::number (id, 16) << "/" << name << endl;
        */ 
        // now we have all the necessary ids with their corresponding flag
        // now get the vCards for all the ids and put them into the KABC::AddressBook

        kdDebug( 2120 ) << "[AddressBookHandler]: got ids.. fetching information" << endl;

        switch ( p_recType ) {
        case ALL:
            getAddressees ( p_addressBook, ids, CHANGED );
            getAddressees ( p_addressBook, ids, DELETED );
            getAddressees ( p_addressBook, ids, UNCHANGED );
            break;
        case CHANGED:
            getAddressees ( p_addressBook, ids, CHANGED );
            break;
        case DELETED:
            getAddressees ( p_addressBook, ids, DELETED );
            break;
        case UNCHANGED:
            getAddressees ( p_addressBook, ids, UNCHANGED );
            break;
        }

        //rra.disconnect();
        m_rra->disconnect();
        return true;
    }


    bool AddressBookHandler::putAddressBook ( KABC::AddressBook& p_addressBook )
    {
        //Rra rra(m_pdaName);
        m_rra->connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            return false;
        }

        // iteratore over all addressees
        KABC::AddressBook::Iterator it = p_addressBook.begin();

        clearIdPairs();

        uint32_t objectId = 0;
        uint32_t newObjectId;
        bool ok;
        KABC::VCardConverter vCardConv;
        QString vCard;
        //bool doPush;
        /*
        uint32_t id;
        m_rra->getMatchMaker()->get_partner_id(1, &id);
        QString partnerId = QString::number (id, 16);
        */
        QString partnerId;
        partnerId = getPartnerId();

        for ( ; it != p_addressBook.end(); ++it ) {
            //doPush = false;
            kdDebug( 2120 ) << "UID: " << ( *it ).uid() << endl;
            QString curId = ( *it ).uid();
            objectId = 0;
            if ( ( *it ).uid().startsWith ( "RRA-ID-" ) ) {
                //doPush = (*it).changed();
                objectId = ( *it ).uid().remove( "RRA-ID-" ).toUInt( &ok, 16 );
                if ( !ok )
                    kdDebug( 2120 ) << "could not convert UID to uint32_t" << endl;
                else
                    kdDebug( 2120 ) << "RRA-ID: " << objectId << endl;
            }
            /*
            else
                doPush = true;
            */ 
            // now convert data to vCard and push it to the device
            vCard = vCardConv.createVCard ( ( *it ) );

            // only push the vCard if it is new (no RRA-ID-) or if it has changed (*it).changed()

            newObjectId = 0;
            newObjectId = m_rra->putVCard( vCard, s_typeId, objectId );
            if ( newObjectId ) {
                // the object was newly generated on the device!!
                // so.. what shall we do with this id?
                // ---> save it in the local addressBook!
                //(*it).setUid ("RRA-ID-" + QString::number(newObjectId, 16));

                //(*it).insertCustom (m_appName, m_keyName + "-" + partnerId, "RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'));

                if ( "RRA-ID-" + QString::number( newObjectId, 16 ).rightJustify( 8, '0' ) != curId ) {
                    kdDebug( 2120 ) << "getting new id: " << newObjectId << "    " << curId << endl;
                    addIdPair( "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ), curId );
                }
            }
        }

        m_rra->disconnect();
        return true;
    }


    bool AddressBookHandler::putAddressBook ( KABC::Addressee::List& p_addresseeList )
    {
        m_rra->connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            return false;
        }

        clearIdPairs();

        // iteratore over all addressees
        //KABC::AddressBook::Iterator it = p_addressBook.begin();
        KABC::Addressee::List::Iterator it = p_addresseeList.begin();

        uint32_t objectId = 0;
        uint32_t newObjectId;
        bool ok;
        KABC::VCardConverter vCardConv;
        QString vCard;
        /*
        uint32_t id;
        m_rra->getMatchMaker()->get_partner_id(1, &id);
        QString partnerId = QString::number (id, 16);
        */
        QString partnerId;
        partnerId = getPartnerId();

        for ( ; it != p_addresseeList.end(); ++it ) {
            kdDebug( 2120 ) << "UID: " << ( *it ).uid() << endl;
            QString curId = ( *it ).uid();
            objectId = 0;
            if ( ( *it ).uid().startsWith ( "RRA-ID-" ) ) {
                objectId = ( *it ).uid().remove( "RRA-ID-" ).toUInt( &ok, 16 );
                if ( !ok )
                    kdDebug( 2120 ) << "could not convert UID to uint32_t" << endl;
                else
                    kdDebug( 2120 ) << "RRA-ID: " << objectId << endl;
            }
            // now convert data to vCard and push it to the device
            vCard = vCardConv.createVCard ( ( *it ) );

            // only push the vCard if it is new (no RRA-ID-) or if it has changed (*it).changed()

            newObjectId = 0;
            newObjectId = m_rra->putVCard( vCard, s_typeId, objectId );
            if ( newObjectId ) {
                // the object was newly generated on the device!!
                // so.. what shall we do with this id?
                // ---> save it in the local addressBook!
                //(*it).setUid ("RRA-ID-" + QString::number(newObjectId, 16));

                //(*it).insertCustom (m_appName, m_keyName + "-" + partnerId, "RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'));

                if ( "RRA-ID-" + QString::number( newObjectId, 16 ).rightJustify( 8, '0' ) != curId )
                    addIdPair( "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ), curId );
            }
        }

        m_rra->disconnect();
        return true;

    }


    void AddressBookHandler::getAddressees ( KABC::AddressBook& p_addressBook, const struct Rra::ids& p_ids, RecordType p_recType )
    {
        QValueList<uint32_t>::const_iterator it;
        QValueList<uint32_t>::const_iterator end;
        QString vCard;

        KABC::VCardConverter vCardConv;
        KABC::Addressee addr;

        //emit progress(0);

        //unsigned int size;

        switch ( p_recType )
        {
        case CHANGED:
            it = p_ids.changedIds.begin();
            end = p_ids.changedIds.end();
            //size = p_ids.changedIds.size();
            break;
        case DELETED:
            it = p_ids.deletedIds.begin();
            end = p_ids.deletedIds.end();
            //size = p_ids.deletedIds.size();
            break;
        case UNCHANGED:
            it = p_ids.unchangedIds.begin();
            end = p_ids.unchangedIds.end();
            //size = p_ids.unchangedIds.size();
            break;
        case ALL:  // not reasonable to have ALL here! added to avoid warning of gcc
            break;
        }

        /*
        uint32_t id;
        m_rra->getMatchMaker()->get_partner_id(1, &id);
        QString partnerId = QString::number (id, 16);
        */
        QString partnerId;
        partnerId = getPartnerId();

        for ( ;it != end; ++it )
        {
            vCard = m_rra->getVCard( s_typeId, *it );
            addr = vCardConv.parseVCard ( vCard );
            /*
            kdDebug(2120) << "[AddressBookHandler]: addressee: " << addr.realName() << endl;
            kdDebug(2120) << "[AddressBookHandler]: addressee: " << addr.assembledName() << endl;
            kdDebug(2120) << "[AddressBookHandler]: vCard" << vCard << endl;
            */ 
            // manipulate addresse to get a real name!
            addr.setNameFromString ( addr.assembledName() );
            //addr.insertCustom (m_appName, m_keyName + "-" + partnerId, addr.uid());
            p_addressBook.insertAddressee ( addr );
            //emit progress (++prog * 100 / size);
            //qApp->processEvents();
        }
    }


    bool AddressBookHandler::getAddressees ( KABC::Addressee::List& p_addresseeList, const QStringList& p_ids )
    {
        if ( p_ids.begin() == p_ids.end() )
            return true;

        //Rra rra(m_pdaName);
        m_rra->connect();
        //rra.connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            //rra.disconnect();
            return false;
        }

        QString vCard;
        KABC::VCardConverter vCardConv;
        KABC::Addressee addr;

        // and now.. geht the data.. :)
        QStringList::const_iterator it = p_ids.begin();
        for ( ; it != p_ids.end(); ++it ) {
            kdDebug( 2120 ) << "getAddressees: " << *it << endl;
            QString id = *it;
            if ( isARraId ( id ) )  // this is not an RRA-ID!!
            {
                vCard = m_rra->getVCard( s_typeId, getOriginalId( ( id ) ) );
                addr = vCardConv.parseVCard ( vCard );
                addr.setNameFromString ( addr.assembledName() );
                kdDebug( 2120 ) << "getAddressees: " << addr.uid() << "   " << addr.realName() << endl;
                p_addresseeList.push_back ( addr );
            } else
                kdDebug( 2120 ) << "could not identify it as a RRA-ID!" << endl;
        }

        m_rra->disconnect();
        return true;
    }


    bool AddressBookHandler::getTypeId ()
    {
        if ( !s_typeId )
            s_typeId = m_rra->getTypeForName( RRA_SYNCMGR_TYPE_CONTACT );

        kdDebug( 2120 ) << "AddressBookHandler::s_typeId: " << s_typeId << endl;

        if ( !s_typeId )
            return false;
        else
            return true;
    }


    bool AddressBookHandler::getIdStatus ( QMap<QString, RecordType>& p_statusMap )
    {
        m_rra->connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            return false;
        }
        //m_rra->finalDisconnect();

        bool ok = getIdStatusPro( p_statusMap, s_typeId );

        m_rra->disconnect();

        return ok;
        //return getIdStatusPro(p_statusMap, s_typeId);
    }


    void AddressBookHandler::deleteAddressBook ()
    {
        kdDebug() << "AddressBookHandler::deleteAddressBook" << endl;
        m_rra->connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            return ;
        }

        struct pocketPCCommunication::Rra::ids ids;
        if ( !m_rra->getIds ( s_typeId, &ids ) ) {
            m_rra->disconnect();
            return ;
        }

        kdDebug() << "AddressBookHandler:: calling deleteEntries" << endl;

        deleteEntries( ids, s_typeId, CHANGED );
        deleteEntries( ids, s_typeId, UNCHANGED );
        deleteEntries( ids, s_typeId, DELETED );

        m_rra->disconnect();
    }


    void AddressBookHandler::deleteEntry ( const uint32_t& p_objectId )
    {
        kdDebug() << "AddressBookHandler::deleteEntry" << endl;
        if ( !getTypeId() ) {
            //m_rra->disconnect();
            return ;
        }
        deleteSingleEntry ( s_typeId, p_objectId );
    }


    void AddressBookHandler::addAddressees( KABC::Addressee::List& p_addresseeList )
    {
        kdDebug() << "AddressBookHandler:: addAddressees" << endl;
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        m_rra->connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            return ;
        }

        clearIdPairs();

        KABC::Addressee::List::Iterator it = p_addresseeList.begin();
        uint32_t newObjectId;
        KABC::VCardConverter vCardConv;
        QString vCard;

        for ( ; it != p_addresseeList.end(); ++it ) {
            //if (!isARraId ((*it).uid())) // this is not an RRA-ID!!
            {
                QString curId = ( *it ).uid();

                uint32_t remId = 0;
                if ( isARraId( curId ) )
                    remId = getOriginalId( curId );

                kdDebug() << "   isARraId: " << remId << endl;
                vCard = vCardConv.createVCard ( ( *it ) );

                newObjectId = 0;
                newObjectId = m_rra->putVCard( vCard, s_typeId, remId ); //getOriginalId(curId));
                if ( newObjectId )  // must be a new object id at this place!!!
                {
                    //(*it).insertCustom (m_appName, m_keyName + "-" + partnerId, "RRA-ID-" + QString::number(newObjectId, 16).rightJustify(8, '0'));
                    kdDebug() << "    adding new entry: " << "RRA-ID-" + QString::number( newObjectId, 16 ).rightJustify( 8, '0' ) << endl;
                    if ( "RRA-ID-" + QString::number( newObjectId, 16 ).rightJustify( 8, '0' ) != curId )
                        addIdPair( "RRA-ID-" + QString::number ( newObjectId, 16 ).rightJustify( 8, '0' ), curId );
                }
            }
        }

        m_rra->disconnect();
    }


    void AddressBookHandler::updateAddressees( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        m_rra->connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            return ;
        }

        KABC::Addressee::List::Iterator it = p_addresseeList.begin();
        KABC::VCardConverter vCardConv;
        QString vCard;

        for ( ; it != p_addresseeList.end(); ++it ) {
            if ( isARraId ( ( *it ).uid() ) ) {
                vCard = vCardConv.createVCard ( ( *it ) );

                m_rra->putVCard ( vCard, s_typeId, getOriginalId( ( *it ).uid() ) );
            }
        }

        m_rra->disconnect();
    }


    void AddressBookHandler::removeAddressees ( KABC::Addressee::List& p_addresseeList )
    {
        if ( p_addresseeList.begin() == p_addresseeList.end() )
            return ;

        m_rra->connect();

        if ( !getTypeId() ) {
            m_rra->disconnect();
            return ;
        }

        KABC::Addressee::List::Iterator it = p_addresseeList.begin();

        for ( ; it != p_addresseeList.end(); ++it ) {
            if ( isARraId ( ( *it ).uid() ) ) {
                deleteSingleEntry ( s_typeId, getOriginalId( ( *it ).uid() ) );
            }
        }

        m_rra->disconnect();
    }

};
