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
 
#ifndef SYNCEVENT_H
#define SYNCEVENT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qevent.h>
#include <qobject.h>

class SyncTaskListItem;

/**
@author Volker Christian,,,
*/

class SyncEvent : public QObject, public QCustomEvent
{
Q_OBJECT
public:
    enum eventTypes {
        INC_TOTAL_STEPS = 1,
        DEC_TOTAL_STEPS,
        ADVANCE_PROGRESS,
        SET_TOTAL_STEPS,
        SET_PROGRESS,
        SET_TASK
    };

    SyncEvent(enum SyncEvent::eventTypes et, int value, SyncTaskListItem *item);
    SyncEvent(enum SyncEvent::eventTypes et, QString string, SyncTaskListItem *item);
    int getValue();
    int getEventType();
    QString getString();
    SyncTaskListItem *getItem();
    
    static void incTotalSteps(SyncTaskListItem *item, int inc);
    static void decTotalSteps(SyncTaskListItem *item, int dec);
    static void advanceProgress(SyncTaskListItem *item);
    static void setTotalSteps(SyncTaskListItem *item, int steps);
    static void setProgress(SyncTaskListItem *item, int progress);
    static void setTask(SyncTaskListItem *item, QString task);
    
private:

    void incTotalStepsEvent(SyncTaskListItem *item, int inc);
    void decTotalStepsEvent(SyncTaskListItem *item, int dec);
    void advanceProgressEvent(SyncTaskListItem *item);
    void setTotalStepsEvent(SyncTaskListItem *item, int steps);
    void setProgressEvent(SyncTaskListItem *item, int progress);
    void setTaskEvent(SyncTaskListItem *item, QString task);
    
    void customEvent (QCustomEvent *customEvent);
    
    enum SyncEvent::eventTypes et;
    int value;
    QString string;
    SyncTaskListItem *item;
};



#endif
