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

#include "rakiapi.h"
#include "installer.h"
extern "C" {
    #include <liborange.h>
    #include <libunshield.h>
}
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <kio/netaccess.h>
#include <klocale.h>
#include <kdebug.h>
#include <qstring.h>


RakiApi::RakiApi()
{}


RakiApi::~RakiApi()
{}



void RakiApi::install( QString pdaName, QStringList installFiles, bool blocking )
{
    Installer::install( pdaName, installFiles, blocking );
}


#define BUFFER_SIZE   (256*1024)
#define FREE(ptr)       { if (ptr) { free(ptr); ptr = NULL; } }
#define FCLOSE(file)    if (file) { fclose(file); file = NULL; }


struct data_exchange
{
    QString output_directory;
    QStringList sl;
};


static bool callback(const char* filename, CabInfo* /*info*/, void* cookie )
{
    bool success = false;

    struct data_exchange *dataExchange = ( struct data_exchange * ) cookie;
    
    QString outputFilename = dataExchange->output_directory + QString("/") + QString(filename).section( '/', -1 );

    kdDebug(2120) << i18n( "squeezing out: %1").arg(outputFilename);

    if (KIO::NetAccess::file_copy(filename, outputFilename, -1, true)) {
        dataExchange->sl.append( QString( outputFilename ) );
        success = true;
    } else {
        kdDebug(2120) << i18n("Failed to copy from '%1' to '%2'").arg(filename).arg(outputFilename ) << endl;
    }

    return success;
}


QStringList RakiApi::extractWithOrange( QString selfInstaller, QString dest )
{
    struct data_exchange dataExchange;

    dataExchange.output_directory = dest;

    if ( !orange_squeeze_file( selfInstaller.ascii(), callback, &dataExchange ) ) {
        dataExchange.sl.clear();
    }

    return dataExchange.sl;
}

