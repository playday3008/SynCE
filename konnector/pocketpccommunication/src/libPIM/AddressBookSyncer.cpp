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
#include "AddressBookSyncer.h"
#include <qstringlist.h>
#include <kdebug.h>

namespace pocketPCPIM {

AddressBookSyncer::AddressBookSyncer(const QString p_partnerId)
    : PIMProcessor(p_partnerId)
{
}


AddressBookSyncer::~AddressBookSyncer()
{
}


void AddressBookSyncer::preProcess (KABC::AddressBook& p_addrBook)
{
    kdDebug(2120) << "AddressBookSyncer::preProcess: start" << endl;
    // iterate over addressBook and set the data...
    KABC::AddressBook::Iterator it = p_addrBook.begin();
    for (; it != p_addrBook.end(); ++it)
    {
        kdDebug(2120) << "current id: " << it->uid() << endl;
        
        //get the current remote-id!!! can be done with current m_partnerId!!
        QString remoteId = it->custom("POCKETPCCOMM", "REMOTE-ID-"+ m_partnerId);
        kdDebug(2120) << "partnerId:remoteId: " << m_partnerId << ":" << remoteId << endl;
        
        if (!remoteId.isEmpty())
        {
            kdDebug(2120) << "setting new local id to: " << m_partnerId << ":" << remoteId << endl;
            storeRemoteOrig(m_partnerId + ":" + remoteId, it->uid());                
            it->setUid(m_partnerId + ":" + remoteId);            
        }
        
        QStringList customs = it->customs();
        QStringList::Iterator strIt = customs.begin();
        
        for (; strIt != customs.end(); ++strIt)
        {
            kdDebug(2120) << "current custom key: " << *strIt << endl;
            if ((*strIt).startsWith ("POCKETPCCOMM-REMOTE-ID-"))
            {
                QString ids;
                ids = (*strIt).remove ("POCKETPCCOMM-REMOTE-ID-");
                QString partnerId = ids.left(ids.find (':'));
                QString remoteId = ids.right (ids.length() - ids.find (':') - 1);
                kdDebug(2120) << "AddressBookSyncer::preProcess: partner: " << partnerId << " remoteId: " << remoteId << endl;
                kdDebug(2120) << "AddressBookSyncer::preProcess: m_partnerId: " << m_partnerId << endl;
                
                appendRemoteKey(it->uid(), partnerId, remoteId);                                 
                
                // remove the custom key...
                it->removeCustom ("POCKETPCCOMM", "REMOTE-ID-"+partnerId);
            }    
        }
    }    
    dumpIdMaps();
    kdDebug(2120) << "AddressBookSyncer::preProcess: end" << endl;
}


void AddressBookSyncer::postProcess (KABC::AddressBook& p_addrBook)
{
    kdDebug(2120) << "AddressBookSyncer::postProcess: start" << endl;
    
    KABC::AddressBook::Iterator it = p_addrBook.begin();
    for (; it != p_addrBook.end(); ++it)
    {
        QString curId = (*it).uid();
        QString origId = getOrigKey(curId);
        if (!origId.isEmpty())
        {
            kdDebug(2120) << "changing from " << curId << " to " << origId << endl;
            it->setUid (origId);
            //p_addrBook.findByUid(curId).setUid(origId);
        }
        
        // get the vector of remote ids....
        const QValueVector<QPair<QString, QString> > customs = getRemoteKeys(curId);
        kdDebug(2120) << "number of saved customs: " << customs.size() << endl;
        QValueVector<QPair<QString, QString> >::ConstIterator custIt = customs.begin();
        for (; custIt != customs.end(); ++custIt)
        {
            kdDebug(2120) << "inserting custom property: " << (*custIt).first << ":" << (*custIt).second << endl;
            it->insertCustom ("POCKETPCCOMM", "REMOTE-ID-"+(*custIt).first, (*custIt).second);
        }        
    }
    
    kdDebug(2120) << "AddressBookSyncer::postProcess: end" << endl;
}



};
