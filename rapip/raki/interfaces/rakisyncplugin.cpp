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

#include "rakisyncplugin.h"
#include "syncevent.h"
#include "workerthreadinterface.h"

#include <qapplication.h>

RakiSyncPlugin::RakiSyncPlugin()
{}


bool RakiSyncPlugin::doSync(WorkerThreadInterface *workerThread, ObjectType *objectType, QString pdaName, SyncTaskListItem *progressItem, Rra *rra)
{
    this->pdaName = pdaName;
    this->progressItem = progressItem;
    this->rra = rra;
    this->workerThread = workerThread;
    this->objectType = objectType;
    return sync();
}


uint32_t RakiSyncPlugin::getObjectTypeId()
{
    return objectType->id;
}


bool RakiSyncPlugin::isRunning()
{
    return workerThread->running();
}


void RakiSyncPlugin::incTotalSteps(int inc)
{
    SyncEvent::incTotalSteps(progressItem, inc);
}


void RakiSyncPlugin::decTotalSteps(int dec)
{
    SyncEvent::decTotalSteps(progressItem, dec);
}


void RakiSyncPlugin::advanceProgress()
{
    SyncEvent::advanceProgress(progressItem);
}


void RakiSyncPlugin::setTotalSteps(int steps)
{
    SyncEvent::setTotalSteps(progressItem, steps);
}


void RakiSyncPlugin::setProgress(int progress)
{
    SyncEvent::setProgress(progressItem, progress);
}


void RakiSyncPlugin::setTask(QString task)
{
    SyncEvent::setTask(progressItem, task);
}
