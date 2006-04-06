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
#ifndef DESCRIPTORMANAGER_H
#define DESCRIPTORMANAGER_H

#include <list>
#include "descriptor.h"

//class Descriptor;
class Multiplexer;

/**
@author Volker Christian
*/
class DescriptorManager{
public:
    DescriptorManager();
    virtual ~DescriptorManager();

    bool add(Descriptor * descriptor);
    bool remove(Descriptor * descriptor);
    void init();
    static bool dataPending(const Descriptor *descriptor, int sec, int usec);
    static bool writable(const Descriptor *descriptor, int sec, int usec);

protected:
    DescriptorManager &operator=(const DescriptorManager &descriptorManager)
    {
        return *this;
    }

    explicit DescriptorManager(const DescriptorManager &descriptorManager)
    {
    }

private:
    fd_set* fdSet();
    Descriptor* getHighestDescriptor();
    int process(enum Descriptor::eventType et, fd_set *);

private:
    fd_set staticFdSet;
    fd_set workingFdSet;
    std::list<Descriptor *> descriptors;
    std::list<Descriptor *>::iterator it;
    bool listDirty;
    bool dontIncrement;

friend class Multiplexer;
};

#endif
