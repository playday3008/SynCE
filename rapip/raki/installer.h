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

#ifndef _INSTALLER_H_
#define _INSTALLER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "installdialog.h"
#include "workerthreadinterface.h"

#include <qwidget.h>
#include <qdict.h>
#include <qthread.h>
#include <kurl.h>
#include <kio/job.h>

#include <unistd.h>

class PDA;

/**
@author Volker Christian,,,
*/
 
class RunInstallerThread : public WorkerThreadInterface
{
public:
    RunInstallerThread(QWidget *parent);
    ~RunInstallerThread();
    void work(QThread *th = NULL, void *data = NULL);
    void setPdaName(QString pdaName);

private:
    QWidget *parent;
    QString pdaName;
};


class Installer : public InstallDialog
{
    Q_OBJECT

public:
    Installer(QWidget *parent, QDict<PDA> *pdaList);
    virtual ~Installer();
    void show(QStringList installFiles);

protected slots:
    void runInstaller(KURL destUrl);
    void copyResult(KIO::Job *fileCopyJob);
    void deleteResult(KIO::Job *deleteJob);
    void deleteFiles(KURL::List& delFile);
    void install();
    void procFiles(KIO::Job *job, const KURL&, const KURL&);

private:   
    RunInstallerThread *runInstallerThread;
    QDict<PDA> *pdaList;
    QStringList installFiles;
};

#endif
