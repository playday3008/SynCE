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
#include <librra.h>

#include <qobject.h>
#include <qstring.h>
#include <qptrdict.h>
#include <qptrlist.h>

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

class Rra : QObject
{
Q_OBJECT
public: 
    struct ids {
        QPtrList<uint32_t> changedIds;
        QPtrList<uint32_t> unchangedIds;
        QPtrList<uint32_t> deletedIds;
        ObjectIdArray* object_ids;
        ObjectType* object_types;
        uint32_t* deleted_ids;
    };
    
    struct Partner {
        QString name;
        uint32_t id;
        int index;
    };

    Rra(QString pdaName);
    virtual ~Rra();
    QPtrDict<ObjectType> getTypes();
    struct Rra::ids& getIds(uint32_t type_id);
    struct Rra::Partner getPartner(uint32_t index);
    struct Rra::Partner getCurrentPartner();
    bool setPartner(struct Rra::Partner& partner);
    bool setCurrentPartner(uint32_t index);
    QString getVCard(uint32_t type_id, uint32_t object_id);
    uint32_t putVCard(QString& vCard, uint32_t type_id, uint32_t object_id);
//    QString getVCal(uint32_t type_id, uint32_t object_id);
    void deleteObject(uint32_t type_id, uint32_t object_id);
    KABC::Addressee getAddressee(uint32_t type_id, uint32_t object_id);
    bool putAddressee(const KABC::Addressee& addressee, uint32_t type_id, uint32_t ceUid, uint32_t *newCeUid);
    bool ok();
    bool connect();
    void disconnect(); 

private:
    HRESULT hr;
    RRA* rra;
    QString pdaName;
    bool rraOk;
    QPtrDict<ObjectType> objectTypes;
    struct ids _ids;
    int useCount;
};

#endif
