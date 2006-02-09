/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
 *                    Christian Fremgen <cfremgen@users.sourceforge.net>   *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining a *
 * copy of this software and associated documentation files (the           *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute, sublicense, and/or sell copies of the Software, and to      *
 * permit persons to whom the Software is furnished to do so, subject to   *
 * the following conditions:                                               *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 ***************************************************************************/

#ifndef POCKETPCCOMMUNICATIONPIMHANDLER_H
#define POCKETPCCOMMUNICATIONPIMHANDLER_H

#include "rra.h"

namespace KPIM {
    class ProgressItem;
}

namespace KSync {
    class KonnectorUIDHelper;
}

namespace PocketPCCommunication {

/**
This is the base class for pim data handlers like @ref AddressBookHandler or @ref CalendarHandler.
It stores the common members and provides the interface to an abstract method.
@author Christian Fremgen cfremgen@users.sourceforge.net
*/
class PimHandler
{
public:

    /** in case we already have a Rra-Connection we can use this constuctor
      */
    PimHandler ();

    virtual ~PimHandler();

    uint32_t getTypeId();

    void setIds(struct Rra::ids ids);

    void setProgressItem(KPIM::ProgressItem *progressItem);
    void setUidHelper(KSync::KonnectorUIDHelper *mUidHelper);
    void setRra(Rra* rra);

    virtual bool init() = 0;

protected:
    uint32_t    mTypeId;   /**< This static member stores the typeId belonging to "Contact" */

    bool initialized;
    struct Rra::ids ids;

    void setMaximumSteps(unsigned int maxSteps);

    void setActualSteps(unsigned int actSteps);

    void incrementSteps();

    void setStatus(QString status);

    void resetSteps();

    virtual void deleteSingleEntry (const uint32_t& p_typeId, const uint32_t& p_objectId);

    uint32_t getOriginalId (const QString& p_id);

    QString         m_pdaName;   /**< The name of the device we want to use. */

    Rra* m_rra;       /**< Connection handler. Shared to have one instance in many objects. */

    KPIM::ProgressItem *mProgressItem;

    unsigned int maxSteps;

    unsigned int actSteps;

    KSync::KonnectorUIDHelper *mUidHelper;
    Rra* rra;
};

}

#endif
