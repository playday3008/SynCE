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
#ifndef TIMERNODEMANAGER_H
#define TIMERNODEMANAGER_H

#include <list>

class TimerNode;
class Multiplexer;

/**
@author Volker Christian
*/
class TimerNodeManager{
public:
    TimerNodeManager();
    ~TimerNodeManager();
    bool add(TimerNode * timerNode);
    bool remove(TimerNode * timerNode);

protected:
    TimerNodeManager &operator=(const TimerNodeManager &timerNodeManager)
    {
        return *this;
    }

    explicit TimerNodeManager(const TimerNodeManager &timerNodeManager)
    {
    }

private:
    struct timeval process();
    void setDirty();
    static bool compare(const TimerNode * tn1, const TimerNode * tn2);

private:
    std::list<TimerNode *> timerNodes;
    std::list<TimerNode *>::iterator it;
    bool listDirty;
    bool dontIncrement;

friend class Multiplexer;
friend class SingleShotNode;
friend class ContinousNode;
};

#endif
