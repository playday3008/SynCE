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
#ifndef KDEADDRESSBOOKSYNC_H
#define KDEADDRESSBOOKSYNC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rakisyncplugin.h>

/**
@author Volker Christian,,,
*/

namespace KABC {
    class Addressee;
    class AddressBook;
    class ResourceFile;
    class Resource;
};

class KdeAddressBookSync : public RakiSyncPlugin
{
Q_OBJECT
public:
    KdeAddressBookSync();
    virtual ~KdeAddressBookSync();
    
private:
    bool sync();
    bool preSync(SyncThread *syncThread, Rra *rra, bool firstSynchronize, uint32_t partnerId);
    bool postSync(SyncThread *syncThread, Rra *rra, bool firstSynchronize, uint32_t partnerId);
    void generatePcDelta();         // Search with PCUid
    void generatePdaDelta();                                    // Download book
    void generateKappaBook();    // Search with CEUid
    void generatePdaItaBook();      // Search with PCUid
    void generatePcItaBook();       // Search with CEUid
    bool compare(const KABC::Addressee &a, const KABC::Addressee &b);
    void printBook(QString label, KABC::AddressBook *book);

    KABC::AddressBook *pcStdBook;
    KABC::AddressBook *pcRhoBook;
    KABC::Resource *rhoResource;    
    KABC::AddressBook * pcDeltaBook;
    KABC::AddressBook *pdaDeltaBook;
    KABC::AddressBook * kappaBook;
};

#endif
