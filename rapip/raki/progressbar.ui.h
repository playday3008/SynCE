/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename functions or slots use
** Qt Designer which will update this file, preserving your code. Create an
** init() function in place of a constructor, and a destroy() function in
** place of a destructor.
*****************************************************************************/


void Progress::setFileSize(size_t size)
{
    progressBar->setRange(0, size);
}


void Progress::setInstallFile(QString file)
{
    struct stat statBuff;
    
    this->file = file;
    fileName->setText(file);
    lstat(file, &statBuff);
    setFileSize(statBuff.st_size);
    progressBar->setProgress(0);
}


void Progress::installFile()
{
    RakiWorkerThread::rakiWorkerThread->stop();
    
    ((Raki *) parent())->setAcceptDrops(false);
    
    installButton->setDisabled(true);
   
    RakiWorkerThread::rakiWorkerThread->start(this, file, RakiWorkerThread::SOFTWARE_INSTALLER);
}



void Progress::showRequest()
{
    installButton->setEnabled(true);
    this->show();
}


void Progress::closeRequest()
{
    RakiWorkerThread::rakiWorkerThread->stop();
        
    if (RakiWorkerThread::rakiWorkerThread->running()) {
        RakiWorkerThread::rakiWorkerThread->wait();
    }
}


void Progress::customEvent( QCustomEvent *event )
{
    if (event->type() == QEvent::User) {
        RakiEvent *pEvent = (RakiEvent *) event;
        switch(pEvent->eventType()) {
        case RakiEvent::PROGRESS:
            progressBar->setProgress(pEvent->getProgress());
            update();
            break;
        case RakiEvent::FINISHED:
            this->close();
            ((Raki *) parent())->setAcceptDrops(true);
            break;
        case RakiEvent::ERROR:
        case RakiEvent::BEGIN:
        case RakiEvent::SYSINFO:
        case RakiEvent::BATINFO:
        case RakiEvent::END:
        default:
            break;
        }
    }
}
