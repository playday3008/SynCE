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

#ifndef AGSYNCCONFIGIMPL_H
#define AGSYNCCONFIGIMPL_H

#include "agsyncconfig.h"
#include "serverconfigimpl.h"

#include <kconfig.h>
#include <klistview.h>
#include <qvaluelist.h>
#include <AGServerConfig.h>
#include <AGUserConfig.h>


class ServerCheckListItem : public QObject, public QCheckListItem
{
Q_OBJECT
public:
    ServerCheckListItem(QListView *parent, const QString & text) :
        QCheckListItem(parent, text, QCheckListItem::CheckBox) {}
    AGServerConfig *serverConfig;

protected:
    virtual void stateChange(bool state) {
        serverConfig->disabled = !QCheckListItem::isOn();
        emit stateChanged(state);
    };
signals:
    void stateChanged(bool state);
};


class AGSyncConfigImpl : public AGSyncConfig
{
Q_OBJECT
public:
    AGSyncConfigImpl(KConfig *ksConfig, QWidget* parent = 0,
            const char* name = 0, bool modal = TRUE, WFlags fl = 0 );
    ~AGSyncConfigImpl();
    void show();
    QString getHttpProxyHost();
    unsigned int getHttpProxyPort();
    QString getHttpUsername();
    QString getHttpPassword();
    QString getSocksProxyHost();
    unsigned int getSocksProxyPort();
    bool getHttpProxy();
    bool getSocksProxy();
    bool getUseAuthentication();
    AGUserConfig *getUserConfig();
    AGUserConfig *getAgreedConfig();
    void setUserConfig(AGUserConfig *userConfig);
    bool installClient();
    void resetInstallClient();

private:
    KConfig *ksConfig;
    ServerConfigImpl *serverConfigDialog;
    ServerCheckListItem *currentItem;
    void readConfig();
    void writeConfig();
    void writeServerList();
    void deleteServer(ServerCheckListItem *cli);
    void readServerList();
    void updateServerList();
    AGUserConfig *userConfig;
    AGUserConfig *agreedConfig;


public slots:
    /*$PUBLIC_SLOTS$*/
    virtual void contentChanged();
    virtual void newServer(QString hostName, int port, QString userName, QString passWord);
    virtual void modifiedServer(QString hostName, int port, QString userName, QString passWord);
    virtual void serverDialogCancel();


protected:
    /*$PROTECTED_FUNCTIONS$*/

protected slots:
    /*$PROTECTED_SLOTS$*/
    virtual void reject();
    virtual void accept();
    virtual void addServerButton_clicked();
    virtual void modifyServerButton_clicked();
    virtual void deleteServerButton_clicked();
};

#endif

