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

#include "pda.h"
#include "pdaconfigdialogimpl.h"
#include "syncdialogimpl.h"
#include "runwindowimpl.h"
#include "managerimpl.h"
#include "passworddialogimpl.h"
#include "raki.h"
#include "rra.h"
#include "matchmaker.h"
#include "rapiwrapper.h"
#include "removepartnershipdialogimpl.h"
#include "initprogress.h"

#include <qcheckbox.h>
#include <qdatetime.h>
#include <kiconloader.h>
#include <klocale.h>
#include <krun.h>
#include <kapplication.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

PDA::PDA( Raki *raki, QString pdaName )
        : QObject()
{
    int menuCount = 0;
    _masqueradeStarted = false;
    partnerOk = false;
    typesRead = false;
    this->raki = raki;
    this->pdaName = pdaName;
    this->rra = new Rra( pdaName );

    runWindow = new RunWindowImpl( pdaName, raki, "RunWindow", false );
    runWindow->setCaption( i18n( "Execute on %1" ).arg( pdaName ) );

    managerWindow = new ManagerImpl( pdaName, raki, "ManagerWindow", false );
    managerWindow->setCaption( i18n( "Info and Management for %1" ).arg( pdaName ) );

    passwordDialog = new PasswordDialogImpl( pdaName, raki,
                     "PasswordWindow", true );
    connect( passwordDialog, SIGNAL( password( QString ) ), this,
             SLOT( setPassword( QString ) ) );
    passwordDialog->setCaption( i18n( "Password for %1" ).arg( pdaName ) );

    configDialog = new PdaConfigDialogImpl( pdaName, raki, "ConfigDialog",
                                            false );
    configDialog->setCaption( i18n( "Configuration for %1" ).arg( pdaName ) );

    syncDialog = new SyncDialogImpl( rra, pdaName, raki, "SynchronizeDialog",
                                     true );
    syncDialog->setCaption( i18n( "Synchronize %1" ).arg( pdaName ) );

    associatedMenu = new KPopupMenu( raki, "PDA-Menu" );

    associatedMenu->clear();
    associatedMenu->setCaption( pdaName );

    associatedMenu->insertTitle( SmallIcon( "rapip" ), pdaName );
    menuCount++;

    syncItem = associatedMenu->insertItem( SmallIcon( "connect_established" ),
                                           i18n( "&Synchronize" ), this, SLOT( synchronize() ) );
    associatedMenu->setItemEnabled( syncItem, false );
    menuCount++;

    associatedMenu->insertItem( SmallIcon( "rotate_cw" ),
                                i18n( "&Info && Management..." ), this, SLOT( manage() ) );
    menuCount++;

    associatedMenu->insertSeparator( menuCount );
    menuCount++;

    associatedMenu->insertItem( SmallIcon( "folder" ), i18n( "&Open %1" ).arg( "rapip://" + QString( pdaName ) + "/" ), this, SLOT( openFs() ) );
    menuCount++;

    pdaMirrorItem = associatedMenu->insertItem( SmallIcon( "pda_blue" ),
                    i18n( "Run &KCeMirror" ), this, SLOT( startPdaMirror() ) );
    menuCount++;

    associatedMenu->insertItem( SmallIcon( "run" ), i18n( "&Execute..." ), this,
                                SLOT( execute() ) );
    menuCount++;

    associatedMenu->insertSeparator( menuCount );
    menuCount++;

    associatedMenu->insertItem( SmallIcon( "configure" ), i18n( "Configure %1" ).arg( QString( pdaName ) ), this, SLOT( configurePda() ) );
    menuCount++;

    associatedMenu->insertTearOffHandle( -1, -1 );
    menuCount++;

    connect( syncDialog, SIGNAL( finished() ), configDialog, SLOT( writeConfig() ) );
    connect( &pdaMirror, SIGNAL( processExited( KProcess* ) ), this, SLOT( pdaMirrorExited( KProcess* ) ) );
}


PDA::~PDA()
{
    kdDebug( 2120 ) << i18n( "Deleting PDA" ) << endl;

    if ( syncDialog->running() ) {
        syncDialog->setDelayedDelete( true );
        syncDialog->setStopRequested( true );
    } else {
        delete syncDialog;
    }

    delete passwordDialog;
    delete runWindow;
    if ( managerWindow->running() ) {
        managerWindow->setDelayedDelete( true );
        managerWindow->WorkerThreadInterface::setStopRequested( true );
    } else {
        delete managerWindow;
    }
    delete configDialog;
    delete associatedMenu;
    delete rra;

    slaveDict.setAutoDelete( true );
}


void PDA::pdaMirrorExited( KProcess* )
{
    associatedMenu->setItemEnabled( pdaMirrorItem, true );
}


void PDA::startPdaMirror()
{
    pdaMirror.clearArguments();

    pdaMirror << "kcemirror" << pdaName;

    associatedMenu->setItemEnabled( pdaMirrorItem, false );

    pdaMirror.start( KProcess::NotifyOnExit, ( KProcess::Communication )
                     ( KProcess::Stdout | KProcess::Stderr ) );
}


bool PDA::running()
{
    return ( WorkerThreadInterface::running() || managerWindow->running() ||
             syncDialog->running() );
}


void PDA::setStopRequested( bool isStopRequested )
{
    if ( managerWindow->running() ) {
        managerWindow->setStopRequested( isStopRequested );
    } else if ( syncDialog->running() ) {
        syncDialog->setStopRequested( isStopRequested );
    } else if ( WorkerThreadInterface::running() ) {
        WorkerThreadInterface::setStopRequested( isStopRequested );
    }
}


bool PDA::getSynchronizationTypes( QMap<int, RRA_SyncMgrType *> *types )
{
    return rra->getTypes( types );
}


void PDA::setMenuIndex( int menuIndex )
{
    this->menuIndex = menuIndex;
}


int PDA::getMenuIndex()
{
    return menuIndex;
}


const char *PDA::getName()
{
    return pdaName.ascii();
}


KPopupMenu *PDA::getMenu()
{
    return associatedMenu;
}


void PDA::execute()
{
    runWindow->show();
}


void PDA::manage()
{
    managerWindow->show();
}


void PDA::openFs()
{
    KRun::runURL( "rapip://" + QString( pdaName ) + "/",
                  QString::fromLatin1( "inode/directory" ) );
}


void PDA::synchronize( bool forced )
{
    if ( ( forced || configDialog->getSyncAtConnect() ) && isPartner() ) {
        QPtrList<SyncTaskListItem>& syncItems =
            configDialog->getSyncTaskItemList();
        syncDialog->show( syncItems );
    }
}


void PDA::configurePda()
{
    configDialog->show();
}


void PDA::requestPassword()
{
    if ( configDialog->getPassword().isEmpty() ) {
        passwordDialog->exec();
    } else {
        emit resolvedPassword( pdaName, configDialog->getPassword() );
    }
}


void PDA::setPassword( QString password )
{
    if ( passwordDialog->rememberPasswordCheck->isChecked() ) {
        configDialog->passwordEdit->setText( password );
        configDialog->applySlot();
        configDialog->writeConfig();
    }
    emit resolvedPassword( pdaName, password );
}


void PDA::passwordInvalid()
{
    configDialog->passwordEdit->setText( "" );
    configDialog->applySlot();
    configDialog->writeConfig();
}


bool PDA::isMasqueradeEnabled()
{
    return configDialog->getMasqueradeEnabled();
}


void PDA::registerCopyJob( KIO::CopyJob *copyJob )
{
    slaveDict.insert( copyJob, new KURL::List::List() );
}


void PDA::addURLByCopyJob( KIO::CopyJob *copyJob, KURL& url )
{
    KURL::List * list = slaveDict.find( copyJob );
    KURL::List::Iterator it;

    it = list->find( url );

    if ( it == list->end() ) {
        list->append( url );
    }
}


void PDA::unregisterCopyJob( KIO::CopyJob *copyJob )
{
    delete slaveDict.take( copyJob );
}


KURL::List PDA::getURLListByCopyJob( KIO::CopyJob *copyJob )
{
    KURL::List * list = slaveDict.find( copyJob );

    return *list;
}


unsigned int PDA::getNumberOfCopyJobs()
{
    return slaveDict.count();
}


void *PDA::advanceProgressEvent( void *data )
{
    int * advance = ( int * ) data;

    if ( *advance == 1 ) {
        progressBar->advance( *advance );
    } else {
        progressBar->setProgress( *advance );
    }
    delete advance;
    return NULL;
}


void *PDA::advanceTotalStepsEvent( void *data )
{
    int * advance = ( int * ) data;

    progressBar->setTotalSteps( progressBar->totalSteps() + *advance );
    delete advance;
    return NULL;
}


QString PDA::getDeviceIp()
{
    return configDialog->getDeviceIp();
}


#define advanceInitProgress(a) { \
    int *pA = new int; \
    *pA = a; \
    postThreadEvent(&PDA::advanceProgressEvent, pA, noBlock); \
}

#define advanceInitTotalSteps(a) { \
    int *pA = new int; \
    *pA = a; \
    postThreadEvent(&PDA::advanceTotalStepsEvent, pA, noBlock); \
}


void *PDA::removePartnershipDialog( void *data )
{
    struct MatchMaker::Partner * partner = ( struct MatchMaker::Partner * ) data;
    int removedPartners;

    initProgress->hide();

    removedPartners = RemovePartnershipDialogImpl::showDialog(
                          QString( partner[ 0 ].name ), QString( partner[ 1 ].name ) , 0,
                          "Remove Partnership", true, 0 );

    initProgress->show();

    return ( void * ) removedPartners;
}


void *PDA::alreadyTwoPartnershipsDialog( void * )
{
    initProgress->hide();
    KMessageBox::error( ( QWidget * ) parent(),
                        i18n( "There are already two partnerships configured on the "
                              "device - using guest" ), i18n( "Error configuring partnership" ) );
    initProgress->show();
    return NULL;
}


void *PDA::removeLocalPartnershipDialog( void * /*data*/ )
{
    int answer = KMessageBox::questionYesNo( ( QWidget * ) parent(),
                 i18n( "A Partnership with name %1 already exists "
                       "on the desktop side. Should this partnership be replaced?" ).arg( pdaName ) );

    if ( answer == KMessageBox::No ) {
        return ( void * ) 0;
    }

    return ( void * ) 1;
}


bool PDA::isPartner()
{
    return partnerOk;
}


void *PDA::progressDialogCancel( void *init )
{
    initProgress->hide();
    emit initialized( this, ( int ) init );
    delete initProgress;
    if ( init ) {
        configDialog->writeConfig();
    }

    return NULL;
}


void *PDA::rraConnectionError( void * )
{
    KMessageBox::error( 0, i18n( "Could not create a RRA-connection" ) );

    return NULL;
}


bool PDA::removePartnership( MatchMaker *matchmaker, int *removedPartnerships )
{
    struct MatchMaker::Partner partners[ 2 ];
    bool removePartnershipOk = true;

    advanceInitProgress( 1 );
    if ( matchmaker->getPartner( 1, &partners[ 0 ] ) ) {
        advanceInitProgress( 1 );
        if ( matchmaker->getPartner( 2, &partners[ 1 ] ) ) {
            int deletedItems = ( int ) postThreadEvent(
                                   &PDA::removePartnershipDialog, partners, block );
            if ( deletedItems > 0 ) {
                if ( deletedItems != 0 ) {
                    struct MatchMaker::Partner deletedPartner;
                    if ( deletedItems & 1 ) {
                        deletedPartner.name = "";
                        deletedPartner.id = 0;
                        deletedPartner.index = 1;
                        advanceInitProgress( 1 );
                        if ( !matchmaker->setPartner( deletedPartner ) ) {
                            removePartnershipOk = false;
                        }
                    }
                    if ( ( deletedItems & 2 ) && removePartnershipOk ) {
                        deletedPartner.name = "";
                        deletedPartner.id = 0;
                        deletedPartner.index = 2;
                        advanceInitProgress( 1 );
                        if ( !matchmaker->setPartner( deletedPartner ) ) {
                            removePartnershipOk = false;
                        }
                    }
                }
                *removedPartnerships = deletedItems;
            }
        } else {
            removePartnershipOk = false;
        }
    } else {
        removePartnershipOk = false;
    }

    return removePartnershipOk;
}


void PDA::setPartnership( QThread */*thread*/, void * )
{
    struct MatchMaker::Partner partners[ 2 ];
    struct MatchMaker::Partner partner;
    MatchMaker matchmaker( pdaName );
    uint32_t matchingPartnerId = 0;
    bool onDesktopKnown = false;
    bool removeOnDesktop = false;
    bool twoOnDevice = false;
    bool error = false;
    bool useGuest = false;
    int deleteOnDevice = 0;

    kdDebug(2120) << i18n("Connecting to the device ...") << endl;
    if ( matchmaker.connect() ) {
// 1    
        advanceInitProgress( 1 );
        
        kdDebug(2120) << i18n("success") << endl;
        kdDebug( 2120 ) << i18n( "Reading from device ...." ) << endl;
        for ( int i = 0; ( i < 2 ) && ( matchingPartnerId == 0 ); i++ ) {
            kdDebug( 2120 ) << i18n( "    get partnership with index %1 ..." ).arg( i + 1 ) << endl;
            if ( matchmaker.getPartner( i + 1, &partners[ i ] ) ) {
                kdDebug( 2120 ) << i18n( "    success" ) << endl;
                if ( partners[ i ].name == configDialog->getPartnerName() && 
                            partners[ i ].id == configDialog->getPartnerId() ) {
                    kdDebug( 2120 ) << i18n( "        matching!" ) << endl;
                    matchingPartnerId = i + 1;
                    partner = partners[ i ];
                }
            } else {
                kdDebug( 2120 ) << i18n( "    failed" ) << endl;
            }
// 3            
            advanceInitProgress( 1 );
        
        }
        kdDebug( 2120 ) << i18n( "finished" ) << endl;

        if ( matchingPartnerId > 0 ) {
            kdDebug( 2120 ) << i18n( "Matching partnership found ..." ) << endl;
            kdDebug( 2120 ) << i18n( "    Trying to make it valid on the Device ... " ) << endl;
            if ( matchmaker.setCurrentPartner( matchingPartnerId ) ) {
                kdDebug( 2120 ) << i18n( "        success!" ) << endl;
            } else {
                error = true;
                kdDebug( 2120 ) << i18n( "        failed!" );
            }
// 4            
            advanceInitProgress( 1 );
            
        } else {
            if ( configDialog->getPartnerId() != 0 ) {
                kdDebug( 2120 ) <<
                i18n( "Partnership with name %1 already known on the desktop" ).arg( configDialog->getPartnerName() ) << endl;
                onDesktopKnown = true;
            }

            if ( onDesktopKnown ) {
                kdDebug( 2120 ) << i18n( "    We should ask user if partnership on the desktop should be deleted" ) << endl;
                removeOnDesktop = ( ( ( int ) postThreadEvent( &PDA::removeLocalPartnershipDialog, NULL, block ) ) > 0 );
            }
// 5
            advanceInitProgress( 1 );

            if ( ( onDesktopKnown && removeOnDesktop ) || !onDesktopKnown ) {

                kdDebug( 2120 ) << i18n( "* Desktop is ready or will be ready for partnership" ) << endl;

                if ( ( partners[ 0 ].id != 0 ) && ( partners[ 1 ].id != 0 ) ) {
                    kdDebug( 2120 ) << i18n( "Already two partnerships existing on the Device" ) << endl;
                    twoOnDevice = true;
                }

                if ( twoOnDevice ) {
                    //                deleteOnDevice = ?-> two on device - what to do?
                    kdDebug( 2120 ) << i18n( "    We should ask user if partnership on the device should be deleted" ) << endl;
                    deleteOnDevice = ( int ) postThreadEvent( &PDA::removePartnershipDialog, partners, block );
                }
// 6
                advanceInitProgress( 1 );
                
                if ( ( twoOnDevice && ( deleteOnDevice > 0 ) ) || !twoOnDevice ) {

                    kdDebug( 2120 ) << i18n( "* Device is ready or will be ready for partnership" ) << endl;

                    if ( deleteOnDevice > 0 ) {
                        //                    -> delete on device
                        kdDebug( 2120 ) << i18n( "We should delete partnerships with mask %1 on the device" ).arg( deleteOnDevice ) << endl;
                        struct MatchMaker::Partner deletePartner;
                        deletePartner.name = "";
                        deletePartner.id = 0;

                        if ( deleteOnDevice & 1 ) {
                            kdDebug( 2120 ) << i18n( "    Deleting partnership 1 on the device" ) << endl;
                            deletePartner.index = 1;
                            if ( !matchmaker.setPartner( deletePartner ) ) {
                                error = true;
                            }
                        }
// 7
                        advanceInitProgress( 1 );
                        
                        if ( ( deleteOnDevice & 2 ) && !error ) {
                            kdDebug( 2120 ) << i18n( "    Deleting partnership 2 on the device" ) << endl;
                            deletePartner.index = 2;
                            if ( !matchmaker.setPartner( deletePartner ) ) {
                                error = true;
                            }
                        }
// 8
                        advanceInitProgress( 1 );
                    }

                    if ( removeOnDesktop && !error ) {
                        //                    -> remove on desctop
                        kdDebug( 2120 ) << i18n( "We delete partnership on the desktop" ) << endl;
                        configDialog->clearConfig();
                        delete configDialog;
                        configDialog = new PdaConfigDialogImpl( pdaName, raki, "ConfigDialog", false );
                        configDialog->setCaption( i18n( "Configuration of %1" ).arg( pdaName ) );
                        connect( syncDialog, SIGNAL( finished() ), configDialog, SLOT( writeConfig() ) );
                    }
// 9
                    advanceInitProgress( 1 );

                    kdDebug( 2120 ) << i18n( "Now we are ready to create the partnership ... " ) << endl;

                    if ( matchmaker.partnerCreate( &matchingPartnerId ) && !error ) {
// 10a
                        advanceInitProgress( 1 );
                        
                        kdDebug( 2120 ) << i18n( "    success on the device ... now we set it on the desktop side" ) << endl;
                        if ( matchmaker.getPartner( matchingPartnerId, &partner ) ) {
                            kdDebug( 2120 ) << i18n( "        success" ) << endl;
                            configDialog->setPartner( partner.name, partner.id );
                        } else {
                            kdDebug( 2120 ) << i18n( "        failed" ) << endl;
                            error = true;
                        }
// 11
                        advanceInitProgress( 1 ); 
                    } else {
                        kdDebug( 2120 ) << i18n( "    failed on the device" ) << endl;
                        error = true;
// 10b
                        advanceInitProgress( 1 );
                    }
                } else {
                    kdDebug( 2120 ) << i18n( "* Device is _not_ ready for partnership" ) << endl;
                    useGuest = true;
                }
            } else {
                kdDebug( 2120 ) << i18n( "* Desktop is _not_ ready for partnership" ) << endl;
                useGuest = true;
            }
        }
        matchmaker.disconnect();
// 12
        advanceInitProgress( 11 );
                        
    } else {
        kdDebug(2120) << i18n("failed") << endl;
        error = true;
    }

    if (error) {
        postThreadEvent( &PDA::progressDialogCancel, 0, noBlock );
    } else {
        if (useGuest) {
            kdDebug( 2120 ) << "Using Guest" << endl;
            delete configDialog;
            configDialog = new PdaConfigDialogImpl( "Guest", raki, "ConfigDialog", false );
            configDialog->setCaption( i18n( "Configuration of %1" ).arg( "Guest" ) );
            connect( syncDialog, SIGNAL( finished() ), configDialog, SLOT( writeConfig() ) );
            partnerOk = false;
            partnerName = "Guest";
            partnerId = 0;
        } else {
            postThreadEvent( &PDA::synchronizationTasks, 0, noBlock );
            associatedMenu->setItemEnabled( syncItem, true );
            partnerOk = true;
            partnerName = partner.name;
            partnerId = partner.id;
        }
        postThreadEvent( &PDA::progressDialogCancel, 1, noBlock );
    }
}

/*
void PDA::setPartnership( QThread * thread, void * )
{
    bool setPartnerOk = false;

    MatchMaker matchmaker( pdaName );

    if ( matchmaker.connect() ) {
        setPartnerOk = setPartnershipThread( &matchmaker );
    }

    // Proposal for a correct connection-initiation
    // ConfigDialog has been created already in the constructor
    // so both, if existing, the local partnerId and both remote partnerIds are known
    //
    // 1) If localPartnerId is equal zero do a createNewPartnership() - end!
    //
    // 2) If localPartnerId matches one of the remotePartnerIds set the index of the matching
    //    remote partnerId as the current partnership on the device and switch call
    //    configDialog->>setPartner() - end!
    //
    // 3) Ask user to delete local partnership? If so,
    //    clearPartnership() and than createNewPartnership()
    //
    //    createNewPartnership():
    // 1) If both remoteIds are not equal zero ask for removal one or both remotePartnerships
    //    1a) If removed create a new Partnership
    //    1b) If no remote Partnership was removed dont create a new Partnership

    if ( setPartnerOk ) {
        if ( partnerId != 0 ) {
            if ( partnerId != configDialog->getPartnerId() ) {
                if ( configDialog->getPartnerId() != 0 ) {
                    // partner with name already known ... remove it from desktop?
                    // if removed create new configuration for device with getPartnerId() == 0
                    kdDebug( 2120 ) << i18n( "Partnership already known but id not valid" ) << endl;
                    kdDebug( 2120 ) << i18n( "Should ask user if partnership should be deleted" ) << endl;
                    int answer = ( int ) postThreadEvent( &PDA::removeLocalPartnershipDialog, NULL, block );
                    if ( answer == 1 ) {
                        configDialog->clearConfig();
                        delete configDialog;
                        configDialog = new PdaConfigDialogImpl( pdaName, raki, "ConfigDialog", false );
                        configDialog->setCaption( i18n( "Configuration of %1" ).arg( pdaName ) );
                        connect( syncDialog, SIGNAL( finished() ), configDialog, SLOT( writeConfig() ) );
                    }
                }
                if ( configDialog->getPartnerId() == 0 ) {
                    // new partner
                    // create partnership
                    kdDebug( 2120 ) << i18n( "New Partnership created" ) << endl;
                    configDialog->setNewPartner( partnerName, partnerId );
                } else {
                    partnerId = 0;
                    configDialog->setPartner( i18n( "Guest" ), 0 );
                }
            } else {
                // partnership already known
                kdDebug( 2120 ) << i18n( "Partnership known" ) << endl;
                configDialog->setPartner( partnerName, partnerId );
            }
        } else {
            kdDebug( 2120 ) << i18n( "Using guest partnership" ) << endl;
            configDialog->setPartner( i18n( "Guest" ), 0 );
            // guest partnership
        }

        if ( partnerOk ) {
            // synchronizationTasks
            postThreadEvent( &PDA::synchronizationTasks, 0, noBlock );
        }

        // progressDialogCancel, 1
        postThreadEvent( &PDA::progressDialogCancel, 1, noBlock );
    } else {
        postThreadEvent( &PDA::progressDialogCancel, 0, noBlock );
        // progressDialogCancel, 0
    }
}
*/

void PDA::init()
{
    kdDebug( 2120 ) << i18n( "PDA-init" ) << endl;

    initProgress = new InitProgress( raki, "InitProgress", true,
                                     WStyle_Customize | WStyle_NoBorder | WStyle_Tool | WX11BypassWM );

    progressBar = initProgress->progressBar;
    progressBar->setTotalSteps( 11 ); /* 7 */
    initProgress->pdaName->setText( pdaName );
    initProgress->show();

    kapp->processEvents();

    startWorkerThread( this, &PDA::setPartnership, NULL );

    kdDebug(2120) << "Ende PDA::init()" << endl;
}


bool PDA::synchronizationTasks( void * )
{
    RRA_SyncMgrType * objectType;
    QDateTime lastSynchronized;
    bool ret = true;

    if ( !typesRead ) {
        typesRead = true;
        QMap<int, RRA_SyncMgrType *> types;
        if ( getSynchronizationTypes( &types ) ) {
            QMapIterator<int, RRA_SyncMgrType *> it;
            for ( it = types.begin() ; it != types.end(); ++it ) {
                objectType = *it;
                configDialog->addSyncTask( objectType, partnerId );
            }
        } else {
            ret = false;
        }
    }

    kdDebug( 2120 ) << i18n( "End PDA::syncronizationTasks()" ) << endl;

    return ret;
}


void PDA::setMasqueradeStarted()
{
    _masqueradeStarted = true;
}


bool PDA::masqueradeStarted()
{
    return _masqueradeStarted;
}
