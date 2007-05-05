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
#ifndef TIMERNODE_H
#define TIMERNODE_H

#include <sys/time.h>


class TimerNodeManager;

/**
@author Volker Christian
*/
class TimerNode{
public:

    virtual ~TimerNode();
    bool expired(struct timeval tv);
    TimerNodeManager *getTimerNodeManager() const;
    virtual operator struct timeval() const
    {
        return tv;
    }

private:
    void setTimerNodeManager(TimerNodeManager * timerNodeManager);

protected:
    virtual void trigger() = 0;
    TimerNode();

protected:
    struct timeval tv;
    TimerNodeManager *timerNodeManager;

friend class TimerNodeManager;
};

bool operator<(const struct timeval &tv1, const struct timeval &tv2);
bool operator>(const struct timeval &tv1, const struct timeval &tv2);
bool operator<=(const struct timeval &tv1, const struct timeval &tv2);
bool operator>=(const struct timeval &tv1, const struct timeval &tv2);
bool operator==(const struct timeval &tv1, const struct timeval &tv2);
struct timeval operator+(const struct timeval &tv1, const struct timeval &tv2);
struct timeval operator-(const struct timeval &tv1, const struct timeval &tv2);


#endif
