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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rakisyncplugin.h"
#include "synctasklistitem.h"
#include "rakiapi.h"

#include <qapplication.h>
#include <qstringlist.h>
#include <kmessagebox.h>
#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>
#include <syncdialogimpl.h>

RakiSyncPlugin::RakiSyncPlugin()
{
    parent = NULL;
    syncDialogImpl = NULL;
}


RakiSyncPlugin::~RakiSyncPlugin()
{
    delete ksConfig;
}


bool RakiSyncPlugin::doSync(SyncDialogImpl *syncDialogImpl, bool firstSynchronize, uint32_t partnerId)
{
    this->firstSynchronize = firstSynchronize;
    this->partnerId = partnerId;
    this->syncDialogImpl = syncDialogImpl;
    return sync();
}


bool RakiSyncPlugin::preSync(bool /*firstSynchronize*/, uint32_t /*partnerId*/)
{
    KApplication::kApplication()->processEvents();
    return true;
}


bool RakiSyncPlugin::postSync(bool /*firstSynchronize*/, uint32_t /*partnerId*/)
{
    return true;
}


void RakiSyncPlugin::init(Rra *rra,
        SyncTaskListItem *progressItem, QString pdaName, QWidget *parent, QString serviceName)
{
    this->rra = rra;
    this->progressItem = progressItem;
    this->pdaName = pdaName;
    this->parent = parent;
    this->_serviceName = serviceName;
    ksConfig = new KConfig("raki/" + pdaName + ".cfg", false, false, "data");
    createConfigureObject(ksConfig);
}


void RakiSyncPlugin::unInit()
{
}


void RakiSyncPlugin::configure()
{
    KMessageBox::information(parent, "<b>" + _serviceName +
            "</b>: " + i18n("Nothing to configure."), QString(progressItem->getObjectType()->name2) + " " + pdaName);
}


void RakiSyncPlugin::createConfigureObject(KConfig */*ksConfig*/)
{
}


void RakiSyncPlugin::install(QString cabFileName)
{
    QStringList sl;
    sl.append(cabFileName);
    RakiApi::install(pdaName, sl, true);
}


QStringList RakiSyncPlugin::extractWithOrange(QString selfInstaller, QString dest)
{
    return RakiApi::extractWithOrange(selfInstaller, dest);
}


QString RakiSyncPlugin::serviceName()
{
    return _serviceName;
}


uint32_t RakiSyncPlugin::getObjectTypeId()
{
    return progressItem->getObjectType()->id;
}


bool RakiSyncPlugin::running()
{
    return syncDialogImpl->isRunning();
}


bool RakiSyncPlugin::stopRequested()
{
    return syncDialogImpl->stopRequested();
}


void RakiSyncPlugin::incTotalSteps(int inc)
{
    progressItem->setTotalSteps(progressItem->totalSteps() + inc);
}


void RakiSyncPlugin::decTotalSteps(int dec)
{
    progressItem->setTotalSteps(progressItem->totalSteps() - dec);
}


void RakiSyncPlugin::advanceProgress()
{
    progressItem->advance(1);
}


void RakiSyncPlugin::setTotalSteps(int steps)
{
    progressItem->setTotalSteps(steps);
}


void RakiSyncPlugin::setProgress(int progress)
{
    progressItem->setProgress(progress);
}


void RakiSyncPlugin::setTask(const char *task)
{
    progressItem->setTaskLabel(task);
}


int RakiSyncPlugin::syncContext()
{
    return ASYNCHRONOUS;
}
