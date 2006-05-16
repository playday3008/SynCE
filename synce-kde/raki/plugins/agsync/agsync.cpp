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

#include "agsync.h"

#ifdef WITH_AGSYNC

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include <AGTypes.h>
#include <AGReader.h>
#include <AGWriter.h>
#include <AGBufferReader.h>
#include <AGNet.h>
#include <AGRecord.h>
#include <AGUserConfig.h>
#include <AGDBConfig.h>
#include <AGServerConfig.h>
#include <AGDeviceInfo.h>
#include <AGCommandProcessor.h>
#include <AGClientProcessor.h>
#include <AGLocationConfig.h>

#include "rapiwrapper.h"
#include "agsyncconfigimpl.h"
#include "avantgoclientinstallationdialogimpl.h"
#include "syncstream.h"
#include "getpassword.h"
#include <qcstring.h>
#include <qstringlist.h>
#include <qlineedit.h>
#include <qlabel.h>

static RakiSyncPlugin *plugin;

static int32 readFunc( void *s, void *data, int32 len )
{
    HRESULT hr = IRAPIStream_Read( ( synce::IRAPIStream* ) s, data, len, NULL );
    if ( SUCCEEDED( hr ) )
        return len;
    else
        return 0;
}


static int32 writeFunc( void *s, void *data, int32 len )
{
    HRESULT hr = IRAPIStream_Write( ( synce::IRAPIStream* ) s, data, len, NULL );
    if ( SUCCEEDED( hr ) )
        return len;
    else
        return 0;
}


int32 taskPrinter( void * /*blah*/, int * /*errBlah*/, char *str, AGBool /*thing*/ )
{
    plugin->setTask( str);
    return 1; // AGCLIENT_CONTINUE
}


int32 itemPrinter( void * /*blah*/, int * /*errBlah*/, int items, int total, char * /*name*/ )
{
    plugin->setTotalSteps( total );
    plugin->setProgress( items);

    return 1; // AGClIENT_CONTINUE
}


AGRecord *pRecord = NULL;

typedef struct _PStoreStruct
{
    AGReader *r;
    AGWriter *w;
    AGCommandProcessor *cmdProc;
}
PStoreStruct;

PStoreStruct pStore;

int32 pNextModifiedRecord( void *pStoreVoid, AGRecord **record, int32 *errCode )
{
    int res;
    if ( NULL != pRecord )
        AGRecordFree( pRecord );

    res = asGetNextModifiedRecord( ( ( PStoreStruct * ) pStoreVoid ) ->r,
                                   ( ( PStoreStruct * ) pStoreVoid ) ->w,
                                   record );
    if ( 2 == res )
        * errCode = asErrno;

    return res;
}


int32 pNextRecord( void *pStoreVoid, AGRecord **record, int32 *errCode )
{
    int res;
    if ( NULL != pRecord )
        AGRecordFree( pRecord );

    res = asGetNextRecord( ( ( PStoreStruct * ) pStoreVoid ) ->r,
                           ( ( PStoreStruct * ) pStoreVoid ) ->w,
                           record );
    if ( 2 == res )
        * errCode = asErrno;

    return res;
}


int32 pOpenDatabase( void *pStoreVoid, AGDBConfig *theDatabase, int32 *errCode )
{
    int res;
    res = asOpenDatabase( ( ( PStoreStruct * ) pStoreVoid ) ->r,
                          ( ( PStoreStruct * ) pStoreVoid ) ->w,
                          theDatabase );
    if ( 2 == res )
        * errCode = asErrno;

    return res;
}


int32 pNextExpansionCommand( void * /*pStoreVoid*/, int32 * /*cmd*/,
                             int32 * /*cmdLen*/, void ** /*cmdData*/ )
{
    // I suspect that the regular case does not use this.
    return 0;
}


int32 pPerformCommand( void *pStoreVoid, int32 *err, AGReader *r )
{
    int result;
    unsigned char *data;
    int dataLen, cmd;
    AGBufferReader *rB = ( AGBufferReader * ) r;

    AGPerformCommandFunc cmdFunc = AGCommandProcessorGetPerformFunc(
                                       ( ( PStoreStruct * ) pStoreVoid ) ->cmdProc );

    result = ( *cmdFunc ) ( ( ( PStoreStruct * ) pStoreVoid ) ->cmdProc, err, r );

    /* I know it's a BufferReader, and when was the last time C++
       allowed be to h4x0r object internals? Heh. Lets enjoy it! */
    rB->currentIndex = 0;
    cmd = AGReadCompactInt( r );
    dataLen = AGReadCompactInt( r );
    data = ( unsigned char * ) ( rB->buffer + rB->currentIndex );

    // All commands except goodbye should return 0
    if ( 1 != result && 0 != cmd )
        kdDebug( 2120 ) << i18n( "ERROR ON COMMAND" ) << " " << cmd << endl;

    asPerformCommand( ( ( PStoreStruct * ) pStoreVoid ) ->r,
                      ( ( PStoreStruct * ) pStoreVoid ) ->w,
                      cmd, data, dataLen );
    return result;
}


AGSync::AGSync()
{
    plugin = this;
}


AGSync::~AGSync()
{
    delete configDialog;
}


void AGSync::createConfigureObject( KConfig *ksConfig )
{
    configDialog = new AGSyncConfigImpl( ksConfig, parent );
}


void AGSync::configure()
{
    configDialog->show();
}


AGPlatformCalls pCalls = {
                             ( void * ) & pStore,
                             pNextModifiedRecord,
                             pNextRecord,
                             pOpenDatabase,
                             pNextExpansionCommand,
                             ( void * ) & pStore,
                             pPerformCommand
                         };

void AGSync::doServerSync( AGReader *r, AGWriter *w, AGServerConfig *s, AGNetCtx *ctx )
{
    int result;
    AGCommandProcessor *cmdProc;

    if ( s->disabled ) {
        return ;
    }

    /* Initialize the command processor */
    cmdProc = AGCommandProcessorNew( s );
    pStore.cmdProc = cmdProc;
    /* Note: If we wanted status messages, we could hook the
             command processor here to handle them.
    */
    cmdProc->commands.performTaskFunc = taskPrinter;
    cmdProc->commands.performItemFunc = itemPrinter;

    /* Start server block */
    if ( 0 != asStartServer( r, w, s->uid ) ) {
        kdDebug( 2120 ) << i18n( "AvantGo error on asStartServer:" ) << " " << asErrno << "!" << endl;
        return ;
    }

    /* A loop over server connections, since we may have to sync
       more than once. This normally doesn't happen, but I think is
       used occasionally to initialize a device which has no config yet.
    */
    do {
        AGDeviceInfo *devInfo;
        AGClientProcessor *clientProc;

        kdDebug( 2120 ) << i18n( "Beginning synchonization attempt on server." ) << endl;

        devInfo = AGDeviceInfoNew();

        if ( NULL == asGetDeviceInfo( r, w, devInfo ) ) {
            kdDebug( 2120 ) << i18n( "Failed to retrieve device information!" ) << endl;
            goto devEnd;
        }

        AGCommandProcessorStart( cmdProc ); /* Error code unused */

        /* Set up client processor...
           TRUE is for buffering of cmds. */
        clientProc = AGClientProcessorNew( s, devInfo, locConfig, &pCalls,
                                           TRUE, ctx );

        /* Apparently we dont?? */
        AGClientProcessorSetBufferServerCommands( clientProc, FALSE );


        /* Basically a connect request? */
        AGClientProcessorSync( clientProc );


        /* Perform the synchronization */
        do {
            result = AGClientProcessorProcess( clientProc );
        } while ( AGCLIENT_CONTINUE == result && !stopRequested() );

        AGClientProcessorFree( clientProc );

    devEnd:
        AGDeviceInfoFree( devInfo );

    } while ( AGCommandProcessorShouldSyncAgain( cmdProc ) && !stopRequested() );

    AGCommandProcessorFree( cmdProc );


    if ( 0 != asEndServer( r, w ) ) {
        kdDebug( 2120 ) << i18n( "Avantgo error on asEndServer:" ) << " " << asErrno << "!" << endl;
    }
}


bool AGSync::preSync(bool /*firstSynchronize*/, uint32_t /*partnerId*/ )
{
    bool ret = true;

    if ( configDialog->installClient() ) {
        AvantGoClientInstallationDialogImpl agcid;

        if ( agcid.exec() == QDialog::Accepted ) {

            QString url = agcid.agceClientPath();

            if ( KIO::NetAccess::exists( url, true, NULL ) ) {

                if ( !Ce::rapiInit( pdaName ) ) {
                    return false;
                }

                synce::SYSTEM_INFO system;
                Ce::getSystemInfo( &system );
                Ce::rapiUninit();

                QString arch;

                switch ( system.wProcessorArchitecture ) {
                case 1:   // Mips
                    arch = "rmips";
                    break;
                case 4:   // SHx
                    arch = "rsh3";
                    break;
                case 5:   // Arm
                    arch = "rarm";
                    break;
                }

                QStringList extractedFiles = extractWithOrange( url, "/tmp" );
                QStringList agClients = extractedFiles.grep( arch, false );

                if ( !agClients.empty() ) {
                    install( agClients.first() );
                    configDialog->resetInstallClient();

                    for ( QStringList::Iterator it = extractedFiles.begin(); it != extractedFiles.end(); ++it ) {
                        KIO::NetAccess::del(*it, NULL);
                    }

                    KMessageBox::information( parent,
                                              i18n( "The AvantGo Client has been installed on your device.\n" \
                                                    "Please finish the installation on the device but do not " \
                                                    "restart it until the first synchronization has finished!\n" \
                                                    "Press <OK> to start syncing your AvantGo channels" ),
                                              i18n( "AvantGo Client installed" ) );
                } else {
                    ret = false;
                }
            } else {
                ret = false;
            }
        } else {
            ret = false;
        }
    }

    return ret;
}


static void checkAccounts(AGUserConfig *resultConfig)
{
    int cnt = AGUserConfigCount(resultConfig);

    int i;
    for (i = 0; i < cnt; i++) {
        AGServerConfig *sc = AGUserConfigGetServerByIndex(resultConfig, i);
        if (sc->userName == NULL ||
            (sc->password == NULL || *sc->password == '\0') &&
            (sc->cleartextPassword == NULL || *sc->cleartextPassword == '\0')) {
            GetPasswordDialog gpd;
            gpd.serverName->setText(sc->serverName);
            if (gpd.exec() == QDialog::Accepted) {
                sc->userName = qstrdup(gpd.userName->text().ascii());
                sc->hashPassword = AG_HASH_PASSWORD_UNKNOWN;
                AGServerConfigChangePassword(sc, (char *) gpd.passWord->text().ascii());
            } else {
                sc->disabled = true;
            }
        }
    }
}


void AGSync::doSync( AGReader *r, AGWriter *w, AGNetCtx *ctx )
{
    AGUserConfig *deviceConfig = NULL;
    AGUserConfig *desktopConfig = NULL;
    AGUserConfig *resultConfig = NULL;
    AGUserConfig *agreedConfig = NULL;

    int cnt, i;

    deviceConfig = AGUserConfigNew();
    if ( NULL == asGetUserConfig( r, w, deviceConfig ) ) {
        kdDebug( 2120 ) << i18n( "Failed to receive user configuration from device." ) << endl;
        goto confEnd;
    }

    desktopConfig = configDialog->getUserConfig();
    agreedConfig = configDialog->getAgreedConfig();

    resultConfig = AGUserConfigSynchronize( agreedConfig, deviceConfig, desktopConfig, false );

    checkAccounts(resultConfig);

    if (0 != asPutUserConfig( r, w, resultConfig )) {
        kdDebug( 2120 ) << i18n( "Failed to store user configuration to device." ) << endl;
    }

    cnt = AGUserConfigCount( resultConfig );

    kdDebug( 2120 ) << i18n( "Processing %1 servers." ).arg( cnt ) << cnt << endl;

    for ( i = 0; ( i < cnt ) && !stopRequested(); i++ ) {
        doServerSync( r, w, AGUserConfigGetServerByIndex( resultConfig, i ), ctx );
    }

    /* TODO: Write updated config */
    configDialog->setUserConfig( resultConfig );
    if ( 0 != asPutUserConfig( r, w, resultConfig ) ) {
        kdDebug( 2120 ) << i18n( "Failed to store user configuration to device." ) << endl;
    }

    AGUserConfigFree( resultConfig );

confEnd:
    AGUserConfigFree( deviceConfig );
}


void AGSync::configAGSync()
{
    if ( configDialog->getHttpProxy() ) {
        kdDebug( 2120 ) << i18n( "Using HttpProxy" ) << endl;
        locConfig = AGLocationConfigNew();
        locConfig->HTTPName = qstrdup( configDialog->getHttpProxyHost().ascii() );
        locConfig->HTTPPort = configDialog->getHttpProxyPort();
        locConfig->HTTPUseProxy = 1;
        if ( configDialog->getUseAuthentication() ) {
            kdDebug( 2120 ) << i18n( "Using HttpProxy Authentification" ) << endl;
            locConfig->HTTPUsername = qstrdup( configDialog->getHttpUsername().ascii() );
            locConfig->HTTPPassword = qstrdup( configDialog->getHttpPassword().ascii() );
            locConfig->HTTPUseAuthentication = 1;
        }
    } else if ( configDialog->getSocksProxy() ) {
        kdDebug( 2120 ) << i18n( "Using SocksProxy" ) << endl;
        locConfig = AGLocationConfigNew();
        locConfig->SOCKSName = qstrdup( configDialog->getSocksProxyHost().ascii() );
        locConfig->SOCKSPort = configDialog->getSocksProxyPort();
        locConfig->SOCKSUseProxy = 1;
    }
}


bool AGSync::sync()
{
    synce::IRAPIStream * s;
    int result;
    AGReader *r;
    AGWriter *w;
    AGNetCtx ctx;
    HRESULT hr;

    locConfig = NULL;

    configAGSync();

    Ce::rapiInit( pdaName );

    hr = Ce::rapiInvokeA(
             "malclmgr.dll",
             "_RAPI_HandleStream2",
             0,
             NULL,
             0,
             NULL,
             &s,
             0 );

    if ( FAILED( hr ) ) {
        Ce::rapiUninit();
        return false;
    }

    r = AGReaderNew( ( void * ) s, readFunc );

    w = AGWriterNew( ( void * ) s, writeFunc );

    pStore.r = r;
    pStore.w = w;

    AGNetInit( &ctx );
    KApplication::kApplication()->processEvents();
    doSync( r, w, &ctx );

    result = asEndSession( r, w );

    AGNetClose( &ctx );
    AGWriterFree( w );
    AGReaderFree( r );

    if ( NULL != locConfig )
        AGLocationConfigFree( locConfig );

    if ( NULL != pRecord )
        AGRecordFree( pRecord );

    IRAPIStream_Release( s );

    Ce::rapiUninit();

    return true;
}


int AGSync::syncContext() {
    return RakiSyncPlugin::SYNCHRONOUS;
}
#endif
