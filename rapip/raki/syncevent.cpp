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
 
#include "syncevent.h"
#include "synctasklistitem.h"

#include <qapplication.h>

SyncEvent::SyncEvent(enum SyncEvent::eventTypes et, int value, SyncTaskListItem *item) : QCustomEvent(QEvent::User)
{
    this->et = et;
    this->value = value;
    this->item = item;
}


SyncEvent::SyncEvent(enum SyncEvent::eventTypes et, QString string, SyncTaskListItem *item) : QCustomEvent(QEvent::User)
{
    this->et = et;
    this->string = string;
    this->item = item;
}


int SyncEvent::getValue()
{
    return value;
}


int SyncEvent::getEventType()
{
    return et;
}


SyncTaskListItem *SyncEvent::getItem()
{
    return item;
}

QString SyncEvent::getString()
{
    return string;
}


void SyncEvent::incTotalSteps(SyncTaskListItem *item, int inc)
{
    SyncEvent *se = new SyncEvent(SyncEvent::INC_TOTAL_STEPS, inc, item);
    QApplication::postEvent(se, se);
}


void SyncEvent::decTotalSteps(SyncTaskListItem *item, int dec)
{
    SyncEvent *se = new SyncEvent(SyncEvent::DEC_TOTAL_STEPS, dec, item);
    QApplication::postEvent(se, se);
}


void SyncEvent::advanceProgress(SyncTaskListItem *item)
{
    SyncEvent *se = new SyncEvent(SyncEvent::ADVANCE_PROGRESS, 0, item);
    QApplication::postEvent(se, se);
}


void SyncEvent::setTotalSteps(SyncTaskListItem *item, int steps)
{
    SyncEvent *se = new SyncEvent(SyncEvent::SET_TOTAL_STEPS, steps, item);
    QApplication::postEvent(se, se);
}


void SyncEvent::setProgress(SyncTaskListItem *item, int progress)
{
    SyncEvent *se = new SyncEvent(SyncEvent::SET_PROGRESS, progress, item);
    QApplication::postEvent(se, se);
}


void SyncEvent::setTask(SyncTaskListItem *item, QString task)
{
    SyncEvent *se = new SyncEvent(SyncEvent::SET_TASK, task, item);
    QApplication::postEvent(se, se);
}


void SyncEvent::incTotalStepsEvent(SyncTaskListItem *item, int inc)
{
    item->setTotalSteps(item->totalSteps() + inc);
}


void SyncEvent::decTotalStepsEvent(SyncTaskListItem *item, int dec)
{
    item->setTotalSteps(item->totalSteps() - dec);
}


void SyncEvent::advanceProgressEvent(SyncTaskListItem *item)
{
    item->advance(1);
}


void SyncEvent::setTotalStepsEvent(SyncTaskListItem *item, int steps)
{
    item->setTotalSteps(steps);
}


void SyncEvent::setProgressEvent(SyncTaskListItem *item, int progress)
{
    item->setProgress(progress);
}


void SyncEvent::setTaskEvent(SyncTaskListItem *item, QString task)
{
    item->setTaskLabel(task);
}


void SyncEvent::customEvent (QCustomEvent *customEvent)
{
    SyncEvent *event = (SyncEvent *) customEvent;
    
    switch(event->getEventType()) {
    case SyncEvent::INC_TOTAL_STEPS:
        incTotalStepsEvent(event->getItem(), event->getValue());
        break;
    case SyncEvent::DEC_TOTAL_STEPS:
        decTotalStepsEvent(event->getItem(), event->getValue());
        break;
    case SyncEvent::ADVANCE_PROGRESS:
        advanceProgressEvent(event->getItem());
        break;
    case SyncEvent::SET_TOTAL_STEPS:
        setTotalStepsEvent(event->getItem(), event->getValue());
        break;
    case SyncEvent::SET_PROGRESS:
        setProgressEvent(event->getItem(), event->getValue());
        break;
    case SyncEvent::SET_TASK:
        setTaskEvent(event->getItem(), event->getString());
        break;
    }
}
