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
#include "kdeaddressbooksync.h"

#include <kdebug.h>
#include <kstandarddirs.h>

#include <kabc/vcardconverter.h>
#include <kabc/addressee.h>
#include <kabc/stdaddressbook.h>
#include <kabc/resource.h>
#include <kabc/resourcefile.h>
#include <kabc/vcardformatplugin.h>
#include <qptrlist.h>
#include <ksimpleconfig.h>
#include <qfile.h>

#include <rra.h>


KdeAddressBookSync::KdeAddressBookSync()
{}


KdeAddressBookSync::~KdeAddressBookSync()
{}


void KdeAddressBookSync::printBook(QString label, KABC::AddressBook *book)
{
    kdDebug(2120) << "==============" + label + "===============" << endl;

    KABC::AddressBook::Iterator it;

    for (it = book->begin(); it != book->end(); it++) {
        kdDebug(2120) << (*it).formattedName() << " " << (*it).uid() << " " << (*it).custom("Raki", "PdaUid-" + pdaName) << " >" << (*it).custom("Raki", "Task") + "<" << endl;
    }

    kdDebug(2120) << "--------------" + label + "---------------" << endl;
}


bool KdeAddressBookSync::compare(const KABC::Addressee &a, const KABC::Addressee &b)
{
    if (a.uid() != b.uid() ) {
        kdDebug(2120) << "Differ:uid: " << a.uid() << " " << b.uid() << endl; return false;
    }
    if (a.name() != b.name() ) {
        kdDebug(2120) << "Differ:name: " << a.name() << " " << b.name() << endl; return false;
    }
    if (a.formattedName() != b.formattedName() ) {
        kdDebug(2120) << "Differ:formattedName: " << a.formattedName() << " " << b.formattedName() << endl; return false;
    }
    if (a.familyName() != b.familyName() ) {
        kdDebug(2120) << "Differ:familyName: " << a.familyName() << " " << b.familyName() << endl; return false;
    }
    if (a.givenName() != b.givenName() ) {
        kdDebug(2120) << "Differ:givenName: " << a.givenName() << " " << b.givenName() << endl; return false;
    }
    if (a.additionalName() != b.additionalName() ) {
        kdDebug(2120) << "Differ:additionalName: " << a.additionalName() << " " << b.additionalName() << endl; return false;
    }
    if (a.prefix() != b.prefix() ) {
        kdDebug(2120) << "Differ:prefix: " << a.prefix() << " " << b.prefix() << endl; return false;
    }
    if (a.suffix() != b.suffix() ) {
        kdDebug(2120) << "Differ:suffix: " << a.suffix() << " " << b.suffix() << endl; return false;
    }
    if (a.nickName() != b.nickName() ) {
        kdDebug(2120) << "Differ:nickName: " << a.nickName() << " " << b.nickName() << endl; return false;
    }
    if (a.birthday() != b.birthday() ) {
        kdDebug(2120) << "Differ:birthday: " << a.birthday().toString() << " " << b.birthday().toString() << endl; return false;
    }
    if (a.mailer() != b.mailer() ) {
        kdDebug(2120) << "Differ:mailer: " << a.mailer() << " " << b.mailer() << endl; return false;
    }
    if (a.timeZone() != b.timeZone() ) {
        kdDebug(2120) << "Differ:timeZone: " << a.timeZone().asString() << " " << b.timeZone().asString() << endl; return false;
    }
    if (a.geo() != b.geo() ) {
        kdDebug(2120) << "Differ:geo: " << a.geo().asString() << " " << b.geo().asString() << endl; return false;
    }
    if (a.title() != b.title() ) {
        kdDebug(2120) << "Differ:title: " << a.title() << " " << b.title() << endl; return false;
    }
    if (a.role() != b.role() ) {
        kdDebug(2120) << "Differ:role: " << a.role() << " " << b.role() << endl; return false;
    }
    if (a.organization() != b.organization() ) {
        kdDebug(2120) << "Differ:organization: " << a.organization() << " " << b.organization() << endl; return false;
    }
    if (a.note() != b.note() ) {
        kdDebug(2120) << "Differ:note: " << a.note() << " " << b.note() << endl; return false;
    }
    if (a.productId() != b.productId() ) {
        kdDebug(2120) << "Differ:productId: " << a.productId() << " " << b.productId() << endl; return false;
    }
    if (a.sortString() != b.sortString() ) {
        kdDebug(2120) << "Differ:sortString: " << a.sortString() << " " << b.sortString() << endl; return false;
    }
    if (a.secrecy() != b.secrecy() ) {
        kdDebug(2120) << "Differ:secrecy: " << a.secrecy().asString() << " " << b.secrecy().asString() << endl; return false;
    }
    if (a.logo() != b.logo() ) {
        kdDebug(2120) << "Differ:logo: " << endl; return false;
    }
    if (a.photo() != b.photo() ) {
        kdDebug(2120) << "Differ:photo: " << endl; return false;
    }
    if (a.sound() != b.sound() ) {
        kdDebug(2120) << "Differ:sound: " << endl; return false;
    }
    if ( (a.url().isValid() || b.url().isValid() ) &&
            (a.url() != b.url() ) ) {
        kdDebug(2120) << "Differ:url: " << endl; return false;
    }
    if (a.phoneNumbers() != b.phoneNumbers() ) {
        kdDebug(2120) << "Differ:phoneNumbers: " << endl; return false;
    }
    if (a.addresses() != b.addresses() ) {
        kdDebug(2120) << "Differ:addresses: " << endl; return false;
    }
    if (a.keys() != b.keys() ) {
        kdDebug(2120) << "Differ:keys: " << endl; return false;
    }
    if (a.emails() != b.emails() ) {
        kdDebug(2120) << "Differ:emails: " << endl; return false;
    }
    if (a.categories() != b.categories() ) {
        kdDebug(2120) << "Differ:categories: " << endl; return false;
    }

    return true;
}


KABC::AddressBook *KdeAddressBookSync::generatePcDelta(KABC::AddressBook *pcStdBook,
        KABC::AddressBook *pcRhoBook)
{
    KABC::AddressBook *pcDeltaBook = new KABC::AddressBook();
    KABC::AddressBook::Iterator it;
    KABC::Addressee rhoAddressee;
    KABC::Addressee stdAddressee;

    for (it = pcStdBook->begin(); it != pcStdBook->end(); it++) {
        rhoAddressee = pcRhoBook->findByUid ((*it).uid());
        stdAddressee = *it;

        if (rhoAddressee.isEmpty()) {
            stdAddressee.insertCustom("Raki", "Task", "new");
            incTotalSteps(1);
        } else if (compare(stdAddressee, rhoAddressee)) {
            stdAddressee.insertCustom("Raki", "Task", "unchanged");
        } else {
            stdAddressee.insertCustom("Raki", "Task", "changed");
            incTotalSteps(1);
        }

        pcDeltaBook->insertAddressee(stdAddressee);
        rhoAddressee.insertCustom("Raki", "Found", "true");
        pcRhoBook->insertAddressee(rhoAddressee);
    }

    for (it = pcRhoBook->begin(); it != pcRhoBook->end(); it++) {
        if ((*it).custom("Raki", "Found").isEmpty()) {
            (*it).insertCustom("Raki", "Task", "deleted");
            pcDeltaBook->insertAddressee((*it));
            incTotalSteps(1);
        }
    }

    printBook("PCStdBook", pcDeltaBook);

    return pcDeltaBook;
}


KABC::AddressBook *KdeAddressBookSync::generatePdaDelta()
{
    QString vCard;
    uint32_t *v;
    QString uidString;
    KABC::VCardConverter vCardConverter;
    KABC::Addressee addressee;
    KABC::AddressBook *pdaBook = new KABC::AddressBook();

    if (rra->connect()) {
        struct Rra::ids& ids = rra->getIds(getObjectTypeId());

        incTotalSteps(ids.changedIds.count() + ids.deletedIds.count());

        for (v = ids.changedIds.first(); v && isRunning(); v = ids.changedIds.next()) {
            addressee = rra->getAddressee(getObjectTypeId(), *v);
            addressee.insertCustom("Raki", "Task", "changed");
            uidString = "RRA-ID-" + (QString("00000000") + QString::number((*v), 16)).right(8);
            addressee.setUid(uidString);
            addressee.insertCustom("Raki", "PdaUid-" + QString::number(partnerId), addressee.uid());
            pdaBook->insertAddressee(addressee);
            advanceProgress();
        }

        for (v = ids.unchangedIds.first(); v && isRunning(); v = ids.unchangedIds.next()) {
            addressee = KABC::Addressee();
            addressee.insertCustom("Raki", "Task", "unchanged");
            uidString = "RRA-ID-" + (QString("00000000") + QString::number((*v), 16)).right(8);
            addressee.setUid(uidString);
            addressee.insertCustom("Raki", "PdaUid-" + QString::number(partnerId), addressee.uid());
            pdaBook->insertAddressee(addressee);
        }

        for (v = ids.deletedIds.first(); v && isRunning(); v = ids.deletedIds.next()) {
            addressee = KABC::Addressee();
            addressee.insertCustom("Raki", "Task", "deleted");
            uidString = "RRA-ID-" + (QString("00000000") + QString::number((*v), 16)).right(8);
            addressee.setUid(uidString);
            addressee.insertCustom("Raki", "PdaUid-" + QString::number(partnerId), addressee.uid());
            pdaBook->insertAddressee(addressee);
            advanceProgress();
        }
        rra->disconnect();
    }

    printBook("PDABook", pdaBook);

    return pdaBook;
}


KABC::AddressBook *KdeAddressBookSync::generateKappaBook(KABC::AddressBook *pcDeltaBook,
        KABC::AddressBook *pdaDeltaBook)
{
    KABC::AddressBook *kappaBook = new KABC::AddressBook();
    KABC::AddressBook::Iterator it;
    KABC::Addressee kappaAddressee;
    KABC::Addressee pdaDeltaAddressee;
    QString pcTask;
    QString pdaTask;

    for (it = pcDeltaBook->begin(); it != pcDeltaBook->end(); it++) {
        QString pdaUid = (*it).custom("Raki", "PdaUid-" + QString::number(partnerId));
        if (!pdaUid.isEmpty()) {
            pdaDeltaAddressee = pdaDeltaBook->findByUid(pdaUid);
            if (!pdaDeltaAddressee.isEmpty()) {
                if (pdaDeltaAddressee.custom("Raki", "Task") == "changed") {
                    if ((*it).custom("Raki", "Task") == "changed") {
                        kappaAddressee = (*it);
                        kappaAddressee.insertCustom("Raki", "PdaUid-" + QString::number(partnerId), pdaUid);
                        kappaBook->insertAddressee(kappaAddressee);
                    }
                }
                pdaDeltaBook->removeAddressee(pdaDeltaAddressee);

                QString task = pdaDeltaAddressee.custom("Raki", "Task");
                pdaDeltaAddressee.setCustoms((*it).customs());
                pdaDeltaAddressee.insertCustom("Raki", "Task", task);
                pdaDeltaAddressee.insertCustom("Raki", "PdaUid-" + QString::number(partnerId), pdaUid);
                pdaDeltaAddressee.setUid((*it).uid());
                pdaDeltaBook->insertAddressee(pdaDeltaAddressee);
            }
        }
    }

    printBook("KappaBook", kappaBook);

    return kappaBook;
}


KABC::AddressBook *KdeAddressBookSync::generatePdaItaBook(KABC::AddressBook *pdaDeltaBook,
        KABC::AddressBook *kappaBook)
{
    KABC::AddressBook::Iterator it;
    KABC::Addressee pdaAddressee;

    for (it = kappaBook->begin(); it != kappaBook->end(); it++) {
        pdaAddressee = (*it);
        QString pdaUid = (*it).custom("Raki", "PdaUid-" + QString::number(partnerId));
        pdaAddressee.setUid(pdaUid);
        pdaDeltaBook->removeAddressee(pdaAddressee);
        decTotalSteps(1);
    }

    printBook("PdaItaBook", pdaDeltaBook);

    return pdaDeltaBook;
}


KABC::AddressBook *KdeAddressBookSync::generatePcItaBook(KABC::AddressBook *pcDeltaBook,
        KABC::AddressBook *kappaBook)
{
    KABC::AddressBook::Iterator it;

    for (it = kappaBook->begin(); it != kappaBook->end(); it++) {
        pcDeltaBook->removeAddressee((*it));
        decTotalSteps(1);
    }

    printBook("PcItaBook", pcDeltaBook);

    return pcDeltaBook;
}


bool KdeAddressBookSync::sync()
{
    QString vCard;
    QString task;
    QString ceUid;
    QString uidString;
    uint32_t newObjectId;
    uint32_t uid;
    bool ok;

    KABC::VCardConverter vCardConverter;

    KABC::AddressBook *pcStdBook = KABC::StdAddressBook::self();

    KABC::AddressBook *pcRhoBook = new KABC::AddressBook();
    KABC::ResourceFile *rhoResource = new KABC::ResourceFile(pcRhoBook,
                                      QFile::encodeName(locateLocal("data", (QString("raki/syncrefbook-") + QString::number(partnerId) + QString(".vcf")).ascii())));
    pcRhoBook->addResource(rhoResource);
    pcRhoBook->load();

    setTask("Search for changes in KDE");
    KABC::AddressBook *pcDeltaBook = generatePcDelta(pcStdBook, pcRhoBook);         // Search with PCUid

    setTask("Search for changes on PDA");
    KABC::AddressBook *pdaDeltaBook = generatePdaDelta();                           // Download book

    setTask("Merging KDE and PDA");
    KABC::AddressBook *kappaBook = generateKappaBook(pcDeltaBook, pdaDeltaBook);    // Search with CEUid

    setTask("Filtering for KDE");
    KABC::AddressBook *pdaItaBook = generatePdaItaBook(pdaDeltaBook, kappaBook);    // Search with PCUid

    setTask("Filtering for PDA");
    KABC::AddressBook *pcItaBook = generatePcItaBook(pcDeltaBook, kappaBook);       // Search with CEUid

    KABC::AddressBook::Iterator it;

    kdDebug(2120) << "+++++++++++ Updateing StdBook ++++++++++++++" << endl;

    setTask("Writing KDE");
    for (it = pdaItaBook->begin(); it != pdaItaBook->end() && isRunning(); it++) {
        task = (*it).custom("Raki", "Task");
        if (task == "changed") {
            kdDebug(2120) << "Update of uid: " << (*it).uid() << "  " << (*it).custom("Raki", "PdaUid-" + QString::number(partnerId)) << endl;
            pcStdBook->insertAddressee((*it));
        } else if (task == "new") {
            kdDebug(2120) << "New Address " << (*it).uid() << "  " << (*it).custom("Raki", "PdaUid-" + QString::number(partnerId)) << endl;
            pcStdBook->insertAddressee((*it));
        } else if (task == "deleted") {
            kdDebug(2120) << "Deleting of " << (*it).uid() << "  " << (*it).custom("Raki", "PdaUid-" + QString::number(partnerId)) << endl;
            pcStdBook->removeAddressee((*it));
        } else if (task == "unchanged") {
            // ??
        }
    }

    kdDebug(2120) << "++++++++++++ Updateing PdaBook +++++++++++++" << endl;

    setTask("Writing PDA");
    for (it = pcItaBook->begin(); it != pcItaBook->end() && isRunning(); it++) {
        task = (*it).custom("Raki", "Task");
        if (task == "changed") {
            kdDebug(2120) << "Update of uid: " << (*it).uid() << "  " << (*it).custom("Raki", "PdaUid-" + QString::number(partnerId)) << endl;
            ceUid = (*it).custom("Raki", "PdaUid-" + QString::number(partnerId));
            (*it).setUid(ceUid);
            uid = ceUid.right(8).toULong(&ok, 16);
            rra->putAddressee(*it, getObjectTypeId(), uid, &newObjectId);
            if (newObjectId != uid) {
                uidString = "RRA-ID-" + (QString("00000000") + QString::number(newObjectId, 16)).right(8);
                (*it).insertCustom("Raki", "PdaUid-" + QString::number(partnerId), uidString);
                pcStdBook->insertAddressee((*it));
            }
            advanceProgress();
        } else if (task == "new") {
            rra->putAddressee(*it, getObjectTypeId(), 0, &newObjectId);
            uidString = "RRA-ID-" + (QString("00000000") + QString::number(newObjectId, 16)).right(8);
            (*it).insertCustom("Raki", "PdaUid-" + QString::number(partnerId), uidString);
            pcStdBook->insertAddressee((*it));
            kdDebug(2120) << "New Address " << (*it).uid() << "  " << (*it).custom("Raki", "PdaUid-" + QString::number(partnerId)) << endl;
            advanceProgress();
        }
    }

    if (isRunning()) {
        if (rra->connect()) {
            for (it = pcItaBook->begin(); it != pcItaBook->end() && isRunning(); it++) {
                task = (*it).custom("Raki", "Task");
                if (task == "deleted") {
                    ceUid = (*it).custom("Raki", "PdaUid-" + QString::number(partnerId));
                    uid = ceUid.right(8).toULong(&ok, 16);
                    rra->deleteObject(getObjectTypeId(), uid);
                    advanceProgress();
                    kdDebug(2120) << "Deleting of " << (*it).uid() << "  " << (*it).custom("Raki", "PdaUid-" + QString::number(partnerId)) << endl;
                } else if (task == "unchanged") {
                    // ??
                }
            }
            rra->disconnect();
        }

        // Todo here: Resolving konflicts in kappa list

        pcRhoBook->clear();
        for (it = pcStdBook->begin(); it != pcStdBook->end(); it++) {
            (*it).removeCustom("Raki", "Task");
            KABC::Addressee rhoAddressee = (*it);
            rhoAddressee.setResource(rhoResource);
            pcRhoBook->insertAddressee(rhoAddressee);
        }
    }

    KABC::Ticket *t = pcRhoBook->requestSaveTicket(rhoResource);
    pcRhoBook->save(t);

    pcDeltaBook->clear();
    pdaDeltaBook->clear();
    kappaBook->clear();

    delete pcDeltaBook;
    delete pdaDeltaBook;
    delete kappaBook;
    delete pcRhoBook;

    KABC::StdAddressBook::save();
    
    setTask("Finished");

    return true;
}
