/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#ifndef _INSTALLER_H_
#define _INSTALLER_H_

#include <unistd.h>
#include <qwidget.h>

#include <kurl.h>
#include <kio/job.h>

#include "workerthreadinterface.h"

/**
 * 
 * Volker Christian,,,
 **/
class RunInstallerThread : public WorkerThreadInterface
{
public:
    RunInstallerThread(QWidget *parent);
    ~RunInstallerThread();
    void work(QThread *th);

private:
    QWidget *parent;
};


class Installer : public QObject
{
    Q_OBJECT

public:
    Installer(QWidget *parent);
    virtual ~Installer();
    void installCabinetFile(KURL fileUrl);

protected slots:
    void runInstaller();
    void copyResult(KIO::Job *fileCopyJob);
    void deleteResult(KIO::Job *deleteJob);
    void deleteFile(KURL delFile);

private:
    RunInstallerThread *runInstallerThread;
    int installCounter;
    int installed;
    int currentInstalled;
};

#endif
