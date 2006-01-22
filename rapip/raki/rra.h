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

#ifndef RRA_H
#define RRA_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rapi.h>
#include <ksharedptr.h>

extern "C" {
#include <rra/syncmgr.h>
#include <rra/matchmaker.h>
#include <rra/timezone.h>
#include <rra/uint32vector.h>
}

#include <qobject.h>
#include <qstring.h>
#include <qmap.h>
#include <qvaluelist.h>

#ifdef WITH_DMALLOC
#include <dmalloc.h>
#include <kde_dmalloc.h>
#endif

namespace KABC {
    class Addressee;
}

/**
@author Volker Christian,,,
*/

class Rra
{
public:
    struct ids {
        QValueList<uint32_t> changedIds;
        QValueList<uint32_t> unchangedIds;
        QValueList<uint32_t> deletedIds;
        RRA_Uint32Vector* uidVector;
    };

    struct Partner {
        QString name;
        uint32_t id;
        int index;
    };

    Rra(QString pdaName);
    Rra();
    virtual ~Rra();

    bool markIdUnchanged(uint32_t type_id, uint32_t object_id);
    bool getTypes(QMap<int, RRA_SyncMgrType *> *);
    uint32_t getTypeForName (const QString& p_typeName);
    bool getIds();
    void getIdsForType(uint32_t type_id, struct Rra::ids *ids);
    QString getVCard(uint32_t type_id, uint32_t object_id);
    uint32_t putVCard(QString& vCard, uint32_t type_id, uint32_t object_id);
    QString getVEvent(uint32_t type_id, uint32_t object_id);
    uint32_t putVEvent(QString& vEvent, uint32_t type_id, uint32_t object_id);
    QString getVToDo(uint32_t type_id, uint32_t object_id);
    uint32_t putVToDo(QString& vToDo, uint32_t type_id, uint32_t object_id);
    void deleteObject(uint32_t type_id, uint32_t object_id);
    bool isConnected () const;
    QString getPdaName () const;
    void subscribeForType(uint32_t typeId);
    void unsubscribeTypes();
    bool removeDeletedObjects(uint32_t mTypeId, RRA_Uint32Vector* deleted_ids);
    bool registerAddedObjects(uint32_t mTypeId, RRA_Uint32Vector* added_ids);

    bool ok();
    bool connect();
    void disconnect();

    void setLogLevel(int p_level);
    bool getTimezone(RRA_Timezone *tzi);


private:
    bool checkForAllIdsRead();
    HRESULT hr;
    QMap<uint32_t, struct Rra::ids *> idMap;
    RRA_SyncMgr* rra;
    QString pdaName;
    bool rraOk;
    int useCount;
    RRA_Timezone tzi;
};

#endif
