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

#include "PocketPCKonnector.h"
#include "PocketPCKonnectorConfig.h"

#include "todohandler.h"
#include "eventhandler.h"
#include "AddressBookHandler.h"

#include <qdir.h>

#include <kitchensync/addressbooksyncee.h>
#include <kitchensync/calendarsyncee.h>
#include <kitchensync/konnectorinfo.h>


class PocketPCKonnectorFactory : public KRES::PluginFactoryBase
{
public:
    KRES::Resource* resource ( const KConfig* p_config )
    {
        return new KSync::PocketPCKonnector( p_config );
    }

    KRES::ConfigWidget* configWidget ( QWidget* p_parent )
    {
        return new pocketPCHelper::PocketPCKonnectorConfig( p_parent, "PocketPCKonnectorConfig" );
    }
};


extern "C"
{
    void* init_libsyncekonnector() {
        return new PocketPCKonnectorFactory();
    }
}


namespace KSync
{
    PocketPCKonnector::PocketPCKonnector( const KConfig* p_config )
            : KSync::Konnector( p_config ), firstSync(false)
    {
        if ( p_config ) {
            m_pdaName = p_config->readEntry( "PDAName" );
            m_rra = new pocketPCCommunication::Rra( m_pdaName );
            m_rra->setLogLevel( 0 );
            mBaseDir = storagePath();

            QDir dir;
            QString dirName = mBaseDir + m_pdaName; // check that dir does not exist!!!!!!!
            if ( !dir.exists( dirName ) ) {
                kdDebug(2120) << "Its the first sync ..." << endl;
                // do something about firstSync!
                firstSync = true;
                dir.mkdir ( dirName );
            } else {
                firstSync = false;
            }
            mUidHelper = new KSync::KonnectorUIDHelper(mBaseDir + "/" + m_pdaName);

            mAddrHandler = new pocketPCCommunication::AddressBookHandler( m_rra, mBaseDir, mUidHelper);
            mTodoHandler = new pocketPCCommunication::TodoHandler(m_rra, mBaseDir, mUidHelper);
            mEventHandler = new pocketPCCommunication::EventHandler(m_rra, mBaseDir, mUidHelper);

            mAddressBookSyncee = new AddressBookSyncee();
            mAddressBookSyncee->setTitle("SynCE");

            mCalendarSyncee = new CalendarSyncee( &mCalendar );
            mCalendarSyncee->setTitle("SynCE");

            mSyncees.append(mCalendarSyncee);
            mSyncees.append(mAddressBookSyncee);
        } else {
            m_rra = NULL;
            mUidHelper = NULL;
            mAddrHandler = NULL;
            mTodoHandler = NULL;
            mEventHandler = NULL;
            mAddressBookSyncee = NULL;
            mCalendarSyncee = NULL;
        }
    }


    PocketPCKonnector::~PocketPCKonnector()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::~PocketPCKonnector" << endl;

        if (mAddressBookSyncee) {
            delete mAddressBookSyncee;
        }
        if (mCalendarSyncee) {
            delete mCalendarSyncee;
        }
        if (mAddrHandler) {
            delete mAddrHandler;
        }
        if (mTodoHandler) {
            delete mTodoHandler;
        }
        if (mEventHandler) {
            delete mEventHandler;
        }
        if (mUidHelper) {
            delete mUidHelper;
        }

        if (m_rra != NULL) {
            if ( m_rra.data() ) {
                kdDebug( 2120 ) << "PocketPCKonnector::~PocketPCKonnector: before m_rra->finalDisconnect()" << endl;
                m_rra->finalDisconnect();
            }
            kdDebug( 2120 ) << "PocketPCKonnector::~PocketPCKonnector: m_rra.count(): " << m_rra.count() << endl;
        }
    }


    SynceeList PocketPCKonnector::syncees()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::syncees() returning syncees" << endl;
        return mSyncees;
    }

    bool PocketPCKonnector::readSyncees()
    {
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "PocketPCKonnector::readSyncees: m_syncees is empty" << endl;
            emit synceeReadError( this );
            return false;
        }

        clearDataStructures();

        if (mAddrHandler) {
            if ( !mAddrHandler->connectDevice() ) {
                emit synceeReadError(this);
                return false;
            }
            if (!mAddrHandler->readSyncee(mAddressBookSyncee, firstSync)) {
                emit synceeReadError(this);
                return false;
            }
            mAddrHandler->disconnectDevice();
        }

        if (mTodoHandler) {
            if ( !mTodoHandler->connectDevice() ) {
                emit synceeReadError(this);
                return false;
            }
            if (!mTodoHandler->readSyncee(mCalendarSyncee, firstSync)) {
                emit synceeReadError(this);
                return false;
            }
            mTodoHandler->disconnectDevice();
        }

        if (mEventHandler) {
            if ( !mEventHandler->connectDevice() ) {
                emit synceeReadError(this);
                return false;
            }
            if (!mEventHandler->readSyncee(mCalendarSyncee, firstSync)) {
                emit synceeReadError(this);
                return false;
            }
            mEventHandler->disconnectDevice();
        }

        emit synceesRead ( this );

        return true;
    }


    bool PocketPCKonnector::writeSyncees()
    {
        kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees..." << endl;

        // write m_syncees to the device
        if ( mSyncees.empty() ) {
            kdDebug( 2120 ) << "PocketPCKonnector::writeSyncees: m_syncees is empty" << endl;
            emit synceeWriteError( this );
            return false;
        }

        QString partnerId;

        if (mAddrHandler) {
            mAddrHandler->connectDevice();
            mAddrHandler->writeSyncee(mAddressBookSyncee);
            mAddrHandler->disconnectDevice();
        }

        if (mTodoHandler) {
            mTodoHandler->connectDevice();
            mTodoHandler->writeSyncee(mCalendarSyncee);
            mTodoHandler->disconnectDevice();
        }

        if(mEventHandler) {
            mEventHandler->connectDevice();
            mEventHandler->writeSyncee(mCalendarSyncee);
            mEventHandler->disconnectDevice();
        }

        clearDataStructures();

        firstSync = false;

        emit synceesWritten ( this );

        return true;
    }


    bool PocketPCKonnector::connectDevice()
    {
        if (m_rra) {
            m_rra->initRapi();
        }

        return true;
    }


    bool PocketPCKonnector::disconnectDevice()
    {
        if (m_rra) {
            m_rra->uninitRapi();
        }

        return true;
    }


    KonnectorInfo PocketPCKonnector::info() const
    {
        if (m_rra) {
            if ( !m_rra.data() ) {
                return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                    QIconSet(),  //iconSet(),
                                    QString ( "WinCE 3.0 up" ),
                                    /*                              "0", //metaId(),
                                                                    "", //iconName(),*/
                                    false );
            } else {
                return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                    QIconSet(),  //iconSet(),
                                    QString ( "WinCE 3.0 up" ),
                                    /*                              "0", //metaId(),
                                                                    "", //iconName(),*/
                                    m_rra->isConnected() );
            }
        } else {
            return KonnectorInfo ( QString ( "PocketPC (WinCE) Konnector" ),
                                   QIconSet(),  //iconSet(),
                                   QString ( "WinCE 3.0 up" ),
                                   /*                              "0", //metaId(),
                                                                 "", //iconName(),*/
                                   m_rra->isConnected() );
        }

    }


    void PocketPCKonnector::writeConfig( KConfig* p_config )
    {
        Konnector::writeConfig ( p_config );

        p_config->writeEntry ( "PDAName", m_pdaName );
    }


    void PocketPCKonnector::setPdaName ( const QString& p_pdaName )
    {
        m_pdaName = p_pdaName;
    }


    const QString PocketPCKonnector::getPdaName () const
    {
        return m_pdaName;
    }


    void PocketPCKonnector::clearDataStructures()
    {
        if (mCalendarSyncee) {
            mCalendarSyncee->reset();
        }

        if (mAddressBookSyncee) {
            mAddressBookSyncee->reset();
        }

        mCalendar.deleteAllEvents();
        mCalendar.deleteAllTodos();
        mCalendar.deleteAllJournals();
    }
}
