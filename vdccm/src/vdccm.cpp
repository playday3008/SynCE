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

#include "dccmserver.h"
#include "rapiserver.h"
#include "windowscedevicefactory.h"
#include "synceclientfactory.h"
#include "rapihandshakeclientfactory.h"
#include "rapiprovisioningclientfactory.h"
#include "devicemanager.h"
#include "multiplexer.h"
#include "cmdlineargs.h"
#include "utils.h"
#include <localserversocket.h>
#include <synce_log.h>
#include <iostream>
#include <wait.h>

using namespace synce;
using namespace std;

int main(int argc, char *argv[])
{
    Utils::dropRootPrivileg();

    if (!CmdLineArgs::parseArgs(argc, argv)) {
        exit(0);
    }

    Utils::acquireRootPrivileg();

    if (!Utils::checkStartingUser(CmdLineArgs::getBypassRootCheck())) {
        synce_error("Could not start - either because vdccm is not installed suid or you start directly as root");
        exit(0);
    }

    Utils::dropRootPrivileg();

    if (Utils::alreadyRunning()) {
        exit(1);
    }

    Utils::setupSignals();

    Multiplexer* mux = Multiplexer::self();

    Utils::acquireRootPrivileg();

    RapiServer rapiServer(new RapiHandshakeClientFactory(), new RapiProvisioningClientFactory(), 990);

    if (!rapiServer.listen()) {
        synce_error("Could not switch RapiServer to listening mode - rapi");
        exit(1);
    }

    Utils::dropRootPrivileg();

    if (!mux->getReadManager()->add(&rapiServer)) {
        synce_error("Could not add RapiServer to manager - rapi");
        rapiServer.shutdown();
        delete mux;
        exit(1);
    }

    DeviceManager *deviceManager = DeviceManager::self();

    if (!mux->getTimerNodeManager()->add(deviceManager)) {
        synce_error("Could not add deviceManager to manager");
        deviceManager->shutdown();
        delete deviceManager;
        delete mux;
        exit(1);
    }

    DccmServer dccmServer(new WindowsCEDeviceFactory());

    if(!dccmServer.listen()) {
        synce_error("Could not switch DccmServer to listening mode");
        deviceManager->shutdown();
        delete deviceManager;
        delete mux;
        exit(1);
    }

    if (!dccmServer.setNonBlocking()) {
        synce_warning("Could not switch DccmServer to nonblocking io - it would remain blocking");
    }

    if (!mux->getReadManager()->add(&dccmServer)) {
        synce_error("Could not add DccmServer to manager");
        dccmServer.shutdown();
        deviceManager->shutdown();
        delete deviceManager;
        delete mux;
        exit(1);
    }

    char *path;

    if (!synce_get_directory(&path)) {
        dccmServer.shutdown();
        deviceManager->shutdown();
        delete deviceManager;
        delete mux;
        exit(1);
    }

    string controlSocketPath = string(path) + "/" + "csock";
    free(path);

    LocalServerSocket localServer(new SynCEClientFactory(), controlSocketPath.c_str());

    if (!localServer.listen()) {
        synce_warning("Could not switch LocalServerSocket to listening mode");
        dccmServer.shutdown();
        deviceManager->shutdown();
        delete deviceManager;
        delete mux;
        exit(1);
    }

    if (!localServer.setNonBlocking()) {
        synce_warning("Could not switch LocalServerSocket socket to nonblocking io - it would remain blocking");
    }

    if (!mux->getReadManager()->add(&localServer)) {
        synce_error("Could not add LocalServerSocket to manager");
        dccmServer.shutdown();
        deviceManager->shutdown();
        delete deviceManager;
        delete mux;
        exit(1);
    }

    if (CmdLineArgs::isDaemon()) {
        daemon(0, 0);
        if (!Utils::writePidFile()) {
            exit(1);
        }
    }

    Utils::runScripts("start");

    while (Utils::isRunning()) {
        mux->multiplex();
        if (Utils::disconnectDevices()) {
            deviceManager->shutdownDevices();
        }
    }

    mux->getReadManager()->remove(&dccmServer);
    mux->getReadManager()->remove(&localServer);
    mux->getReadManager()->remove(&rapiServer);

    mux->getTimerNodeManager()->remove(deviceManager);

    dccmServer.shutdown();
    localServer.shutdown();
    rapiServer.shutdown();

    deviceManager->shutdown();

    delete deviceManager;
    delete mux;

    Utils::removePidFile();

    Utils::runScripts("stop");

    return 0;
}
