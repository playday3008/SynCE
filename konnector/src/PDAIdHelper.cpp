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
#include "PDAIdHelper.h"

#include <qfile.h>
#include <kdebug.h>

namespace KSync {

PDAIdHelper::PDAIdHelper(const QString& p_dirName)
    : m_dirName(p_dirName)
{
}


PDAIdHelper::~PDAIdHelper()
{
}


void PDAIdHelper::savePDAIds (const QString& p_fileName, QMap<QString, pocketPCCommunication::RecordType>& p_statusMap)
{
    QFile f( m_dirName + "/" + p_fileName );
    if ( !f.open( IO_WriteOnly ) ) 
    {
        kdDebug(2120) << "could not open file " << m_dirName << "/" << p_fileName << " for writing" << endl;
        return;
    }
    QTextStream t( &f );
        
    QMap<QString, pocketPCCommunication::RecordType>::iterator it = p_statusMap.begin();
    QString delimiter = "||";
    QString string;
    for (; it != p_statusMap.end(); ++it)
    {
        string += it.key() + delimiter;
    }    
    
    t << string;
    
    f.close();
}


void PDAIdHelper::readPDAIds (const QString& p_fileName, QStringList& p_ids)
{
    QFile f(m_dirName + "/" + p_fileName);
    if (!f.open(IO_ReadOnly))
    {
        kdDebug(2120) << "could not open file " << m_dirName << "/" << p_fileName << " for reading" << endl;
        return;
    }
    
    QTextStream t(&f);
    QString string;
    
    t >> string;
    
    // split the string
    p_ids = QStringList::split ("||", string, false);
    
    f.close();        
}


void PDAIdHelper::appendPDAIds (const QString& p_fileName, QStringList& p_ids)
{
    QFile f(m_dirName + "/" + p_fileName);
    if (!f.open(IO_WriteOnly | IO_Append))
    {
        kdDebug(2120) << "could not open file " << m_dirName << "/" << p_fileName << " for appending" << endl;
        return;
    }
    
    QTextStream t(&f);
    QStringList::iterator it = p_ids.begin();
    QString delimiter = "||";
    QString string;
    
    for (; it != p_ids.end(); ++it)
    {
        string += *it + delimiter;
    }
    
    t << string;
    f.close();
}


void PDAIdHelper::dumpPDAIds(const QStringList& p_ids)
{
    QStringList::const_iterator it = p_ids.begin();
    for (; it != p_ids.end(); ++it)
        kdDebug(2120) << "locally stored ids: " << *it << endl;
}


QStringList PDAIdHelper::getPDAAddedIds (const QStringList& p_pdaIds, const QStringList& p_localIds)
{
    // ids which exists on the pda but not locally are added
    QStringList addedIds;
    
    addedIds = getDiffIds (p_pdaIds, p_localIds);    
    
    return addedIds;
}


QStringList PDAIdHelper::getPDARemovedIds (const QStringList& p_pdaIds, const QStringList& p_localIds)
{
    // ids which exists locally, but not on the pda are removed
    QStringList removedIds;
    
    removedIds = getDiffIds (p_localIds, p_pdaIds);
    
    return removedIds;
}


QStringList PDAIdHelper::getPDAModifiedIds(QMap<QString, pocketPCCommunication::RecordType>& p_statusMap, const QStringList& p_addedIds)
{
    QStringList idList;    
    
    QMap<QString, pocketPCCommunication::RecordType>::const_iterator it = p_statusMap.begin();
    for (; it != p_statusMap.end(); ++it)
    {
        if (it.data() == pocketPCCommunication::CHANGED && p_addedIds.find(it.key()) == p_addedIds.end()) // really modified!!!
            idList.append (it.key());
    }
    
    return idList;
}


QStringList PDAIdHelper::getDiffIds (const QStringList& p_firstIds, const QStringList& p_secondIds)
{
    QStringList diffIds;
    
    QStringList::const_iterator it = p_firstIds.begin();
    for (; it != p_firstIds.end(); ++it)
    {
        if (p_secondIds.find(*it) == p_secondIds.end())
            diffIds.append(*it);
    }
    
    return diffIds;
}


QStringList PDAIdHelper::createIdQStringList (const QMap<QString, pocketPCCommunication::RecordType>& p_statusMap)
{
    QStringList idList;
    
    QMap<QString, pocketPCCommunication::RecordType>::const_iterator it = p_statusMap.begin();
    for (; it != p_statusMap.end(); ++it)
        idList.append (it.key());
    
    return idList;
}


};
