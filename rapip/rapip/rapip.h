#ifndef _rapip_H_
#define _rapip_H_

#include <kio/slavebase.h>

#include <rapi.h>
#include <synce_log.h>

HANDLE remote;

class kio_rapipProtocol : public KIO::SlaveBase
{
public:
    kio_rapipProtocol(const QCString &pool_socket, const QCString &app_socket);
    virtual ~kio_rapipProtocol();
    virtual void mimetype(const KURL& url);
    virtual void get(const KURL& url);
    virtual void put(const KURL & url, int mode, bool overwrite, bool resume);
    virtual void listDir( const KURL& url);
    virtual void stat( const KURL & url);
    virtual void mkdir(const KURL & url, int permissions); 
    virtual void del(const KURL & url, bool isFile);
    virtual void rename (const KURL & src, const KURL & dest, bool  overwrite);
    virtual void copy (const KURL &  src, const KURL & dest, int permissions, bool overwrite);

private:
    bool rapiInit();
    WCHAR* adjust_remote_path(WCHAR* old_path, bool free_path);
    bool list_matching_files(WCHAR* wide_path);
};

#endif
