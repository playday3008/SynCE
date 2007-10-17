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

#include "configdialogimpl.h"
#include "welcomedialogimpl.h"
#include "raki.h"

#include <synce.h>

#include <qcheckbox.h>
#include <klineedit.h>
#include <kpushbutton.h>
#include <kurlrequester.h>
#include <kapplication.h>
#include <kstandarddirs.h>
#include <kmessagebox.h>
#include <kurl.h>
#include <klocale.h>
#include <kdebug.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

ConfigDialogImpl::ConfigDialogImpl(QWidget* parent, const char* name,
        bool modal, WFlags fl) : ConfigDialog(parent, name, modal, fl)
{
    readConfig();
    updateFields();
    buttonApply->setEnabled(false);
    dccmChanged = false;
}


ConfigDialogImpl::~ConfigDialogImpl()
{}


void ConfigDialogImpl::chmodResult(KIO::Job *job)
{
    KIO::SimpleJob *sj = (KIO::SimpleJob *) job;

    if (sj->error()) {
        emit configError();
    } else {
        rakiVersion = VERSION;
        updateFields();
        writeConfig();
        emit restartDccm();
    }
}


void ConfigDialogImpl::copyResult(KIO::Job *job)
{
    KIO::FileCopyJob *fc = (KIO::FileCopyJob *) job;
    if (fc->error()) {
        KMessageBox::error(0,
                i18n("Could not copy a valid dccm.sh script.\n"
                "Error was: %1\nPlease check your installation and start again!").arg(job->errorString()));
        emit configError();
    } else {
        KIO::SimpleJob *sj =  KIO::chmod(fc->destURL(), 0755);
        connect(sj, SIGNAL(result(KIO::Job *)), this,
                SLOT(chmodResult(KIO::Job *)));
    }
}


void ConfigDialogImpl::checkRunningVersion()
{
    QString dccm;

    configurationValid = true;

    if (rakiVersion != VERSION) {
        WelcomeDialogImpl *welcomeDialog = new WelcomeDialogImpl(this,
                "WelcomeDialog", true);
        welcomeDialog->exec();
        if (welcomeDialog->result() == QDialog::Accepted) {
            if (welcomeDialog->getSelectedDccm() == WelcomeDialogImpl::VDCCM) {
                dccm = "vdccm";
            } else if (welcomeDialog->getSelectedDccm() ==
                    WelcomeDialogImpl::DCCM) {
                dccm = "dccm";
            } else {
                configurationValid = false;
                emit configError();
            }
            if (configurationValid) {
                dccmPath = dccm;
                KStandardDirs *dirs = KApplication::kApplication()->dirs();
                connectNotify = dirs->findResource("data", "raki/Infbeg.wav");
                disconnectNotify = dirs->findResource(
                        "data", "raki/Infend.wav");
                passwordRequestNotify = dirs->findResource(
                        "data", "raki/Infend.wav");
                passwordRejectNotify = dirs->findResource(
                        "data", "raki/Infbeg.wav");
                startDccm = true;
                KURL srcDccm = KURL(QString("file:" + dirs->findResource(
                        "data", "raki/scripts/" + dccm + ".sh")));
                char *destDir;
                synce::synce_get_script_directory(&destDir);
                KURL destDccm = KURL(QString("file:") + QString(destDir) +
                        "/" + "dccm.sh");
                kdDebug(2120) << "Copy from " << srcDccm.prettyURL() <<
                        " to " << destDccm.prettyURL() << endl;
                KIO::FileCopyJob *fc = KIO::file_copy (srcDccm, destDccm, -1,
                        true, false, false);
                connect(fc, SIGNAL(result(KIO::Job *)), this,
                        SLOT(copyResult(KIO::Job *)));
                
            }
        } else {
            emit configError();
        }
    } else {
        emit restartDccm();
    }
}


bool ConfigDialogImpl::configureValid()
{
    return configurationValid;
}


void ConfigDialogImpl::updateFields()
{
    startDccmCheckbox->setChecked(startDccm);
    dccmPathInput->setText(dccmPath);
    ipTablesInput->setText(ipTables);
    buttonApply->setDisabled(true);
    connectNotification->setURL(connectNotify);
    disconnectNotification->setURL(disconnectNotify);
    passwordRequestNotification->setURL(passwordRequestNotify);
    passwordRejectNotification->setURL(passwordRejectNotify);
    dccmChanged = false;
}


bool ConfigDialogImpl::getStartDccm()
{
    return startDccm;
}


QString ConfigDialogImpl::getDccmPath()
{
    return dccmPath;
}


QString ConfigDialogImpl::getIpTables()
{
    return ipTables;
}


QString ConfigDialogImpl::getConnectNotify()
{
    return connectNotify;
}


QString ConfigDialogImpl::getDisconnectNotify()
{
    return disconnectNotify;
}


QString ConfigDialogImpl::getPasswordRequestNotify()
{
    return passwordRequestNotify;
}


QString ConfigDialogImpl::getPasswordRejectNotify()
{
    return passwordRejectNotify;
}


void ConfigDialogImpl::writeConfig()
{
    ksConfig->setGroup("RAKI");
    ksConfig->writeEntry("Version", rakiVersion);
    ksConfig->setGroup("DCCM");
    ksConfig->writeEntry("StartDccm", startDccm);
    ksConfig->writeEntry("DccmPath", dccmPath);
    ksConfig->writeEntry("IpTables", ipTables);
    ksConfig->writeEntry("Connect", connectNotify);
    ksConfig->writeEntry("Disconnect", disconnectNotify);
    ksConfig->writeEntry("PasswordRequest", passwordRequestNotify);
    ksConfig->writeEntry("PasswordReject", passwordRejectNotify);

    ksConfig->sync();
}


void ConfigDialogImpl::readConfig()
{
    ksConfig=kapp->config();
    ksConfig->setGroup("RAKI");
    rakiVersion = ksConfig->readEntry("Version");
    ksConfig->setGroup("DCCM");
    startDccm = ksConfig->readBoolEntry("StartDccm");
    dccmPath = ksConfig->readEntry("DccmPath");
    ipTables = ksConfig->readEntry("IpTables");
    connectNotify = ksConfig->readEntry("Connect");
    disconnectNotify = ksConfig->readEntry("Disconnect");
    passwordRequestNotify = ksConfig->readEntry("PasswordRequest");
    passwordRejectNotify = ksConfig->readEntry("PasswordReject");

    if (dccmPath.isEmpty()) {
        dccmPath = "vdccm";
    }
    if (ipTables.isEmpty()) {
        ipTables = "/sbin/iptables";
    }
}


void ConfigDialogImpl::applySlot()
{
    if (buttonApply->isEnabled()) {
        this->dccmPath = dccmPathInput->text();
        startDccm = startDccmCheckbox->isChecked();
        ipTables = ipTablesInput->text();
        connectNotify = connectNotification->url();
        disconnectNotify = disconnectNotification->url();
        passwordRequestNotify = passwordRequestNotification->url();
        passwordRejectNotify = passwordRejectNotification->url();
        writeConfig();
        buttonApply->setDisabled(true);
        if (dccmChanged) {
            emit restartDccm();
            dccmChanged = false;
        }
    }
}


void ConfigDialogImpl::changedSlot()
{
    dccmChanged = true;
    buttonApply->setEnabled(true);
}


void ConfigDialogImpl::pathChangedSlot()
{
    buttonApply->setEnabled(true);
}


void ConfigDialogImpl::disableApply()
{
    updateFields();
}
