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

#include "pimhandler.h"
#include <kdebug.h>

#include <libkdepim/progressmanager.h>


namespace PocketPCCommunication {

KPIM::ProgressItem *PimHandler::mProgressItem = NULL;
QString PimHandler::errorString = "";
QStringList PimHandler::errorEntries = QStringList();

PimHandler::PimHandler ()
    : m_pdaName("")
{
    actSteps = 0;
    maxSteps = 0;
}


PimHandler::~PimHandler()
{
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


void PimHandler::progressItemSetCompleted()
{
    mProgressItem->setComplete();
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


void PimHandler::setUidHelper(KSync::KonnectorUIDHelper *mUidHelper)
{
    this->mUidHelper = mUidHelper;
}

void PimHandler::setRra(Rra* rra)
{
    m_rra = rra;
    m_pdaName = m_rra->getPdaName();
}


void PimHandler::setError(QString errorString)
{
    PimHandler::errorString = errorString;
}

void PimHandler::addErrorEntry(QString errorEntry)
{
    errorEntries += errorEntry;
}

QString PimHandler::getError() {
    return errorString;
}


QStringList PimHandler::getErrorEntries()
{
    return errorEntries;
}


void PimHandler::resetError() {
    errorString = "";
    errorEntries.clear();
}
}
