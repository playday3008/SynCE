/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


void Manager::tabChanged( QWidget *widget )
{
    RakiWorkerThread::rakiWorkerThread->stop();

    if (widget == sysInfo) {
        refreshSystemInfoSlot();
    } else if (widget == pwStatus) {
        refreshBatteryStatusSlot();
    } else if (widget == swManager) {
        refreshSoftwareSlot();
    }
}


void Manager::stopSoftware()
{
    RakiWorkerThread::rakiWorkerThread->stop();
}


void Manager::rejectSoftware()
{
    stopSoftware();
    reject();
}


void Manager::uninstallButtonEnabled()
{
    uninstallButton->setEnabled(true);
}


void Manager::refreshSystemInfoSlot()
{

}


void Manager::refreshBatteryStatusSlot()
{

}


void Manager::refreshSoftwareSlot()
{

}


void Manager::uninstallSoftwareSlot()
{

}
