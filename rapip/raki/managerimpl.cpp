/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <klineedit.h>
#include <kled.h>
#include <qcursor.h>
#include <kpushbutton.h>
#include "managerimpl.h"
#include "rapiwrapper.h"


static const char* version_string(CEOSVERSIONINFO* version)
{
    const char* result = "Unknown";

    if (version->dwMajorVersion == 3 &&
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


ManagerImpl::ManagerImpl(QWidget* parent, const char* name, bool modal, 
                         WFlags fl)
        : Manager(parent, name, modal, fl)
{
    statusLine->setText("Ready");
}


ManagerImpl::~ManagerImpl()
{}


void ManagerImpl::customEvent (QCustomEvent *e)
{
    struct WorkerThreadInterface::sysinfo_s sysinfo;
    QString platformString;
    QString detailsString;
    QListBoxItem *item;

    if (e->type() == QEvent::User) {
        RakiEvent *event = (RakiEvent *) e;
        switch (event->eventType())  {
        case RakiEvent::BEGIN:
            statusLine->setText(event->getMessage());
            QApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
            refreshButton->setEnabled(false);
            powerRefreshButton->setEnabled(false);
            sysInfoRefreshButton->setEnabled(false);
            uninstallButton->setEnabled(false);
            stopButton->setEnabled(true);
            break;
        case RakiEvent::END:
            statusLine->setText("Ready");
            QApplication::setOverrideCursor( QCursor(Qt::ArrowCursor) );
            refreshButton->setEnabled(true);
            stopButton->setEnabled(false);
            powerRefreshButton->setEnabled(true);
            sysInfoRefreshButton->setEnabled(true);
            break;
        case RakiEvent::SYSINFO:
            sysinfo = event->getSysinfo();
            detailsString = synce::wstr_to_ascii(sysinfo.version.szCSDVersion);
            if (VER_PLATFORM_WIN32_CE == sysinfo.version.dwPlatformId)
                platformString = "Windows CE";
            major->setText(QString::number(sysinfo.version.dwMajorVersion));
            minor->setText(QString::number(sysinfo.version.dwMinorVersion));
            build->setText(QString::number(sysinfo.version.dwBuildNumber));
            sysName->setText(QString(version_string(&sysinfo.version)));
            id->setText(QString::number(sysinfo.version.dwPlatformId));
            platform->setText(platformString);
            details->setText(detailsString);
            architecture->setText(
                    QString::number(sysinfo.system.wProcessorArchitecture) + 
                    " " + QString((sysinfo.system.wProcessorArchitecture <
                    PROCESSOR_ARCHITECTURE_COUNT) ?
                    architectures[sysinfo.system.wProcessorArchitecture] : 
                    "Unknown"));
            procType->setText(QString::number(sysinfo.system.dwProcessorType) +
                    " " + QString(processor(sysinfo.system.dwProcessorType)));
            pageSize->setText("0x" + 
                    QString::number(sysinfo.system.dwAllocationGranularity, 16));
            storageSize->setText(QString::number(sysinfo.store.dwStoreSize) + 
                    " B  (" + QString::number(sysinfo.store.dwStoreSize / 
                    (1024 * 1024)) + " MB)");
            freeSpace->setText(QString::number(sysinfo.store.dwFreeSize) + 
                    " B  (" + QString::number(sysinfo.store.dwFreeSize / 
                    (1024 * 1024)) + " MB)");
            break;
        case RakiEvent::BATINFO:
            sysinfo = event->getSysinfo();
            bat1Flag->setText(QString::number(sysinfo.power.BatteryFlag) + 
                    " (" +
                    get_battery_flag_string(sysinfo.power.BatteryFlag) + ")");
            bat1LifePer->setText((BATTERY_PERCENTAGE_UNKNOWN == 
                    sysinfo.power.BatteryLifePercent) ? QString("Unknown") : 
                    QString::number(sysinfo.power.BatteryLifePercent) + "%");
            bat1LifeTime->setText((BATTERY_LIFE_UNKNOWN == 
                    sysinfo.power.BatteryLifeTime) ? QString("Unknown") : 
                    QString::number(sysinfo.power.BatteryLifeTime));
            bat1FullLife->setText((BATTERY_LIFE_UNKNOWN == 
                    sysinfo.power.BatteryFullLifeTime) ? QString("Unknown") : 
                    QString::number(sysinfo.power.BatteryFullLifeTime));
            bat2Flag->setText(QString::number(sysinfo.power.BackupBatteryFlag) +
                    " (" + 
                    get_battery_flag_string(sysinfo.power.BackupBatteryFlag) + 
                    ")");
            bat2LifePer->setText((BATTERY_PERCENTAGE_UNKNOWN == 
                    sysinfo.power.BackupBatteryLifePercent) ? 
                    QString("Unknown") : 
                    QString::number(sysinfo.power.BackupBatteryLifePercent) + 
                    "%");
            bat2LifeTime->setText((BATTERY_LIFE_UNKNOWN == 
                    sysinfo.power.BackupBatteryLifeTime) ? QString("Unknown") : 
                    QString::number(sysinfo.power.BackupBatteryLifeTime));
            bat2FullLife->setText((BATTERY_LIFE_UNKNOWN == 
                    sysinfo.power.BackupBatteryFullLifeTime) ? 
                    QString("Unknown") : 
                    QString::number(sysinfo.power.BackupBatteryFullLifeTime));
            switch (sysinfo.power.ACLineStatus) {
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
            break;
        case RakiEvent::UNINSTALLED:
            item = event->getItem();
            softwareList->removeItem(softwareList->index(item));
            break;
        case RakiEvent::INSTALLED:
            softwareList->insertItem(event->getMessage());
            break;
        case RakiEvent::ERROR:
        case RakiEvent::PROGRESS:
        case RakiEvent::FINISHED:
        default:
            break;
        }
    }
}


void ManagerImpl::closeEvent(QCloseEvent *e)
{
    stopSoftware();
    e->accept();
}


void ManagerImpl::uninstallSoftware()
{
    WCHAR* wide_program = NULL;
    WCHAR* wide_parameters = NULL;
    PROCESS_INFORMATION info = {0, 0, 0, 0 };
    
    QListBoxItem *item = softwareList->item(softwareList->currentItem());

    QApplication::postEvent(this, new RakiEvent(RakiEvent::BEGIN,
            "Uninstalling software-list from the PDA ..."));
    if (Ce::rapiInit()) {
        if ((wide_program = synce::wstr_from_ascii("unload.exe"))) {
            wide_parameters = synce::wstr_from_ascii(item->text());
            if(Ce::createProcess(wide_program, wide_parameters,
                    NULL,
                    NULL,
                    false,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    &info)) {
            QApplication::postEvent(this, new RakiEvent(RakiEvent::UNINSTALLED, 
                                    item));
            }
        }
    }

    Ce::rapiUninit();

    Ce::destroy_wpath(wide_program);
    Ce::destroy_wpath(wide_parameters);
    
    QApplication::postEvent(this, new RakiEvent(RakiEvent::END));
}


void ManagerImpl::fetchSystemInfo()
{
    struct sysinfo_s sysinfo;

    if (Ce::rapiInit()) {
        QApplication::postEvent(this, new RakiEvent(RakiEvent::BEGIN,
                                "Retrieving system-info from the PDA ..."));
        Ce::getVersionEx(&sysinfo.version);
        Ce::getSystemInfo(&sysinfo.system);
        Ce::getStoreInformation(&sysinfo.store);
        QApplication::postEvent(this, 
                                new RakiEvent(RakiEvent::SYSINFO, sysinfo));
    }

    QApplication::postEvent(this, new RakiEvent(RakiEvent::END));

    Ce::rapiUninit();
}


void ManagerImpl::fetchBatteryStatus()
{
    struct sysinfo_s sysinfo;

    if (Ce::rapiInit()) {
        QApplication::postEvent(this, new RakiEvent(RakiEvent::BEGIN,
                                "Retrieving batery-info from the PDA ..."));
        Ce::getSystemPowerStatusEx(&sysinfo.power, false);
        QApplication::postEvent(this, 
                                new RakiEvent(RakiEvent::BATINFO, sysinfo));
    }

    QApplication::postEvent(this, new RakiEvent(RakiEvent::END));

    Ce::rapiUninit();
}


void ManagerImpl::fetchSoftwareList()
{
    LONG result;
    HKEY parent_key;
    WCHAR* parent_key_name = NULL;
    WCHAR* value_name = NULL;
    DWORD i;

    if (Ce::rapiInit()) {
        QApplication::postEvent(this, new RakiEvent(RakiEvent::BEGIN,
                                "Retrieving software-list from the PDA ..."));
        value_name       = synce::wstr_from_ascii("Instl");
        parent_key_name  = synce::wstr_from_ascii("Software\\Apps");
        result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, 
                                &parent_key);
        if (ERROR_SUCCESS == result) {
            for (i = 0; isRunning; i++) {
                WCHAR wide_name[MAX_PATH];
                DWORD name_size = sizeof(wide_name);
                HKEY program_key;
                DWORD installed = 0;
                DWORD value_size = sizeof(installed);

                result = CeRegEnumKeyEx(parent_key, i, wide_name, &name_size, 
                                        NULL, NULL, NULL, NULL);

                if (ERROR_SUCCESS != result) {
                    break;
                }

                result = CeRegOpenKeyEx(parent_key, wide_name, 0, 0, 
                                        &program_key);

                if (ERROR_SUCCESS != result) {
                    continue;
                }

                result = CeRegQueryValueEx(program_key, value_name, NULL, NULL,
                                           (LPBYTE)&installed, &value_size);

                if (ERROR_SUCCESS == result && installed) {
                    char* name = synce::wstr_to_ascii(wide_name);
                    QApplication::postEvent(this, 
                                            new RakiEvent(RakiEvent::INSTALLED, 
                                            name));
                    synce::wstr_free_string(name);
                }

                CeRegCloseKey(program_key);
            }
            CeRegCloseKey(parent_key);
        }
        QApplication::postEvent(this, new RakiEvent(RakiEvent::END));
    }
    Ce::rapiUninit();
}


void ManagerImpl::work(QThread */*qt*/)
{
    switch (type) {
    case SOFTWARE_FETCHER:
        fetchSoftwareList();
        break;
    case SYSINFO_FETCHER:
        fetchSystemInfo();
        break;
    case BATTERYSTATUS_FETCHER:
        fetchBatteryStatus();
        break;
    case SOFTWARE_INSTALLER:
        break;
    case SOFTWARE_UNINSTALLER:
        uninstallSoftware();
        break;
    }
}


void ManagerImpl::uninstallSoftwareSlot()
{
    uninstallButton->setEnabled(false);
    RakiWorkerThread::rakiWorkerThread->stop();
    RakiWorkerThread::rakiWorkerThread->start(this, 
                            WorkerThreadInterface::SOFTWARE_UNINSTALLER);
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
    RakiWorkerThread::rakiWorkerThread->start(this, 
                            WorkerThreadInterface::SYSINFO_FETCHER);
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
    RakiWorkerThread::rakiWorkerThread->start(this, 
                            WorkerThreadInterface::BATTERYSTATUS_FETCHER);
}


void ManagerImpl::refreshSoftwareSlot()
{
    softwareList->clear();
    uninstallButton->setDisabled(true);
    RakiWorkerThread::rakiWorkerThread->start(this, 
                            WorkerThreadInterface::SOFTWARE_FETCHER);
}
