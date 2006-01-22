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

#include "syncthread.h"
#include "synctasklistitem.h"


SyncThread::SyncThread()
  : WorkerThreadInterface()
{
    sttd = new SyncThreadThreadData(this);
}


SyncThread::~SyncThread()
{
    delete sttd;
}


void SyncThread::setActualSyncItem(SyncTaskListItem *actualSyncItem)
{
    this->actualSyncItem = actualSyncItem;
}


void *SyncThread::incTotalSteps(void *inc)
{
    actualSyncItem->setTotalSteps(actualSyncItem->totalSteps() + *(int *) inc);
    delete (int *) inc;
    return NULL;
}


void *SyncThread::decTotalSteps(void *dec)
{
    actualSyncItem->setTotalSteps(actualSyncItem->totalSteps() - *(int *) dec);
    delete (int *) dec;
    return NULL;
}


void *SyncThread::advanceProgress(void *)
{
    actualSyncItem->advance(1);
    return NULL;
}


void *SyncThread::setTotalSteps(void *steps)
{
    actualSyncItem->setTotalSteps(*(int *) steps);
    delete (int *) steps;
    return NULL;
}


void *SyncThread::setProgress(void *progress)
{
    actualSyncItem->setProgress(*(int *) progress);
    delete (int *) progress;
    return NULL;
}


void *SyncThread::setTask(void *task)
{
    actualSyncItem->setTaskLabel((char *) task);
    delete [] (char *) task;
    return NULL;
}
