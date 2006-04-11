/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                    Christian Fremgen <cfremgen@users.sourceforge.net>   *
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

namespace KABC
{
    class ResourceFile;
}

namespace KSync
{

    class LocalKonnectorConfig;

    class SynCELocalKonnector : public KSync::SynCEKonnectorBase
    {
            Q_OBJECT
        public:
            SynCELocalKonnector( const KConfig *config );
            ~SynCELocalKonnector();

            void writeConfig( KConfig * );

            SynceeList syncees()
            {
                return mSyncees;
            }

            bool readSyncees();
            bool writeSyncees();

            bool connectDevice();
            bool disconnectDevice();

            /** the state and some informations */
            KSync::KonnectorInfo info() const;

            void setCalendarFile( const QString &f )
            {
                mCalendarFile = f;
            }
            QString calendarFile() const
            {
                return mCalendarFile;
            }

            void setAddressBookFile( const QString &f )
            {
                mAddressBookFile = f;
            }
            QString addressBookFile() const
            {
                return mAddressBookFile;
            }

            void setBookmarkFile( const QString &f )
            {
                mBookmarkFile = f;
            }
            QString bookmarkFile() const
            {
                return mBookmarkFile;
            }

            virtual void actualSyncType( int type );

            QStringList supportedFilterTypes() const
            {
                QStringList types;
                types << "addressbook" << "calendar";

                return types;
            };
            void setPdaName( const QString& pdaName );
            void setPairUid( const QString &pairUid );

        private:
            void clearDataStructures();

            LocalKonnectorConfig *mConfigWidget;
            QString mCalendarFile;
            QString mAddressBookFile;
            QString mBookmarkFile;

            QString mMd5sumEvent;
            QString mMd5sumTodo;
            QString mMd5sumAbk;

            KCal::CalendarLocal mCalendar;
            KABC::AddressBook mAddressBook;
            KABC::ResourceFile *mAddressBookResourceFile;

            KSync::AddressBookSyncee *mAddressBookSyncee;
            KSync::EventSyncee *mEventSyncee;
            KSync::TodoSyncee *mTodoSyncee;

            SynceeList mSyncees;

            int _actualSyncType;
    };

}

#endif
