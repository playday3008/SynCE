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


#ifndef _rapip_H_
#define _rapip_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <kio/slavebase.h>
#include <rapi.h>


class kio_rapipProtocol : public KIO::SlaveBase
{
public:
    kio_rapipProtocol(const QCString &pool_socket, const QCString &app_socket);
    virtual ~kio_rapipProtocol();
    virtual void setHost(const QString& _host, int _port, const QString& _user, const QString& _pass );
    virtual void openConnection ();
    virtual void closeConnection ();
    virtual void mimetype(const KURL& url);
    virtual void get(const KURL& url);
    virtual void put(const KURL & url, int mode, bool overwrite, bool resume);
    virtual void listDir( const KURL& url);
    virtual void stat( const KURL & url);
    virtual void mkdir(const KURL & url, int permissions);
    virtual void del(const KURL & url, bool isFile);
    virtual void rename (const KURL & src, const KURL & dest, bool  overwrite);
    virtual void copy (const KURL &  src, const KURL & dest, int permissions, bool overwrite);
    virtual void special( const QByteArray & data );
    virtual void slave_status();

private:
    QString adjust_remote_path();
    bool checkRequestURL(const KURL& url);
    bool list_matching_files(QString &path);
    bool ceOk;
    bool isConnected;
    QString actualHost;
    synce::RapiConnection *rapiconn;
};

#endif
