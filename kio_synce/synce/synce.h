/*
    kio_synce - an ioslave for KDE2
    Copyright (C) 2001  David Eriksson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef __synce_h__
#define __synce_h__

#include <qstring.h>
#include <qcstring.h>


#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

//
// Use a special namespace for this, because of name clashes
// (for example HANDLE)
//
namespace RAPI
{
extern "C"
{
#include "rapi.h"
};
};

class QCString;

class kio_synceProtocol : public KIO::SlaveBase
{
private:
	bool mIsInitialized;

public:
  kio_synceProtocol(const QCString &pool_socket, const QCString &app_socket);
  virtual ~kio_synceProtocol();

  virtual void listDir( const KURL & url );
	virtual void stat( const KURL & url );
	virtual void get(const KURL& url);

private:
	bool init();
	void uninit();

	QString slashToBackslash(const QString& path);
	void createUDSEntry( const RAPI::CE_FIND_DATA* source, KIO::UDSEntry & destination );
};


#endif
