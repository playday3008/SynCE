/***************************************************************************
* Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
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

#include "kdeaddressbooksync.h"

#include <kdebug.h>
#include <kstandarddirs.h>
#include <kdeversion.h>

#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>
#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>
#include <kabc/resourcefile.h>
#include <kabc/vcardformatplugin.h>
#include <qptrlist.h>
#include <ksimpleconfig.h>
#include <qfile.h>

#include <rra.h>


KdeAddressBookSync::KdeAddressBookSync()
{}


KdeAddressBookSync::~KdeAddressBookSync()
{}


void KdeAddressBookSync::printBook( QString label, KABC::AddressBook *book )
{
    kdDebug( 2120 ) << "==============" + label + "===============" << endl;

    KABC::AddressBook::Iterator it;

    for ( it = book->begin(); it != book->end(); it++ ) {
        kdDebug( 2120 ) << ( *it ).formattedName() << " " << ( *it ).uid() << " "
        << ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) )
        << " >" << ( *it ).custom( "Raki", "Task" ) + "<" << endl;
    }

    kdDebug( 2120 ) << "--------------" + label + "---------------" << endl;
}


bool KdeAddressBookSync::compare( const KABC::Addressee &a, const KABC::Addressee &b )
{
    if ( a.uid() != b.uid() ) {
        kdDebug( 2120 ) << "Differ:uid: " << a.uid() << " " << b.uid() << endl;
        return false;
    }
    if ( a.name() != b.name() ) {
        kdDebug( 2120 ) << "Differ:name: " << a.name() << " " << b.name() << endl;
        return false;
    }
    if ( a.formattedName() != b.formattedName() ) {
        kdDebug( 2120 ) << "Differ:formattedName: " << a.formattedName() << " " << b.formattedName() << endl;
        return false;
    }
    if ( a.familyName() != b.familyName() ) {
        kdDebug( 2120 ) << "Differ:familyName: " << a.familyName() << " " << b.familyName() << endl;
        return false;
    }
    if ( a.givenName() != b.givenName() ) {
        kdDebug( 2120 ) << "Differ:givenName: " << a.givenName() << " " << b.givenName() << endl;
        return false;
    }
    if ( a.additionalName() != b.additionalName() ) {
        kdDebug( 2120 ) << "Differ:additionalName: " << a.additionalName() << " " << b.additionalName() << endl;
        return false;
    }
    if ( a.prefix() != b.prefix() ) {
        kdDebug( 2120 ) << "Differ:prefix: " << a.prefix() << " " << b.prefix() << endl;
        return false;
    }
    if ( a.suffix() != b.suffix() ) {
        kdDebug( 2120 ) << "Differ:suffix: " << a.suffix() << " " << b.suffix() << endl;
        return false;
    }
    if ( a.nickName() != b.nickName() ) {
        kdDebug( 2120 ) << "Differ:nickName: " << a.nickName() << " " << b.nickName() << endl;
        return false;
    }
    if ( a.birthday() != b.birthday() ) {
        kdDebug( 2120 ) << "Differ:birthday: " << a.birthday().toString() << " " << b.birthday().toString() << endl;
        return false;
    }
    if ( a.mailer() != b.mailer() ) {
        kdDebug( 2120 ) << "Differ:mailer: " << a.mailer() << " " << b.mailer() << endl;
        return false;
    }
    if ( a.timeZone() != b.timeZone() ) {
        kdDebug( 2120 ) << "Differ:timeZone: " << a.timeZone().asString() << " " << b.timeZone().asString() << endl;
        return false;
    }
    if ( a.geo() != b.geo() ) {
        kdDebug( 2120 ) << "Differ:geo: " << a.geo().asString() << " " << b.geo().asString() << endl;
        return false;
    }
    if ( a.title() != b.title() ) {
        kdDebug( 2120 ) << "Differ:title: " << a.title() << " " << b.title() << endl;
        return false;
    }
    if ( a.role() != b.role() ) {
        kdDebug( 2120 ) << "Differ:role: " << a.role() << " " << b.role() << endl;
        return false;
    }
    if ( a.organization() != b.organization() ) {
        kdDebug( 2120 ) << "Differ:organization: " << a.organization() << " " << b.organization() << endl;
        return false;
    }
    if ( a.note() != b.note() ) {
        kdDebug( 2120 ) << "Differ:note: " << a.note() << " " << b.note() << endl;
        return false;
    }
    if ( a.productId() != b.productId() ) {
        kdDebug( 2120 ) << "Differ:productId: " << a.productId() << " " << b.productId() << endl;
        return false;
    }
    if ( a.sortString() != b.sortString() ) {
        kdDebug( 2120 ) << "Differ:sortString: " << a.sortString() << " " << b.sortString() << endl;
        return false;
    }
    if ( a.secrecy() != b.secrecy() ) {
        kdDebug( 2120 ) << "Differ:secrecy: " << a.secrecy().asString() << " " << b.secrecy().asString() << endl;
        return false;
    }
    if ( a.logo() != b.logo() ) {
        kdDebug( 2120 ) << "Differ:logo: " << endl;
        return false;
    }
    if ( a.photo() != b.photo() ) {
        kdDebug( 2120 ) << "Differ:photo: " << endl;
        return false;
    }
    if ( a.sound() != b.sound() ) {
        kdDebug( 2120 ) << "Differ:sound: " << endl;
        return false;
    }
    if ( ( a.url().isValid() || b.url().isValid() ) &&
            ( a.url() != b.url() ) ) {
        kdDebug( 2120 ) << "Differ:url: " << endl;
        return false;
    }
    if ( a.phoneNumbers() != b.phoneNumbers() ) {
        kdDebug( 2120 ) << "Differ:phoneNumbers: " << endl;
        return false;
    }
    if ( a.addresses() != b.addresses() ) {
        kdDebug( 2120 ) << "Differ:addresses: " << endl;
        return false;
    }
    if ( a.keys() != b.keys() ) {
        kdDebug( 2120 ) << "Differ:keys: " << endl;
        return false;
    }
    if ( a.emails() != b.emails() ) {
        kdDebug( 2120 ) << "Differ:emails: " << endl;
        return false;
    }
    if ( a.categories() != b.categories() ) {
        kdDebug( 2120 ) << "Differ:categories: " << endl;
        return false;
    }

    return true;
}


void KdeAddressBookSync::generatePcDelta()
{
    KABC::AddressBook::Iterator it;
    
    KABC::Addressee rhoAddressee;
    
    KABC::Addressee stdAddressee;

    for ( it = pcStdBook->begin(); it != pcStdBook->end(); it++ ) {
        rhoAddressee = pcRhoBook->findByUid ( ( *it ).uid() );
        stdAddressee = *it;
        stdAddressee.setResource(NULL);

        if ( rhoAddressee.isEmpty() ) {
            stdAddressee.insertCustom( "Raki", "Task", "new" );
            incTotalSteps( 1 );
        } else {
            rhoAddressee.insertCustom( "Raki", "Found", "true" );
            pcRhoBook->insertAddressee( rhoAddressee );
            if ( compare( stdAddressee, rhoAddressee ) ) {
                stdAddressee.insertCustom( "Raki", "Task", "unchanged" );
            } else {
                stdAddressee.insertCustom( "Raki", "Task", "changed" );
                incTotalSteps( 1 );
            }
        }

        pcDeltaBook->insertAddressee( stdAddressee );
    }

    for ( it = pcRhoBook->begin(); it != pcRhoBook->end(); it++ ) {
        if ( ( *it ).custom( "Raki", "Found" ).isEmpty() ) {
            ( *it ).insertCustom( "Raki", "Task", "deleted" );
            KABC::Addressee addr = *it;
            addr.setResource(NULL);
            pcDeltaBook->insertAddressee( addr );
            incTotalSteps( 1 );
        }
    }

    printBook( "PCDeltaBook", pcDeltaBook );
}


void KdeAddressBookSync::generatePdaDelta()
{
    QString vCard;
    uint32_t v;
    QString uidString;
    KABC::VCardConverter vCardConverter;
    KABC::Addressee addressee;
    
    if ( rra->connect() ) {
        struct Rra::ids ids;

        rra->getIds( getObjectTypeId(), &ids );   // errorcheck

        incTotalSteps( ids.changedIds.count() + ids.deletedIds.count() );

        QValueList<uint32_t>::iterator it;
        for ( it = ids.changedIds.begin(); ( it != ids.changedIds.end() ) && !stopRequested(); ++it ) {
            v = *it;
            addressee = rra->getAddressee( getObjectTypeId(), v );
            addressee.insertCustom( "Raki", "Task", "changed" );
            uidString = "RRA-ID-" + ( QString( "00000000" ) + QString::number( v, 16 ) ).right( 8 );
            addressee.setUid( uidString );
            addressee.insertCustom( "Raki", "PdaUid-" + QString::number( partnerId ), addressee.uid() );
            pdaDeltaBook->insertAddressee( addressee );
            advanceProgress();
        }
        for ( it = ids.unchangedIds.begin(); ( it != ids.unchangedIds.end() ) && !stopRequested(); ++it ) {
            v = *it;
            addressee = KABC::Addressee();
            if ( firstSynchronize ) {
                addressee = rra->getAddressee( getObjectTypeId(), v );
                addressee.insertCustom( "Raki", "Task", "changed" );
            } else {
                addressee.insertCustom( "Raki", "Task", "unchanged" );
            }
            uidString = "RRA-ID-" + ( QString( "00000000" ) + QString::number( v, 16 ) ).right( 8 );
            addressee.setUid( uidString );
            addressee.insertCustom( "Raki", "PdaUid-" + QString::number( partnerId ), addressee.uid() );
            pdaDeltaBook->insertAddressee( addressee );
        }
        for ( it = ids.deletedIds.begin(); ( it != ids.deletedIds.end() ) && !stopRequested(); ++it ) {
            v = *it;
            addressee = KABC::Addressee();
            addressee.insertCustom( "Raki", "Task", "deleted" );
            uidString = "RRA-ID-" + ( QString( "00000000" ) + QString::number( v, 16 ) ).right( 8 );
            addressee.setUid( uidString );
            addressee.insertCustom( "Raki", "PdaUid-" + QString::number( partnerId ), addressee.uid() );
            pdaDeltaBook->insertAddressee( addressee );
            advanceProgress();
        }
        rra->disconnect();
    }
    
    printBook( "PDADeltaBook", pdaDeltaBook );
}


void KdeAddressBookSync::generateKappaBook()
{
    KABC::AddressBook::Iterator it;
    KABC::Addressee kappaAddressee;
    KABC::Addressee pdaDeltaAddressee;
    QString pcTask;
    QString pdaTask;

    for ( it = pcDeltaBook->begin(); it != pcDeltaBook->end(); it++ ) {
        QString pdaUid = ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) );
        if ( !pdaUid.isEmpty() ) {
            pdaDeltaAddressee = pdaDeltaBook->findByUid( pdaUid );
            if ( !pdaDeltaAddressee.isEmpty() ) {
                if ( pdaDeltaAddressee.custom( "Raki", "Task" ) == "changed" ) {
                    if ( ( *it ).custom( "Raki", "Task" ) == "changed" ) {
                        kappaAddressee = ( *it );
                        kappaAddressee.setResource(NULL);
                        kappaAddressee.insertCustom( "Raki", "PdaUid-" + QString::number( partnerId ), pdaUid );
                        kappaBook->insertAddressee( kappaAddressee );
                    }
                }
                pdaDeltaBook->removeAddressee( pdaDeltaAddressee );

                QString task = pdaDeltaAddressee.custom( "Raki", "Task" );
                pdaDeltaAddressee.setCustoms( ( *it ).customs() );
                pdaDeltaAddressee.insertCustom( "Raki", "Task", task );
                pdaDeltaAddressee.insertCustom( "Raki", "PdaUid-" + QString::number( partnerId ), pdaUid );
                pdaDeltaAddressee.setUid( ( *it ).uid() );
                pdaDeltaBook->insertAddressee( pdaDeltaAddressee );
            }
        }
    }

    printBook( "KappaBook", kappaBook );
}


void KdeAddressBookSync::generatePdaItaBook()
{
    KABC::AddressBook::Iterator it;
    KABC::Addressee pdaAddressee;

    for ( it = kappaBook->begin(); it != kappaBook->end(); it++ ) {
        pdaAddressee = ( *it );
        pdaAddressee.setResource(NULL);
        QString pdaUid = ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) );
        pdaAddressee.setUid( pdaUid );
        pdaDeltaBook->insertAddressee( pdaAddressee );
        pdaDeltaBook->removeAddressee( pdaAddressee );
        decTotalSteps( 1 );
    }

    printBook( "PdaItaBook", pdaDeltaBook );
}


void KdeAddressBookSync::generatePcItaBook()
{
    KABC::AddressBook::Iterator it;
    KABC::Addressee addressee;
    
    for ( it = kappaBook->begin(); it != kappaBook->end(); it++ ) {
        addressee = (*it);
        addressee.setResource(NULL);
        pcDeltaBook->insertAddressee( addressee );
        pcDeltaBook->removeAddressee( addressee );
        decTotalSteps( 1 );
    }

    printBook( "PcItaBook", pcDeltaBook );
}


bool KdeAddressBookSync::preSync(SyncThread */*syncThread*/, Rra */*rra*/, bool /*firstSynchronize*/, 
        uint32_t partnerId)
{
    pcStdBook = KABC::StdAddressBook::self();
    pcRhoBook = new KABC::AddressBook();
    pcDeltaBook = new KABC::AddressBook();
    pdaDeltaBook = new KABC::AddressBook();
    kappaBook = new KABC::AddressBook();

#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0)
    rhoResource = new KABC::ResourceFile(pcRhoBook,
            QFile::encodeName(locateLocal("data", (QString("raki/syncrefbook-") + QString::number(partnerId) + QString(".vcf")).ascii())));
#else
    rhoResource = new KABC::ResourceFile(locateLocal("data", QString("raki/syncrefbook-") + 
            QString::number(partnerId) + QString(".vcf")));
    KABC::Resource *pcDeltaResource = new KABC::ResourceFile(locateLocal("data", QString("raki/pcdelta-") + 
            QString::number(partnerId) + QString(".vcf")));
    pcDeltaBook->addResource(pcDeltaResource);
    pcDeltaBook->load();
    pcDeltaBook->clear();
    KABC::Resource *pdaDeltaResource = new KABC::ResourceFile(locateLocal("data", QString("raki/pdadelta-") + 
            QString::number(partnerId) + QString(".vcf")));
    pdaDeltaBook->addResource(pdaDeltaResource);
    pdaDeltaBook->load();
    pdaDeltaBook->clear();
    KABC::Resource *kappaResource = new KABC::ResourceFile(locateLocal("data", QString("raki/kappa-") + 
            QString::number(partnerId) + QString(".vcf")));
    kappaBook->addResource(kappaResource);
    kappaBook->load();
    kappaBook->clear();
#endif
    pcRhoBook->addResource(rhoResource);
    pcRhoBook->load();

    printBook( "PcRhoBook", pcRhoBook );
    return true;
}


bool KdeAddressBookSync::postSync(SyncThread */*syncThread*/, Rra */*rra*/, bool /*firstSynchronize*/, 
        uint32_t /*partnerId*/)
{
    KABC::StdAddressBook::save();
    KABC::StdAddressBook::close();
    
    KABC::Ticket *t = pcRhoBook->requestSaveTicket( rhoResource );
    pcRhoBook->save( t );

    pcDeltaBook->clear();
    pdaDeltaBook->clear();
    kappaBook->clear();
    pcRhoBook->clear();

    delete pcDeltaBook;
    delete pdaDeltaBook;
    delete kappaBook;
    delete pcRhoBook;
    
    return true;
}


bool KdeAddressBookSync::sync()
{
//    QString vCard;
    QString task;
    QString ceUid;
    QString uidString;
    uint32_t newObjectId;
    uint32_t uid;
    bool ok;

    rra->connect();
    setTask( "Search for changes in KDE" );
    generatePcDelta();          // Search with PCUid
    
    setTask( "Search for changes on PDA" );
    generatePdaDelta();         // Download book

    setTask( "Merging KDE and PDA" );
    generateKappaBook();        // Search with CEUid

    setTask( "Filtering for KDE" );
    generatePdaItaBook();       // Search with PCUid
    KABC::AddressBook *pdaItaBook = pdaDeltaBook;

    setTask( "Filtering for PDA" );
    generatePcItaBook();        // Search with CEUid
    KABC::AddressBook *pcItaBook = pcDeltaBook;

    KABC::AddressBook::Iterator it;

    kdDebug( 2120 ) << "+++++++++++ Updateing StdBook ++++++++++++++" << endl;

    setTask( "Writing KDE" );
    for ( it = pdaItaBook->begin(); it != pdaItaBook->end() && !stopRequested(); it++ ) {
        task = ( *it ).custom( "Raki", "Task" );
        if ( task == "changed" ) {
            kdDebug( 2120 ) << "Update of uid: " << ( *it ).uid() << "  " << ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ) << endl;
            kdDebug(2120) << "---- " << (*it).realName() << endl;
            KABC::Addressee a = *it;
            a.setResource(NULL);
            pcStdBook->insertAddressee( a );
            uid = ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ).right( 8 ).toULong( &ok, 16 );
            rra->resetAddressee( getObjectTypeId(), uid );
        } else if ( task == "new" ) {
            kdDebug( 2120 ) << "New Address " << ( *it ).uid() << "  " << ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ) << endl;
            KABC::Addressee a = *it;
            a.setResource(NULL);
            pcStdBook->insertAddressee( a );
            uid = ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ).right( 8 ).toULong( &ok, 16 );
            rra->resetAddressee( getObjectTypeId(), uid );
        } else if ( task == "deleted" ) {
            kdDebug( 2120 ) << "Deleting of " << ( *it ).uid() << "  " << ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ) << endl;
            KABC::Addressee a = *it;
            a.setResource(NULL);
            pcStdBook->insertAddressee( a );
            pcStdBook->removeAddressee( a );
        } else if ( task == "unchanged" ) {
            // ??
        }
    }

    kdDebug( 2120 ) << "++++++++++++ Updateing PdaBook +++++++++++++" << endl;

    setTask( "Writing PDA" );
    for ( it = pcItaBook->begin(); it != pcItaBook->end() && !stopRequested(); it++ ) {
        task = ( *it ).custom( "Raki", "Task" );
        if ( task == "changed" ) {
            kdDebug( 2120 ) << "Update of uid: " << ( *it ).uid() << "  " << ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ) << endl;
            ceUid = ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) );
            ( *it ).setUid( ceUid );
            uid = ceUid.right( 8 ).toULong( &ok, 16 );
            rra->putAddressee( *it, getObjectTypeId(), uid, &newObjectId );
            if ( newObjectId != uid ) {
                uidString = "RRA-ID-" + ( QString( "00000000" ) + QString::number( newObjectId, 16 ) ).right( 8 );
                ( *it ).insertCustom( "Raki", "PdaUid-" + QString::number( partnerId ), uidString );
                KABC::Addressee addr = *it;
                addr.setResource(NULL);
                pcStdBook->insertAddressee( addr );
            }
            advanceProgress();
        } else if ( task == "new" ) {
            rra->putAddressee( *it, getObjectTypeId(), 0, &newObjectId );
            uidString = "RRA-ID-" + ( QString( "00000000" ) + QString::number( newObjectId, 16 ) ).right( 8 );
            ( *it ).insertCustom( "Raki", "PdaUid-" + QString::number( partnerId ), uidString );
            KABC::Addressee addr = *it;
            addr.setResource(NULL);
            pcStdBook->insertAddressee( addr );
            kdDebug( 2120 ) << "New Address " << ( *it ).uid() << "  " << ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ) << endl;
            advanceProgress();
        }
    }

    if ( !stopRequested() && pcItaBook->begin() != pcItaBook->end() ) {
        if ( rra->connect() ) {
            for ( it = pcItaBook->begin(); it != pcItaBook->end() && !stopRequested(); it++ ) {
                task = ( *it ).custom( "Raki", "Task" );
                if ( task == "deleted" ) {
                    ceUid = ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) );
                    uid = ceUid.right( 8 ).toULong( &ok, 16 );
                    rra->deleteObject( getObjectTypeId(), uid );
                    advanceProgress();
                    kdDebug( 2120 ) << "Deleting of " << ( *it ).uid() << "  " << ( *it ).custom( "Raki", "PdaUid-" + QString::number( partnerId ) ) << endl;
                } else if ( task == "unchanged" ) {
                    // ??
                }
            }
            rra->disconnect();
        }
    }

    rra->disconnect();
    // Todo here: Resolving konflicts in kappa list

    pcRhoBook->clear();
//    pcRhoBook->addResource(rhoResource);
    for ( it = pcStdBook->begin(); it != pcStdBook->end(); it++ ) {
        ( *it ).removeCustom( "Raki", "Task" );
        KABC::Addressee rhoAddressee = ( *it );
        rhoAddressee.setResource(rhoResource);
        pcRhoBook->insertAddressee( rhoAddressee );
    }

    printBook( "PcRhoBook", pcRhoBook );

    return true;
}
