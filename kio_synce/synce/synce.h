#ifndef __synce_h__
#define __synce_h__

#include <qstring.h>
#include <qcstring.h>


#include <kurl.h>
#include <kio/global.h>
#include <kio/slavebase.h>

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
