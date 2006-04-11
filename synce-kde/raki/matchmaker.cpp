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
#include "matchmaker.h"
#include "rapiwrapper.h"
#include <klocale.h>
#include <kdebug.h>

MatchMaker::MatchMaker(QString pdaName)
{
    this->pdaName = pdaName;
    this->matchmaker = NULL;
    this->useCount = 0;
}


bool MatchMaker::connect()
{
    rraOk = true;

    if (useCount == 0) {
        if (!Ce::rapiInit(pdaName)) {
            rraOk = false;
        } else {
            matchmaker = rra_matchmaker_new();
            if (!matchmaker) {
                Ce::rapiUninit();
                rraOk = false;
            } else {
                kdDebug(2120) << i18n("Matchmaker-Connect") << endl;
            }
        }
    }

    if (rraOk) {
        useCount++;
    }

    return rraOk;
}


bool MatchMaker::disconnect()
{
    if (useCount > 0) {
        useCount--;
    }

    if(useCount == 0) {
        kdDebug(2120) << i18n("Matchmaker-Disconnect") << endl;
        rra_matchmaker_destroy(matchmaker);
        Ce::rapiUninit();
        matchmaker = NULL;
    }

    return true;
}


bool MatchMaker::set_current_partner(uint32_t index)
{
    return rra_matchmaker_set_current_partner(matchmaker, index);
}


bool MatchMaker::get_current_partner(uint32_t* index)
{
    return rra_matchmaker_get_current_partner(matchmaker, index);
}


bool MatchMaker::set_partner_id(uint32_t index, uint32_t id)
{
    return rra_matchmaker_set_partner_id(matchmaker, index, id);
}


bool MatchMaker::get_partner_id(uint32_t index, uint32_t* id)
{
    return rra_matchmaker_get_partner_id(matchmaker, index, id);
}


bool MatchMaker::set_partner_name(uint32_t index, const char* name)
{
    return rra_matchmaker_set_partner_name(matchmaker, index, name);
}


bool MatchMaker::get_partner_name(uint32_t index, char** name)
{
    // TODO delete the string name is not done
    return rra_matchmaker_get_partner_name(matchmaker, index, name);
}


bool MatchMaker::replace_partnership(uint32_t index)
{
    return rra_matchmaker_replace_partnership(matchmaker, index);
}


bool MatchMaker::create_partnership(uint32_t* index)
{
    return rra_matchmaker_create_partnership(matchmaker, index);
}


bool MatchMaker::getPartner(uint32_t index, struct MatchMaker::Partner *partner)
{
    char *name = NULL;

    rraOk = true;

    if (!get_partner_id(index, &partner->id)) {
        partner->id = 0;
        rraOk = false;
    }

    if (!get_partner_name(index, &name)) {
        partner->name = "";
        rraOk = false;
    } else {
        partner->name = name;
        free(name);
    }

    partner->index = index;

    return rraOk;
}


bool MatchMaker::getCurrentPartner(struct MatchMaker::Partner *partner)
{
    uint32_t currentIndex;

    rraOk = true;

    if ((rraOk = get_current_partner(&currentIndex))) {
        rraOk = getPartner(currentIndex, partner);
    }

    if (!rraOk) {
        partner->id = 0;
        partner->name = "";
        partner->index = 0;
        rraOk = false;
    }

    return rraOk;
}


bool MatchMaker::partnerCreate(uint32_t *index)
{
    rraOk = true;

    if (create_partnership(index)) {
        kdDebug(2120) <<
                i18n("Partnership creation succeeded. Using partnership index %1").arg(*index) << endl;
    } else {
        kdDebug(2120) << i18n("Partnership creation failed.") << endl;
        *index = 0;
        rraOk = false;
    }

    return rraOk;
}


bool MatchMaker::partnerReplace(int index)
{
    rraOk = true;

    if (index == 1 || index == 2) {
        if (replace_partnership(index)) {
            kdDebug(2120) << i18n("Partnership replacement succeeded.") << endl;
        } else {
            kdDebug(2120) << i18n("Partnership replacement failed.") << endl;
            rraOk = false;
        }
    } else {
        kdDebug(2120) <<
                i18n("Invalid or missing index of partnership to replace.") <<
                endl;
        rraOk = false;
    }

    return rraOk;
}


bool MatchMaker::setPartner(struct MatchMaker::Partner& partner)
{
    if (!set_partner_id(partner.index, partner.id)) {
        return false;
    }

    if (!set_partner_name(partner.index, partner.name.ascii())) {
        return false;
    }

    return true;
}


bool MatchMaker::setCurrentPartner(uint32_t index)
{
    if (!set_current_partner(index)) {
        return false;
    }

    return true;
}


MatchMaker::~MatchMaker()
{
}
