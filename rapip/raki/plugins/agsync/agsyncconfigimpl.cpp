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

AGSyncConfigImpl::AGSyncConfigImpl(KConfig *ksConfig, QWidget* parent, const char* name, bool modal, WFlags fl)
        : AGSyncConfig(parent, name, modal, fl)
{
    this->ksConfig = ksConfig;
    readConfig();
}


AGSyncConfigImpl::~AGSyncConfigImpl()
{}


void AGSyncConfigImpl::contentChanged()
{
    buttonOk->setEnabled(true);
}


void AGSyncConfigImpl::reject()
{
    QDialog::reject();
    readConfig();
}


void AGSyncConfigImpl::accept()
{
    QDialog::accept();
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
    ksConfig->sync();
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
    buttonOk->setEnabled(false);
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
    readConfig();
    AGSyncConfig::show();
}
