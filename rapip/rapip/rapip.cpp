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


#include <qfile.h>
#include <kinstance.h>
#include <kmimetype.h>
#include <synce.h>
#include <stdlib.h>
#include "rapip.h"

#define ANYFILE_BUFFER_SIZE (64*1024)

static bool show_hidden_files = true;


kio_rapipProtocol::kio_rapipProtocol(const QCString &pool_socket, const QCString &app_socket)
        : SlaveBase("kio_rapi", pool_socket, app_socket)
{
    isConnected = false;
}


kio_rapipProtocol::~kio_rapipProtocol()
{
    closeConnection();
}


void kio_rapipProtocol::setHost(const QString& _host, int /*_port*/,
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
    
    if (!actualHost.isEmpty())
        synce::synce_set_connection_filename(actualHost.ascii());
    else
        synce::synce_set_default_connection_filename();

    ceOk = true;

    hr = synce::CeRapiInit();
    
    if (FAILED(hr)) {
        ceOk = false;
        isConnected = false;
        error(KIO::ERR_COULD_NOT_CONNECT, actualHost);
    } else {
        isConnected = true;
        connected();
    }
}


void kio_rapipProtocol::closeConnection()
{
    if (isConnected)
        synce::CeRapiUninit();
    isConnected = false;
}


QString kio_rapipProtocol::adjust_remote_path()
{
    WCHAR path[MAX_PATH];
    QString returnPath;

    if (ceOk) {
        if (synce::CeGetSpecialFolderPath(CSIDL_PERSONAL, sizeof(path), path)) {
            returnPath = QString::fromUcs2(path);
        } else {
            ceOk = false;
        }
    }
    return returnPath;
}


bool kio_rapipProtocol::list_matching_files(QString &path)
{
    bool success = false;
    synce::CE_FIND_DATA *find_data = NULL;
    DWORD file_count = 0;
    KIO::UDSEntry udsEntry;
    synce::CE_FIND_DATA *entry = NULL;
    KIO::UDSAtom atom;
    KMimeType::Ptr mt;
    KURL tmpUrl;

    if (ceOk) {
        ceOk = synce::CeFindAllFiles(
                   path.ucs2(),
                   (show_hidden_files ? 0 : FAF_ATTRIB_NO_HIDDEN) |
                   FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
                   &file_count, &find_data);      
        totalSize(file_count);
        if (ceOk) {
            for (DWORD i = 0; i < file_count; i++) {
                udsEntry.clear();
                entry = find_data + i;

                atom.m_uds = KIO::UDS_NAME;
                atom.m_str = QString::fromUcs2(entry->cFileName).ascii();
                udsEntry.append( atom );

                atom.m_uds = KIO::UDS_SIZE;
                atom.m_long = entry->nFileSizeLow;
                udsEntry.append(atom);

                atom.m_uds = KIO::UDS_ACCESS;
                atom.m_long = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH |S_IXUSR | S_IXGRP | S_IXOTH;
                udsEntry.append(atom);

                atom.m_uds = KIO::UDS_MODIFICATION_TIME;
                atom.m_long = synce::filetime_to_unix_time(&entry->ftLastWriteTime);
                udsEntry.append(atom);

                if (entry->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    atom.m_uds = KIO::UDS_FILE_TYPE;
                    atom.m_long = S_IFDIR;
                    udsEntry.append(atom);

                    atom.m_uds = KIO::UDS_MIME_TYPE;
                    atom.m_str = "inode/directory";
                } else {
                    atom.m_uds = KIO::UDS_FILE_TYPE;
                    atom.m_long = S_IFREG;
                    udsEntry.append(atom);

                    tmpUrl.setPath(synce::wstr_to_ascii(entry->cFileName));
                    mt = KMimeType::findByURL(tmpUrl);
                    atom.m_uds = KIO::UDS_MIME_TYPE;
                    atom.m_str=mt->name();
                }
                udsEntry.append(atom);
                listEntry(udsEntry, false);
            }
            listEntry(udsEntry, true);
            success = true;
        }
        synce::CeRapiFreeBuffer(find_data);
    }
    return success;
}


void kio_rapipProtocol::get(const KURL& url)
{
    size_t bytes_read;
    unsigned char buffer[ANYFILE_BUFFER_SIZE];
    QByteArray array;
    KIO::filesize_t processed_size = 0;
    QString qPath;
    KMimeType::Ptr mt;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        mt = KMimeType::findByURL(url);
        mimeType(mt->name());
        qPath = url.path().replace("/", "\\");
        HANDLE remote = synce::CeCreateFile(qPath.ucs2(), GENERIC_READ, 0, NULL,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        if (!(INVALID_HANDLE_VALUE == remote)) {
            do {
                if ((ceOk = synce::CeReadFile(remote, buffer, ANYFILE_BUFFER_SIZE, &bytes_read, NULL))) {
                    if (ceOk && bytes_read > 0) {
                        array.setRawData((char *) buffer, bytes_read);
                        data(array);
                        array.resetRawData((char *) buffer, bytes_read);
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
                error(KIO::ERR_COULD_NOT_READ, url.prettyURL());
                closeConnection();
            }
            synce::CeCloseHandle(remote);
        } else {
            error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.prettyURL());
            closeConnection();
        }
    }
}


void kio_rapipProtocol::put(const KURL& url, int /* mode */, bool overwrite, bool /* resume */)
{
    int result;
    size_t bytes_written;
    QByteArray buffer;
    KMimeType::Ptr mt;
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        qPath = url.path().replace("/", "\\");
        if (synce::CeGetFileAttributes(qPath.ucs2()) != 0xFFFFFFFF) {
            if (overwrite) {
                if (!(ceOk = synce::CeDeleteFile(qPath.ucs2()))) {
                    error(KIO::ERR_CANNOT_DELETE, url.prettyURL());
                    closeConnection();
                    ceOk = false;
                }
            } else {
                error(KIO::ERR_FILE_ALREADY_EXIST, url.prettyURL());
                ceOk = false;
            }
        }
        if (ceOk) {
            HANDLE remote = synce::CeCreateFile(qPath.ucs2(), GENERIC_WRITE, 0, NULL,
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
                    error(KIO::ERR_COULD_NOT_WRITE, url.prettyURL());
                    closeConnection();
                }
                synce::CeCloseHandle(remote);
            } else {
                error(KIO::ERR_CANNOT_OPEN_FOR_WRITING, url.prettyURL());
                closeConnection();
            }
        }
    }
}


void kio_rapipProtocol::listDir(const KURL& _url)
{
    KURL url(_url);
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        if (url.path().isEmpty()) {
            url.setPath(adjust_remote_path().replace("\\", "/"));
            redirection(url);
            finished();
        } else {
            qPath = url.path();
            if (qPath.right(1) != "/" ) {
                qPath.append("/");
            }
            qPath.append("*").replace("/", "\\");
            if (list_matching_files(qPath)) {
                finished();
            } else {
                error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.prettyURL());
                closeConnection();
            }
        }
    }
}


void kio_rapipProtocol::mkdir(const KURL& url, int /* permissions */)
{
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        qPath = url.path().replace("/", "\\");
        if (synce::CeCreateDirectory(qPath.ucs2(), NULL)) {
            finished();
        } else {
            error(KIO::ERR_DIR_ALREADY_EXIST, url.prettyURL());
            closeConnection();
        }
    }
}


void kio_rapipProtocol::del(const KURL& url, bool isFile)
{
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        qPath = url.path().replace("/", "\\");
        if (isFile) {
            ceOk = synce::CeDeleteFile(qPath.ucs2());
        } else {
            ceOk = synce::CeRemoveDirectory(qPath.ucs2());
        }
        if (ceOk) {
            finished();
        } else {
            error(KIO::ERR_CANNOT_DELETE, url.prettyURL());
            closeConnection();
        }
    }
}


void kio_rapipProtocol::stat(const KURL & url)
{
    KIO::UDSEntry udsEntry;
    KIO::UDSAtom atom;
    DWORD attributes;
    DWORD fileSize;
    KMimeType::Ptr mt;
    QString qPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        qPath = url.path().replace("/","\\");
        if ((attributes = synce::CeGetFileAttributes(qPath.ucs2())) !=  0xFFFFFFFF) {
            atom.m_uds = KIO::UDS_NAME;
            atom.m_str = url.filename();
            udsEntry.append(atom);


            atom.m_uds = KIO::UDS_ACCESS;
            atom.m_long = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH |S_IXUSR | S_IXGRP | S_IXOTH;
            udsEntry.append(atom);

            if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
                atom.m_uds = KIO::UDS_FILE_TYPE;
                atom.m_long = S_IFDIR;
                udsEntry.append(atom);

                atom.m_uds = KIO::UDS_SIZE;
                atom.m_long = 0;
                udsEntry.append(atom);
            
                atom.m_uds = KIO::UDS_MIME_TYPE;
                atom.m_str="inode/directory";
                mimeType(atom.m_str);
            } else {
                HANDLE remote = synce::CeCreateFile(qPath.ucs2(), GENERIC_READ, 0, NULL,
                                     OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
                atom.m_uds = KIO::UDS_SIZE;
                if (!(INVALID_HANDLE_VALUE == remote)) {
                    if ((fileSize = synce::CeGetFileSize(remote, NULL)) != 0xFFFFFFFF) {
                        atom.m_long = fileSize;
                    } else {
                        atom.m_long = 0;
                    }
                    synce::CeCloseHandle(remote);
                } else {
                    atom.m_long = 0;
                }
                udsEntry.append(atom);
                
                atom.m_uds = KIO::UDS_FILE_TYPE;
                atom.m_long = S_IFREG;
                udsEntry.append(atom);

                mt = KMimeType::findByURL(url);
                atom.m_uds = KIO::UDS_MIME_TYPE;
                atom.m_str=mt->name();

                mimeType(atom.m_str);
            }
            udsEntry.append(atom);
            statEntry(udsEntry);

            finished();
        } else {
            closeConnection();
        }
    }
}


void kio_rapipProtocol::mimetype( const KURL& url)
{
    QString qPath;
    DWORD attributes;
    KMimeType::Ptr mt;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        qPath = url.path();
        if ((attributes = synce::CeGetFileAttributes(qPath.ucs2())) !=  0xFFFFFFFF) {
            if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
                mimeType("inode/directory");
            } else {
                mt = KMimeType::findByURL(url);
                mimeType(mt->name());
            }
            finished();
        } else {
            error(KIO::ERR_DOES_NOT_EXIST, url.prettyURL());
            closeConnection();
        }
    }
}


void kio_rapipProtocol::rename(const KURL& src, const KURL& dst, bool overwrite)
{
    QString sPath;
    QString dPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        sPath = src.path().replace("/", "\\");
        dPath = dst.path().replace("/", "\\");
        if (synce::CeGetFileAttributes(dPath.ucs2()) !=  0xFFFFFFFF) {
            if (overwrite) {
                if (!(ceOk = synce::CeDeleteFile(dPath.ucs2()))) {
                    error(KIO::ERR_CANNOT_DELETE, dst.prettyURL());
                    closeConnection();
                    ceOk = false;
                }
            } else {
                error(KIO::ERR_FILE_ALREADY_EXIST, dPath);
                ceOk = false;
            }
        }
        if (ceOk) {
            if (synce::CeGetFileAttributes(sPath.ucs2()) !=  0xFFFFFFFF) {
                if (synce::CeMoveFile(sPath.ucs2(), dPath.ucs2())) {
                    finished();
                } else {
                    error(KIO::ERR_CANNOT_RENAME, dst.prettyURL());
                    closeConnection();
                }
            } else {
                error(KIO::ERR_DOES_NOT_EXIST, src.prettyURL());
                closeConnection();
            }
        }
    }
}


void kio_rapipProtocol::copy(const KURL& src, const KURL& dst, int /* permissions */, bool overwrite)
{
    QString sPath;
    QString dPath;

    ceOk = true;

    if (!isConnected)
        openConnection();

    if (ceOk) {
        sPath = src.path().replace("/", "\\");
        dPath = dst.path().replace("/", "\\");
        if (synce::CeGetFileAttributes(dPath.ucs2()) !=  0xFFFFFFFF) {
            if (overwrite) {
                if (!(ceOk = synce::CeDeleteFile(dPath.ucs2()))) {
                    error(KIO::ERR_CANNOT_DELETE, dst.prettyURL());
                    closeConnection();
                    ceOk = false;
                }
            } else {
                error(KIO::ERR_FILE_ALREADY_EXIST, dst.prettyURL());
                ceOk = false;
            }
        }
        if (ceOk) {
            if (synce::CeGetFileAttributes(sPath.ucs2()) !=  0xFFFFFFFF) {
                if (synce::CeCopyFile(sPath.ucs2(), dPath.ucs2(), true)) {
                    finished();
                } else {
                    error(KIO::ERR_CANNOT_RENAME, dst.prettyURL());
                    closeConnection();
                }
            } else {
                error(KIO::ERR_DOES_NOT_EXIST, src.prettyURL());
                closeConnection();
            }
        }
    }
}


extern "C"
{
    int kdemain(int argc, char **argv) {
        KInstance instance( "kio_rapi" );

        if (argc != 4) {
            exit(-1);
        }

        kio_rapipProtocol slave(argv[2], argv[3]);
        slave.dispatchLoop();

        return 0;
    }
}
