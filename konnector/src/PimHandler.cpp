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
#include "PimHandler.h"
#include <kdebug.h>

namespace pocketPCCommunication {


PimHandler::PimHandler(const QString& p_pdaName)
    : m_pdaName(p_pdaName), m_appName("POCKETPCCOMM"), m_keyName("REMOTE-ID"), m_partnerId("")
{
    m_rra = new Rra(p_pdaName);
}


PimHandler::PimHandler (KSharedPtr<Rra> p_rra)
    : m_pdaName(""), m_appName("POCKETPCCOMM"), m_keyName("REMOTE-ID"), m_partnerId("")
{
    m_pdaName = p_rra->getPdaName();
    m_rra = p_rra;
}

PimHandler::~PimHandler()
{
}


bool PimHandler::getIdStatusPro (QMap<QString,RecordType>& p_statusMap, const uint32_t& p_typeId)
{
    //m_rra->connect();

    struct Rra::ids ids;
    /*
    if (!m_rra->getIds (p_typeId, &ids))
    {
        m_rra->disconnect();
        return false;
    }
    */
    if (!m_rra->getIds (p_typeId, &ids))
        return false;

    // now we have the necessary data...
    // iterate over the complete data structure and fill the map

    fillStatusMap (p_statusMap, ids, CHANGED);
    fillStatusMap (p_statusMap, ids, DELETED);
    fillStatusMap (p_statusMap, ids, UNCHANGED);

    //m_rra->disconnect();
    return true;
}


void PimHandler::fillStatusMap(QMap<QString,RecordType>& p_statusMap, const struct Rra::ids& p_ids, RecordType p_recType)
{
    //kdDebug() << "PimHandler::fillStatusMap" << endl;

    QValueList<uint32_t>::const_iterator it;
    QValueList<uint32_t>::const_iterator end;

    switch (p_recType)
    {
        case CHANGED:
            it = p_ids.changedIds.begin();
            end = p_ids.changedIds.end();
            //size = p_ids.changedIds.size();
            break;
        case DELETED:
            it = p_ids.deletedIds.begin();
            end = p_ids.deletedIds.end();
            //size = p_ids.deletedIds.size();
            break;
        case UNCHANGED:
            it = p_ids.unchangedIds.begin();
            end = p_ids.unchangedIds.end();
            //size = p_ids.unchangedIds.size();
            break;
        case ALL: // not reasonable to have ALL here! added to avoid warning of gcc
            break;
    }

    for (; it != end; ++it)
    {
        //kdDebug() << "     " << QString::number(*it,16).rightJustify(8, '0') << endl;
        p_statusMap["RRA-ID-" + QString::number(*it,16).rightJustify(8, '0')] = p_recType; // number must be padded!
    }
}


void PimHandler::deleteEntries (const struct Rra::ids& p_ids, const uint32_t& p_typeId, RecordType p_recType)
{
    kdDebug() << "PimHandler::deleteEntries" << endl;

    m_rra->connect();
    QValueList<uint32_t>::const_iterator it;
    QValueList<uint32_t>::const_iterator end;

    switch (p_recType)
    {
        case CHANGED:
            it = p_ids.changedIds.begin();
            end = p_ids.changedIds.end();
            break;
        case DELETED:
            it = p_ids.deletedIds.begin();
            end = p_ids.deletedIds.end();
            break;
        case UNCHANGED:
            it = p_ids.unchangedIds.begin();
            end = p_ids.unchangedIds.end();
            break;
        case ALL: // not reasonable to have ALL here! added to avoid warning of gcc
            break;
    }

    for (; it != end; ++it)
    {
        kdDebug() << "deleting: " << p_typeId << " " << *it << endl;
        m_rra->deleteObject (p_typeId, *it);
    }

    m_rra->disconnect();
}


void PimHandler::deleteSingleEntry (const uint32_t& p_typeId, const uint32_t& p_objectId)
{
    m_rra->deleteObject (p_typeId, p_objectId);
}


QString PimHandler::getPartnerId()
{
    if (m_partnerId.isEmpty())
    {
        m_rra->connect();
        uint32_t id;
        m_rra->getMatchMaker()->get_partner_id(1, &id);
        m_partnerId = QString::number (id, 16);
        m_rra->disconnect();
    }
    return m_partnerId;
}


void PimHandler::clearIdPairs ()
{
    m_idPairs.clear();
}


void PimHandler::addIdPair (const QString p_device, const QString p_local)
{
    m_idPairs.push_back (qMakePair (p_device, p_local));
}


QValueList<QPair<QString, QString> > PimHandler::getIdPairs ()
{
    return m_idPairs;
}


uint32_t PimHandler::getOriginalId(const QString& p_id)
{
    QString id = p_id;
    bool ok;

    return id.remove("RRA-ID-").toUInt(&ok, 16);
}


bool PimHandler::isARraId (const QString& p_id)
{
    return p_id.startsWith ("RRA-ID-");
}

};
