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

#include <qapplication.h>

RakiSyncPlugin::RakiSyncPlugin()
{}


bool RakiSyncPlugin::doSync(SyncThread *syncThread, ObjectType *objectType, QString pdaName, uint32_t partnerId, SyncTaskListItem *progressItem, Rra *rra)
{
    this->pdaName = pdaName;
    this->progressItem = progressItem;
    this->rra = rra;
    this->syncThread = syncThread;
    this->objectType = objectType;
    this->partnerId = partnerId;
    return sync();
}


uint32_t RakiSyncPlugin::getObjectTypeId()
{
    return objectType->id;
}


bool RakiSyncPlugin::isRunning()
{
    return syncThread->running();
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
