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
#ifndef MULTIPLEXER_H
#define MULTIPLEXER_H

#include <sys/time.h>
#include <descriptormanager.h>
#include <timernodemanager.h>


/**
@author Volker Christian
*/
class Multiplexer {
public:
    ~Multiplexer();
    TimerNodeManager* getTimerNodeManager() const;
    DescriptorManager* getExceptionManager() const;
    DescriptorManager* getReadManager() const;
    DescriptorManager* getWriteManager() const;
    int multiplex();
    static Multiplexer* self();

protected:
    Multiplexer();
    Multiplexer &operator=(Multiplexer &multiplexer)
    {
        return multiplexer;
    }

    explicit Multiplexer(const Multiplexer &multiplexer)
    {
    }

private:
    static Multiplexer *multiplexer;
    struct timeval tv;
    DescriptorManager* readManager;
    DescriptorManager* writeManager;
    DescriptorManager* exceptionManager;
    TimerNodeManager* timerNodeManager;
};

#endif
