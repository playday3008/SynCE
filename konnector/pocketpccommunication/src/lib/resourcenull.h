/*
    This file is part of libkdepim.

    Copyright (c) 2004 Cornelius Schumacher <schumacher@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

/* adapted by Christian Fremgen <cfremgen@users.sourceforge.net>
   this file was lost in the KDE-PIM 3.3.0 installation, but I needed it. :)
*/ 

#ifndef POCKETPCCOMMUNICATIONRESOURCENULL_H
#define POCKETPCCOMMUNICATIONRESOURCENULL_H

#include <kabc/resource.h>

namespace pocketPCCommunication {

/**
  This resource does nothing.
*/
class ResourceNull : public KABC::Resource
{
  public:
    ResourceNull( const KConfig *cfg ) : Resource( cfg ) {}
    ResourceNull() : Resource( 0 ) {}
    virtual ~ResourceNull() {}
  
    KABC::Ticket *requestSaveTicket() { return 0; }
    void releaseSaveTicket( KABC::Ticket * ) {}
    bool load() { return false; }
    bool save( KABC::Ticket * ) { return false; }
};

}

#endif
