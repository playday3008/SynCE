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
#include "descriptormanager.h"
#include "descriptor.h"
#include "multiplexer.h"
#include <algorithm>

using namespace std;

DescriptorManager::DescriptorManager()
{
    FD_ZERO(&this->staticFdSet);
    FD_ZERO(&this->workingFdSet);
    it = descriptors.end();
    listDirty = false;
    dontIncrement = false;
}


DescriptorManager::~DescriptorManager()
{
}


/*!
    \fn DescriptorManager::add(Descriptor *descriptor)
 */
bool DescriptorManager::add(Descriptor * descriptor)
{
    bool ret = true;

    if (descriptor->getDescriptor() >= 0) {
        if (find(descriptors.begin(), descriptors.end(), descriptor) == descriptors.end()) {
        descriptors.push_back(descriptor);
            FD_SET(descriptor->getDescriptor(), &staticFdSet);
            listDirty = true;
        } else {
            ret = false;
        }
    } else {
        ret = false;
    }

    return ret;
}


/*!
    \fn DescriptorManager::remove(Descriptor *descriptor)
 */
bool DescriptorManager::remove(Descriptor * descriptor)
{
    bool ret = true;

    list<Descriptor *>::iterator rit;

    rit = find(descriptors.begin(), descriptors.end(), descriptor);

    if (rit != descriptors.end()) {
        if (rit == it) {
            it = descriptors.erase(rit);
            dontIncrement = true;
        } else {
            descriptors.erase(rit);
        }
        FD_CLR(descriptor->getDescriptor(), &staticFdSet);
        listDirty = true;
    } else {
        ret = false;
    }

    return ret;
}


/*!
    \fn DescriptorManager::fdSet()
 */
fd_set* DescriptorManager::fdSet()
{
    workingFdSet = staticFdSet;

    return &workingFdSet;
}


/*!
    \fn DescriptorManager::getHighestDescriptor()
 */
Descriptor* DescriptorManager::getHighestDescriptor()
{
    Descriptor* ret = NULL;

    if (!descriptors.empty()) {
        if (listDirty) {
            descriptors.sort(Descriptor::sortcrit);
            listDirty = false;
        }
        ret = descriptors.back();
    }

    return ret;
}


/*!
    \fn DescriptorManager::process(fd_set *fdSet)
 */
int DescriptorManager::process(enum Descriptor::eventType et, fd_set *fdSet)
{
    int numberProcessed = 0;

    dontIncrement = false;
    it = descriptors.begin();

    while(it != descriptors.end()) {
        if (FD_ISSET((*it)->getDescriptor(), fdSet)) {
            (*it)->event(et);
            numberProcessed++;
        }
        if (!dontIncrement) {
             ++it;
        } else {
            dontIncrement = false;
        }
    }

    return numberProcessed;
}


/*!
    \fn DescriptorManager::dataPending(const Descriptor *descriptor, int sec, int usec)
 */
bool DescriptorManager::dataPending(const Descriptor *descriptor, int sec, int usec)
{
    bool ret = false;
    fd_set fdSet;
    FD_ZERO(&fdSet);

    FD_SET(descriptor->getDescriptor(), &fdSet);

    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    if (select(descriptor->getDescriptor() + 1, &fdSet, NULL, NULL, &tv) > 0) {
        ret = true;
    }

    return ret;
}


/*!
    \fn DescriptorManager::dataPending(const Descriptor *descriptor, int sec, int usec)
 */
bool DescriptorManager::writable(const Descriptor *descriptor, int sec, int usec)
{
    bool ret = false;
    fd_set fdSet;
    FD_ZERO(&fdSet);

    FD_SET(descriptor->getDescriptor(), &fdSet);

    struct timeval tv;

    tv.tv_sec = sec;
    tv.tv_usec = usec;

    if (select(descriptor->getDescriptor() + 1, NULL, &fdSet, NULL, &tv) > 0) {
        ret = true;
    }

    return ret;
}
