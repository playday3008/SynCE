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

#ifndef MANAGERIMPL_H
#define MANAGERIMPL_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "management.h"
#include "workerthreadinterface.h"

#include <rapi.h>

class PDA;

/**
@author Volker Christian,,,
*/

class ManagerImpl : public Manager, public WorkerThreadInterface
{    
public:
    ManagerImpl(QString pdaName, QWidget *parent, const char* name = 0, bool modal = FALSE, WFlags fl = 0);
    ~ManagerImpl();
    void closeEvent(QCloseEvent *e);
    
private slots:
    void uninstallSoftwareSlot();
    void refreshSystemInfoSlot();
    void refreshSoftwareSlot();
    void refreshBatteryStatusSlot();

private:
    struct sysinfo_s {
        synce::CEOSVERSIONINFO version;
        synce::SYSTEM_INFO system;
        synce::STORE_INFORMATION store;
        synce::SYSTEM_POWER_STATUS_EX power;
    };
  
    void uninstallSoftware(QThread *qt = NULL, void *data = NULL);
    void fetchSystemInfo(QThread *qt = NULL, void *data = NULL);
    void fetchBatteryStatus(QThread *qt = NULL, void *data = NULL);
    void fetchSoftwareList(QThread *qt = NULL, void *data = NULL);
    void *beginEvent(void *data = NULL);
    void *endEvent(void *data = NULL);
    void *insertInstalledItemEvent(void *data);
    void *systemInfoEvent(void *data);
    void *batInfoEvent(void *data);
    void *uninstalledEvent(void *data);
    QString pdaName;
};

#endif
