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

#include <KAboutData>
#include <KApplication>
#include <KCmdLineArgs>
#include <KDebug>
#include "cescreen.h"

static const KLocalizedString description = ki18n(
    "PDAMirror, a PocketPC-Control Tool");

static const KLocalizedString MITlicense = ki18n(
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


int main(int argc, char *argv[])
{
    KAboutData aboutData(QByteArray("kcemirror"),
                         NULL,
                         ki18n("KCeMirror"),
                         QVariant(VERSION).toByteArray(),
                         description,
                         KAboutData::License_Custom,
                         ki18n("(c) 2003, Volker Christian (voc)"),
                         ki18n(""),
                         QByteArray("http://synce.sourceforge.net/synce/kde/"),
                         QByteArray("voc@users.sourceforge.net"));

    aboutData.addAuthor(ki18n("Volker Christian"),
                        ki18n("is the original author"),
                        QByteArray("voc@users.sourceforge.net"),
                        NULL);
    aboutData.addCredit(ki18n("Ludovic Lange"),
                        ki18n("is the Initiator of the SynCE-Project."),
                        QByteArray("llange@users.sourceforge.net"),
                        NULL);
    aboutData.addCredit(ki18n("David Eriksson"),
                        ki18n("is the current Project Manager."),
                        QByteArray("twogood@users.sourceforge.net"),
                        NULL);
    aboutData.addCredit(ki18n("Ganesh Varadarajan"),
                        ki18n("has developed the serial-over-USB driver."),
                        QByteArray("vganesh@users.sourceforge.net"),
                        NULL);
    aboutData.setLicenseText(MITlicense);

    KCmdLineOptions options;
    options.add(QByteArray("nosynce"), ki18n("the pdaname is an IP-Address/Hostname."), NULL);
    options.add(QByteArray("forceinstall"), ki18n("force the installation of the pda component"), "0");
    options.add(QByteArray("+pdaname"), ki18n("IP-Address/Hostname or SynCE-Name of the PDA"), "");

    KCmdLineArgs::init( argc, argv, &aboutData );
    KCmdLineArgs::addCmdLineOptions( options );
    KCmdLineArgs::addStdCmdLineOptions();

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    if (args->count() < 1)
            KCmdLineArgs::usage();

    QString pdaName = args->arg(0);
    bool synce = args->isSet("synce");
    bool forceInstall = args->isSet("forceinstall");

    kDebug(2120) << "Synce: " << synce << endl;
    kDebug(2120) << "ForceInstall: " << forceInstall << endl;

    KApplication a;

    CeScreen *ceScreen = new CeScreen();

    QObject::connect(ceScreen, SIGNAL(pdaError()), &a, SLOT(quit()));

    if (!ceScreen->connectPda(pdaName, synce, forceInstall)) {
        kDebug(2120) << "Could not contact PDA " << pdaName << endl;
        return -1;
    }

    ceScreen->show();

    return a.exec();
}

