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
 
#include "kdeaddressbooksyncfactory.h"
#include "kdeaddressbooksync.h"
#include <qobject.h>


KdeAddressBookSyncFactory::KdeAddressBookSyncFactory(QObject *parent, const char *name)
 : RakiSyncFactory(parent, name)
{
}


KdeAddressBookSyncFactory::~KdeAddressBookSyncFactory()
{
}


QObject *KdeAddressBookSyncFactory::createObject (QObject */*parent*/, const char */*name*/, 
                    const char */*classname*/, const QStringList &/*args*/)
{
    return new KdeAddressBookSync;
}


extern "C" {
    void *init_librakikdeaddressbooksync()
    {
        return new KdeAddressBookSyncFactory();
    }
};
