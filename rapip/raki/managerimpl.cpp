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

#include "managerimpl.h"
#include "rapiwrapper.h"

#include <qapplication.h>
#include <qcstring.h>
#include <klineedit.h>
#include <kled.h>
#include <kpushbutton.h>
#include <klistbox.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

static const char* version_string(synce::CEOSVERSIONINFO* version)
{
    const char* result = "Unknown";

    if (version->dwMajorVersion == 4 &&
            version->dwMinorVersion == 20 &&
            version->dwBuildNumber == 1081) {
        result = "Ozone: Pocket PC 2003";
    } else if (version->dwMajorVersion == 3 &&
            version->dwMinorVersion == 0) {
        switch (version->dwBuildNumber) {
        case 9348:
            result = "Rapier: Pocket PC"; break;
        case 11171:
            result = "Merlin: Pocket PC 2002"; break;
        }
    }

    return result;
}


static const char* processor(int n)
{
    const char* result;

    switch (n) {
    case PROCESSOR_STRONGARM:
        result = "StrongARM";
        break;
    case PROCESSOR_HITACHI_SH3:
        result = "SH3";
        break;
    default:
        result = "Unknown";
        break;
    }

    return result;
}


#define PROCESSOR_ARCHITECTURE_COUNT 8


static const char* architectures[] =
    {
        "Intel",
        "MIPS",
        "Alpha",
        "PPC",
        "SHX",
        "ARM",
        "IA64",
        "ALPHA64"
    };


static const char* get_battery_flag_string(unsigned flag)
{
    const char* name;

    switch (flag) {
    case BATTERY_FLAG_HIGH:
        name = "High";
        break;
    case BATTERY_FLAG_LOW:
        name = "Low";
        break;
    case BATTERY_FLAG_CRITICAL:
        name = "Critical";
        break;
    case BATTERY_FLAG_CHARGING:
        name = "Charging";
        break;
    case BATTERY_FLAG_NO_BATTERY:
        name = "NoBattery";
        break;
    default:
        name = "Unknown";
        break;
    }
    return name;
}


ManagerImpl::ManagerImpl(QString pdaName, QWidget *parent, const char* name, bool modal,
                         WFlags fl)
        : Manager(parent, name, modal, fl)
{
    statusLine->setText("Ready");
    this->pdaName = pdaName;
}


ManagerImpl::~ManagerImpl()
{}


void *ManagerImpl::beginEvent(void *data)
{
    statusLine->setText(QString((char *) data));
    refreshButton->setEnabled(false);
    powerRefreshButton->setEnabled(false);
    sysInfoRefreshButton->setEnabled(false);
    uninstallButton->setEnabled(false);
    stopButton->setEnabled(true);
    
    return NULL;
}


void *ManagerImpl::endEvent(void */*data*/)
{
    statusLine->setText("Ready");
    refreshButton->setEnabled(true);
    stopButton->setEnabled(false);
    powerRefreshButton->setEnabled(true);
    sysInfoRefreshButton->setEnabled(true);
    
    return NULL;
}


void *ManagerImpl::insertInstalledItemEvent(void *data)
{
    softwareList->insertItem(QString((char *) data));
    delete[] (char *) data;
    
    return NULL;
}


void *ManagerImpl::systemInfoEvent(void *data)
{
    struct sysinfo_s *sysinfo = (struct sysinfo_s *) data;
    QString platformString;
    QString detailsString;

    detailsString = QString::fromUcs2(sysinfo->version.szCSDVersion);
    if (VER_PLATFORM_WIN32_CE == sysinfo->version.dwPlatformId)
        platformString = "Windows CE";
    major->setText(QString::number(sysinfo->version.dwMajorVersion));
    minor->setText(QString::number(sysinfo->version.dwMinorVersion));
    build->setText(QString::number(sysinfo->version.dwBuildNumber));
    sysName->setText(QString(version_string(&sysinfo->version)));
    id->setText(QString::number(sysinfo->version.dwPlatformId));
    platform->setText(platformString);
    details->setText(detailsString);
    architecture->setText(
        QString::number(sysinfo->system.wProcessorArchitecture) +
        " " + QString((sysinfo->system.wProcessorArchitecture <
                       PROCESSOR_ARCHITECTURE_COUNT) ?
                       architectures[sysinfo->system.wProcessorArchitecture] :
                       "Unknown"));
    procType->setText(QString::number(sysinfo->system.dwProcessorType) +
                      " " + QString(processor(sysinfo->system.dwProcessorType)));
    pageSize->setText("0x" +
                      QString::number(sysinfo->system.dwAllocationGranularity, 16));
    storageSize->setText(QString::number(sysinfo->store.dwStoreSize) +
                         " B  (" + QString::number(sysinfo->store.dwStoreSize /
                                                   (1024 * 1024)) + " MB)");
    freeSpace->setText(QString::number(sysinfo->store.dwFreeSize) +
                       " B  (" + QString::number(sysinfo->store.dwFreeSize /
                                                 (1024 * 1024)) + " MB)");

    delete sysinfo;
    
    return NULL;
}


void *ManagerImpl::batInfoEvent(void *data)
{
    struct sysinfo_s *sysinfo = (struct sysinfo_s *) data;
    
    
    bat1Flag->setText(QString::number(sysinfo->power.BatteryFlag) + " (" +
                      get_battery_flag_string(sysinfo->power.BatteryFlag) + ")");
    bat1LifePer->setText((BATTERY_PERCENTAGE_UNKNOWN ==
                          sysinfo->power.BatteryLifePercent) ? QString("Unknown") :
                          QString::number(sysinfo->power.BatteryLifePercent) + "%");
    bat1LifeTime->setText((BATTERY_LIFE_UNKNOWN ==
                           sysinfo->power.BatteryLifeTime) ? QString("Unknown") :
                           QString::number(sysinfo->power.BatteryLifeTime));
    bat1FullLife->setText((BATTERY_LIFE_UNKNOWN ==
                           sysinfo->power.BatteryFullLifeTime) ? QString("Unknown") :
                           QString::number(sysinfo->power.BatteryFullLifeTime));
    bat2Flag->setText(QString::number(sysinfo->power.BackupBatteryFlag) + " (" +
                      get_battery_flag_string(sysinfo->power.BackupBatteryFlag) + ")");
    bat2LifePer->setText((BATTERY_PERCENTAGE_UNKNOWN ==
                          sysinfo->power.BackupBatteryLifePercent) ?
                          QString("Unknown") :
                          QString::number(sysinfo->power.BackupBatteryLifePercent) + "%");
    bat2LifeTime->setText((BATTERY_LIFE_UNKNOWN ==
                           sysinfo->power.BackupBatteryLifeTime) ? QString("Unknown") :
                           QString::number(sysinfo->power.BackupBatteryLifeTime));
    bat2FullLife->setText((BATTERY_LIFE_UNKNOWN ==
                           sysinfo->power.BackupBatteryFullLifeTime) ?
                           QString("Unknown") :
                           QString::number(sysinfo->power.BackupBatteryFullLifeTime));
    switch (sysinfo->power.ACLineStatus) {
    case AC_LINE_OFFLINE:
        online->off();
        offline->on();
        backup->off();
        invalid->off();
        break;
    case AC_LINE_ONLINE:
        online->on();
        offline->off();
        backup->off();
        invalid->off();
        break;
    case AC_LINE_BACKUP_POWER:
        online->off();
        offline->off();
        backup->on();
        invalid->off();
        break;
    case AC_LINE_UNKNOWN:
        online->off();
        offline->off();
        backup->off();
        invalid->on();
        break;
    default:
        break;
    }
    
    delete sysinfo;
    
    return NULL;
}


void *ManagerImpl::uninstalledEvent(void *data)
{
    QListBoxItem *item = (QListBoxItem *) data;
    
    softwareList->removeItem(softwareList->index(item));
    
    return NULL;
}


void ManagerImpl::closeEvent(QCloseEvent *e)
{
    stopSoftware();
    e->accept();
}


void ManagerImpl::uninstallSoftware(QThread */*qt*/, void */*data*/)
{
    synce::PROCESS_INFORMATION info = {0, 0, 0, 0 };

    QListBoxItem *item = softwareList->item(softwareList->currentItem());

    if (Ce::rapiInit(pdaName)) {
        postThreadEvent(&ManagerImpl::beginEvent, "Uninstalling software from the PDA ...", noBlock);

        if(Ce::createProcess(QString("unload.exe").ucs2(), QString(item->text()).ucs2(),
                             NULL, NULL, false, 0, NULL, NULL, NULL, &info)) {
            
            postThreadEvent(&ManagerImpl::uninstalledEvent, item, noBlock);
        }
        Ce::rapiUninit();

        postThreadEvent(&ManagerImpl::endEvent, NULL, noBlock);
    }

}


void ManagerImpl::fetchSystemInfo(QThread */*qt*/, void */*data*/)
{
    struct sysinfo_s *sysinfo = new sysinfo_s;

    if (Ce::rapiInit(pdaName)) {
        postThreadEvent(&ManagerImpl::beginEvent, "Retrieving system-info from the PDA ...", noBlock);

        Ce::getVersionEx(&sysinfo->version);
        Ce::getSystemInfo(&sysinfo->system);
        Ce::getStoreInformation(&sysinfo->store);

        postThreadEvent(&ManagerImpl::systemInfoEvent, sysinfo, noBlock);
        
        Ce::rapiUninit();

        postThreadEvent(&ManagerImpl::endEvent, NULL, noBlock);
    }
}


void ManagerImpl::fetchBatteryStatus(QThread */*qt*/, void */*data*/)
{
    struct sysinfo_s *sysinfo = new sysinfo_s;

    if (Ce::rapiInit(pdaName)) {
        postThreadEvent(&ManagerImpl::beginEvent, "Retrieving batery-info from the PDA ...", noBlock);

        Ce::getSystemPowerStatusEx(&sysinfo->power, false);
        
        postThreadEvent(&ManagerImpl::batInfoEvent, sysinfo, noBlock);
        
        Ce::rapiUninit();

        postThreadEvent(&ManagerImpl::endEvent, NULL, noBlock);
    }


}


void ManagerImpl::fetchSoftwareList(QThread */*qt*/, void */*data*/)
{
    LONG result;
    HKEY parent_key;
    DWORD i;

    if (Ce::rapiInit(pdaName)) {
        postThreadEvent(&ManagerImpl::beginEvent, "Retrieving software-list from the PDA ...", noBlock);

        result = synce::CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, QString("Software\\Apps").ucs2(), 0, 0,
                                       &parent_key);
        if (ERROR_SUCCESS == result) {
            for (i = 0; !stopRequested(); i++) {
                WCHAR wide_name[MAX_PATH];
                DWORD name_size = sizeof(wide_name);
                HKEY program_key;
                DWORD installed = 0;
                DWORD value_size = sizeof(installed);
                DWORD type;

                result = synce::CeRegEnumKeyEx(parent_key, i, wide_name, &name_size,
                                               NULL, NULL, NULL, NULL);

                if (ERROR_SUCCESS != result) {
                    break;
                }

                result = synce::CeRegOpenKeyEx(parent_key, wide_name, 0, 0,
                                               &program_key);

                if (ERROR_SUCCESS != result) {
                    continue;
                }

                result = synce::CeRegQueryValueEx(program_key, QString("Instl").ucs2(),
                                                  NULL, &type, (LPBYTE)&installed, &value_size);

                synce::CeRegCloseKey(program_key);

                if (ERROR_SUCCESS == result && installed) {
                    postThreadEvent(&ManagerImpl::insertInstalledItemEvent,
                                    qstrdup(QString::fromUcs2(wide_name).ascii()), noBlock);
                } else if (ERROR_SUCCESS != result) {
                    break;
                }
            }
            synce::CeRegCloseKey(parent_key);
        }
        Ce::rapiUninit();

        postThreadEvent(&ManagerImpl::endEvent, NULL, noBlock);
    }
}


void ManagerImpl::uninstallSoftwareSlot()
{
    uninstallButton->setEnabled(false);
    RakiWorkerThread::rakiWorkerThread->stop();
    startWorkerThread(this, &ManagerImpl::uninstallSoftware, NULL);
}


void ManagerImpl::refreshSystemInfoSlot()
{
    major->clear();
    minor->clear();
    build->clear();
    sysName->clear();
    id->clear();
    platform->clear();
    details->clear();
    architecture->clear();
    procType->clear();
    pageSize->clear();
    storageSize->clear();
    freeSpace->clear();
    RakiWorkerThread::rakiWorkerThread->stop();
    startWorkerThread(this, &ManagerImpl::fetchSystemInfo, NULL);
}


void ManagerImpl::refreshBatteryStatusSlot()
{
    bat1Flag->clear();
    bat1LifePer->clear();
    bat1LifeTime->clear();
    bat1FullLife->clear();
    bat2Flag->clear();
    bat2LifePer->clear();
    bat2LifeTime->clear();
    bat2FullLife->clear();
    online->off();
    offline->off();
    backup->off();
    invalid->off();
    RakiWorkerThread::rakiWorkerThread->stop();
    startWorkerThread(this, &ManagerImpl::fetchBatteryStatus, NULL);
}


void ManagerImpl::refreshSoftwareSlot()
{
    softwareList->clear();
    uninstallButton->setDisabled(true);
    RakiWorkerThread::rakiWorkerThread->stop();
    startWorkerThread(this, &ManagerImpl::fetchSoftwareList, NULL);
}
