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

#ifndef WORKERTHREADINTERFACE_H
#define WORKERTHREADINTERFACE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "threadevent.h"

#include <qmutex.h>
#include <qwaitcondition.h>

/**
@author Volker Christian,,,
*/

#define postThreadEvent(a, b, c) \
    this->postEvent((void *(WorkerThreadInterface::*)(void *data = NULL)) a, (void *) b, c);
    
class WorkerThreadInterface
{
public:
    WorkerThreadInterface();
    ~WorkerThreadInterface();
    int type;
    bool running();
    void setRunning(bool);
    bool stopRequested();
    void setStopRequested(bool isStopRequested);
    void *postEvent(void *(WorkerThreadInterface::*userEventMethode)(void *data = 0), void *data = 0, int block = 0);
    void setEventReturnValue(void *value);
    void *eventReturnValue();
    void *synchronizeGui();
    void setDelayedDelete(bool delayedDelete);
    bool delayedDelete();
    
    enum eventType {
        noBlock = 0,
        block
    };

private:
    void *guiSynchronizator(void *data = 0);
    ThreadEventObject threadEventObject;
    bool isRunning;
    bool isStopRequested;
    bool isDelayedDelete;
    void *eventReturn;
    QMutex syncMutex;
    QWaitCondition syncWaitCondition;
};


#endif
