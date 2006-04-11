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


#include "pimsyncconfigimpl.h"

#include <libkcal/calendarlocal.h>
#include <pocketpccomm/CalendarHandler.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <klineedit.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <kabc/resourcefile.h>
#include <klistbox.h>
#include <kabc/addressbook.h>
#include <kabc/vcardconverter.h>
#include <pocketpccomm/AddressBookHandler.h>
#include <pocketpccomm/resourcenull.h>

PIMSyncConfigImpl::PIMSyncConfigImpl(KConfig *ksConfig, QWidget* parent, const char* name, bool modal, WFlags fl)
        : PIMSyncConfig(parent, name, modal, fl)
{
    this->ksConfig = ksConfig;


    ContactReadFilename="~/.pimsync/Contact.read";
    CalenderReadFilename="~/.pimsync/Calender.read";
    TodoReadFilename="~/.pimsync/Todos.read";
    EventReadFilename="~/.pimsync/Events.read";
    ContactWriteFilename="~/.pimsync/Contact.write";
    CalenderWriteFilename="~/.pimsync/Calender.write";
    TodoWriteFilename="~/.pimsync/Todos.write";
    EventWriteFilename="~/.pimsync/Events.write";
    pda_name="pocket_pc";


    // Initialize the fileboxes

    kURLComboRequester_ContactRead->setURL(ContactReadFilename);
    kURLComboRequester_CalenderRead->setURL(CalenderReadFilename);
    kURLComboRequester_TodoRead->setURL(TodoReadFilename);
    kURLComboRequester_EventRead->setURL(EventReadFilename);
    kURLComboRequester_ContactWrite->setURL(ContactWriteFilename);
    kURLComboRequester_CalenderWrite->setURL(CalenderWriteFilename);
    kURLComboRequester_TodoWrite->setURL(TodoWriteFilename);
    kURLComboRequester_EventWrite->setURL(EventWriteFilename);
    lineEdit_pdaname->setText(pda_name);

}


PIMSyncConfigImpl::~PIMSyncConfigImpl()
{}


void PIMSyncConfigImpl::TodoTestButtonReadclicked()
{
    kListBox1->insertItem("[PIMSync] entering TodoRead");

    if ( kURLComboRequester_ContactRead->url() == "" ) {
        kListBox1->insertItem("[PIMSync] no filename given!");
        return;
    }

    KCal::CalendarLocal    m_calendar;
    pocketPCCommunication::CalendarHandler calHandler(lineEdit_pdaname->text());

    m_calendar.deleteAllEvents();
    m_calendar.deleteAllTodos();

    if (!calHandler.getCalendarTodos(m_calendar, pocketPCCommunication::ALL))
        kListBox1->insertItem("[PIMSync] could not get todos!");
    else
        kListBox1->insertItem("[PIMSync] got todos!");

    kListBox1->insertItem("[PIMSync] Saving ToDos in file: ");
    kListBox1->insertItem( kURLComboRequester_TodoRead->url() );
    m_calendar.save( kURLComboRequester_TodoRead->url());
    return;
}


void PIMSyncConfigImpl::show()
{
    printf("[PIMSync] Trying to open window ..\n");
    PIMSyncConfig::show();
}


void PIMSyncConfigImpl::ContactTestButtonReadclicked()
{
    kListBox1->insertItem("[PIMSync] entering ContactRead");

    KSharedPtr<pocketPCCommunication::Rra> rra = new pocketPCCommunication::Rra();

    KABC::AddressBook* addrBook = new KABC::AddressBook();

    addrBook->addResource(new pocketPCCommunication::ResourceNull( ) );

    pocketPCCommunication::AddressBookHandler addrHandler(rra);

    if (!addrHandler.getAddressBook(*addrBook, pocketPCCommunication::ALL))
        kListBox1->insertItem("[PIMSync] could not get addressBook!!");
    else {
        kListBox1->insertItem("[PIMSync] read addressBook");

        KABC::Addressee::List addrList = addrBook->allAddressees();
    }

    kListBox1->insertItem("[PIMSync] Saving Contacts in file: ");
    kListBox1->insertItem(kURLComboRequester_ContactRead->url() );

    QFile f(kURLComboRequester_ContactRead->url());
    f.open(IO_WriteOnly);
    QTextStream t( &f );
    KABC::VCardConverter vcard;
    t << vcard.createVCards( addrBook->allAddressees() );
}


void PIMSyncConfigImpl::EventTestButtonReadclicked()
{
    kListBox1->insertItem("[PIMSync] entering EventRead");

    pocketPCCommunication::CalendarHandler calHandler(lineEdit_pdaname->text());
    KCal::CalendarLocal    m_calendar;
    m_calendar.deleteAllEvents();
    m_calendar.deleteAllTodos();

    if (!calHandler.getCalendarEvents(m_calendar, pocketPCCommunication::ALL))
        kListBox1->insertItem("[PIMSync] could not get events!");
    else
        kListBox1->insertItem("[PIMSync] got events!");

    m_calendar.save( kURLComboRequester_EventRead->url());
    kListBox1->insertItem("[PIMSync] saving Events in");
    kListBox1->insertItem(kURLComboRequester_EventRead->url());
}


void PIMSyncConfigImpl::CalenderTestButtonReadclicked()
{
    kListBox1->insertItem("[PIMSync] entering CalenderRead");


    pocketPCCommunication::CalendarHandler calHandler(lineEdit_pdaname->text());
    KCal::CalendarLocal    m_calendar;

    m_calendar.deleteAllEvents();
    m_calendar.deleteAllTodos();

    if (!calHandler.getCalendarEvents (m_calendar, pocketPCCommunication::ALL))
        kListBox1->insertItem("[PIMSync] could not get appointments!");
    else
        kListBox1->insertItem("[PIMSync] got appointments");

    if (!calHandler.getCalendarTodos (m_calendar, pocketPCCommunication::ALL))
        kListBox1->insertItem("[PIMSync] could not get appointments!");
    else
        kListBox1->insertItem("[PIMSync] got appointments");

    m_calendar.save (kURLComboRequester_CalenderRead->url());
    kListBox1->insertItem("[PIMSync] saving Calender in");
    kListBox1->insertItem(kURLComboRequester_CalenderRead->url());
}


void PIMSyncConfigImpl::ContactTestButtonWriteclicked()
{
    kListBox1->insertItem("[PIMSync] entering ContactWrite");
    pocketPCCommunication::AddressBookHandler addrHandler(lineEdit_pdaname->text());

    KABC::AddressBook* addrBook;
    KABC::ResourceFile* rFile = new KABC::ResourceFile(kURLComboRequester_ContactWrite->url());

    addrBook = new KABC::AddressBook();
    addrBook->addResource (rFile);
    addrBook->load();

    if (!addrHandler.putAddressBook(*addrBook))
        kListBox1->insertItem ("[PIMSync] could not push addressBook!");
    else
        kListBox1->insertItem ("[PIMSync] pushed addressBook");
}


void PIMSyncConfigImpl::TodoTestButtonWriteclicked()
{
    kListBox1->insertItem("[PIMSync] entering TodoWrite");
    pocketPCCommunication::CalendarHandler calHandler(lineEdit_pdaname->text());

    KCal::CalendarLocal    m_calendar;

    if (!m_calendar.load (kURLComboRequester_ContactWrite->url()))
        kListBox1->insertItem("[PIMSync] could not load " + kURLComboRequester_ContactWrite->url());

    kListBox1->insertItem("[PIMSync] trying to put todos");
    calHandler.putCalendarTodos(m_calendar);
}


void PIMSyncConfigImpl::EventTestButtonWriteclicked()
{
    kListBox1->insertItem("[PIMSync] entering EventWrite");
    pocketPCCommunication::CalendarHandler calHandler(lineEdit_pdaname->text());


    KCal::CalendarLocal    m_calendar;
    m_calendar.load (kURLComboRequester_EventWrite->url());

    kListBox1->insertItem("[PIMSync] trying to put events");
    calHandler.putCalendarEvents(m_calendar);
}


void PIMSyncConfigImpl::CalenderTestButtonWriteclicked()
{
    kListBox1->insertItem("[PIMSync] entering CalenderWrite");

    pocketPCCommunication::CalendarHandler calHandler(lineEdit_pdaname->text());

    KCal::CalendarLocal    m_calendar;
    m_calendar.load (kURLComboRequester_CalenderWrite->url());

    kListBox1->insertItem("[PIMSync] trying to put events");
    calHandler.putCalendarEvents(m_calendar);

    kListBox1->insertItem("[PIMSync] trying to put todos");
    calHandler.putCalendarTodos(m_calendar);
}


void PIMSyncConfigImpl::NewConfigFile()
{
    kListBox1->insertItem("[PIMSync] entering ReadNewConfigFile");
}
