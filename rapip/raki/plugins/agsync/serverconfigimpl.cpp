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


#include "serverconfigimpl.h"
#include <qlineedit.h>
#include <qpushbutton.h>
#include <kdebug.h>

ServerConfigImpl::ServerConfigImpl(QWidget* parent, const char* name, bool modal, WFlags fl)
: serverSetting(parent,name, modal,fl)
{
}

ServerConfigImpl::~ServerConfigImpl()
{
}

void ServerConfigImpl::add()
{
    hostName->clear();
    hostPort->setText("80");
    userName->clear();
    passWord->clear();
    okButton->setDisabled(true);
    mode = 1;
    this->show();
}


void ServerConfigImpl::modify(QString hostNameS, ushort portS, QString userNameS, QString passWordS)
{
    hostName->setText(hostNameS);
    hostPort->setText(QString::number(portS));
    userName->setText(userNameS);
    passWord->setText(passWordS);
    okButton->setDisabled(true);
    mode = 2;
    this->show();
}
    

/*$SPECIALIZATION$*/
void ServerConfigImpl::okButton_clicked()
{
    if (mode == 1) {
        emit newServer(hostName->text(), hostPort->text().toInt(), userName->text(), passWord->text());
    } else if (mode == 2) {
        emit modifiedServer(hostName->text(), hostPort->text().toInt(), userName->text(), passWord->text());
    }
    this->hide();
}

void ServerConfigImpl::cancelButton_clicked()
{
    emit cancelClicked();
    this->hide();
}

void ServerConfigImpl::widgetChanged()
{
    okButton->setEnabled(true);
}



#include "serverconfigimpl.moc"

