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
#include "timernodemanager.h"
#include "timernode.h"
#include <algorithm>


using namespace std;

TimerNodeManager::TimerNodeManager()
{
    listDirty = false;
    dontIncrement = false;
    it = timerNodes.end();
}


TimerNodeManager::~TimerNodeManager()
{
}


/*!
    \fn TimerNodeManager::add(TimerNode *timerNode)
 */
bool TimerNodeManager::add(TimerNode * timerNode)
{
    bool ret = true;
    if (find(timerNodes.begin(), timerNodes.end(), timerNode) == timerNodes.end()) {
        timerNodes.push_back(timerNode);
        timerNode->setTimerNodeManager(this);
        listDirty = true;
    } else {
        ret = false;
    }

    return ret;
}


/*!
    \fn TimerNodeManager::remove(TimerNode *timerNode)
 */
bool TimerNodeManager::remove(TimerNode * timerNode)
{
    bool ret = true;
    list<TimerNode *>::iterator rit;

    rit = find(timerNodes.begin(), timerNodes.end(), timerNode);

    if (rit != timerNodes.end()) {
        if (rit == it) {
            it = timerNodes.erase(rit);
            dontIncrement = true;
        } else {
            timerNodes.erase(rit);
        }
        timerNode->setTimerNodeManager(NULL);
        listDirty = true;
    } else {
        ret = false;
    }

    return ret;
}


bool TimerNodeManager::compare(const TimerNode * const tn1, const TimerNode * const tn2)
{
    return *tn1 < *tn2;
}


/*!
    \fn TimerNodeManager::process()
 */
struct timeval TimerNodeManager::process()
{
    if (listDirty) {
        timerNodes.sort(TimerNodeManager::compare);
        listDirty = false;
    }

    int numberProcessed;
    dontIncrement = false;

    struct timeval actualTime;

    do {
        it = timerNodes.begin();
        numberProcessed = 0;
        while(it != timerNodes.end()) {
            gettimeofday(&actualTime, NULL);
            if ((*it)->expired(actualTime)) {
                (*it)->trigger();
                numberProcessed++;
            }
            if (!dontIncrement) {
                ++it;
            } else {
                dontIncrement = false;
            }
        }
        if (listDirty) {
            timerNodes.sort(TimerNodeManager::compare);
            listDirty = false;
        }
    } while(numberProcessed > 0);

    struct timeval tv;

    if (timerNodes.size() == 0) {
        tv.tv_sec = 10;
        tv.tv_usec = 0;
    } else {
        gettimeofday(&actualTime, NULL);
        tv = *timerNodes.front() - actualTime;
    }

    return tv;
}


/*!
    \fn TimerNodeManager::setDirty()
 */
 void TimerNodeManager::setDirty()
{
    listDirty = true;
}
