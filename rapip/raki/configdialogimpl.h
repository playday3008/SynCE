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

#ifndef _CONFIGDIALOGIMPL_H_
#define _CONFIGDIALOGIMPL_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "configdialog.h"

#include <kconfig.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

/**
@author Volker Christian,,,
*/
 
class ConfigDialogImpl : public ConfigDialog
{
    Q_OBJECT
    
public:
    ConfigDialogImpl(QWidget* parent = 0, const char* name = 0,
            bool modal = FALSE, WFlags fl = 0);
    ~ConfigDialogImpl();
    QString getDccmPath();
    QString getIpTables();
    QString getConnectNotify();
    QString getDisconnectNotify();
    QString getPasswordRequestNotify();
    QString getPasswordRejectNotify();
    bool getStartDccm();
    bool configureValid();
    void checkRunningVersion();
    
private slots:
    void applySlot();
    void changedSlot();
    void pathChangedSlot();
    void disableApply();
    void copyResult(KIO::Job *job);
    void chmodResult(KIO::Job *job);
    
private:
    void readConfig();
    void updateFields();
    void writeConfig();
    
    KConfig *ksConfig;
    QString dccmPath;
    QString ipTables;
    QString connectNotify;
    QString disconnectNotify;
    QString passwordRequestNotify;
    QString passwordRejectNotify;
    QString rakiVersion;
    bool startDccm;
    bool dccmChanged;
    bool configurationValid;

signals:
    void restartDccm();
    void configError();
};

#endif
