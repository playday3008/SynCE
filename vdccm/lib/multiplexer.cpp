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
#include "multiplexer.h"
#include "timernodemanager.h"
#include "descriptormanager.h"
#include "descriptor.h"
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>

Multiplexer* Multiplexer::multiplexer = NULL;


Multiplexer::Multiplexer()
{
    readManager = new DescriptorManager();
    writeManager = new DescriptorManager();
    exceptionManager = new DescriptorManager();
    timerNodeManager = new TimerNodeManager();
    multiplexer = this;
}


Multiplexer::~Multiplexer()
{
    delete readManager;
    delete writeManager;
    delete exceptionManager;
    delete timerNodeManager;
    multiplexer = NULL;
}



/*!
    \fn Multiplexer::getTimerNodeManager() const
 */
TimerNodeManager* Multiplexer::getTimerNodeManager() const
{
    return timerNodeManager;
}


/*!
    \fn Multiplexer::exceptionManager()
 */
DescriptorManager* Multiplexer::getExceptionManager() const
{
    return exceptionManager;
}


/*!
    \fn Multiplexer::readManager()
 */
DescriptorManager* Multiplexer::getReadManager() const
{
    return readManager;
}


/*!
    \fn Multiplexer::writeManager()
 */
DescriptorManager* Multiplexer::getWriteManager() const
{
    return  writeManager;
}


/*!
    \fn Multiplexer::multiplex()
    \retval 0 on select error, if negativ, the absolut values is the number of not processed descriptors, if positiv number of processed descriptors - all ok!
 */
int Multiplexer::multiplex()
{
    tv = timerNodeManager->process();

    Descriptor* rhDescriptor = readManager->getHighestDescriptor();
    Descriptor* whDescriptor = writeManager->getHighestDescriptor();
    Descriptor* ehDescriptor = exceptionManager->getHighestDescriptor();

    fd_set *rfdSet = NULL;
    int rhfd = 0;

    fd_set *wfdSet = NULL;
    int whfd = 0;

    fd_set *efdSet = NULL;
    int ehfd = 0;


    if (rhDescriptor != NULL) {
        rhfd = rhDescriptor->getDescriptor();
        rfdSet = readManager->fdSet();
    }

    if (whDescriptor != NULL) {
        whfd = whDescriptor->getDescriptor();
        wfdSet = writeManager->fdSet();
    }

    if (ehDescriptor != NULL) {
        ehfd = ehDescriptor->getDescriptor();
        efdSet = exceptionManager->fdSet();
    }

    int maxDesc = 0;

    if (rhfd > whfd) {
        if (ehfd > rhfd) {
            maxDesc = ehfd;
        } else {
            maxDesc = rhfd;
        }
    } else {
        if (ehfd > whfd) {
            maxDesc = ehfd;
        } else {
            maxDesc = whfd;
        }
    }

    int numFds = select(maxDesc + 1, rfdSet, wfdSet, efdSet, &tv);

    int numberProcessed = 0;

    if (numFds > 0) {
        if (numFds != numberProcessed) {
            numberProcessed += readManager->process(Descriptor::READ, rfdSet);
        }

        if (numFds != numberProcessed) {
            numberProcessed += writeManager->process(Descriptor::WRITE, wfdSet);
        }

        if (numFds != numberProcessed) {
            numberProcessed += exceptionManager->process(Descriptor::EXCEPTION, efdSet);
        }
    }

    int ret = 0;

    if (numFds < 0) {
        ret = 0;
    }

    if (numberProcessed == numFds) {
        ret = numberProcessed;
    } else {
        ret = numberProcessed - numFds;
    }

    return ret;
}


/*!
    \fn Multiplexer::self()
 */
Multiplexer* Multiplexer::self()
{
    Multiplexer *mux = multiplexer;

    if (multiplexer == NULL) {
        mux = new Multiplexer();
    }

    return mux;
}
