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
#ifndef POCKETPCCOMMUNICATIONADDRESSBOOKHANDLER_H
#define POCKETPCCOMMUNICATIONADDRESSBOOKHANDLER_H

#include "PimHandler.h"
#include <kabc/addressbook.h>
#include "RecordType.h"

namespace pocketPCCommunication {

/**
This class handles an AddressBook which can be read from and written to a Windows CE device.

@author Christian Fremgen cfremgen@users.sourceforge.net
*/
class AddressBookHandler : public PimHandler
{
public:   
    /** Just a simple constructor.
      * @param p_pdaName as the name says :)
      */
    AddressBookHandler(const QString p_pdaName);

    AddressBookHandler (KSharedPtr<Rra> p_rra);

    virtual ~AddressBookHandler();

    /** This method gets the contacts from the device and stores them in p_addressBook.      
      * @param p_addressBook the addressees are stored in here
      * @param p_recType tell the method in what kind of records you are interested @see RecordType
      */
    bool getAddressBook (KABC::AddressBook& p_addressBook, RecordType p_recType = ALL);
    
    /** This method pushes the contacts from the addressBook to the device.      
      * @param p_addressBook the addressees are stored in here
      */
    bool putAddressBook (KABC::AddressBook& p_addressBook); // needs RecordType??

    /** Since KABC::AddressBook::insertAddressee does not always work, this method
      * can work with a list of addressees.
      */
    bool putAddressBook (KABC::Addressee::List& p_addresseeList);
        
    
    bool getIdStatus (QMap<QString, RecordType>& p_statusMap);
    
    void deleteAddressBook ();
        
    void addAddressees    (KABC::Addressee::List& p_addresseeList);
    void updateAddressees (KABC::Addressee::List& p_addresseeList);
    void removeAddressees (KABC::Addressee::List& p_addresseeList);
    
    bool getAddressees    (KABC::Addressee::List& p_addresseeList, const QStringList& p_ids);
    
    /*)
signals:
    void progress (int p_progress);
    */
protected:
    bool getTypeId ();
    
private:
    /** This private method retrieves all the requested information to fill the address book.
      * @param p_addressBook the addressees are stored in here
      * @param p_ids this struct holds all the ids (changed, unchanged, deleted)
      * @param p_recType specify in which information you are interested
      */
    void getAddressees (KABC::AddressBook& p_addressBook, const struct Rra::ids& p_ids, RecordType p_recType);
    
    virtual void deleteEntry (const uint32_t& p_objectId);

    static uint32_t    s_typeId;   /**< This static member stores the typeId belonging to "Contact" */
};

};

#endif
