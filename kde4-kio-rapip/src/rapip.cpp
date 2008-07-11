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

#include <KComponentData>
#include <KMimeType>
#include <QFile>
#include <synce.h>
#include <rapip.h>
#include <stdlib.h>

#define ANYFILE_BUFFER_SIZE (64*1024)

static bool show_hidden_files = true;


kio_rapipProtocol::kio_rapipProtocol(const QByteArray &pool_socket, const QByteArray &app_socket)
        : SlaveBase("kio_rapip", pool_socket, app_socket)
{
    isConnected = false;
}


kio_rapipProtocol::~kio_rapipProtocol()
{
    closeConnection();
}


void kio_rapipProtocol::setHost(const QString& _host, quint16 /*_port*/,
				const QString& /*_user*/, const QString& /*_pass*/ )
{
    if (actualHost != _host) {
        actualHost = _host;
        closeConnection();
    }
}


void kio_rapipProtocol::openConnection()
{
    HRESULT hr;

    if (!actualHost.isEmpty()) {
      //See http://doc.trolltech.com/4.4/porting4.html#qstring for why this is done
      QByteArray asciiData = actualHost.toAscii();
      const char *goodData = asciiData.constData();
      rapiconn = synce::rapi_connection_from_name(goodData);
    } else {
      rapiconn = synce::rapi_connection_from_name(NULL);
    }

    if (!rapiconn) {
      ceOk = false;
      isConnected = false;
      error(KIO::ERR_COULD_NOT_CONNECT, actualHost);
      return;
    }

    synce::rapi_connection_select(rapiconn);

    ceOk = true;

    hr = synce::CeRapiInit();

    if (FAILED(hr)) {
        ceOk = false;
        isConnected = false;
	synce::rapi_connection_destroy(rapiconn);
        error(KIO::ERR_COULD_NOT_CONNECT, actualHost);
    } else {
        isConnected = true;
        connected();
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::closeConnection()
{
    if (isConnected) {
        synce::CeRapiUninit();
	synce::rapi_connection_destroy(rapiconn);
    }

    isConnected = false;
}


QString kio_rapipProtocol::adjust_remote_path()
{
    WCHAR path[MAX_PATH];
    QString returnPath;

    if (ceOk) {
        if (synce::CeGetSpecialFolderPath(CSIDL_PERSONAL, sizeof(path), path)) {
            returnPath = QString::fromUtf16(path);
        } else {
            ceOk = false;
        }
    }

    return returnPath;
}


bool kio_rapipProtocol::checkRequestURL(const KUrl& url)
{
    if (url.path().isEmpty()) {
        const QString path = adjust_remote_path().replace("\\", "/");
        if (path.isEmpty()) {
            closeConnection();
            KUrl newUrl(url);
            redirection(newUrl);
        } else {
            KUrl newUrl(url);
            newUrl.setPath(path);
            redirection(newUrl);
        }
        finished();
        return false;
    }
    return true;
}


bool kio_rapipProtocol::list_matching_files(QString &path)
{
    bool success = false;
    synce::CE_FIND_DATA *find_data = NULL;
    DWORD file_count = 0;
    KIO::UDSEntry udsEntry;
    synce::CE_FIND_DATA *entry = NULL;
    KMimeType::Ptr mt;
    KUrl tmpUrl;

    if (ceOk) {
        ceOk = synce::CeFindAllFiles(
                   path.utf16(),
                   (show_hidden_files ? 0 : FAF_ATTRIB_NO_HIDDEN) |
                   FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
                   &file_count, &find_data);
        if (ceOk) {
            totalSize(file_count);
            for (DWORD i = 0; i < file_count; i++) {
                udsEntry.clear();
                entry = find_data + i;

		udsEntry.insert( KIO::UDSEntry::UDS_NAME, QString::fromUtf16(entry->cFileName) );
		udsEntry.insert( KIO::UDSEntry::UDS_SIZE, entry->nFileSizeLow );
		udsEntry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH |S_IXUSR | S_IXGRP | S_IXOTH );
		udsEntry.insert( KIO::UDSEntry::UDS_MODIFICATION_TIME, synce::filetime_to_unix_time(&entry->ftLastWriteTime));

                if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
		    udsEntry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR );
		    udsEntry.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory" );
                } else {
		    udsEntry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG );

                    tmpUrl.setPath(synce::wstr_to_ascii(entry->cFileName));
                    mt = KMimeType::findByUrl(tmpUrl);
		    udsEntry.insert( KIO::UDSEntry::UDS_MIME_TYPE, mt->name() );
                }
                listEntry(udsEntry, false);
            }
            listEntry(udsEntry, true);
            success = true;
        } else {
            closeConnection();
        }
        synce::CeRapiFreeBuffer(find_data);
    }

    return success;
}


void kio_rapipProtocol::get(const KUrl& url)
{
    DWORD bytes_read;
    unsigned char buffer[ANYFILE_BUFFER_SIZE];
    QByteArray array;
    KIO::filesize_t processed_size = 0;
    QString qPath;
    KMimeType::Ptr mt;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(url)) {
            mt = KMimeType::findByUrl(url);
            mimeType(mt->name());
            qPath = url.path().replace("/", "\\");
            HANDLE remote = synce::CeCreateFile(qPath.utf16(), GENERIC_READ, 0, NULL,
                                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
            if (!(INVALID_HANDLE_VALUE == remote)) {
                do {
                    if ((ceOk = synce::CeReadFile(remote, buffer, ANYFILE_BUFFER_SIZE, &bytes_read, NULL))) {
                        if (ceOk && bytes_read > 0) {
			    /*QByteArray.setRawData and resetRawData are deprecated; this method WFM but 
			     *is probably not optimal - Tejas Guruswamy <masterpatricko@gmail.com>
			     */
                            array = QByteArray((char *) buffer, bytes_read);
                            data(array);
			    array.clear();
                            processed_size += bytes_read;
                            processedSize(processed_size);
                        }
                    }
                } while (ceOk && bytes_read > 0);
                if (ceOk) {
                    data(QByteArray());
                    processedSize(processed_size);
                    finished();
                } else {
                    error(KIO::ERR_COULD_NOT_READ, url.prettyUrl());
                    closeConnection();
                }
                synce::CeCloseHandle(remote);
            } else {
                error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyUrl());
                closeConnection();
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::put(const KUrl& url, int /* permissions */, KIO::JobFlags flags)
{
    int result;
    DWORD bytes_written;
    QByteArray buffer;
    KMimeType::Ptr mt;
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(url)) {
            qPath = url.path().replace("/", "\\");
            if (synce::CeGetFileAttributes(qPath.utf16()) != 0xFFFFFFFF) {
                if (flags & KIO::Overwrite) {
                    if (!(ceOk = synce::CeDeleteFile(qPath.utf16()))) {
                        error(KIO::ERR_CANNOT_DELETE, url.prettyUrl());
                        closeConnection();
                        ceOk = false;
                    }
                } else {
                    error(KIO::ERR_FILE_ALREADY_EXIST, url.prettyUrl());
                    ceOk = false;
                }
            }
            if (ceOk) {
                Qt::HANDLE remote = synce::CeCreateFile(qPath.utf16(), GENERIC_WRITE, 0, NULL,
                                                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
                if (!(INVALID_HANDLE_VALUE == remote)) {
                    do {
                        dataReq();
                        result = readData(buffer);
                        if (result > 0) {
                            ceOk = synce::CeWriteFile(remote, (unsigned char *) buffer.data(), buffer.size(), &bytes_written, NULL);
                        }
                    } while (result > 0 && ceOk);
                    if (ceOk) {
                        finished();
                    } else {
                        error(KIO::ERR_COULD_NOT_WRITE, url.prettyUrl());
                        closeConnection();
                    }
                    synce::CeCloseHandle(remote);
                } else {
                    error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, url.prettyUrl());
                    closeConnection();
                }
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::listDir(const KUrl& _url)
{
    KUrl url(_url);
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(url)) {
            qPath = url.path();
            if (qPath.right(1) != "/" ) {
                qPath.append("/");
            }
            qPath.append("*").replace("/", "\\");
            if (list_matching_files(qPath)) {
                finished();
            } else {
                error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.prettyUrl());
                closeConnection();
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::mkdir(const KUrl& url, int /* permissions */)
{
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(url)) {
            qPath = url.path().replace("/", "\\");
            if (synce::CeCreateDirectory(qPath.utf16(), NULL)) {
                finished();
            } else {
                error(KIO::ERR_DIR_ALREADY_EXIST, url.prettyUrl());
                closeConnection();
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::del(const KUrl& url, bool isFile)
{
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(url)) {
            qPath = url.path().replace("/", "\\");
            if (isFile) {
                ceOk = synce::CeDeleteFile(qPath.utf16());
            } else {
                ceOk = synce::CeRemoveDirectory(qPath.utf16());
            }
            if (ceOk) {
                finished();
            } else {
                error(KIO::ERR_CANNOT_DELETE, url.prettyUrl());
                closeConnection();
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::stat(const KUrl & url)
{
    KIO::UDSEntry udsEntry;
    DWORD attributes;
    DWORD fileSize;
    KMimeType::Ptr mt;
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(url)) {
            qPath = url.path().replace("/","\\");
            if ((attributes = synce::CeGetFileAttributes(qPath.utf16())) !=  0xFFFFFFFF) {

		udsEntry.insert( KIO::UDSEntry::UDS_NAME, url.fileName());
		udsEntry.insert( KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH |S_IXUSR | S_IXGRP | S_IXOTH);

                if (attributes & FILE_ATTRIBUTE_DIRECTORY) {

		    udsEntry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
		    udsEntry.insert( KIO::UDSEntry::UDS_SIZE, 0);
		    udsEntry.insert( KIO::UDSEntry::UDS_MIME_TYPE, "inode/directory");
                    mimeType("inode/directory");

                } else {
		    int tmpFileSize;
                    HANDLE remote = synce::CeCreateFile(qPath.utf16(), GENERIC_READ, 0, NULL,
                                                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		    /* For Qt4 inserting udsEntries is a one step process so tmpFile size is used
		     * to hold the file size instead of assigning it inside the if-block
		     */
                    if (!(INVALID_HANDLE_VALUE == remote)) {
                        if ((fileSize = synce::CeGetFileSize(remote, NULL)) != 0xFFFFFFFF) {
                            tmpFileSize = fileSize;
                        } else {
                            tmpFileSize = 0;
                        }
                        synce::CeCloseHandle(remote);
                    } else {
                        tmpFileSize = 0;
                    }

		    udsEntry.insert( KIO::UDSEntry::UDS_SIZE, tmpFileSize);
		    udsEntry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);

                    mt = KMimeType::findByUrl(url);
		    udsEntry.insert( KIO::UDSEntry::UDS_MIME_TYPE, mt->name());
                    mimeType(mt->name());
                }
                statEntry(udsEntry);
                finished();
            } else {
                unsigned int lastError = synce::CeGetLastError();
                if (lastError == S_OK) {
                    closeConnection();
                    redirection(url);
                    finished();
                } else if (lastError == E_ABORT) {
                } else if (lastError == E_ACCESSDENIED) {
                    error(KIO::ERR_ACCESS_DENIED, url.prettyUrl());
                } else if (lastError == E_HANDLE) {
                } else if (lastError == E_INVALIDARG) {
                } else if (lastError == E_NOTIMPL) {
                } else if (lastError == E_OUTOFMEMORY) {
                } else if (lastError == E_PENDING) {
                } else if (lastError == E_POINTER) {
                } else if (lastError == E_UNEXPECTED) {
                } else if (lastError == ERROR_FILE_NOT_FOUND) {
                    error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
                } else if (lastError == ERROR_PATH_NOT_FOUND) {
                    error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
                } else if (lastError == ERROR_DIR_NOT_EMPTY) {
                } else if (lastError == ERROR_INVALID_PARAMETER) {
                } else {
                    closeConnection();
                    redirection(url);
                    finished();
                }
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::mimetype( const KUrl& url)
{
    QString qPath;
    DWORD attributes;
    KMimeType::Ptr mt;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(url)) {
            qPath = url.path();
            if ((attributes = synce::CeGetFileAttributes(qPath.utf16())) !=  0xFFFFFFFF) {
                if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
                    mimeType("inode/directory");
                } else {
                    mt = KMimeType::findByUrl(url);
                    mimeType(mt->name());
                }
                finished();
            } else {
                error(KIO::ERR_DOES_NOT_EXIST, url.prettyUrl());
                closeConnection();
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::rename(const KUrl& src, const KUrl& dst, KIO::JobFlags flags)
{
    QString sPath;
    QString dPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(src) && checkRequestURL(dst)) {
            sPath = src.path().replace("/", "\\");
            dPath = dst.path().replace("/", "\\");
            if (synce::CeGetFileAttributes(dPath.utf16()) !=  0xFFFFFFFF) {
                if (flags & KIO::Overwrite) {
                    if (!(ceOk = synce::CeDeleteFile(dPath.utf16()))) {
                        error(KIO::ERR_CANNOT_DELETE, dst.prettyUrl());
                        closeConnection();
                        ceOk = false;
                    }
                } else {
                    error(KIO::ERR_FILE_ALREADY_EXIST, dPath);
                    ceOk = false;
                }
            }
            if (ceOk) {
                if (synce::CeGetFileAttributes(sPath.utf16()) !=  0xFFFFFFFF) {
                    if (synce::CeMoveFile(sPath.utf16(), dPath.utf16())) {
                        finished();
                    } else {
                        error(KIO::ERR_CANNOT_RENAME, dst.prettyUrl());
                        closeConnection();
                    }
                } else {
                    error(KIO::ERR_DOES_NOT_EXIST, src.prettyUrl());
                    closeConnection();
                }
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::copy(const KUrl& src, const KUrl& dst, int /* permissions */, KIO::JobFlags flags)
{
    QString sPath;
    QString dPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (checkRequestURL(src) && checkRequestURL(dst)) {
            sPath = src.path().replace("/", "\\");
            dPath = dst.path().replace("/", "\\");
            if (synce::CeGetFileAttributes(dPath.utf16()) !=  0xFFFFFFFF) {
                if (flags & KIO::Overwrite) {
                    if (!(ceOk = synce::CeDeleteFile(dPath.utf16()))) {
                        error(KIO::ERR_CANNOT_DELETE, dst.prettyUrl());
                        closeConnection();
                        ceOk = false;
                    }
                } else {
                    error(KIO::ERR_FILE_ALREADY_EXIST, dst.prettyUrl());
                    ceOk = false;
                }
            }
            if (ceOk) {
                if (synce::CeGetFileAttributes(sPath.utf16()) !=  0xFFFFFFFF) {
                    if (synce::CeCopyFile(sPath.utf16(), dPath.utf16(), true)) {
                        finished();
                    } else {
                        error(KIO::ERR_CANNOT_RENAME, dst.prettyUrl());
                        closeConnection();
                    }
                } else {
                    error(KIO::ERR_DOES_NOT_EXIST, src.prettyUrl());
                    closeConnection();
                }
            }
        }
    }
    setTimeoutSpecialCommand(60);
}


void kio_rapipProtocol::slave_status() {
    slaveStatus(actualHost, isConnected);
}


void kio_rapipProtocol::special(const QByteArray & /* data*/)
{
    closeConnection();
    error(KIO::ERR_CONNECTION_BROKEN, actualHost);
}


extern "C"
{
    KDE_EXPORT int kdemain(int argc, char **argv)
    {
        KComponentData componentData( "kio_rapip" );

        if (argc != 4) {
            exit(-1);
        }

        kio_rapipProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }
}
