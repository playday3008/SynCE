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
#ifndef POCKETPCCOMMUNICATIONPIMHANDLER_H
#define POCKETPCCOMMUNICATIONPIMHANDLER_H

#include "rra.h"
#include <ksharedptr.h>
#include "RecordType.h"

namespace pocketPCCommunication {

/**
This is the base class for pim data handlers like @ref AddressBookHandler or @ref CalendarHandler.
It stores the common members and provides the interface to an abstract method.
@author Christian Fremgen cfremgen@users.sourceforge.net
*/
class PimHandler
{
public:
    /** Just a simple constructor to initialize the members.
      * @param p_pdaName as the name says :)
      */
//    PimHandler(const QString& p_pdaName);

    /** in case we already have a Rra-Connection we can use this constuctor
      */
    PimHandler (KSharedPtr<Rra> p_rra);

    virtual ~PimHandler();

    uint32_t getTypeId();

    void setIds(struct Rra::ids ids);

    virtual bool init() = 0;

protected:
    uint32_t    mTypeId;   /**< This static member stores the typeId belonging to "Contact" */

    bool initialized;
    struct Rra::ids ids;

    virtual void deleteSingleEntry (const uint32_t& p_typeId, const uint32_t& p_objectId);

    uint32_t getOriginalId (const QString& p_id);

    QString         m_pdaName;   /**< The name of the device we want to use. */

    KSharedPtr<Rra> m_rra;       /**< Connection handler. Shared to have one instance in many objects. */
};

}

#endif
