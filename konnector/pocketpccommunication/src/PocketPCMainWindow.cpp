/***************************************************************************
 *   Copyright (C) 2004 by Christian Fremgen                               *
 *   cfremgen@users.sourceforge.net                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "PocketPCMainWindow.h"
#include "PdaNameDialog.h"
#include "Preferences.h"
//#include "KSyncTest.h"
#include "resourcenull.h"

#include <qcolor.h>
#include <qtextedit.h>
#include <qlistview.h>
#include <qprogressbar.h>
#include <qstatusbar.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qmessagebox.h>
#include <qfile.h>
#include <qaction.h>

#include <kstandarddirs.h>

#include <kabc/addressbook.h>
#include <kabc/resourcefile.h>
#include <kabc/stdaddressbook.h>
#include <kabc/vcardconverter.h>
#include <libkcal/calendarnull.h>
#include <libkcal/calendarlocal.h>
#include <libkcal/vcalformat.h>

#include <rra.h>
#include <AddressBookHandler.h>
#include <CalendarHandler.h>
#include <Addressee.h>
#include <AddressBookSyncer.h>
#include <CalendarSyncer.h>

//#include <PocketPCKonnector.h>


PocketPCMainWindow::PocketPCMainWindow()
 : PocketPCMainWindowUI()
{
    m_progressBar = new QProgressBar (this);
    m_progressBar->setPercentageVisible(true);
    m_progressBar->setTotalSteps(100);
    statusBar()->addWidget (m_progressBar);
    
    m_preferences = new Preferences (this, "Preferences");    
    
    //slotGetTypesAction();
}


PocketPCMainWindow::~PocketPCMainWindow()
{
}


void PocketPCMainWindow::slotGetTypesAction()
{           
    appendRedText ("Trying to get all available types from pda (" + m_preferences->getPdaName() +")");
    
    //getPdaName();    
    
    if (!m_preferences->isInitialized())
        m_preferences->exec();
    
    // ok.. lets try to connect to the pda....
    /*
    pocketPCCommunication::Rra rra(m_preferences->getPdaName());
    
    QMap<int, RRA_SyncMgrType *> typeMap;
    if (rra.getTypes(&typeMap))
    {
        QMap<int, RRA_SyncMgrType*>::Iterator it;
        for (it = typeMap.begin(); it != typeMap.end(); ++it)
        {
            m_listViewPdaTypes->insertItem (new QListViewItem (m_listViewPdaTypes, QString::number(it.data()->id, 16), 
                QString::number(it.data()->count), QString::number(it.data()->modified),
                QString(it.data()->name1), QString(it.data()->name2), QString::number(it.data()->total_size)));            
        }
    }
    else
        appendRedText ("could not get the types from " + m_preferences->getPdaName());
    
    
    uint32_t typeId;
    typeId = rra.getTypeForName (RRA_SYNCMGR_TYPE_CONTACT);
    appendBlueText (RRA_SYNCMGR_TYPE_CONTACT + QString::number(typeId, 16));
    
    typeId = rra.getTypeForName (RRA_SYNCMGR_TYPE_APPOINTMENT);
    appendBlueText (RRA_SYNCMGR_TYPE_APPOINTMENT + QString::number(typeId, 16));
    
    typeId = rra.getTypeForName (RRA_SYNCMGR_TYPE_TASK);
    appendBlueText (RRA_SYNCMGR_TYPE_TASK + QString::number(typeId, 16));
    
    rra.finalDisconnect();
    */
    
    // ksharedptr test...
    KSharedPtr<pocketPCCommunication::Rra> rra = new pocketPCCommunication::Rra(m_preferences->getPdaName());
    
    // just for testing
    rra->connect();
    
    QMap<int, RRA_SyncMgrType *> typeMap;
    if (rra->getTypes(&typeMap))
    {
        QMap<int, RRA_SyncMgrType*>::Iterator it;
        for (it = typeMap.begin(); it != typeMap.end(); ++it)
        {
            m_listViewPdaTypes->insertItem (new QListViewItem (m_listViewPdaTypes, QString::number(it.data()->id, 16), 
                                            QString::number(it.data()->count), QString::number(it.data()->modified),
                                            QString(it.data()->name1), QString(it.data()->name2), QString::number(it.data()->total_size)));            
        }
    }
    else
        appendRedText ("could not get the types from " + m_preferences->getPdaName());
    
    
    uint32_t typeId;
    typeId = rra->getTypeForName (RRA_SYNCMGR_TYPE_CONTACT);
    appendBlueText (RRA_SYNCMGR_TYPE_CONTACT + QString::number(typeId, 16));
    
    typeId = rra->getTypeForName (RRA_SYNCMGR_TYPE_APPOINTMENT);
    appendBlueText (RRA_SYNCMGR_TYPE_APPOINTMENT + QString::number(typeId, 16));
    
    typeId = rra->getTypeForName (RRA_SYNCMGR_TYPE_TASK);
    appendBlueText (RRA_SYNCMGR_TYPE_TASK + QString::number(typeId, 16));
    
    rra->finalDisconnect();
    
    //uint32_t id;
    //rra.getMatchMaker()->get_partner_id(1, &id);
    m_partnerId = "MYPARTNERID";
}


void PocketPCMainWindow::slotGetObjectIds (QListViewItem* p_item)
{
    appendRedText ("Trying to get all available object ids for: " + p_item->text(0));
    
    // ok.. connect to the pda...
    //pocketPCCommunication::Rra rra(m_preferences->getPdaName());
    KSharedPtr<pocketPCCommunication::Rra> rra = new pocketPCCommunication::Rra(m_preferences->getPdaName());
    
    struct pocketPCCommunication::Rra::ids ids;
    
    bool ok;
    if (rra->getIds(p_item->text(0).toUInt(&ok, 16), &ids))
    {
        if (ids.changedIds.size() > 0)
            printObjectIds (ids.changedIds, "changed", QColor (150, 0, 0));
        if (ids.deletedIds.size() > 0)
            printObjectIds (ids.deletedIds, "deleted", QColor (125, 255, 125));
        if (ids.unchangedIds.size() > 0)
            printObjectIds (ids.unchangedIds, "unchanged", QColor (125, 125, 255));        
    }
    else
        appendRedText ("could not get object ids for type " + p_item->text(0));
}

/*
void PocketPCMainWindow::slotSetPdaName ()
{
    PdaNameDialog* pdaDiag = new PdaNameDialog(this);
    pdaDiag->setText (m_pdaName);
    switch (pdaDiag->exec())
    {
        case QDialog::Accepted:
            m_pdaName = pdaDiag->getText();
            appendRedText ("changed pda name to: " + m_pdaName);
            break;
    }
}
*/

void PocketPCMainWindow::slotGetAddresses()
{
    kdDebug(2120) << endl << endl << "PocketPCMainWindow::slotGetAddresses called" << endl;
    kdDebug(2120) << "pdaname: " << m_preferences->getPdaName() << endl;
    KSharedPtr<pocketPCCommunication::Rra> rra = new pocketPCCommunication::Rra(m_preferences->getPdaName());
    
    //rra->connect();
    //pocketPCCommunication::AddressBookHandler addrHandler(rra);
    
    
    KABC::AddressBook* addrBook = new KABC::AddressBook();
    //KABC::ResourceFile* rFile = new KABC::ResourceFile(locateLocal("data", m_preferences->getAddressBookFileName()));
    kdDebug(2120) << "what the hell: " << m_preferences->getAddressBookFileName() << endl;
    //KABC::ResourceFile* rFile = new KABC::ResourceFile(m_preferences->getAddressBookFileName());
    
    addrBook->addResource (new pocketPCCommunication::ResourceNull());
    //addrBook.addResource(rFile);
    /*
    KABC::ResourceFile rFile(m_preferences->getAddressBookFileName());
    rFile.doOpen();
    addrBook.addResource (&rFile);
    */
    /*
    if (m_preferences->useStdAddressBook())    
        addrBook = KABC::StdAddressBook::self();
    else
    {
        addrBook = new KABC::AddressBook();            
        addrBook->addResource (rFile);
    }
    */
    //connect(&addrHandler, SIGNAL(progress(int )), this, SLOT(slotProgress(int)));
    
    //update();
    //QApplication::setOverrideCursor (QCursor(Qt::WaitCursor));
    
    //pocketPCCommunication::AddressBookHandler addrHandler(m_preferences->getPdaName());
    pocketPCCommunication::AddressBookHandler addrHandler(rra);
    
    if (!addrHandler.getAddressBook(*addrBook, pocketPCCommunication::ALL))
        appendRedText ("could not get addressBook!!");
    else
    {
        appendBlueText ("read addressBook");
        
        KABC::Addressee::List addrList = addrBook->allAddressees();
        QValueList<KABC::Addressee>::const_iterator it = addrList.begin();
        for (; it != addrList.end(); ++it)
        {
            appendBlueText ((*it).assembledName());
            appendColoredText(QColor(125, 150, 125), (*it).realName());
        }                                 
        /*
        if (m_preferences->useStdAddressBook())
        {
            KABC::Ticket* ticket = addrBook->requestSaveTicket();
            if (ticket)
            {
                addrBook->save(ticket);
                appendRedText ("saved standard address book");
            }
            else
                appendRedText ("could not save standard address book");
        }
        else
        {
            KABC::Ticket* ticket = addrBook->requestSaveTicket (rFile);
            if (!ticket)
                appendRedText ("could not get ticket for addressbook saving!");
            else
            {    
                if (!addrBook->save(ticket))
                {
                    appendRedText ("saving failed!");
                    kdDebug(2120) << "saving of addressBook failed!!!" << endl;
                }
                else
                    appendRedText ("saved address book: " + m_preferences->getAddressBookFileName());
                //addrBook.releaseSaveTicket(ticket);             
            }
        }
        */
    }
        
    //QApplication::restoreOverrideCursor();
    //disconnect(&addrHandler, SIGNAL(progress(int )), this, SLOT(slotProgress(int)));
    /*
    if (m_preferences->useStdAddressBook())
        delete addrBook;    
    */
    //rra->finalDisconnect();
}


void PocketPCMainWindow::slotPushAddresses()
{
    //KABC::StdAddressBook* addrBook = KABC::StdAddressBook::self();
    pocketPCCommunication::AddressBookHandler addrHandler(m_preferences->getPdaName());
    
    KABC::AddressBook* addrBook;
    //KABC::ResourceFile* rFile = new KABC::ResourceFile(locateLocal("data", m_preferences->getAddressBookFileName()));
    KABC::ResourceFile* rFile = new KABC::ResourceFile(m_preferences->getAddressBookFileName());
    
    if (m_preferences->useStdAddressBook())    
        addrBook = KABC::StdAddressBook::self();
    else        
    {
        addrBook = new KABC::AddressBook();
        addrBook->addResource (rFile);
        addrBook->load();
    }
    
    
    //KABC::AddressBook* addrBook = dynamic_cast<KABC::AddressBook>(KABC::StdAddressBook::self());
        
    if (!addrHandler.putAddressBook(*addrBook))
        appendRedText ("could not push addressBook!");
    else
        appendBlueText ("pushed addressBook");
}


void PocketPCMainWindow::slotGetCalendar()
{
    
    //pocketPCCommunication::CalendarHandler calHandler(m_preferences->getPdaName());
    KSharedPtr<pocketPCCommunication::Rra> rra = new pocketPCCommunication::Rra(m_preferences->getPdaName());
    rra->connect();
    pocketPCCommunication::CalendarHandler calHandler(rra);
    
    //KCal::CalendarLocal cal; 
    
    
    appendBlueText ("trying to get appointments");
    if (!calHandler.getCalendarEvents (m_calendar, pocketPCCommunication::ALL))
        appendRedText ("could not get appointments!");
    else
        appendBlueText ("got appointments");
    
    m_calendar.save (m_preferences->getCalendarFileName());
    //m_calendar.save (m_preferences->get);
    /* 
    cal.deleteAllEvents();
    cal.deleteAllTodos();
    */
    rra->finalDisconnect();
    appendBlueText ("trying to get todos");
    if (!calHandler.getCalendarTodos (m_calendar, pocketPCCommunication::ALL))
        appendRedText ("could not get todos!");
    else
        appendBlueText ("got todos");
    
    m_calendar.save (m_preferences->getCalendarFileName());
    //cal.save("/home/fips/myCalendar.ics", new KCal::VCalFormat()); // well.. where is this saved???
}


void PocketPCMainWindow::slotPutCalendar()
{
    pocketPCCommunication::CalendarHandler calHandler(m_preferences->getPdaName());
    
    //m_calendar.load ("/home/fips/Documents/icalout.ics");
    m_calendar.load (m_preferences->getCalendarFileName());
    
    appendBlueText ("trying to put appointments");
    calHandler.putCalendarEvents(m_calendar);
    
    appendBlueText ("trying to put todos");
    calHandler.putCalendarTodos(m_calendar);
}


void PocketPCMainWindow::slotGetEvents()
{
    appendBlueText ("only getting events into " + m_preferences->getEventsFileName()); ///home/fips/CalendarData/events.vcs");
    pocketPCCommunication::CalendarHandler calHandler(m_preferences->getPdaName());
    
    //m_calendar.close();
    m_calendar.deleteAllEvents();
    m_calendar.deleteAllTodos();
    
    if (!calHandler.getCalendarEvents(m_calendar, pocketPCCommunication::ALL))
        appendRedText ("could not get events!");
    else
        appendRedText ("got events!");
    
    //m_calendar.save ("/home/fips/CalendarData/events.vcs");
    m_calendar.save (m_preferences->getEventsFileName());
}


void PocketPCMainWindow::slotGetTodos()
{
    appendBlueText ("only getting todos into " + m_preferences->getTodosFileName()); ///home/fips/CalendarData/todos.ics");
    pocketPCCommunication::CalendarHandler calHandler(m_preferences->getPdaName());
    
    m_calendar.deleteAllEvents();
    m_calendar.deleteAllTodos();
    
    if (!calHandler.getCalendarTodos(m_calendar, pocketPCCommunication::ALL))
        appendRedText ("could not get todos!");
    else
        appendRedText ("got todos!");
    
    //m_calendar.save ("/home/fips/CalendarData/todos.ics");
    m_calendar.save (m_preferences->getTodosFileName());
}


void PocketPCMainWindow::slotPushEvents()
{
    pocketPCCommunication::CalendarHandler calHandler(m_preferences->getPdaName());
    
    //m_calendar.load ("/home/fips/Documents/icalout.ics");
    m_calendar.close();
    //m_calendar.load ("/home/fips/CalendarData/events.vcs");    
    m_calendar.load (m_preferences->getEventsFileName());    
    
    appendBlueText ("trying to put events");
    calHandler.putCalendarEvents(m_calendar);    
}


void PocketPCMainWindow::slotPushTodos()
{
    pocketPCCommunication::CalendarHandler calHandler(m_preferences->getPdaName());
    
    //m_calendar.load ("/home/fips/Documents/icalout.ics");
    m_calendar.close();
    if (!m_calendar.load (m_preferences->getTodosFileName()))
        appendRedText ("could not load " + m_preferences->getTodosFileName()); ///home/fips/CalendarDate/todo.ics");
        
    //m_calendar.save ("/home/fips/CalendarData/local_todos.ics");
    appendBlueText ("trying to put todos");
    calHandler.putCalendarTodos(m_calendar);
}


void PocketPCMainWindow::slotRestore() 
{
    appendRedText ("restoring backup");

    KSharedPtr<pocketPCCommunication::Rra> rra;
    rra = new pocketPCCommunication::Rra(m_preferences->getPdaName());
    if (!rra->connect())
    {
        appendRedText ("could not connect to pda!");
        return;
    }
    
    pocketPCCommunication::AddressBookHandler addrHandler(rra);
    
    KABC::AddressBook* addrBook = new KABC::AddressBook();
    KABC::ResourceFile* rFile = new KABC::ResourceFile(locateLocal ("appdata", "AddressBook-Backup-"+addrHandler.getPartnerId()+".vcf"));
    addrBook->addResource(rFile);
    addrBook->load();
    
    rra->finalDisconnect(); // just in case the partnerships does not work..... maybe this helps
    
    // as it is not possible to restore the original RRA-IDs we must manipulate them here... this sucks...
    // because so we will never have the same addressBook (EXACTLY the same I mean..)
    KABC::AddressBook::Iterator it = addrBook->begin();
    for (; it != addrBook->end(); ++it)
        (*it).setUid((*it).uid().mid(1));
    
    if (!addrHandler.putAddressBook(*addrBook))
    {
        appendRedText ("could not restore the addressBook!");
    }
    
    delete addrBook;
    
    // and now for the calendar...
    
    rra->finalDisconnect();
    
    pocketPCCommunication::CalendarHandler calHandler(rra);
    m_calendar.deleteAllEvents();
    m_calendar.deleteAllTodos();
    rra->connect();
    m_calendar.load (locateLocal ("appdata", "Calendar-Backup-"+calHandler.getPartnerId()+".ics"));
    rra->finalDisconnect();
    
    // and as above.. change the ids...
    KCal::Incidence::List incidences = m_calendar.incidences();
    KCal::Incidence::List::Iterator calIt = incidences.begin();
    for (; calIt != incidences.end(); ++calIt)
        (*calIt)->setUid ((*calIt)->uid().mid(1));
    
    appendBlueText ("trying to put appointments");
    calHandler.putCalendarEvents(m_calendar);

    rra->finalDisconnect(); // just for testing.. maybe it is more stable if this is here....    
    appendBlueText ("trying to put todos");
    calHandler.putCalendarTodos(m_calendar);
    
    //rra->disconnect();
}


void PocketPCMainWindow::slotBackup()
{
    appendRedText ("reading backup");
    
    KSharedPtr<pocketPCCommunication::Rra> rra;
    rra = new pocketPCCommunication::Rra(m_preferences->getPdaName());
    if (!rra->connect())
    {
        appendRedText ("could not connect to pda!");
        return;
    }
    
    pocketPCCommunication::AddressBookHandler addrHandler(rra);         
    
    // now get everything and write it to these files...
    KABC::AddressBook* addrBook;
    addrBook = new KABC::AddressBook();            
    addrBook->addResource(new pocketPCCommunication::ResourceNull());
    if (!addrHandler.getAddressBook(*addrBook, pocketPCCommunication::ALL))
        appendRedText ("could not get addressBook!!");
    else
    {
        QString addrFileName = locateLocal ("appdata", "AddressBook-Backup-"+addrHandler.getPartnerId()+".vcf");
        appendRedText ("Filename for addressbook: " + addrFileName);
    
        QFile f( addrFileName );
        if ( !f.open( IO_WriteOnly ) ) 
        {
            appendRedText ("could not save  addressBook-backup!");
            return;
        }
        QTextStream t( &f );
        KABC::VCardConverter vcard;
        t << vcard.createVCards( addrBook->allAddressees() );                
    }
    
    rra->finalDisconnect();    
    
    pocketPCCommunication::CalendarHandler calHandler(rra);
    
    if (!calHandler.getCalendarEvents (m_calendar, pocketPCCommunication::ALL))
        appendRedText ("could not get appointments!");
    else
        appendBlueText ("got appointments");
    
    //m_calendar.save (calFileName);
    
    if (!calHandler.getCalendarTodos (m_calendar, pocketPCCommunication::ALL))
        appendRedText ("could not get todos!");
    else
        appendBlueText ("got todos");
    
    QString calFileName = locateLocal ("appdata", "Calendar-Backup-"+calHandler.getPartnerId()+".ics");
    appendRedText ("Filename for calendar: " + calFileName);
    
    
    m_calendar.save (calFileName);
    //rra->finalDisconnect();
}


void PocketPCMainWindow::slotDeleteCalendar()
{
    pocketPCCommunication::CalendarHandler calHandler(m_preferences->getPdaName());
    
    calHandler.deleteCalendar ();
}
 
           
void PocketPCMainWindow::slotDeleteAddressBook ()
{
    appendRedText ("deleting addressBook");
    pocketPCCommunication::AddressBookHandler addrHandler(m_preferences->getPdaName());
    
    addrHandler.deleteAddressBook();
}


void PocketPCMainWindow::slotDeleteEverything ()
{
    slotDeleteCalendar();
    slotDeleteAddressBook();
}


void PocketPCMainWindow::slotProgress(int p_progress)
{
    m_progressBar->setProgress(p_progress);
    m_progressBar->update();
    qApp->processEvents();
}


void PocketPCMainWindow::slotShowPreferences()
{
    m_preferences->show();
}

/*
void PocketPCMainWindow::slotSyncAddressBooks()
{
    if (m_preferences->getUseKonnector())
        syncAddressBookKonnector();
    else
        syncAddressBookNoKonnector();
}
*/

void PocketPCMainWindow::slotSyncAddressBooks()
{
    appendRedText("syncing addressbook without konnector");
    
    QString local;
    QString output;
    QString device;
    
    device = m_preferences->getAddressBookFileName();
    local = m_preferences->getAddressBookLocal();
    output = m_preferences->getAddressBookOutput();
    
    if (device.isEmpty() || local.isEmpty() || output.isEmpty())
    {
        QMessageBox::critical (this, "Critical failure", "Please provide all file names in your preferences!");
        return;
    }    
    
    KABC::AddressBook addrBook1;
    KABC::AddressBook addrBook2;
    
    KABC::ResourceFile* rFile1 = new KABC::ResourceFile(device);
    addrBook1.addResource (rFile1);
    addrBook1.load();
    
    
    KABC::ResourceFile* rFile2 = new KABC::ResourceFile(local);
    addrBook2.addResource (rFile2);
    addrBook2.load();
    
    pocketPCPIM::AddressBookSyncer addrSyncer2("MYPARTNERID");
    kdDebug(2120) << "Calling AddressBookSyncer::preProcess" << endl;
    addrSyncer2.preProcess (addrBook2);
    saveAddressBook(addrBook2, "/home/fips/saved_adrbooks/local_book_preProcess.vcf");

    pocketPCPIM::AddressBookSyncer addrSyncer1("MYPARTNERID");
    kdDebug(2120) << "Calling AddressBookSyncer::preProcess" << endl;
    addrSyncer1.preProcess (addrBook1);
    saveAddressBook(addrBook1, "/home/fips/saved_adrbooks/device_book_preProcess.vcf");
    
    KSyncTest kSyncer;
    KABC::AddressBook targetBook;
    KABC::ResourceFile* rFile3 = new KABC::ResourceFile("/home/fips/target.vcf");
    //KABC::ResourceNull* rFile3 = new KABC::ResourceNull(); //("/home/fips/target.vcf");
    targetBook.addResource (rFile3);
    //targetBook.load();
    
    kSyncer.syncAddressBook(&addrBook1, &addrBook2, m_preferences->getAddressBookFirstSync(), m_preferences->getAddressBookOverwrite(), 
                            m_preferences->getAddressBookLoadLog(), m_preferences->getAddressBookSyncing(), m_partnerId, &targetBook);
    
    kdDebug(2120) << "Calling AddressBookSyncer::postProcess" << endl;
    addrSyncer2.postProcess(addrBook2);
    
    kdDebug(2120) << "Calling AddressBookSyncer::postProcess" << endl;
    addrSyncer1.postProcess(addrBook1);
    
    
    addrSyncer2.postProcess(targetBook);        
    addrSyncer1.postProcess(targetBook);
    
    // now both address books should have the same content!! (<- this is deprecated!)
    // now... addrBook2 contains the synced data sets..
    // for testing: write one of them into the output-file and not into the original file
    
    KABC::Addressee::List addressees;
     
    switch (m_preferences->getAddressBookSyncing())
    {
        case MERGE:
            kdDebug(2120) << "Merging addressbooks" << endl;
            addressees = targetBook.allAddressees();
            break;
        case DEVICE:
            kdDebug(2120) << "sync to device" << endl;
            addressees = addrBook1.allAddressees();
            break;
        case LOCAL:
            kdDebug(2120) << "sync to local file" << endl;
            addressees = addrBook2.allAddressees();
            break;
    }
  
    QFile f( output );
    if ( !f.open( IO_WriteOnly ) ) 
    {
        appendRedText ("could not save synced address book!!");
        return;
    }
    QTextStream t( &f );
    KABC::VCardConverter vcard;
    t << vcard.createVCards( addressees );
}


void PocketPCMainWindow::slotSyncKonnector()
{
    /*
    appendRedText("syncing addressbook using konnector");
    
    // create the connector....
    KSync::PocketPCKonnector konnector(0);
    konnector.setPdaName(m_preferences->getPdaName());
    
    KSync::SynceeList synceeList;
    if (!konnector.readSyncees())
    {
        appendRedText("could not read syncees!");
        return;
    }
    
    //konnectorwriteAction->setEnabled(true);
     
    synceeList = konnector.syncees();
    */
}


void PocketPCMainWindow::slotSyncCalendars()
{
    syncCalendars (m_preferences->getCalendarFileName(), m_preferences->getCalendarLocal(), m_preferences->getCalendarOutput(), 
                   m_preferences->getCalendarFirstSync(), m_preferences->getCalendarOverwrite(), 
                   m_preferences->getCalendarLoadLog(), m_preferences->getCalendarSyncing(), CALENDAR);
}


void PocketPCMainWindow::slotSyncEvents()
{
    syncCalendars (m_preferences->getEventsFileName(), m_preferences->getEventsLocal(), m_preferences->getEventsOutput(), 
                   m_preferences->getEventsFirstSync(), m_preferences->getEventsOverwrite(), 
                   m_preferences->getEventsLoadLog(), m_preferences->getEventsSyncing(), EVENTS);

}


void PocketPCMainWindow::slotSyncTodos()
{
    syncCalendars (m_preferences->getTodosFileName(), m_preferences->getTodosLocal(), m_preferences->getTodosOutput(), 
                   m_preferences->getTodosFirstSync(), m_preferences->getTodosOverwrite(), 
                   m_preferences->getTodosLoadLog(), m_preferences->getTodosSyncing(), TODOS);

}


void PocketPCMainWindow::syncCalendars (const QString& p_fileDevice, const QString& p_fileLocal, const QString& p_fileOutput, 
                                        bool p_firstSync, bool p_overwrite, bool p_loadLog, SyncingOption p_syncOption, 
                                        const CalendarSyncName p_name)
{
    if (p_fileDevice.isEmpty() || p_fileLocal.isEmpty() || p_fileOutput.isEmpty())
    {
        QMessageBox::critical (this, "Critical failure", "Please provide all file names in your preferences!");
        return;
    }
    // load the data into calendar-objects
    KCal::CalendarLocal deviceCal;
    KCal::CalendarLocal localCal;
    
    deviceCal.load(p_fileDevice);
    localCal.load(p_fileLocal);
    
    // manipulate local calendar...
    pocketPCPIM::CalendarSyncer calSyncer1("MYPARTNERID");
    calSyncer1.preProcess(localCal);        
    
    pocketPCPIM::CalendarSyncer calSyncer2("MYPARTNERID");
    calSyncer2.preProcess(deviceCal);

    KCal::CalendarLocal targetCal;
        
    KSyncTest kSyncer;
    kSyncer.syncCalendar(&deviceCal, &localCal, p_firstSync, p_overwrite, p_loadLog, p_syncOption, p_name, &targetCal);
    
    if (targetCal.incidences().begin() == targetCal.incidences().end())
        kdDebug(2120) << "DER KALENDAR SUCKT VIELLEICHT!!!!" << endl;
    
    kdDebug(2120) << "postprocessing targetCal with local calendar" << endl;
    calSyncer1.postProcess(targetCal);
    kdDebug(2120) << "postprocessing targetCal with device calendar" << endl;
    calSyncer2.postProcess(targetCal);
    
    switch (p_syncOption)
    {
        case MERGE:
            targetCal.save(p_fileOutput);
            break;
        case DEVICE:
            deviceCal.save(p_fileOutput);
            break;
        case LOCAL:
            localCal.save(p_fileOutput);
            break;
    }
}


void PocketPCMainWindow::appendColoredText (const QColor& p_col, const QString& p_text)
{
    QColor col = m_outputTextWidget->color();
    m_outputTextWidget->setColor (p_col);
    m_outputTextWidget->append (p_text);
    m_outputTextWidget->setColor (col);
}


void PocketPCMainWindow::appendRedText (const QString& p_text)
{
    appendColoredText (QColor(255, 125, 125), p_text);
}


void PocketPCMainWindow::appendBlueText (const QString& p_text)
{
    appendColoredText (QColor(125, 125, 255), p_text);
}


void PocketPCMainWindow::printObjectIds (const QValueList<uint32_t>& p_valList, const QString& p_text, const QColor& p_col)
{    
    QValueList<uint32_t>::const_iterator it = p_valList.begin();
    for (; it != p_valList.end(); ++it)
        appendColoredText (p_col, QString::number(*it, 16) + " " + p_text);
}


void PocketPCMainWindow::saveAddressBook (KABC::AddressBook& p_addrBook, const QString& p_fileName)
{    
    QFile f( p_fileName );
    if ( !f.open( IO_WriteOnly ) ) 
    {
        appendRedText ("could not save address book!!");
        return;
    }
    QTextStream t( &f );
    KABC::VCardConverter vcard;
    t << vcard.createVCards( p_addrBook.allAddressees() );
}
