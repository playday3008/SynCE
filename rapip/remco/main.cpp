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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kglobal.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include "remco.h"

static const char *description = I18N_NOOP(
    "Raki, a PocketPC-Management Tool");

static const char *MITlicense = I18N_NOOP(
    "Copyright (c) 2003 Volker Christian\n"
    "\n"
    "Permission is hereby granted, free of charge, to\n"
    "any person obtaining a copy of this software and\n"
    "associated documentation files (the \"Software\"), to\n"
    "deal in the Software without restriction, including\n"
    "without limitation the rights to use, copy, modify,\n"
    "merge, publish, distribute, sublicense, and/or sell\n"
    "copies of the Software, and to permit persons to whom\n"
    "the Software is furnished to do so, subject to the\n"
    "following conditions:\n"
    "\n"
    "The above copyright notice and this permission notice\n"
    "shall be included in all copies or substantial portions\n"
    "of the Software.\n"
    "\n"
    "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF\n"
    "ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED\n"
    "TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A\n"
    "PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT\n"
    "SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR\n"
    "ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN\n"
    "ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT\n"
    "OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
    "OTHER DEALINGS IN THE SOFTWARE.");

static KCmdLineOptions options[] =
    {
        { 0, 0, 0 }
        // INSERT YOUR COMMANDLINE OPTIONS HERE
    };


int main(int argc, char *argv[])
{
    KAboutData aboutData("remco", I18N_NOOP("RemCo"), VERSION, description,
            KAboutData::License_Custom,
            "(c) 2003, Volker Christian (voc)", 0,
            "http://synce.sourceforge.net/synce/kde/",
            "voc@users.sourceforge.net");
    aboutData.addAuthor("Volker Christian", 0, "voc@users.sourceforge.net");
    aboutData.addCredit("Ludovic Lange",
            I18N_NOOP("is the Initiator of the SynCE-Project."),
            "llange@users.sourceforge.net");
    aboutData.addCredit("David Eriksson",
            I18N_NOOP("is the current Project Manager."),
            "twogood@users.sourceforge.net");
    aboutData.addCredit("Ganesh Varadarajan",
            I18N_NOOP("has developed the serial-over-USB driver."),
            "vganesh@users.sourceforge.net");
    aboutData.addCredit("Michael Jarrett",
            I18N_NOOP("ported the AvantGo support from Malsync to SynCE."),
            "JudgeBeavis@hotmail.com");
    aboutData.setLicenseText(MITlicense);

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );

    KApplication a;

    QString pdaName = "Volker";

    RemCo *remCo = new RemCo(pdaName, &aboutData, new KAboutApplication(&aboutData));

    a.setMainWidget(remCo);
//    remCo->connect (&a, SIGNAL (aboutToQuit()), remCo, SLOT (shutDown()));
    remCo->show();

    return a.exec();
}
