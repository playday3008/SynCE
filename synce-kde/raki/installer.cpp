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

#include <string>
#include <qstringlist.h>
#include <kapplication.h>
#include <kcombobox.h>
#include <kio/netaccess.h>
#include <kdebug.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

Installer* Installer::self = NULL;
QStringList Installer::installFiles;
QDict<PDA> *Installer::pdaList = NULL;
bool Installer::ready = true;
std::string storage = "/Storage";

Installer::Installer(QWidget *parent, QDict<PDA> *pdaList)
        : InstallDialog(parent)
{
    this->pdaList = pdaList;
    self = this;
}


Installer::~Installer()
{
}


void Installer::runInstaller(KURL destUrl)
{
    PDA *pda = (PDA *) pdaList->find(destUrl.host());

    if (pda != NULL) {
        synce::PROCESS_INFORMATION info = {0, 0, 0, 0 };

        if (Ce::rapiInit(destUrl.host())) {
            if (!Ce::createProcess(QString("wceload.exe").ucs2(), NULL,
                           NULL, NULL, false, 0, NULL, NULL, NULL, &info)) {}
            Ce::rapiUninit();
        }
    }
    
    ready = true;
}


void Installer::deleteResult(KIO::Job *deleteJob)
{
    if (deleteJob->error()) {
        deleteJob->showErrorDialog((QWidget *) parent());
    }

    KURL::List destUrls = ((KIO::DeleteJob *) deleteJob)->urls();

    PDA *pda = (PDA *) pdaList->find(destUrls.first().host());

    if (pda != NULL) {
        if (pda->getNumberOfCopyJobs() == 0) {
            runInstaller(destUrls.first());
        }
    }
    
    ready = true;
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
            RakiWorkerThread::sleep(1);
            deleteFiles(copiedFiles);
        } else {
            pda->unregisterCopyJob((KIO::CopyJob *) copyJob);
            if (pda->getNumberOfCopyJobs() == 0) {
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
        dest = KURL("rapip://" + pdaName + storage + "/Windows/AppMgr/Install/" +
                from.fileName());
        pda->addURLByCopyJob((KIO::CopyJob *) job, dest);
    }
}


void Installer::prepareInstall(QStringList installFiles)
{
    self->pdas->clear();
    Installer::installFiles = installFiles;

    QDictIterator<PDA> it(*pdaList);

    for (; it.current(); ++it ) {
        self->pdas->insertItem((*it)->getName());
    }
}


void Installer::installReal(Installer *installer, QString pdaName)
{ 
    KURL::List ul;
    QStringList::Iterator slit;

    for (slit = installFiles.begin(); slit != installFiles.end(); ++slit) {
        if ((*slit).lower().endsWith(".cab")) {
            ul.append(*slit);
        }
    }

    PDA *pda = (PDA *) pdaList->find(pdaName);

    if (pda != NULL && !ul.empty()) {
        bool mkdirSuccess = true;


#if KDE_VERSION < KDE_MAKE_VERSION(3,2,0) // KDE-3.1
        if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Storage/Windows/AppMgr")) {
            storage = "";
        }
        if (!KIO::NetAccess::exists("rapip://" + pdaName + storage +
                "/Windows/AppMgr/Install")) {
            if (!KIO::NetAccess::exists("rapip://" + pdaName + storage +
                    "/Windows/AppMgr")) {                
                mkdirSuccess = KIO::NetAccess::mkdir("rapip://" + pdaName + storage +
                        "/Windows/AppMgr");
            }
            if (mkdirSuccess) {
                mkdirSuccess = KIO::NetAccess::mkdir("rapip://" + pdaName + storage +
                        "/Windows/AppMgr/Install");
            }
        }
#else // KDE-3.2
        if (!KIO::NetAccess::exists("rapip://" + pdaName + "/Storage/Windows/AppMgr", false, NULL)) {
            storage = "";
        }
        if (!KIO::NetAccess::exists("rapip://" + pdaName + storage +
                "/Windows/AppMgr/Install", false, NULL)) {
            if (!KIO::NetAccess::exists("rapip://" + pdaName + storage +
                    "/Windows/AppMgr", false, NULL)) {
                mkdirSuccess = KIO::NetAccess::mkdir("rapip://" + pdaName + storage +
                        "/Windows/AppMgr", (QWidget *) NULL);
            }
            if (mkdirSuccess) {
                mkdirSuccess = KIO::NetAccess::mkdir("rapip://" + pdaName + storage +
                        "/Windows/AppMgr/Install", (QWidget *) NULL);
            }
        }
#endif

        if (mkdirSuccess) {
            KIO::CopyJob *copyJob = KIO::copy(ul, KURL("rapip://" + pdaName + storage +
                    "/Windows/AppMgr/Install/"), true);
            connect(copyJob, SIGNAL(result(KIO::Job *)), installer,
                    SLOT(copyResult(KIO::Job *)));
            connect(copyJob, SIGNAL(copying(KIO::Job *, const KURL&,
                    const KURL&)), installer, SLOT(procFiles (KIO::Job *,
                    const KURL&, const KURL&)));
            pda->registerCopyJob(copyJob);
        }
    }

    self->close();
} 


void Installer::install(QString pdaName, QStringList installFiles, bool blocking)
{
    ready = false;
    prepareInstall(installFiles);
    installReal(self, pdaName);
    if (blocking) {
        while (!ready) {
            kapp->processEvents();
        }
    }
}


void Installer::install()
{
    QString pdaName = pdas->currentText();
    installReal(this, pdaName);
}


void Installer::show(QStringList installFiles)
{
    prepareInstall(installFiles);
    InstallDialog::show();
}
