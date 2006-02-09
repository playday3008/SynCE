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

#ifndef SYNCEKONNECTORBASE_H
#define SYNCEKONNECTORBASE_H

#include <konnector.h>
#include <rra.h>

/**
@author Christian Fremgen; Volker Christian
*/

#define CONTACTS    0x01
#define EVENTS      0x02
#define TODOS       0x04

namespace KSync
{
    class SynCEKonnectorBase : public Konnector
    {
        public:
            SynCEKonnectorBase( const KConfig* p_config );

            ~SynCEKonnectorBase();

            virtual void actualSyncType( int type ) = 0;

            virtual void subscribeTo( Rra* rra, int type );

            virtual void init( const QString& pdaName, const QString &pairUid ) {
                setPdaName(pdaName);
                this->pairUid = pairUid;
            };

            void setPdaName(const QString &pdaName) {
                this->pdaName = pdaName;
            }

            QString getPdaName() {
                return pdaName;
            };

            QString getPairUid() {
                return pairUid;
            };

            virtual void unsubscribeFrom( int type );

        private:
            QString pdaName;
            QString pairUid;
    };
}
#endif
