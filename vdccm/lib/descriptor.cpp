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
#include "descriptor.h"
#include "descriptormanager.h"
#include <unistd.h>
#include <fcntl.h>


Descriptor::Descriptor()
{
    descriptor = 0;
    descriptorManager = NULL;
}


Descriptor::Descriptor(const Descriptor &descriptor, bool releaseFromManager)
{
    this->descriptor = descriptor.descriptor;
    if (!releaseFromManager) {
        this->descriptorManager = descriptor.descriptorManager;
    } else {
        this->descriptorManager = NULL;
    }
}


Descriptor::~Descriptor()
{
}


int Descriptor::getDescriptor() const
{
    return descriptor;
}


void Descriptor::setDescriptor(int descriptor)
{
    this->descriptor = descriptor;
}


bool Descriptor::setDescriptorManager(DescriptorManager *descriptorManager)
{
    if (this->descriptorManager == NULL) {
        this->descriptorManager = descriptorManager;
    } else {
        return false;
    }

    return true;
}


DescriptorManager *Descriptor::getDescriptorManager() const
{
    return descriptorManager;
}


bool Descriptor::sortcrit(const Descriptor *d1, const Descriptor *d2)
{
    return d1->getDescriptor() < d2->getDescriptor();
}


bool Descriptor::close()
{
    return ::close(descriptor) == 0;
}


void Descriptor::event()
{
}


bool Descriptor::dataPending(int sec, int usec)
{
    return DescriptorManager::dataPending(this, sec, usec);
}


bool Descriptor::setNonBlocking()
{
    int flags = fcntl (descriptor, F_GETFL);
    return fcntl (descriptor, F_SETFL, flags | O_NONBLOCK) >= 0;
}
