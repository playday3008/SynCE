/***************************************************************************
 * Copyright (c) 2005 Mirko Kohns  <Mirko.Kohns@KashmirEvolution.de>       *
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

#include "pimsync.h"

#ifdef WITH_PIMSYNC

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kapplication.h>
#include <kmessagebox.h>
#include <kio/netaccess.h>
#include "rapiwrapper.h"
#include <qcstring.h>
#include <qstringlist.h>

static RakiSyncPlugin *plugin;



PIMSync::PIMSync()
{
    plugin = this;
    printf("[PIMsync] is starting\n");
}


PIMSync::~PIMSync()
{
    printf("[PIMsync] is terminating\n");
}


void PIMSync::createConfigureObject( KConfig *ksConfig )
{
    printf("[PIMSync] creating Config Object!\n");
    configDialog = new PIMSyncConfigImpl( ksConfig, parent );
}


void PIMSync::configure()
{
    printf("[PIMSync] entering config\n");
    configDialog->show();
}


bool PIMSync::preSync( QWidget * /*parent*/, Rra * /*rra*/,
                       bool /*firstSynchronize*/, uint32_t /*partnerId*/ )
{
    bool ret = true;
    return ret;
}




bool PIMSync::sync()
{
    return true;
}
#endif
