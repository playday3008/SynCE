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
#include "CalendarSyncer.h"
#include <kdebug.h>

namespace pocketPCPIM {

CalendarSyncer::CalendarSyncer(const QString p_partnerId)
    : PIMProcessor(p_partnerId)
{
}


CalendarSyncer::~CalendarSyncer()
{
}


void CalendarSyncer::preProcess  (KCal::Calendar& p_calendar)
{
    kdDebug(2120) << "CalendarSyncer::preProcess: start" << endl;
    
    KCal::Incidence::List incidences = p_calendar.incidences();
    KCal::Incidence::List::Iterator it = incidences.begin();
    for (; it != incidences.end(); ++it)
    {
        kdDebug(2120) << "current id: " << (*it)->uid() << endl;
        
        //get the current remote-id!!! can be done with current m_partnerId!!
        QString remoteId = (*it)->nonKDECustomProperty(("X-POCKETPCCOMM-REMOTE-ID-"+ m_partnerId).latin1());
        kdDebug(2120) << "partnerId:remoteId: " << m_partnerId << ":" << remoteId << endl;
        
        if (!remoteId.isEmpty())
        {
            //kdDebug(2120) << "setting new local id to: " << m_partnerId << ":" << remoteId << endl;
            kdDebug(2120) << "setting new local id to: " << m_partnerId << ":" << remoteId << endl;
            storeRemoteOrig(m_partnerId + ":" + remoteId, (*it)->uid());                
            //storeRemoteOrig(m_partnerId + ":" + remoteId, (*it)->uid());                
            (*it)->setUid(m_partnerId + ":" + remoteId);            
            //(*it)->setUid(m_partnerId + ":" + remoteId);            
        }
        
        QMap<QCString,QString> customs = (*it)->customProperties();
        QMap<QCString,QString>::Iterator strIt = customs.begin();
        
        for (; strIt != customs.end(); ++strIt)
        {
            kdDebug(2120) << "current custom key: " << *strIt << endl;
            QString key = strIt.key();
            if (key.startsWith ("X-POCKETPCCOMM-REMOTE-ID-"))
            {
                QString ids;
                ids = key.remove ("X-POCKETPCCOMM-REMOTE-ID-");
                QString partnerId = ids.left(ids.find (':'));
                QString remoteId = strIt.data(); //ids.right (ids.length() - ids.find (':') - 1);
                kdDebug(2120) << "CalendarSyncer::preProcess: partner: " << partnerId << " remoteId: " << remoteId << endl;
                kdDebug(2120) << "CalendarSyncer::preProcess: m_partnerId: " << m_partnerId << endl;
                
                appendRemoteKey((*it)->uid(), partnerId, remoteId);                                 
                
                // remove the custom key...
                (*it)->removeNonKDECustomProperty (("X-POCKETPCCOMM-REMOTE-ID-"+partnerId).latin1());
            }    
        }
    }    
    dumpIdMaps();
    kdDebug(2120) << "CalendarSyncer::preProcess: end" << endl;
}


void CalendarSyncer::postProcess (KCal::Calendar& p_calendar)
{
    kdDebug(2120) << "CalendarSyncer::postProcess: start" << endl;
    KCal::Incidence::List incidences = p_calendar.incidences();
    KCal::Incidence::List::Iterator it = incidences.begin();
    for (; it != incidences.end(); ++it)
    {
        QString curId = (*it)->uid();
        QString origId = getOrigKey(curId);
        if (!origId.isEmpty())
        {
            kdDebug(2120) << "changing from " << curId << " to " << origId << endl;
            (*it)->setUid (origId);
        }
        
        // get the vector of remote ids....
        const QValueVector<QPair<QString, QString> > customs = getRemoteKeys(curId);
        kdDebug(2120) << "number of saved customs: " << customs.size() << endl;
        QValueVector<QPair<QString, QString> >::ConstIterator custIt = customs.begin();
        for (; custIt != customs.end(); ++custIt)
        {
            kdDebug(2120) << "inserting custom property: " << (*custIt).first << ":" << (*custIt).second << endl;
            (*it)->setNonKDECustomProperty (QString("X-POCKETPCCOMM-REMOTE-ID-"+(*custIt).first).latin1(), (*custIt).second);
        }        
    }
    
    kdDebug(2120) << "CalendarSyncer::postProcess: end" << endl;
}

};
