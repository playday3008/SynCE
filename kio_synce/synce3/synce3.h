/*
	vim: expandtab tw=75

	kio_synce - an ioslave for KDE3

	Copyright (c) 2001-2003 David Eriksson <twogood@users.sourceforge.net>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the
  "Software"), to deal in the Software without restriction, including
  without limitation the rights to use, copy, modify, merge, publish,
  distribute, sublicense, and/or sell copies of the Software, and to permit
  persons to whom the Software is furnished to do so, subject to the
  following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
  NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
  USE OR OTHER DEALINGS IN THE SOFTWARE.

*/
#ifndef __synce3_h__
#define __synce3_h__

#include <qstring.h>
#include <qcstring.h>


#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

#include <rapi.h>


class QCString;

class kio_synceProtocol : public KIO::SlaveBase
{
	private:
		bool mIsInitialized;

	public:
		kio_synceProtocol(const QCString &pool, const QCString &app);
		virtual ~kio_synceProtocol();

		virtual void listDir( const KURL & url );
		virtual void stat( const KURL & url );
		virtual void get(const KURL& url);
		virtual void put(const KURL& url, int permissions, bool overwrite, bool resume);
		virtual void del(const KURL& url, bool isfile);
		virtual void mkdir(const KURL&url, int permissions);
		virtual void rename(const KURL& src, const KURL& dest, bool overwrite);

	private:
		bool init();
		void uninit();

		QString slashToBackslash(const QString& path);
		void createUDSEntry( const synce::CE_FIND_DATA* source, KIO::UDSEntry & destination );
};


#endif

