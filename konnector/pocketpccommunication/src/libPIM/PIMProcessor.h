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
#ifndef POCKETPCPIMPIMPROCESSOR_H
#define POCKETPCPIMPIMPROCESSOR_H

#include <qmap.h>
#include <qstring.h>
#include <qvaluevector.h>
#include <qstringlist.h>
#include <qpair.h>

namespace pocketPCPIM {

struct IdMapping
{
        QString        m_origId;
        QStringList    m_remoteIds;
};

/**
@author Christian Fremgen
*/
class PIMProcessor{
public:
    PIMProcessor(const QString p_partnerId);

    ~PIMProcessor();
    
protected:    
    
    void dumpIdMaps ();
    void appendRemoteKey (const QString& p_origId, const QString& p_partnerId, const QString& p_remoteId);
    void storeRemoteOrig (const QString& p_remoteId, const QString& p_origId);
    
    const QString getOrigKey   (const QString& p_remoteId);
    const QValueVector<QPair<QString, QString> > getRemoteKeys (const QString& p_origId);
    
    QString                     m_partnerId;
    QMap<QString, struct IdMapping>    m_remoteIds;
    
private:
    QMap<QString, QValueVector<QPair<QString, QString> > >    m_origAllKeys;  // orig-key, pair of partnerId and remoteId
    QMap<QString, QString>                                    m_remoteOrig;
    
};

};

#endif
