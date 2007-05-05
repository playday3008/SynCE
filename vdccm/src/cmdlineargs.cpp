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
#include "cmdlineargs.h"
#include <synce_log.h>
#include <unistd.h>
#include <stdlib.h>

#include <iostream>

#define DCCM_PING_INTERVAL      5       /* seconds */
#define DCCM_MAX_PING_COUNT     3       /* max number of pings without reply */

using namespace std;

int CmdLineArgs::logLevel = SYNCE_LOG_LEVEL_ERROR;
bool CmdLineArgs::_isDaemon = true;
string CmdLineArgs::password = "";
bool CmdLineArgs::_useIp = false;
int CmdLineArgs::missingPingCount = DCCM_MAX_PING_COUNT;
int CmdLineArgs::pingDelay = DCCM_PING_INTERVAL ;
bool CmdLineArgs::syncClock = false;
bool CmdLineArgs::bypassRootCheck = false;

CmdLineArgs::CmdLineArgs()
{
}


CmdLineArgs::~CmdLineArgs()
{
}

void CmdLineArgs::usage(const char *name)
{
    cout << "Syntax:" << endl << endl
         << "\t" << name << " [-d level] [-f] [-h] [-p password] [-i] [-u count] [-s sec]" << endl << endl
         << "\t-t           Synchronize clock of WM5 devices with host-time" << endl
         << "\t-d level     Set debug log level" << endl
         << "\t           0 - No logging" << endl
         << "\t           1 - Errors only (default)" << endl
         << "\t           2 - Warnings + Errors" << endl
         << "\t           3 - Infos + Warnings + Errors" << endl
         << "\t           4 - Debug" << endl
         << "\t           5 - Trace (like Debug)" << endl
         << "\t-f           Do not run as daemon" << endl
         << "\t-h           Show this help message" << endl
         << "\t-p password  Use this password when device connects" << endl
         << "\t-i           Use ip-address of device for identification" << endl
         << "\t-u count     Allowed numbers of unanswered pings (default 3)" << endl
         << "\t-s sec       Delay between pings in seconds (default 5)" << endl
         << "\t-r           Bypass \"do not start as root\"-check" << endl;
}


bool CmdLineArgs::parseArgs(int argc, char *argv[])
{
    int c;

    while ((c = getopt(argc, argv, "d:fhtrp:iu:s:")) != -1) {
        switch (c) {
        case 'd':
            logLevel = atoi(optarg);
            break;

        case 'f':
            _isDaemon = false;
            break;

        case 'p':
            password = optarg;
            break;

        case 'i':
            _useIp = true;
            break;

        case 'u':
            missingPingCount = atoi(optarg);
            break;

        case 's':
            pingDelay = atoi(optarg);
            break;

        case 't':
            syncClock = true;
            break;

        case 'r':
            bypassRootCheck = true;
            break;

        case 'h':
        default:
            usage(argv[0]);
            return false;
        }
    }

    synce_log_set_level(logLevel);

    if (!password.empty()) {
        int i;
        char *p;

        /* Protect password */
        for (i = 0; i < argc; i++) {
            if (argv[i] == password) {
                p = argv[i];
                if (*p) {
                    *p = 'X';
                    p++;

                    for (; *p; p++)
                        *p = '\0';
                }
                break;
            }
        }
    }

    return true;
}


/*!
    \fn CmdLineArgs::isDaemon()
 */
bool CmdLineArgs::isDaemon()
{
    return _isDaemon;
}


/*!
    \fn CmdLineArgs::useIp()
 */
bool CmdLineArgs::useIp()
{
    return _useIp;
}


/*!
    \fn CmdLineArgs::getMissingPingCount()
 */
int CmdLineArgs::getMissingPingCount()
{
    return missingPingCount;
}


/*!
    \fn CmdLineArgs::getPassword()
 */
string CmdLineArgs::getPassword()
{
    return password;
}


/*!
    \fn CmdLineArgs::getPingDelay()
 */
int CmdLineArgs::getPingDelay()
{
    return pingDelay;
}


bool CmdLineArgs::getSyncClock()
{
    return syncClock;
}


int CmdLineArgs::getLogLevel()
{
    return logLevel;
}


bool CmdLineArgs::getBypassRootCheck()
{
    return bypassRootCheck;
}
