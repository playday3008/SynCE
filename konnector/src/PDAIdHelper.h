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
#ifndef KSYNCPDAIDHELPER_H
#define KSYNCPDAIDHELPER_H

#include "RecordType.h"

#include <qstring.h>
#include <qmap.h>
#include <qstringlist.h>

namespace KSync {     

/**
This little class assists with removed and added entries on the pda.

@author Christian Fremgen
*/
class PDAIdHelper{
public:
    PDAIdHelper(const QString& p_dirName);

    ~PDAIdHelper();
    
    void savePDAIds      (const QString& p_fileName, QMap<QString, pocketPCCommunication::RecordType>& p_statusMap);
    void readPDAIds      (const QString& p_fileName, QStringList& p_ids);
    void appendPDAIds    (const QString& p_fileName, QStringList& p_ids);
    void dumpPDAIds      (const QStringList& p_ids);
    
    QStringList getPDAAddedIds    (const QStringList& p_pdaIds, const QStringList& p_localIds);
    QStringList getPDARemovedIds  (const QStringList& p_pdaIds, const QStringList& p_localIds);
    QStringList getPDAModifiedIds (QMap<QString, pocketPCCommunication::RecordType>& p_statusMap, const QStringList& p_addedIds);
    
    QStringList createIdQStringList (const QMap<QString, pocketPCCommunication::RecordType>& p_statusMap);
    
private:
    QStringList getDiffIds (const QStringList& p_firstIds, const QStringList& p_secondIds);
            
    QString    m_dirName;
};

};

#endif
