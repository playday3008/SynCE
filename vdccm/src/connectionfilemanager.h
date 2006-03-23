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
#ifndef CONNECTIONFILEMANAGER_H
#define CONNECTIONFILEMANAGER_H

#include <string>
#include <list>

class WindowsCEDevice;

/**
@author Volker Christian
*/
class ConnectionFileManager{
public:
    ConnectionFileManager();

    ~ConnectionFileManager();
    bool writeConnectionFile(std::string fileName, const WindowsCEDevice *windowsCEDevice);
    bool writeDefaultConnectionFile(const WindowsCEDevice *windowsCEDevice, bool isNewDevice = true);
    bool _writeConnectionFile(std::string fileName, const WindowsCEDevice *windowsCEDevice);
    bool removeConnectionFile(std::string fileName, const WindowsCEDevice *windowsCEDevice);
    bool removeDefaultConnectionFile(const WindowsCEDevice *windowsCEDevice);

protected:
    std::list<const WindowsCEDevice*> defaultDevices;
    std::string getDefaultConnectionFileName();
};

#endif
