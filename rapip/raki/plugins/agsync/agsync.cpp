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
#include <kdebug.h>
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

#include "syncstream.h"


static RakiSyncPlugin *plugin;

static int32 readFunc(void *s, void *data, int32 len)
{
    HRESULT hr = IRAPIStream_Read((synce::IRAPIStream*)s, data, len, NULL);
    if (SUCCEEDED(hr))
        return len;
    else
        return 0;
}


static int32 writeFunc(void *s, void *data, int32 len)
{
    HRESULT hr = IRAPIStream_Write((synce::IRAPIStream*)s, data, len, NULL);
    if (SUCCEEDED(hr))
        return len;
    else
        return 0;
}


int32 taskPrinter(void */*blah*/, int */*errBlah*/, char *str, AGBool /*thing*/)
{
    plugin->setTask(str);
    return 1; // AGCLIENT_CONTINUE
}


int32 itemPrinter(void */*blah*/, int */*errBlah*/, int items, int total, char */*name*/)
{
    plugin->setTotalSteps(total);
    plugin->setProgress(items);

    return 1; // AGClIENT_CONTINUE
}


AGRecord *pRecord= NULL;

typedef struct _PStoreStruct
{
    AGReader *r;
    AGWriter *w;
    AGCommandProcessor *cmdProc;
}
PStoreStruct;

PStoreStruct pStore;

int32 pNextModifiedRecord(void *pStoreVoid, AGRecord **record, int32 *errCode)
{
    int res;
    if(NULL != pRecord)
        AGRecordFree(pRecord);

    res= asGetNextModifiedRecord(((PStoreStruct *)pStoreVoid)->r,
                                 ((PStoreStruct *)pStoreVoid)->w,
                                 record);
    if(2 == res)
        *errCode= asErrno;

    return res;
}


int32 pNextRecord(void *pStoreVoid, AGRecord **record, int32 *errCode)
{
    int res;
    if(NULL != pRecord)
        AGRecordFree(pRecord);

    res= asGetNextRecord(((PStoreStruct *)pStoreVoid)->r,
                         ((PStoreStruct *)pStoreVoid)->w,
                         record);
    if(2 == res)
        *errCode= asErrno;

    return res;
}


int32 pOpenDatabase(void *pStoreVoid, AGDBConfig *theDatabase, int32 *errCode)
{
    int res;
    res= asOpenDatabase(((PStoreStruct *)pStoreVoid)->r,
                        ((PStoreStruct *)pStoreVoid)->w,
                        theDatabase);
    if(2 == res)
        *errCode= asErrno;

    return res;
}


int32 pNextExpansionCommand(void */*pStoreVoid*/, int32 */*cmd*/,
                            int32 */*cmdLen*/, void **/*cmdData*/)
{
    // I suspect that the regular case does not use this.
    return 0;
}


int32 pPerformCommand(void *pStoreVoid, int32 *err, AGReader *r)
{
    int result;
    unsigned char *data;
    int dataLen, cmd;
    AGBufferReader *rB= (AGBufferReader *)r;

    AGPerformCommandFunc cmdFunc= AGCommandProcessorGetPerformFunc(
                                      ((PStoreStruct *)pStoreVoid)->cmdProc);

    result= (*cmdFunc)(((PStoreStruct *)pStoreVoid)->cmdProc, err, r);

    /* I know it's a BufferReader, and when was the last time C++
       allowed be to h4x0r object internals? Heh. Lets enjoy it! */
    rB->currentIndex= 0;
    cmd= AGReadCompactInt(r);
    dataLen= AGReadCompactInt(r);
    data= (unsigned char *) (rB->buffer + rB->currentIndex);

    // All commands except goodbye should return 0
    if(1 != result && 0 != cmd)
        kdDebug(2120) << "ERROR ON COMMAND " << cmd;

    asPerformCommand(((PStoreStruct *)pStoreVoid)->r,
                     ((PStoreStruct *)pStoreVoid)->w,
                     cmd, data, dataLen);
    return result;
}


AGSync::AGSync()
{
    locConfig = NULL;
    plugin = this;
}


AGSync::~AGSync()
{}


void AGSync::setProxy(QString host, unsigned int port)
{
    proxyHost = host;
    proxyPort = port;
}


void AGSync::setSocks(QString host, unsigned int port)
{
    socksHost = host;
    socksPort = port;
}


void AGSync::setUser(QString user, QString password)
{
    this->user = user;
    this->password = password;
}


AGPlatformCalls pCalls= {
                            (void *)&pStore,
                            pNextModifiedRecord,
                            pNextRecord,
                            pOpenDatabase,
                            pNextExpansionCommand,
                            (void *)&pStore,
                            pPerformCommand
                        };

void AGSync::doServerSync(AGReader *r, AGWriter *w, AGServerConfig *s, AGNetCtx *ctx)
{
    int result;
    AGCommandProcessor *cmdProc;

    if(s->disabled) {
        return;
    }

    /* Initialize the command processor */
    cmdProc= AGCommandProcessorNew(s);
    pStore.cmdProc= cmdProc;
    /* Note: If we wanted status messages, we could hook the
             command processor here to handle them.
    */
    cmdProc->commands.performTaskFunc= taskPrinter;
    cmdProc->commands.performItemFunc= itemPrinter;


    /* Start server block */
    if(0 != asStartServer(r, w, s->uid)) {
        kdDebug(2120) << "AvantGo error on asStartServer: " << asErrno << "!" << endl;
        return;
    }

    /* A loop over server connections, since we may have to sync
       more than once. This normally doesn't happen, but I think is
       used occasionally to initialize a device which has no config yet.
    */
    do {
        AGDeviceInfo *devInfo;
        AGClientProcessor *clientProc;

        kdDebug(2120) << "Beginning synchonization attempt on server." << endl;

        devInfo= AGDeviceInfoNew();

        if(NULL == asGetDeviceInfo(r, w, devInfo)) {
            kdDebug(2120) << "Failed to retrieve device information!" << endl;
//            goto devEnd;
            break;
        }

        AGCommandProcessorStart(cmdProc); /* Error code unused */

        /* Set up client processor...
           TRUE is for buffering of cmds. */
        clientProc= AGClientProcessorNew(s, devInfo, locConfig, &pCalls,
                                         TRUE, ctx);
        /* Apparently we dont?? */
        AGClientProcessorSetBufferServerCommands(clientProc, FALSE);


        /* Basically a connect request? */
        AGClientProcessorSync(clientProc);


        /* Perform the synchronization */
        do {
            result= AGClientProcessorProcess(clientProc);
        } while(AGCLIENT_CONTINUE == result && !stopRequested());

        AGClientProcessorFree(clientProc);

//    devEnd:
        AGDeviceInfoFree(devInfo);

    } while(AGCommandProcessorShouldSyncAgain(cmdProc) && !stopRequested());

    AGCommandProcessorFree(cmdProc);


    if(0 != asEndServer(r, w)) {
        kdDebug(2120) << "Avantgo error on asEndServer: " << asErrno << "!" << endl;
    }
}


void AGSync::doSync(AGReader *r, AGWriter *w, AGNetCtx *ctx)
{
    AGUserConfig *userConfig;
    int cnt, i;

    /* Read configuration */
    userConfig= AGUserConfigNew();

    /* TODO: Most sync. apps will read a configuration from disk here,
             determine an "agreed" configuration, and push that back
      to the device. We should do this eventually.
    */
    if(NULL == asGetUserConfig(r, w, userConfig)) {
        kdDebug(2120) << "Failed to receive user configuration from device." << endl;
        goto confEnd;
    }


    /* Loop through servers and sync. */
    cnt= AGUserConfigCount(userConfig);  /* Implied (Server)Count */

    kdDebug(2120) << "Processing " << cnt << " servers." << endl;

    for(i= 0; (i < cnt) && !stopRequested(); i++) {
        doServerSync(r, w, AGUserConfigGetServerByIndex(userConfig, i), ctx);
    }

    /* TODO: Write updated config */
    if (!stopRequested()) {
        if(0 != asPutUserConfig(r, w, userConfig)) {
            kdDebug(2120) << "Failed to store user configuration to device." << endl;
        }
    }

confEnd:
    AGUserConfigFree(userConfig);
}


bool AGSync::sync()
{
    synce::IRAPIStream *s;
    int result;
    AGReader *r;
    AGWriter *w;
    AGNetCtx ctx;
    HRESULT hr;
    
    Ce::rapiInit(pdaName);

    hr = Ce::rapiInvokeA(
             "malclmgr.dll",
             "_RAPI_HandleStream2",
             0,
             NULL,
             0,
             NULL,
             &s,
             0);

    if (FAILED(hr)) {
        Ce::rapiUninit();
        return false;
    }

    r= AGReaderNew((void *)s, readFunc);

    w= AGWriterNew((void *)s, writeFunc);

    pStore.r= r; pStore.w = w;

    AGNetInit(&ctx);

    doSync(r, w, &ctx);

    result= asEndSession(r, w);

    AGNetClose(&ctx);
    AGWriterFree(w);
    AGReaderFree(r);

    if(NULL != locConfig)
        AGLocationConfigFree(locConfig);

    if(NULL != pRecord)
        AGRecordFree(pRecord);

    IRAPIStream_Release(s);

    Ce::rapiUninit();

    return true;
}
#endif
