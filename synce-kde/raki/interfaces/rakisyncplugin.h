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

#ifndef SYNCPLUGIN_H
#define SYNCPLUGIN_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rra/syncmgr.h>

#include <kconfig.h>

#include <qobject.h>
#include <qstring.h>
#include <qevent.h>
#include <qwidget.h>


/**
@author Volker Christian,,,
*/

class SyncThread;
class SyncTaskListItem;
class Rra;

class RakiSyncPlugin : public QObject
{
    Q_OBJECT
    public:
        enum SyncContext {
            ASYNCHRONOUS = 1,
            SYNCHRONOUS = 2
        };
public:
    RakiSyncPlugin();
    virtual ~RakiSyncPlugin();

    bool doSync(SyncThread *syncThread, bool firstSynchronize, uint32_t partnerId);
    virtual bool preSync(QWidget *parent, bool firstSynchronize, uint32_t partnerId);
    virtual bool postSync(QWidget *parent, bool firstSynchronize, uint32_t partnerId);
    uint32_t getObjectTypeId();
    bool running();
    bool stopRequested();
    void incTotalSteps(int inc, bool directCall = false);
    void decTotalSteps(int dec, bool directCall = false);
    void advanceProgress(bool directCall = false);
    void setTotalSteps(int steps, bool directCall = false);
    void setProgress(int progress, bool directCall = false);
    void setTask(const char *task, bool directCall = false);
    virtual void init(Rra *rra, SyncTaskListItem *progressItem, QString pdaName, QWidget *parent,
            QString serviceName);
    virtual void unInit();
    virtual void createConfigureObject(KConfig *ksConfig);
    virtual void configure();
    QString serviceName();
    void install(QString cabFileName);
    QStringList extractWithOrange(QString selfInstaller, QString dest = "");
    virtual int syncContext();

private:
    virtual bool sync() = 0;

protected:
    KConfig *ksConfig;
    QString pdaName;
    SyncTaskListItem *progressItem;
    Rra *rra;
    SyncThread *syncThread;
    uint32_t partnerId;
    bool firstSynchronize;
    QWidget *parent;
    QString _serviceName;
};

#endif
