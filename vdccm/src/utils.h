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
#ifndef UTILS_H
#define UTILS_H

#include <string>


/**
@author Volker Christian
*/
class Utils{
public:

    static bool alreadyRunning(bool noCheck = false);
    static bool writePidFile();
    static void removePidFile();
    static void setupSignals();
    static bool isRunning();
    static bool disconnectDevices();
    static void runScripts(std::string action, std::string deviceName = "");
    static bool dropRootPrivileg();
    static bool acquireRootPrivileg();
    static bool checkStartingUser(bool bypassRootCheck);

private:
    Utils();
    ~Utils();

    static void vdccm_handle_sighup(int n);
    static void vdccm_handle_terminating_signals(int n);

    static bool _isRunning;
    static bool _disconnectDevices;
};

#endif
