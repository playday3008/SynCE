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
#include "timernode.h"
#include "timernodemanager.h"
#include <unistd.h>


TimerNode::TimerNode()
{
}


TimerNode::~TimerNode()
{
}


TimerNodeManager *TimerNode::getTimerNodeManager() const
{
    return timerNodeManager;
}


/*!
    \fn TimerNode::setTimerNodeManager(TimerNodeManager *timerNodeManager)
 */
void TimerNode::setTimerNodeManager(TimerNodeManager * timerNodeManager)
{
    this->timerNodeManager = timerNodeManager;
}


/*!
    \fn TimerNode::expired(struct timeval tv)
 */
bool TimerNode::expired(struct timeval tv)
{
    return tv >= this->tv;
}


bool operator<(const struct timeval &tv1, const struct timeval &tv2)
{
    return (tv1.tv_sec < tv2.tv_sec) || ((tv1.tv_sec == tv2.tv_sec) && (tv1.tv_usec < tv2.tv_usec));
}


bool operator>(const struct timeval &tv1, const struct timeval &tv2)
{
    return (tv2.tv_sec < tv1.tv_sec) || ((tv2.tv_sec == tv1.tv_sec) && (tv2.tv_usec < tv1.tv_usec));
}


bool operator<=(const struct timeval &tv1, const struct timeval &tv2)
{
    return !(tv1 > tv2);
}


bool operator>=(const struct timeval &tv1, const struct timeval &tv2)
{
    return !(tv1 < tv2);
}


bool operator==(const struct timeval &tv1, const struct timeval &tv2)
{
    return !(tv1 < tv2) && !(tv2 < tv1);
}


struct timeval operator+(const struct timeval &tv1, const struct timeval &tv2)
{
    struct timeval help;

    help.tv_sec = tv1.tv_sec + tv2.tv_sec;

    help.tv_usec = tv1.tv_usec + tv2.tv_usec;

    if (help.tv_usec > 999999) {
        help.tv_usec -= 1000000;
        help.tv_sec++;
    }

    return help;
}


struct timeval operator-(const struct timeval &tv1, const struct timeval &tv2)
{
    struct timeval help;

    help.tv_sec = tv1.tv_sec - tv2.tv_sec;
    help.tv_usec = tv1.tv_usec - tv2.tv_usec;

    if (help.tv_usec < 0) {
        help.tv_usec += 1000000;
        help.tv_sec--;
    }

    return help;
}
