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
#include "PIMProcessor.h"
#include <kdebug.h>

namespace pocketPCPIM {

PIMProcessor::PIMProcessor(const QString p_partnerId)
    : m_partnerId(p_partnerId)
{
}


PIMProcessor::~PIMProcessor()
{
}


void PIMProcessor::dumpIdMaps()
{   
    kdDebug(2120) << "Dumping m_remoteOrig: " << endl;
    QMap<QString, QString>::Iterator it = m_remoteOrig.begin();
    for (; it != m_remoteOrig.end(); ++it)
    {
        kdDebug(2120) << "remoteId: " << it.key() << "    origId: " << it.data() << endl;
    }
    
    kdDebug(2120) << "Dumping m_origAllKeys: " << endl;
    QMap<QString, QValueVector<QPair<QString, QString> > >::Iterator mapIt = m_origAllKeys.begin();    
    for (; mapIt != m_origAllKeys.end(); ++mapIt)
    {
        kdDebug(2120) << "current original key: " << mapIt.key() << endl;
        QValueVector<QPair<QString, QString> >::Iterator vecIt = (*mapIt).begin();
        for (; vecIt != (*mapIt).end(); ++vecIt)
        {
            kdDebug(2120) << "stored: partnerId: " << (*vecIt).first << "    remoteId: " << (*vecIt).second << endl;
        }
    }       
}


void PIMProcessor::appendRemoteKey (const QString& p_origId, const QString& p_partnerId, const QString& p_remoteId)
{
    m_origAllKeys[p_origId].append (qMakePair (p_partnerId, p_remoteId));
}


void PIMProcessor::storeRemoteOrig (const QString& p_remoteId, const QString& p_origId)
{
    m_remoteOrig[p_remoteId] = p_origId;
}


const QString PIMProcessor::getOrigKey (const QString& p_remoteId)
{
    return m_remoteOrig[p_remoteId];
}


const QValueVector<QPair<QString, QString> > PIMProcessor::getRemoteKeys (const QString& p_origId)
{
    return m_origAllKeys[p_origId];
}


};
