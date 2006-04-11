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

#ifndef POCKETPCCOMMUNICATIONPIMHANDLER_H
#define POCKETPCCOMMUNICATIONPIMHANDLER_H

#include <rra.h>
#include <qstringlist.h>

namespace KPIM
{
    class ProgressItem;
}

namespace KSync
{
    class KonnectorUIDHelper;
}

namespace PocketPCCommunication
{
    /**
    This is the base class for pim data handlers like @ref AddressBookHandler or @ref CalendarHandler.
    It stores the common members and provides the interface to an abstract method.
    @author Christian Fremgen cfremgen@users.sourceforge.net
    */
    class PimHandler
    {
        public:
            PimHandler ();
            virtual ~PimHandler();
            uint32_t getTypeId();
            void setIds( struct Rra::ids ids );
            static void setProgressItem( KPIM::ProgressItem *progressItem );
            static void progressItemSetCompleted();
            static void setError( QString error );
            static void addErrorEntry(QString errorEntry);
            static QStringList getErrorEntries();
            static QString getError();
            static void resetError();
            void setUidHelper( KSync::KonnectorUIDHelper *mUidHelper );
            void setRra( Rra* rra );
            virtual bool init() = 0;

        protected:
            uint32_t mTypeId;
            struct Rra::ids ids;
            void setMaximumSteps( unsigned int maxSteps );
            void setActualSteps( unsigned int actSteps );
            void incrementSteps();
            void setStatus( QString status );
            void resetSteps();
            uint32_t getOriginalId ( const QString& p_id );
            QString m_pdaName;
            Rra* m_rra;
            static KPIM::ProgressItem *mProgressItem;
            static QString errorString;
            static QStringList errorEntries;
            unsigned int maxSteps;
            unsigned int actSteps;
            KSync::KonnectorUIDHelper *mUidHelper;
    };
}

#endif
