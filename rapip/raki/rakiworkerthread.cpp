/***************************************************************************
 *   Copyright (C) 2003 by Volker Christian,,,                             *
 *   voc@soft.uni-linz.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/

#include <rapi.h>
#include "rakiworkerthread.h"
#include "rakievent.h"
#include "errorevent.h"


RakiWorkerThread *RakiWorkerThread::rakiWorkerThread = new RakiWorkerThread();


RakiWorkerThread::RakiWorkerThread() : QThread()
{}


RakiWorkerThread::~RakiWorkerThread()
{}


void RakiWorkerThread::start(Manager *manager, enum RakiWorkerThread::threadType type)
{
    this->manager = manager;
    this->type = type;
    this->isRunning = true;
    QThread::start();
}


void RakiWorkerThread::start(Manager *manager, QListBoxItem *item, enum RakiWorkerThread::threadType type)
{
    this->manager = manager;
    this->type = type;
    this->item = item;
    this->isRunning = true;
    QThread::start();
}


void RakiWorkerThread::start(Progress *progress, QString fileName, enum RakiWorkerThread::threadType type)
{
    this->progress = progress;
    this->type = type;
    this->fileName = fileName;
    this->isRunning = true;
    QThread::start();
}


void RakiWorkerThread::stop()
{
    isRunning = false;
    wait();
}


void RakiWorkerThread::fetchSystemInfo()
{
    struct sysinfo_s sysinfo;

    if (Ce::rapiInit()) {
        QApplication::postEvent(manager, new RakiEvent(RakiEvent::BEGIN,
                                "Retrieving system-info from the PDA ..."));
        Ce::getVersionEx(&sysinfo.version);
        Ce::getSystemInfo(&sysinfo.system);
        Ce::getStoreInformation(&sysinfo.store);
        QApplication::postEvent(manager, new RakiEvent(RakiEvent::SYSINFO, sysinfo));
    }

    QApplication::postEvent(manager, new RakiEvent(RakiEvent::END));

    Ce::rapiUninit();
}


void RakiWorkerThread::fetchBatteryStatus()
{
    struct sysinfo_s sysinfo;

    if (Ce::rapiInit()) {
        QApplication::postEvent(manager, new RakiEvent(RakiEvent::BEGIN,
                                "Retrieving batery-info from the PDA ..."));
        Ce::getSystemPowerStatusEx(&sysinfo.power, false);
        QApplication::postEvent(manager, new RakiEvent(RakiEvent::BATINFO, sysinfo));
    }

    QApplication::postEvent(manager, new RakiEvent(RakiEvent::END));

    Ce::rapiUninit();
}


void RakiWorkerThread::fetchSoftwareList()
{
    LONG result;
    HKEY parent_key;
    WCHAR* parent_key_name = NULL;
    WCHAR* value_name = NULL;
    DWORD i;

    if (Ce::rapiInit()) {
        QApplication::postEvent(manager, new RakiEvent(RakiEvent::BEGIN,
                                "Retrieving software-list from the PDA ..."));
        value_name       = synce::wstr_from_ascii("Instl");
        parent_key_name  = synce::wstr_from_ascii("Software\\Apps");
        result = CeRegOpenKeyEx(HKEY_LOCAL_MACHINE, parent_key_name, 0, 0, &parent_key);
        if (ERROR_SUCCESS == result) {
            for (i = 0; isRunning; i++) {
                WCHAR wide_name[MAX_PATH];
                DWORD name_size = sizeof(wide_name);
                HKEY program_key;
                DWORD installed = 0;
                DWORD value_size = sizeof(installed);

                result = CeRegEnumKeyEx(parent_key, i, wide_name, &name_size, NULL, NULL, NULL, NULL);

                if (ERROR_SUCCESS != result) {
                    break;
                }

                result = CeRegOpenKeyEx(parent_key, wide_name, 0, 0, &program_key);

                if (ERROR_SUCCESS != result) {
                    continue;
                }

                result = CeRegQueryValueEx(program_key, value_name, NULL, NULL,
                                           (LPBYTE)&installed, &value_size);

                if (ERROR_SUCCESS == result && installed) {
                    char* name = synce::wstr_to_ascii(wide_name);
                    QApplication::postEvent(manager, new RakiEvent(RakiEvent::INSTALLED, name));
                    synce::wstr_free_string(name);
                    manager->update();
                }

                CeRegCloseKey(program_key);
            }
            CeRegCloseKey(parent_key);
        }
        QApplication::postEvent(manager, new RakiEvent(RakiEvent::END));
    }
    Ce::rapiUninit();
}


void RakiWorkerThread::installSoftware()
{
  WCHAR* wPath = NULL;
  WCHAR* wPara = NULL;
  PROCESS_INFORMATION info = {0, 0, 0, 0 };
  Ce::AnyFile copyFile;
  char buffer[Ce::ANYFILE_BUFFER_SIZE];
  size_t bytesRead = 0;
  size_t bytesWritten = 0;
  size_t bytesCopied = 0;

  if (Ce::rapiInit()) {
    QString qs("/Windows/AppMgr");
    wPath = Ce::wpath_from_upath(qs);
    Ce::createDirectory(wPath, NULL);
    Ce::destroy_wpath(wPath);

    qs = QString("/Windows/AppMgr/Install");
    wPath = Ce::wpath_from_upath(qs);
    Ce::createDirectory(wPath, NULL);
    Ce::destroy_wpath(wPath);

    if ((copyFile.local = fopen(fileName.ascii(), "r")) == NULL) {
      QApplication::postEvent(progress->parent(),
                              new ErrorEvent(ErrorEvent::LOCALE_FILE_OPEN_ERROR,
                                             (void *) new QString(fileName)));
      goto exit;
    }

    qs = QString("/Windows/AppMgr/Install/synce-install.cab");
    wPath = Ce::wpath_from_upath(qs);
    copyFile.remote = Ce::createFile(wPath, GENERIC_WRITE, 0, NULL,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    Ce::destroy_wpath(wPath);

    if (INVALID_HANDLE_VALUE == copyFile.remote) {
      QApplication::postEvent(progress->parent(),
                              new ErrorEvent(ErrorEvent::REMOTE_FILE_CREATE_ERROR,
                                             (void *) new QString(qs)));
      goto exit;
    }

    while(isRunning)  {
      bytesRead = fread(buffer, 1, Ce::ANYFILE_BUFFER_SIZE, copyFile.local);

      if (ferror(copyFile.local)) {
        QApplication::postEvent(progress->parent(),
                                new ErrorEvent(ErrorEvent::LOCALE_FILE_READ_ERROR,
                                               (void *) new QString(fileName)));
        goto exit;
      }

      if (bytesRead == 0) {
        break;
      }


      if (!Ce::writeFile(copyFile.remote, buffer, bytesRead, &bytesWritten, NULL)) {
        QApplication::postEvent(progress->parent(),
                                new ErrorEvent(ErrorEvent::REMOTE_FILE_WRITE_ERROR,
                                               (void *) new QString(qs)));
        goto exit;
      }

      if (bytesRead != bytesWritten) {
        QApplication::postEvent(progress->parent(),
                                new ErrorEvent(ErrorEvent::REMOTE_FILE_WRITE_ERROR,
                                               (void *) new QString(qs)));
        goto exit;
      }

      bytesCopied += bytesWritten;

      QApplication::postEvent(progress,
                              new RakiEvent(RakiEvent::PROGRESS, bytesCopied));
    }

    if (isRunning) {
      qs = "wceload.exe";
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
          QApplication::postEvent(progress->parent(),
                                  new ErrorEvent(ErrorEvent::REMOTE_FILE_EXECUTE_ERROR,
                                                 (void *) new QString(qs)));
        }
        Ce::destroy_wpath(wPath);
      } else {
        QApplication::postEvent(progress->parent(),
                                new ErrorEvent(ErrorEvent::NO_FILENAME_ERROR,
                                               (void *) new QString(qs)));
      }
    }

    Ce::destroy_wpath(wPara);

  exit:
    if (copyFile.local != NULL) {
      fclose(copyFile.local);
    }

    Ce::closeHandle(copyFile.remote);
  }

  Ce::rapiUninit();

  QApplication::postEvent(progress,
                          new RakiEvent(RakiEvent::FINISHED));
}


void RakiWorkerThread::uninstallSoftware()
{
    WCHAR* wide_program = NULL;
    WCHAR* wide_parameters = NULL;
    PROCESS_INFORMATION info = {0, 0, 0, 0 };
    
    
    QApplication::postEvent(manager, new RakiEvent(RakiEvent::BEGIN,
            "Uninstalling software-list from the PDA ..."));
    if (Ce::rapiInit()) {
        if ((wide_program = synce::wstr_from_ascii("unload.exe"))) {
            wide_parameters = synce::wstr_from_ascii(item->text());
            if(Ce::createProcess(wide_program, wide_parameters,
                    NULL,
                    NULL,
                    false,
                    0,
                    NULL,
                    NULL,
                    NULL,
                    &info)) {
            QApplication::postEvent(manager, new RakiEvent(RakiEvent::UNINSTALLED, item));
            }
        }
    }

    Ce::rapiUninit();

    Ce::destroy_wpath(wide_program);
    Ce::destroy_wpath(wide_parameters);
    
    QApplication::postEvent(manager, new RakiEvent(RakiEvent::END));
}


void RakiWorkerThread::run()
{
    switch (type) {
    case RakiWorkerThread::SOFTWARE_FETCHER:
        fetchSoftwareList();
        break;
    case RakiWorkerThread::SYSINFO_FETCHER:
        fetchSystemInfo();
        break;
    case RakiWorkerThread::BATTERYSTATUS_FETCHER:
        fetchBatteryStatus();
        break;
    case RakiWorkerThread::SOFTWARE_INSTALLER:
        installSoftware();
        break;
    case RakiWorkerThread::SOFTWARE_UNINSTALLER:
        uninstallSoftware();
        break;
    }
}
