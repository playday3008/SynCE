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

#ifndef SYNCTASKLISTITEM_H
#define SYNCTASKLISTITEM_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kpopupmenu.h>
#include <ktrader.h>
#include <klibloader.h>
#include <qlistview.h>
#include <qlabel.h>
#include <qstring.h>

#include <rapi.h>

/**
@author Volker Christian,,,
*/

class KProgress;
class PdaConfigDialogImpl;
class WorkerThreadInterface;
class Rra;
typedef struct _ObjectType ObjectType;

class SyncTaskListItem : public QObject, public QCheckListItem
{
    Q_OBJECT
    
public:
    SyncTaskListItem(ObjectType* objectType, QListView* listView, const QString& name, Type tt, QString pdaName, uint32_t partnerId);
    void undo();
    void makePersistent();
    bool isOn();
    void setOn(bool state);
    ObjectType* getObjectType();
    QString getObjectTypeName();
    void setObjectType(ObjectType *objectType);
    void setTotalSteps(int totalSteps);
    void setProgress(int progress);
    void advance(int offset);
    int totalSteps();
    void setTaskLabel(QString task);
    QWidget *widget();
    QWidget *taskLabel();
    KTrader::OfferList getOffers();
    void openPopup(PdaConfigDialogImpl *pdaConfig);
    QString getPreferedOffer();
    QString getPreferedLibrary();  
    void setPreferedOffer(QString preferedOffer);
    void setPreferedLibrary(QString preferedLibrary);
    bool synchronize(WorkerThreadInterface *workerThread, Rra *rra);

private slots:
    void clickedMenu(int item);
    
protected:
    virtual void stateChange(bool state);

private:
    ObjectType *objectType;
    bool isOnStore;
    KProgress *progress;
    QLabel *taskLabelWidget;
    KPopupMenu itemMenu;
    KTrader::OfferList offers;
    QString preferedOffer;
    QString preferedLibrary;
    QString preferedOfferTemp;
    QString preferedLibraryTemp;
    PdaConfigDialogImpl *pdaConfig;
    QString pdaName;
    uint32_t partnerId;
    
signals:
    void stateChanged(bool state);
};

#endif
