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

#ifndef RAKIWORKERTHREAD_H
#define RAKIWORKERTHREAD_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>

class WorkerThreadInterface;

/**
@author Volker Christian,,,
*/

#define startWorkerThread(a, b, c) \
    RakiWorkerThread::rakiWorkerThread->start(a, \
            (void (WorkerThreadInterface::*)(QThread *thread = NULL, \
            void *data = NULL)) b, (void *) c)
            
class RakiWorkerThread : public QThread
{
protected:
    RakiWorkerThread();
    ~RakiWorkerThread();
    
public:
    void run();
    void start(WorkerThreadInterface *wti,
            void (WorkerThreadInterface::*userRun)(QThread *thread,
            void *data), void *data = NULL);
    WorkerThreadInterface *getCurrentInterface();
    void stop();
    static bool running();
    static bool finished();
    static void sleep(unsigned long sec);
      
private:
    WorkerThreadInterface *wti;
    void (WorkerThreadInterface::*userRun)(QThread *thread = NULL,
            void *data = NULL);
    void *data;
    QWaitCondition waitCondition;
    QMutex threadMutex;
    
public:
    static RakiWorkerThread *rakiWorkerThread;
};

#endif
