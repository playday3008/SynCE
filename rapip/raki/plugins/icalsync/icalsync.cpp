/***************************************************************************
 * Copyright (c) 2003 Volker Christian <voc@users.sourceforge.net>         *
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


#include "icalsync.h"

#include <rra.h>

#include <kdebug.h>

namespace ICAL {
    #include <ical.h>
};


IcalSync::~IcalSync()
{
}


void IcalSync::generatePdaDelta()
{
    uint32_t *v;
    ICAL::icalcomponent *comp;

    if (rra->connect()) {
        struct Rra::ids ids;

        rra->getIds(getObjectTypeId(), &ids);   // errorcheck

        incTotalSteps(ids.changedIds.count() + ids.deletedIds.count());

        for (v = ids.changedIds.first(); v && !stopRequested(); v = ids.changedIds.next()) {
            comp = rra->getEvent(getObjectTypeId(), *v);
            kdDebug(2120) << icalcomponent_as_ical_string(comp) << endl;
        }

        for (v = ids.unchangedIds.first(); v && !stopRequested(); v = ids.unchangedIds.next()) {
            comp = rra->getEvent(getObjectTypeId(), *v);
            kdDebug(2120) << icalcomponent_as_ical_string(comp) << endl;
        }

        for (v = ids.deletedIds.first(); v && !stopRequested(); v = ids.deletedIds.next()) {
            comp = rra->getEvent(getObjectTypeId(), *v);
            kdDebug(2120) << icalcomponent_as_ical_string(comp) << endl;
        }
        rra->disconnect();
    }
}


bool IcalSync::sync()
{
    kdDebug(2120) << "--------------" << endl;
    generatePdaDelta();
    kdDebug(2120) << "--------------" << endl;
    return true;
}

