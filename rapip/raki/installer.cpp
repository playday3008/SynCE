/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <kapplication.h>
#include "installer.h"
#include "rapiwrapper.h"
#include "errorevent.h"
#include "rakiworkerthread.h"

Installer::Installer(QWidget *parent, const char */*name*/)
{
    this->parent = parent;
    currentInstalled = 0;
    installed = 0;
    installCounter = 0;
    runInstallerThread = new RunInstallerThread(parent);
}


Installer::~Installer()
{}


void Installer::runInstaller()
{
    if (currentInstalled) {
        currentInstalled = 0;
        RakiWorkerThread::rakiWorkerThread->stop();
        RakiWorkerThread::rakiWorkerThread->start(runInstallerThread);
    }
}


void Installer::deleteResult(KIO::Job *deleteJob)
{
    if ( deleteJob->error() ) {
        deleteJob->showErrorDialog(parent);
    }
    installed++;
    if (installed == installCounter) {
        runInstaller();
    }
}


void Installer::deleteFile(KURL delFile)
{
    KIO::SimpleJob *deleteJob = KIO::file_delete(delFile, false);
    connect(deleteJob, SIGNAL( result( KIO::Job *)),
            this, SLOT(deleteResult( KIO::Job *)));
}


void Installer::copyResult(KIO::Job *fileCopyJob)
{
    if ( fileCopyJob->error() ) {
        fileCopyJob->showErrorDialog(parent);
        KURL delUrl = ((KIO::FileCopyJob *)fileCopyJob)->destURL();
        sleep(1);
        deleteFile(delUrl);
    } else {
        installed++;
        currentInstalled++;
        if (installed == installCounter) {
            runInstaller();
        }
    }
}


void Installer::installCabinetFile(KURL fileUrl)
{
    installCounter++;
    KURL destUrl("rapip:///Windows/AppMgr/Install/synce-install-" +
                 QString::number(installCounter) + ".cab");

    KIO::FileCopyJob *fileCopyJob = KIO::file_copy (fileUrl, destUrl,-1, true, false, true);
    connect(fileCopyJob, SIGNAL( result( KIO::Job *)),
            this, SLOT(copyResult( KIO::Job *)));
}


RunInstallerThread::RunInstallerThread(QWidget *parent)
{
    this->parent = parent;
}


RunInstallerThread::~RunInstallerThread()
{}


void RunInstallerThread::work(QThread */*th*/)
{
    WCHAR* wPath = NULL;
    WCHAR* wPara = NULL;
    PROCESS_INFORMATION info = {0, 0, 0, 0 };

    if (Ce::rapiInit()) {
        QString qs = "wceload.exe";
        if ((wPath = Ce::wpath_from_upath(qs))) {
            if (!Ce::createProcess(wPath, wPara,
                                   NULL,
                                   NULL,
                                   false,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &info)) {
                QApplication::postEvent(parent,
                                        new ErrorEvent(ErrorEvent::REMOTE_FILE_EXECUTE_ERROR,
                                                       (void *) new QString(qs)));
            }
            Ce::destroy_wpath(wPath);
        } else {
            QApplication::postEvent(parent,
                                    new ErrorEvent(ErrorEvent::NO_FILENAME_ERROR,
                                                   (void *) new QString(qs)));
        }
        Ce::rapiUninit();
    }
}
