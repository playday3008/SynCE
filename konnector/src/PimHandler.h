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
    PimHandler(const QString& p_pdaName);

    /** in case we already have a Rra-Connection we can use this constuctor
      */
    PimHandler (KSharedPtr<Rra> p_rra);

    virtual ~PimHandler();

    /** A method to get a map which includes the id and the status.
      * status: CHANGED, DELETED, UNCHANGED
      * @see getIdStatusPro for how it is done
      */
//    virtual bool getIdStatus (QMap<QString, RecordType>& p_statusMap) = 0;

    virtual QString getPartnerId ();

    virtual QValueList<QPair<QString, QString> > getIdPairs ();


protected:

    bool getIdStatusPro (QMap<QString, RecordType>& p_statusMap, const uint32_t& p_typeId);

    /** A method to permanently delete entries on the pda!
      */
    virtual void deleteEntries (const struct Rra::ids& p_ids, const uint32_t& p_typeId, RecordType p_recType);

    /** This abstract method must be overwritten within each subclass to fill appropriate members/static members
      * with the type id.
      */
//    virtual bool getTypeId () = 0;

    virtual void deleteSingleEntry (const uint32_t& p_typeId, const uint32_t& p_objectId);

    uint32_t getOriginalId (const QString& p_id);
    bool isARraId (const QString& p_id);

    void clearIdPairs ();
    void addIdPair (const QString p_device, const QString p_local);

    QString         m_pdaName;   /**< The name of the device we want to use. */
    //Rra             m_rra;       /**< Connection handler. */
    KSharedPtr<Rra> m_rra;       /**< Connection handler. Shared to have one instance in many objects. */
    //Rra*            m_rra;

    const QString    m_appName;
    const QString    m_keyName;


private:
    void fillStatusMap (QMap<QString, RecordType>& p_statusMap, const struct Rra::ids& p_ids, RecordType p_recType);

    QString m_partnerId;
    QValueList<QPair<QString, QString> >    m_idPairs;
};

};

#endif
