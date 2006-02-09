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


#include "agsyncconfigimpl.h"

#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <klineedit.h>
#include <kdebug.h>
#include <AGBase64.h>

AGSyncConfigImpl::AGSyncConfigImpl(KConfig *ksConfig, QWidget* parent, const char* name, bool modal, WFlags fl)
        : AGSyncConfig(parent, name, modal, fl)
{
    this->ksConfig = ksConfig;
    userConfig = AGUserConfigNew();
    AGUserConfigInit(userConfig);
    serverList->setColumnWidthMode(0, QListView::Manual);
    serverList->setColumnWidthMode(1, QListView::Manual);
    serverList->setFullWidth(true);
    serverConfigDialog = new ServerConfigImpl(this, "ServerConfig", true);
    connect(serverConfigDialog, SIGNAL(newServer(QString, int, QString, QString)), this,
            SLOT(newServer(QString, int, QString, QString)));
    connect(serverConfigDialog, SIGNAL(modifiedServer(QString, int, QString, QString)), this,
            SLOT(modifiedServer(QString, int, QString, QString)));
    connect(serverConfigDialog, SIGNAL(cancelClicked()), this, SLOT(serverDialogCancel()));
    readConfig();
    readServerList();
}


AGSyncConfigImpl::~AGSyncConfigImpl()
{
    delete serverConfigDialog;

    AGUserConfigFree(userConfig);
    AGUserConfigFree(agreedConfig);
}


void AGSyncConfigImpl::contentChanged()
{
    buttonCancel->setEnabled(true);
}


void AGSyncConfigImpl::reject()
{
    QDialog::reject();

    AGUserConfigFree(userConfig);
    userConfig = AGUserConfigNew();
    AGUserConfigInit(userConfig);

    readConfig();
    readServerList();
}


void AGSyncConfigImpl::addServerButton_clicked()
{
    serverConfigDialog->add();
}


void AGSyncConfigImpl::modifyServerButton_clicked()
{
    currentItem = (ServerCheckListItem *) serverList->currentItem();

    if (currentItem != NULL) {
        QString hostName = currentItem->text(0);
        ushort hostPort = currentItem->text(1).toUShort();
        QString userName = currentItem->serverConfig->userName;
        serverConfigDialog->modify(hostName, hostPort, userName, "");
    }
}


void AGSyncConfigImpl::deleteServer(ServerCheckListItem *cli)
{
    if (cli != NULL) {
        serverList->takeItem(cli);
        AGUserConfigRemoveServer(userConfig, cli->serverConfig->uid);
        delete cli;
        contentChanged();
    }
}


void AGSyncConfigImpl::deleteServerButton_clicked()
{
    deleteServer((ServerCheckListItem *) serverList->currentItem());
}


void AGSyncConfigImpl::updateServerList()
{
    serverList->clear();
    int cnt = AGUserConfigCount(userConfig);

    for(int i = 0; i < cnt; i++) {
        AGServerConfig *serverConfig = AGUserConfigGetServerByIndex(userConfig, i);
        ServerCheckListItem *cli =
                new ServerCheckListItem(serverList, serverConfig->serverName);
        cli->setText(1, QString::number(serverConfig->serverPort));
        cli->serverConfig = serverConfig;
        cli->setOn(!serverConfig->disabled);
        connect(cli, SIGNAL(stateChanged(bool)), this, SLOT(contentChanged()));
    }
    serverList->update();
}


void AGSyncConfigImpl::newServer(QString hostName, int port, QString userName, QString passWord)
{
    AGServerConfig *serverConfig = AGServerConfigNew();
    AGServerConfigInit(serverConfig);
    serverConfig->serverName = qstrdup(hostName.ascii());
    serverConfig->serverPort = port;
    serverConfig->userName = qstrdup(userName.ascii());
    AGServerConfigChangePassword(serverConfig, (char *) passWord.ascii());
    AGUserConfigAddServer(userConfig, serverConfig, false);
    serverConfig->resetCookie = true;
    serverConfig->notRemovable = false;
    updateServerList();
    contentChanged();
}


void AGSyncConfigImpl::modifiedServer(QString hostName, int port, QString userName, QString passWord)
{
    currentItem->setText(0, hostName);
    currentItem->setText(1, QString::number(port));
    delete[] currentItem->serverConfig->serverName;
    delete[] currentItem->serverConfig->userName;
    currentItem->serverConfig->serverName = qstrdup(hostName.ascii());
    currentItem->serverConfig->serverPort = QString::number(port).toUShort();
    currentItem->serverConfig->userName = qstrdup(userName.ascii());
    AGServerConfigChangePassword(
            currentItem->serverConfig, (char *)passWord.ascii());
    currentItem->serverConfig->disabled = !currentItem->isOn();
    serverList->update();
    contentChanged();
}


void AGSyncConfigImpl::serverDialogCancel()
{
}


void AGSyncConfigImpl::accept()
{
    QDialog::accept();
    writeConfig();
}


void AGSyncConfigImpl::writeConfig()
{
    ksConfig->setGroup("AGSync Synchronizer");
    ksConfig->writeEntry("HttpProxyUserName", userName->text());
    ksConfig->writeEntry("HttpProxyPassword", passWord->text());
    ksConfig->writeEntry("HttpProxyHost", httpProxyHost->text());
    ksConfig->writeEntry("HttpProxyPort", httpProxyPort->text());
    ksConfig->writeEntry("SocksProxyHost", socksProxyHost->text());
    ksConfig->writeEntry("SocksProxyPort", socksProxyPort->text());
    ksConfig->writeEntry("SocksProxyActive", socksProxy->isChecked());
    ksConfig->writeEntry("HttpProxyActive", httpProxy->isChecked());
    ksConfig->writeEntry("UseAuthentication", useAuthentication->isChecked());
    ksConfig->writeEntry("InstallAGClient", installClientCheckbox->isChecked());
    ksConfig->sync();
    writeServerList();
}


void AGSyncConfigImpl::readConfig()
{
    ksConfig->setGroup("AGSync Synchronizer");
    userName->setText(ksConfig->readEntry("HttpProxyUserName"));
    passWord->setText(ksConfig->readEntry("HttpProxyPassword"));
    httpProxyHost->setText(ksConfig->readEntry("HttpProxyHost"));
    httpProxyPort->setText(ksConfig->readEntry("HttpProxyPort"));
    socksProxyHost->setText(ksConfig->readEntry("SocksProxyHost"));
    socksProxyPort->setText(ksConfig->readEntry("SocksProxyPort"));
    socksProxy->setChecked(ksConfig->readBoolEntry("SocksProxyActive"));
    httpProxy->setChecked(ksConfig->readBoolEntry("HttpProxyActive"));
    if (!socksProxy->isChecked() && !httpProxy->isChecked()) {
        noProxy->setChecked(true);
    }
    useAuthentication->setChecked(ksConfig->readBoolEntry("UseAuthentication"));
    installClientCheckbox->setChecked(
            ksConfig->readBoolEntry("InstallAGClient", true));
    buttonCancel->setEnabled(false);
}


void AGSyncConfigImpl::writeServerList()
{
    QListViewItemIterator it(serverList);
    int serverCount = 0;
    while (it.current()) {
        ServerCheckListItem *scli = (ServerCheckListItem *) it.current();
        serverCount++;
        ksConfig->setGroup("AGSyncServer-" + QString::number(serverCount));
        ksConfig->writeEntry("ServerName", scli->serverConfig->serverName);
        ksConfig->writeEntry("ServerPort", scli->serverConfig->serverPort);
        ksConfig->writeEntry("UserName", scli->serverConfig->userName);
        ksConfig->writeEntry("Disabled", scli->serverConfig->disabled);
        ksConfig->writeEntry("ServerUID", scli->serverConfig->uid);
        ksConfig->writeEntry("ResetCookie", scli->serverConfig->resetCookie);
        ksConfig->writeEntry("NotRemovable", scli->serverConfig->notRemovable);
        ++it;
    }
    ksConfig->setGroup("AGSyncServer");
    ksConfig->writeEntry("ServerCount", serverCount);
    ksConfig->sync();
}


void AGSyncConfigImpl::readServerList()
{
    ksConfig->setGroup("AGSyncServer");
    int serverCount = ksConfig->readEntry("ServerCount").toInt();
    for (int i = 1; i <= serverCount; i++) {
        ksConfig->setGroup("AGSyncServer-" + QString::number(i));
        AGServerConfig *serverConfig = AGServerConfigNew();
        AGServerConfigInit(serverConfig);
        serverConfig->serverName = qstrdup(ksConfig->readEntry("ServerName").ascii()); // don't use qstrdup here
        serverConfig->serverPort = ksConfig->readEntry("ServerPort").toUShort();
        serverConfig->userName = qstrdup(ksConfig->readEntry("UserName").ascii()); // don't use qstrdup here
        AGServerConfigChangePassword(serverConfig, (char *) "");
        serverConfig->uid = ksConfig->readEntry("ServerUID").toInt();
        serverConfig->disabled = ksConfig->readBoolEntry("Disabled");
        serverConfig->resetCookie = ksConfig->readBoolEntry("ResetCookie");
        serverConfig->notRemovable = ksConfig->readBoolEntry("NotRemovable");
        AGUserConfigAddServer(userConfig, serverConfig, false);
    }
    agreedConfig = AGUserConfigDup(userConfig);
    updateServerList();
}


bool AGSyncConfigImpl::installClient()
{
    return installClientCheckbox->isChecked();
}


void AGSyncConfigImpl::resetInstallClient()
{
    installClientCheckbox->setChecked(false);
    accept();
}


AGUserConfig *AGSyncConfigImpl::getUserConfig()
{
    return userConfig;
}


AGUserConfig *AGSyncConfigImpl::getAgreedConfig()
{
    return agreedConfig;
}


void AGSyncConfigImpl::setUserConfig(AGUserConfig *userConfig)
{
    AGUserConfigFree(this->userConfig);
    this->userConfig = AGUserConfigDup(userConfig);
    AGUserConfigFree(agreedConfig);
    agreedConfig = AGUserConfigDup(userConfig);
    updateServerList();
    writeConfig();
}


QString AGSyncConfigImpl::getHttpProxyHost()
{
    return httpProxyHost->text();
}


unsigned int AGSyncConfigImpl::getHttpProxyPort()
{
    return QString(httpProxyPort->text()).toUInt();
}


QString AGSyncConfigImpl::getHttpUsername()
{
    return userName->text();
}


QString AGSyncConfigImpl::getHttpPassword()
{
    return passWord->text();
}


QString AGSyncConfigImpl::getSocksProxyHost()
{
    return socksProxyHost->text();
}


unsigned int AGSyncConfigImpl::getSocksProxyPort()
{
    return QString(socksProxyPort->text()).toUInt();
}


bool AGSyncConfigImpl::getHttpProxy()
{
    return httpProxy->isChecked();
}


bool AGSyncConfigImpl::getSocksProxy()
{
    return socksProxy->isChecked();
}

bool AGSyncConfigImpl::getUseAuthentication()
{
    return useAuthentication->isChecked();
}


void AGSyncConfigImpl::show()
{
    AGSyncConfig::show();
}
