/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


void Manager::init()
{
    statusLine->setText("Ready");
}

static const char* version_string(CEOSVERSIONINFO* version)
{
    const char* result = "Unknown";

    if (version->dwMajorVersion == 3 &&
            version->dwMinorVersion == 0) {
        switch (version->dwBuildNumber) {
        case 9348:	result = "Rapier: Pocket PC"; break;
        case 11171:	result = "Merlin: Pocket PC 2002"; break;
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


void Manager::tabChanged( QWidget *widget )
{
    RakiWorkerThread::rakiWorkerThread->stop();

    if (widget == sysInfo) {
        refreshSystemInfo();
    } else if (widget == pwStatus) {
        refreshBatteryStatus();
    } else if (widget == swManager) {
        refreshSoftware();
    }
}


void Manager::stopSoftware()
{
    RakiWorkerThread::rakiWorkerThread->stop();
}


void Manager::uninstallSoftware()
{
    QListBoxItem *item = softwareList->selectedItem();
    
    if (item != NULL) {
        uninstallButton->setEnabled(false);
        RakiWorkerThread::rakiWorkerThread->stop();
        RakiWorkerThread::rakiWorkerThread->start(this, item, RakiWorkerThread::SOFTWARE_UNINSTALLER);
    }
}


void Manager::refreshSystemInfo()
{ 
    major->clear();
    minor->clear();
    build->clear();
    sysName->clear();
    id->clear();
    platform->clear();
    details->clear();
    architecture->clear();
    type->clear();
    pageSize->clear();
    storageSize->clear();
    freeSpace->clear();
    RakiWorkerThread::rakiWorkerThread->start(this, RakiWorkerThread::SYSINFO_FETCHER);
}


void Manager::refreshSoftware()
{
    softwareList->clear();
    uninstallButton->setDisabled(true);
    RakiWorkerThread::rakiWorkerThread->start(this, RakiWorkerThread::SOFTWARE_FETCHER);
}


void Manager::refreshBatteryStatus()
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
    RakiWorkerThread::rakiWorkerThread->start(this, RakiWorkerThread::BATTERYSTATUS_FETCHER);
}


void Manager::customEvent (QCustomEvent *e)
{
    struct RakiWorkerThread::sysinfo_s sysinfo;
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
            architecture->setText(QString::number(sysinfo.system.wProcessorArchitecture) + " " +
                                  QString((sysinfo.system.wProcessorArchitecture <
                                           PROCESSOR_ARCHITECTURE_COUNT) ?
                                          architectures[sysinfo.system.wProcessorArchitecture] : "Unknown"));
            type->setText(QString::number(sysinfo.system.dwProcessorType) +" " +
                          QString(processor(sysinfo.system.dwProcessorType)));
            pageSize->setText("0x" + QString::number(sysinfo.system.dwAllocationGranularity, 16));
            storageSize->setText(QString::number(sysinfo.store.dwStoreSize) + " B  (" +
                                 QString::number(sysinfo.store.dwStoreSize / (1024 * 1024)) + " MB)");
            freeSpace->setText(QString::number(sysinfo.store.dwFreeSize) + " B  (" +
                               QString::number(sysinfo.store.dwFreeSize / (1024 * 1024)) + " MB)");

            break;
        case RakiEvent::BATINFO:
            sysinfo = event->getSysinfo();
            bat1Flag->setText(QString::number(sysinfo.power.BatteryFlag) + " (" + get_battery_flag_string(sysinfo.power.BatteryFlag) + ")");
            bat1LifePer->setText((BATTERY_PERCENTAGE_UNKNOWN == sysinfo.power.BatteryLifePercent) ? QString("Unknown") : QString::number(sysinfo.power.BatteryLifePercent) + "%");
            bat1LifeTime->setText((BATTERY_LIFE_UNKNOWN == sysinfo.power.BatteryLifeTime) ? QString("Unknown") : QString::number(sysinfo.power.BatteryLifeTime));
            bat1FullLife->setText((BATTERY_LIFE_UNKNOWN == sysinfo.power.BatteryFullLifeTime) ? QString("Unknown") : QString::number(sysinfo.power.BatteryFullLifeTime));
            bat2Flag->setText(QString::number(sysinfo.power.BackupBatteryFlag) + " (" + get_battery_flag_string(sysinfo.power.BackupBatteryFlag) + ")");
            bat2LifePer->setText((BATTERY_PERCENTAGE_UNKNOWN == sysinfo.power.BackupBatteryLifePercent) ? QString("Unknown") : QString::number(sysinfo.power.BackupBatteryLifePercent) + "%");
            bat2LifeTime->setText((BATTERY_LIFE_UNKNOWN == sysinfo.power.BackupBatteryLifeTime) ? QString("Unknown") : QString::number(sysinfo.power.BackupBatteryLifeTime));
            bat2FullLife->setText((BATTERY_LIFE_UNKNOWN == sysinfo.power.BackupBatteryFullLifeTime) ? QString("Unknown") : QString::number(sysinfo.power.BackupBatteryFullLifeTime));
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


void Manager::rejectSoftware()
{
    stopSoftware();
    reject();
}


void Manager::closeEvent( QCloseEvent *e )
{
    stopSoftware();
    e->accept();
}


void Manager::uninstallButtonEnabled()
{
    uninstallButton->setEnabled(true);
}
