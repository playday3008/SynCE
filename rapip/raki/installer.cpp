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

#include "installer.h"
#include "rapiwrapper.h"
#include "rakiworkerthread.h"
#include "pda.h"

#include <qstringlist.h>
#include <kapplication.h>
#include <kcombobox.h>
#include <kio/netaccess.h>
#include <kdebug.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

Installer::Installer(QWidget *parent, QDict<PDA> *pdaList)
        : InstallDialog(parent)
{
    this->pdaList = pdaList;
    runInstallerThread = new RunInstallerThread(parent);
}


Installer::~Installer()
{
    delete runInstallerThread;
}


void Installer::runInstaller(KURL destUrl)
{
    PDA *pda = (PDA *) pdaList->find(destUrl.host());

    if (pda != NULL) {
        RakiWorkerThread::rakiWorkerThread->stop();
        runInstallerThread->setPdaName(destUrl.host());
        startWorkerThread(runInstallerThread, &RunInstallerThread::work, NULL);
    }
}


void Installer::deleteResult(KIO::Job *deleteJob)
{
    if (deleteJob->error()) {
        deleteJob->showErrorDialog((QWidget *) parent());
    }

    KURL::List destUrls = ((KIO::DeleteJob *) deleteJob)->urls();

    PDA *pda = (PDA *) pdaList->find(destUrls.first().host());

    if (pda != NULL) {
        if (!pda->getNumberOfCopyJobs()) {
            runInstaller(destUrls.first());
        }
    }
}


void Installer::deleteFiles(KURL::List delFiles)
{
    KIO::DeleteJob *deleteJob = KIO::del (delFiles, true, false);
    connect(deleteJob, SIGNAL( result( KIO::Job *)),
            this, SLOT(deleteResult( KIO::Job *)));
}


void Installer::copyResult(KIO::Job *copyJob)
{
    KURL destUrl = ((KIO::CopyJob *) copyJob)->destURL();

    PDA *pda = (PDA *) pdaList->find(destUrl.host());

    if (pda != NULL) {
        KURL::List copiedFiles = pda->getURLListByCopyJob(
                                     (KIO::CopyJob *) copyJob);
        if (copyJob->error()) {
            copyJob->showErrorDialog((QWidget *) parent());
            pda->unregisterCopyJob((KIO::CopyJob *) copyJob);
            sleep(1);
            deleteFiles(copiedFiles);
        } else {
            pda->unregisterCopyJob((KIO::CopyJob *) copyJob);
            if (!pda->getNumberOfCopyJobs()) {
                runInstaller(destUrl);
            }
        }
    }
}


void Installer::procFiles(KIO::Job *job, const KURL& from, const KURL& to)
{
    QString pdaName = to.host();
    KURL dest;
    PDA *pda = (PDA *) pdaList->find(pdaName);

    if (pda != NULL) {
        dest = KURL("rapip://" + pdaName + "/Windows/AppMgr/Install/" + from.fileName());
        pda->addURLByCopyJob((KIO::CopyJob *) job, dest);
    }
}


void Installer::install()
{
    KURL::List ul;
    QStringList::Iterator slit;

    for (slit = installFiles.begin(); slit != installFiles.end(); ++slit) {
        if ((*slit).lower().endsWith(".cab")) {
            ul.append(*slit);
        }
    }

    QString pdaName = pdas->currentText();

    PDA *pda = (PDA *) pdaList->find(pdaName);

    if (pda != NULL && !ul.empty()) {
        bool mkdirSuccess = true;

        if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Windows/AppMgr/Install")) {
            if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Windows/AppMgr")) {
                mkdirSuccess = KIO::NetAccess::mkdir("rapip://" + pdaName + "/Windows/AppMgr");
            }
            if (mkdirSuccess) {
                mkdirSuccess = KIO::NetAccess::mkdir("rapip://" + pdaName + "/Windows/AppMgr/Install");
            }
        }

        if (mkdirSuccess) {
            KIO::CopyJob *copyJob = KIO::copy(ul, KURL("rapip://" + pdaName +
                                              "/Windows/AppMgr/Install/"), true);
            connect(copyJob, SIGNAL(result(KIO::Job *)), this,
                    SLOT(copyResult(KIO::Job *)));
            connect(copyJob, SIGNAL(copying(KIO::Job *, const KURL&, const KURL&)),
                    this, SLOT(procFiles (KIO::Job *, const KURL&, const KURL&)));
            pda->registerCopyJob(copyJob);
        }
    }

    close();
}


void Installer::show(QStringList installFiles)
{
    pdas->clear();
    this->installFiles = installFiles;

    QDictIterator<PDA> it(*pdaList);

    for (; it.current(); ++it ) {
        pdas->insertItem((*it)->getName());
    }

    InstallDialog::show();
}


RunInstallerThread::RunInstallerThread(QWidget *parent)
{
    this->parent = parent;
}


RunInstallerThread::~RunInstallerThread()
{}


void RunInstallerThread::setPdaName(QString pdaName)
{
    this->pdaName = pdaName;
}


void RunInstallerThread::work(QThread */*th*/, void */*data*/)
{
    synce::PROCESS_INFORMATION info = {0, 0, 0, 0 };

    if (Ce::rapiInit(pdaName)) {
        if (!Ce::createProcess(QString("wceload.exe").ucs2(), NULL,
                           NULL, NULL, false, 0, NULL, NULL, NULL, &info)) {}
        Ce::rapiUninit();
    }
}
