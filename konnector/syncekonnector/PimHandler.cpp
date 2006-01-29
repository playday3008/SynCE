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

#include <libkdepim/progressmanager.h>

namespace pocketPCCommunication {

PimHandler::PimHandler (Rra *p_rra, KSync::KonnectorUIDHelper */*mUidHelper*/ )
    : m_pdaName(""), mProgressItem(NULL)
{
    m_pdaName = p_rra->getPdaName();
    m_rra = p_rra;
    actSteps = 0;
    maxSteps = 0;
}

PimHandler::~PimHandler()
{
}

void PimHandler::deleteSingleEntry (const uint32_t& p_typeId, const uint32_t& p_objectId)
{
    m_rra->deleteObject (p_typeId, p_objectId);
}

uint32_t PimHandler::getOriginalId(const QString& p_id)
{
    QString id = p_id;
    bool ok;

    return id.remove("RRA-ID-").toUInt(&ok, 16);
}


uint32_t PimHandler::getTypeId()
{
    init();
    return mTypeId;
}


void PimHandler::setIds(struct Rra::ids ids)
{
    this->ids = ids;
}


void PimHandler::setMaximumSteps(unsigned int maxSteps)
{
    this->maxSteps = maxSteps;
}

void PimHandler::setActualSteps(unsigned int actSteps)
{
    this->actSteps = actSteps;
    if (maxSteps > 0 && mProgressItem) {
        mProgressItem->setProgress((unsigned int) ((actSteps * 100) / maxSteps));
    }
}

void PimHandler::incrementSteps()
{
    actSteps++;
    if (maxSteps > 0 && mProgressItem) {
        mProgressItem->setProgress((unsigned int) ((actSteps * 100) / maxSteps));
    }
}

void PimHandler::setProgressItem(KPIM::ProgressItem *progressItem)
{
    mProgressItem = progressItem;
}

void PimHandler::setStatus(QString status)
{
    if (mProgressItem) {
        mProgressItem->setStatus(status);
    }
}

void PimHandler::resetSteps()
{
    actSteps = 0;
}
}
