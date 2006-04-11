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
#ifndef MATCHMAKER_H
#define MATCHMAKER_H

#include <rra/matchmaker.h>
#include <qstring.h>

/**
@author Volker Christian
*/
class MatchMaker{
public:
    struct Partner {
        QString name;
        uint32_t id;
        int index;
    };

    MatchMaker(QString pdaName);

    ~MatchMaker();

    bool connect();

    bool disconnect();

    /** Set the current partnership index (1 or 2) */
    bool set_current_partner(uint32_t index);

    /** Get the current partnership index (1 or 2) */
    bool get_current_partner(uint32_t* index);

    /** Set numeric ID for partnership 1 or 2 */
    bool set_partner_id(uint32_t index, uint32_t id);

    /** Get numeric ID for partnership 1 or 2 */
    bool get_partner_id(uint32_t index, uint32_t* id);

    /** Set computer name for partnership 1 or 2 */
    bool set_partner_name(uint32_t index, const char* name);

    /** Get computer name for partnership 1 or 2 */
    bool get_partner_name(uint32_t index, char** name);

    /** Generate a new partnership at index 1 or 2 */
    bool replace_partnership(uint32_t index);

    bool getPartner(uint32_t index, struct MatchMaker::Partner *partner);
    bool getCurrentPartner(struct MatchMaker::Partner *partner);
    bool partnerCreate(uint32_t *index);
    bool partnerReplace(int index);
    bool setPartner(struct MatchMaker::Partner& partner);
    bool setCurrentPartner(uint32_t index);


    /**
    If we don't have a partnership with this device:
        If there is an empty partnership slot:
            Create a partnership in empty slot
        Else
            Fail

    If we now have a partnership with this device:
        Set current partnership to our partnership with the device
    */

    bool create_partnership(uint32_t* index);

private:
    RRA_Matchmaker *matchmaker;
    QString pdaName;
    unsigned int useCount;
    bool rraOk;
};

#endif
