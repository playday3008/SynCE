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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rakisyncplugin.h"
#include "syncthread.h"
#include "synctasklistitem.h"

#include <qapplication.h>
#include <kmessagebox.h>

RakiSyncPlugin::RakiSyncPlugin()
{
    parent = NULL;
}


RakiSyncPlugin::~RakiSyncPlugin()
{
    delete ksConfig;
}


bool RakiSyncPlugin::doSync(SyncThread *syncThread, Rra *rra,
        SyncTaskListItem *progressItem, bool firstSynchronize, uint32_t partnerId)
{
    this->syncThread = syncThread;
    this->progressItem = progressItem;
    this->rra = rra;
    this->firstSynchronize = firstSynchronize;
    this->partnerId = partnerId;
    return sync();
}


bool RakiSyncPlugin::preSync(SyncThread */*syncThread*/, Rra */*rra*/,
        bool /*firstSynchronize*/, uint32_t /*partnerId*/)
{
    return true;
}


bool RakiSyncPlugin::postSync(SyncThread */*syncThread*/, Rra */*rra*/,
        bool /*firstSynchronize*/, uint32_t /*partnerId*/)
{
    return true;
}


void RakiSyncPlugin::init(RRA_SyncMgrType *objectType, QString pdaName, QWidget *parent, QString serviceName)
{
    this->objectType = objectType;
    this->pdaName = pdaName;
    this->parent = parent;
    this->_serviceName = serviceName;
    ksConfig = new KConfig("raki/" + pdaName + ".cfg", false, false, "data");
    createConfigureObject(ksConfig);
}


void RakiSyncPlugin::configure()
{
    KMessageBox::information(parent, "<b>" + _serviceName +
            "</b>: Nothing to configure.", QString(objectType->name2) + " " + pdaName);
}


void RakiSyncPlugin::createConfigureObject(KConfig */*ksConfig*/)
{
}


QString RakiSyncPlugin::serviceName()
{
    return _serviceName;
}


uint32_t RakiSyncPlugin::getObjectTypeId()
{
    return objectType->id;
}


bool RakiSyncPlugin::running()
{
    return syncThread->running();
}


bool RakiSyncPlugin::stopRequested()
{
    return syncThread->stopRequested();
}


void RakiSyncPlugin::incTotalSteps(int inc)
{
    postSyncThreadEvent(SyncThread::incTotalSteps, (void *) inc);
}


void RakiSyncPlugin::decTotalSteps(int dec)
{
    postSyncThreadEvent(SyncThread::decTotalSteps, (void *) dec);
}


void RakiSyncPlugin::advanceProgress()
{
    postSyncThreadEvent(SyncThread::advanceProgress, (void *) 0);
}


void RakiSyncPlugin::setTotalSteps(int steps)
{
    postSyncThreadEvent(SyncThread::setTotalSteps, (void *) steps);
}


void RakiSyncPlugin::setProgress(int progress)
{
    postSyncThreadEvent(SyncThread::setProgress, (void *) progress);
}


void RakiSyncPlugin::setTask(const char *task)
{
    postSyncThreadEvent(SyncThread::setTask, (void *) qstrdup(task));
}
