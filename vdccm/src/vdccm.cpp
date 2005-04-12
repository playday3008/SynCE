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
#include "localserver.h"
#include "dccmserver.h"
#include "devicemanager.h"
#include "multiplexer.h"
#include "cmdlineargs.h"
#include "utils.h"
#include <synce.h>
#include <synce_log.h>
#include <iostream>

using namespace synce;

int main(int argc, char *argv[])
{
    if (!CmdLineArgs::parseArgs(argc, argv)) {
        exit(0);
    }

    if (Utils::alreadyRunning()) {
        exit(1);
    }

    if (!Utils::writePidFile()) {
        exit(1);
    }

    Utils::setupSignals();

    Multiplexer* mux = Multiplexer::self();

    DeviceManager deviceManager;

    if (!mux->getTimerNodeManager()->add(&deviceManager)) {
        synce_error("Could not add deviceManager to manager");
        deviceManager.shutdown();
        delete mux;
        exit(1);
    }

    DccmServer dccmServer(&deviceManager);

    if(!dccmServer.listen()) {
        synce_error("Could not switch to listening mode");
        deviceManager.shutdown();
        delete mux;
        exit(1);
    }

    if (!dccmServer.setNonBlocking()) {
        synce_warning("Could not switch local server socket to nonblocking io - it would remain blocking");
    }

    if (!mux->getReadManager()->add(&dccmServer)) {
        synce_error("Could not add dccmServer to manager");
        dccmServer.shutdown();
        deviceManager.shutdown();
        delete mux;
        exit(1);
    }

    char *path;

    if (!synce_get_directory(&path)) {
        dccmServer.shutdown();
        deviceManager.shutdown();
        delete mux;
        exit(1);
    }

    string controlSocketPath = string(path) + "/" + "csock";
    free(path);

    LocalServer localServer(controlSocketPath.c_str(), &deviceManager);

    if (!localServer.listen()) {
        dccmServer.shutdown();
        deviceManager.shutdown();
        delete mux;
        exit(1);
    }

    if (!localServer.setNonBlocking()) {
        synce_warning("Could not switch local server socket to nonblocking io - it would remain blocking");
    }

    if (!mux->getReadManager()->add(&localServer)) {
        synce_error("Could not add localServer to manager");
        dccmServer.shutdown();
        deviceManager.shutdown();
        delete mux;
        exit(1);
    }

    if (CmdLineArgs::isDaemon()) {
        daemon(0, 0);
    }

    Utils::runScripts("start");

    while (Utils::isRunning()) {
        mux->multiplex();
        if (Utils::disconnectDevices()) {
            deviceManager.shutdownDevices();
        }
    }

    Utils::runScripts("stop");

    mux->getReadManager()->remove(&dccmServer);
    mux->getReadManager()->remove(&localServer);
    mux->getTimerNodeManager()->remove(&deviceManager);

    dccmServer.shutdown();
    localServer.shutdown();
    deviceManager.shutdown();

    delete mux;

    Utils::removePidFile();

    return 0;
}