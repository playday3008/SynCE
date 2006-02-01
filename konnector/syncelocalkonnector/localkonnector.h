/*
    This file is part of KitchenSync.

    Copyright (c) 2003,2004 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/
#ifndef KSYNC_LOCALKONNECTOR_H
#define KSYNC_LOCALKONNECTOR_H

#include <libkcal/calendarlocal.h>
#include <kabc/addressbook.h>
#include <kbookmarkmanager.h>

#include "todosyncee.h"
#include "eventsyncee.h"

//#include <konnector.h>
#include "syncekonnectorbase.h"
#include <qiconset.h>
#include <qptrlist.h>

namespace KABC {
class ResourceFile;
}

namespace KSync {

class LocalKonnectorConfig;

class LocalKonnector : public KSync::SynCEKonnectorBase
{
    Q_OBJECT
  public:
    LocalKonnector( const KConfig *config );
    ~LocalKonnector();

    void writeConfig( KConfig * );

    SynceeList syncees() { return mSyncees; }

    bool readSyncees();
    bool writeSyncees();

    bool connectDevice();
    bool disconnectDevice();

    /** the state and some informations */
    KSync::KonnectorInfo info() const;

    void setCalendarFile( const QString &f ) { mCalendarFile = f; }
    QString calendarFile() const { return mCalendarFile; }

    void setAddressBookFile( const QString &f ) { mAddressBookFile = f; }
    QString addressBookFile() const { return mAddressBookFile; }

    void setBookmarkFile( const QString &f ) { mBookmarkFile = f; }
    QString bookmarkFile() const { return mBookmarkFile; }

    virtual void actualSyncType(int type);

    QStringList supportedFilterTypes() const {
        QStringList types;
        types << "addressbook" << "calendar";

        return types;
    };

  private:
    LocalKonnectorConfig *mConfigWidget;
    QString mCalendarFile;
    QString mAddressBookFile;
    QString mBookmarkFile;

    QString mMd5sumEvent;
    QString mMd5sumTodo;
    QString mMd5sumAbk;
    QString mMd5sumBkm;

    KCal::CalendarLocal mCalendar;
    KCal::CalendarLocal mTodoCalendar;
    KCal::CalendarLocal mEventCalendar;
    KABC::AddressBook mAddressBook;
    KABC::ResourceFile *mAddressBookResourceFile;

    KSync::AddressBookSyncee *mAddressBookSyncee;
    KSync::EventSyncee *mEventSyncee;
    KSync::TodoSyncee *mTodoSyncee;

    class LocalBookmarkManager : public KBookmarkManager
    {
      public:
        LocalBookmarkManager() : KBookmarkManager() {}
    };
    LocalBookmarkManager mBookmarkManager;

    SynceeList mSyncees;

//    int subscribtions;
//    QString pdaName;
    int _actualSyncType;

//    KPIM::ProgressItem *mProgressItem;
};

}

#endif
