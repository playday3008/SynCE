#include <qfile.h>

#include <kinstance.h>
#include <kmimetype.h>

#include "rapip.h"

#define WIDE_BACKSLASH   htole16('\\')

static bool show_hidden_files = false;
static bool rapiInitialized;

using namespace KIO;

kio_rapipProtocol::kio_rapipProtocol(const QCString &pool_socket, const QCString &app_socket)
    : SlaveBase("kio_rapi", pool_socket, app_socket)
{
  rapiInitialized = false;
}


kio_rapipProtocol::~kio_rapipProtocol()
{
  rapiInitialized = false;
}


bool kio_rapipProtocol::rapiInit()
{

  HRESULT hr;

  hr = CeRapiInit();

  if (FAILED(hr)) {
    return false;
  }

  rapiInitialized = true;

  return true;
}


WCHAR* kio_rapipProtocol::adjust_remote_path(WCHAR* old_path, bool free_path)
{
  WCHAR wide_backslash[2];
  WCHAR path[MAX_PATH];

  wide_backslash[0] = WIDE_BACKSLASH;
  wide_backslash[1] = '\0';

  /* Nothing to adjust if we have an absolute path */
  if (WIDE_BACKSLASH == old_path[0])
    return old_path;

  if (!CeGetSpecialFolderPath(CSIDL_PERSONAL, sizeof(path), path)) {
    return NULL;
  }

  synce::wstr_append(path, wide_backslash, sizeof(path));
  synce::wstr_append(path, old_path, sizeof(path));

  if (free_path)
    synce::wstr_free_string(old_path);

  synce_trace_wstr(path);
  return synce::wstrdup(path);
}


bool kio_rapipProtocol::list_matching_files(WCHAR* wide_path)
{
  bool success = false;
  BOOL result;
  CE_FIND_DATA* find_data = NULL;
  DWORD file_count = 0;
  DWORD i;

  synce_trace_wstr(wide_path);
  wide_path = adjust_remote_path(wide_path, true);
  synce_trace_wstr(wide_path);

  result = CeFindAllFiles(
             wide_path,
             (show_hidden_files ? 0 : FAF_ATTRIB_NO_HIDDEN) |
             FAF_ATTRIBUTES|FAF_LASTWRITE_TIME|FAF_NAME|FAF_SIZE_LOW|FAF_OID,
             &file_count, &find_data);

  UDSEntry udsEntry;

  if (!result) {
    goto exit;
  }

  for (i = 0; i < file_count; i++) {
    udsEntry.clear();
    CE_FIND_DATA *entry = find_data + i;
    UDSAtom atom;

    atom.m_uds = KIO::UDS_NAME;
    atom.m_str = synce::wstr_to_ascii(entry->cFileName);
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
      atom.m_str="inode/directory";
    } else {

      atom.m_uds = KIO::UDS_FILE_TYPE;
      atom.m_long = S_IFREG;
      udsEntry.append(atom);

      KURL u;
      u.setPath(synce::wstr_to_ascii(entry->cFileName));
      KMimeType::Ptr mt = KMimeType::findByURL(u);
      atom.m_uds = KIO::UDS_MIME_TYPE;
      atom.m_str=mt->name();
    }

    udsEntry.append( atom );
    listEntry(udsEntry, false);
  }

  listEntry(udsEntry, true);

  success = true;

exit:
  CeRapiFreeBuffer(find_data);

  return success;
}

#define ANYFILE_BUFFER_SIZE (64*1024)

void kio_rapipProtocol::get(const KURL& url)
{
  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, url.path());
      return;
    }
  }

  size_t bytes_read;

  KMimeType::Ptr mt = KMimeType::findByURL(url);
  mimeType(mt->name());

  QString fName = QFile::encodeName(url.path());

  unsigned char buffer[ANYFILE_BUFFER_SIZE];

  QByteArray array;

  KIO::filesize_t processed_size = 0;

  WCHAR* wide_filename = synce::wstr_from_ascii(fName.ascii());
  remote = CeCreateFile(wide_filename, GENERIC_READ, 0, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

  if (wide_filename)
    synce::wstr_free_string(wide_filename);

  if (INVALID_HANDLE_VALUE == remote) {
    error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.path());
    goto exit;
  }

  for(;;) {
    if (!CeReadFile(remote, buffer, ANYFILE_BUFFER_SIZE, &bytes_read, NULL)) {
      error(KIO::ERR_COULD_NOT_READ, url.path());
      goto exit;
    }

    if (0 == bytes_read) {
      break;
    }

    array.setRawData((char *) buffer, bytes_read);
    data(array);
    array.resetRawData((char *) buffer, bytes_read);

    processed_size += bytes_read;
    processedSize(processed_size);
  }

  data(QByteArray());

  processedSize(processed_size);

  finished();

exit:
  CeCloseHandle(remote);
}


void kio_rapipProtocol::put(const KURL & url, int /* mode */, bool /* overwrite */, bool /* resume */)
{
  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, url.path());
      return;
    }
  }

  size_t bytes_written;

  KMimeType::Ptr mt = KMimeType::findByURL(url);
  emit mimeType(mt->name());

  QString qPath = QFile::encodeName(url.path());
  qPath.replace('/', "\\");

  int result;

  WCHAR* wide_filename = synce::wstr_from_ascii(qPath.ascii());
  remote = CeCreateFile(wide_filename, GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);

  if (wide_filename)
    synce::wstr_free_string(wide_filename);

  if (INVALID_HANDLE_VALUE == remote) {
    error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.path());
    goto exit;
  }

  do {
    QByteArray buffer;
    dataReq();
    result = readData(buffer);

    if (result > 0) {
      if (!CeWriteFile(remote, (unsigned char *) buffer.data(), buffer.size(), &bytes_written, NULL)) {
        error(KIO::ERR_COULD_NOT_WRITE, url.path());
        goto exit;
      }
    }
  } while (result > 0);


  finished();

exit:
  CeCloseHandle(remote);
}


void kio_rapipProtocol::listDir( const KURL& _url)
{
  if (!rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, _url.path());
      return;
    }
  }

  KURL url(_url);

  QString qPath( QFile::encodeName(url.path()));

  if (qPath.isEmpty()) {
    url.setPath("/");
    redirection(url);
    finished();
    return;
  }

  if (qPath.right(1) != "/") {
    qPath = qPath.append('/');
  }

  qPath.replace('/', "\\");

  char new_path[MAX_PATH];
  snprintf(new_path, sizeof(new_path), "%s*", qPath.ascii());
  WCHAR* wide_path = synce::wstr_from_ascii(new_path);

  if (list_matching_files(wide_path)) {
    finished();
  } else {
    error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path());
  }

  if (wide_path)
    synce::wstr_free_string(wide_path);
}


void kio_rapipProtocol::mkdir(const KURL & url, int /* permissions */)
{
  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, url.path());
      return;
    }
  }

  QString qPath( QFile::encodeName(url.path()));
  qPath.replace('/', "\\");
  WCHAR* wide_path = synce::wstr_from_ascii(qPath.ascii());

  if (!CeCreateDirectory(wide_path, NULL)) {
    error(KIO::ERR_DIR_ALREADY_EXIST, url.path());
    goto exit;
  }

  finished();

exit:
  if (wide_path)
    synce::wstr_free_string(wide_path);

  return;
}


void kio_rapipProtocol::del(const KURL & url, bool isFile)
{
  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, url.path());
      return;
    }
  }

  QString qPath( QFile::encodeName(url.path()));
  qPath.replace('/', "\\");

  WCHAR *wide_path = synce::wstr_from_ascii(qPath.ascii());

  if (isFile) {
    if (!CeDeleteFile(wide_path)) {
      error(KIO::ERR_CANNOT_DELETE, url.path());
      goto exit;
    }
  } else {
    if (!CeRemoveDirectory(wide_path)) {
      error(KIO::ERR_CANNOT_DELETE, url.path());
      goto exit;
    }
  }

  if (wide_path)
    synce::wstr_free_string(wide_path);

  finished();

exit:
  return;
}


void kio_rapipProtocol::stat( const KURL & url)
{
  UDSEntry udsEntry;

  UDSAtom atom;

  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, url.path());
      return;
    }
  }

  QString qPath( QFile::encodeName(url.path()));
  qPath.replace('/', "\\");

  WCHAR* wide_path = synce::wstr_from_ascii(qPath.ascii());

  DWORD attributes = CeGetFileAttributes(wide_path);

  atom.m_uds = KIO::UDS_NAME;
  atom.m_str = url.filename();
  udsEntry.append( atom );

  atom.m_uds = KIO::UDS_SIZE;
  atom.m_long = 1024;
  udsEntry.append(atom);

  atom.m_uds = KIO::UDS_ACCESS;
  atom.m_long = S_IRUSR | S_IRGRP | S_IROTH | S_IWUSR | S_IWGRP | S_IWOTH |S_IXUSR | S_IXGRP | S_IXOTH;
  udsEntry.append(atom);

  if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
    atom.m_uds = KIO::UDS_FILE_TYPE;
    atom.m_long = S_IFDIR;
    udsEntry.append(atom);

    atom.m_uds = KIO::UDS_MIME_TYPE;
    atom.m_str="inode/directory";

    mimeType(atom.m_str);
  } else /* if (attributes > 0) */ {
    atom.m_uds = KIO::UDS_FILE_TYPE;
    atom.m_long = S_IFREG;
    udsEntry.append(atom);

    KMimeType::Ptr mt = KMimeType::findByURL(url);
    atom.m_uds = KIO::UDS_MIME_TYPE;
    atom.m_str=mt->name();

    mimeType(atom.m_str);
  } /*
        else {
            error(KIO::ERR_COULD_NOT_STAT, url.path());
            goto exit;
        }*/

  udsEntry.append( atom );
  statEntry(udsEntry);

  if (wide_path)
    synce::wstr_free_string(wide_path);

  finished();

  return;
};


void kio_rapipProtocol::mimetype( const KURL& url)
{
  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, url.path());
      return;
    }
  }

  QString qPath( QFile::encodeName(url.path()));
  qPath.replace('/', "\\");
  WCHAR* wide_path = synce::wstr_from_ascii(qPath.ascii());

  DWORD attributes = CeGetFileAttributes(wide_path);

  if (attributes & FILE_ATTRIBUTE_DIRECTORY) {
    mimeType("inode/directory");
  } else /*if (attributes > 0)*/ {
    KMimeType::Ptr mt = KMimeType::findByURL(url);
    mimeType(mt->name());
  }/* else {
            error(KIO::ERR_COULD_NOT_STAT, url.path());
            goto exit;
        }*/

  if (wide_path)
    synce::wstr_free_string(wide_path);

  finished();

  return;
}


void kio_rapipProtocol::rename (const KURL & src, const KURL & dest, bool  overwrite)
{
  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, src.path());
      return;
    }
  }

  QString sPath( QFile::encodeName(src.path()));
  sPath.replace('/', "\\");
  WCHAR* src_path = synce::wstr_from_ascii(sPath.ascii());

  QString dPath( QFile::encodeName(dest.path()));
  dPath.replace('/', "\\");
  WCHAR* dest_path = synce::wstr_from_ascii(dPath.ascii());

  if (overwrite) {
    if (!CeDeleteFile(dest_path)) {
      error(KIO::ERR_CANNOT_DELETE, dest.path());
      goto exit;
    }
  }

  if (!CeMoveFile(src_path, dest_path)) {
    error(KIO::ERR_FILE_ALREADY_EXIST, dPath);
    goto exit;
  }

  finished();

exit:
  if (dest_path)
    synce::wstr_free_string(dest_path);

  if (src_path)
    synce::wstr_free_string(src_path);
}


void kio_rapipProtocol::copy (const KURL &  src, const KURL & dest, int /* permissions */, bool overwrite)
{
  if (! rapiInitialized) {
    if (rapiInit() == false) {
      error(KIO::ERR_COULD_NOT_CONNECT, src.path());
      return;
    }
  }

  QString sPath( QFile::encodeName(src.path()));
  sPath.replace('/', "\\");
  WCHAR* src_path = synce::wstr_from_ascii(sPath.ascii());

  QString dPath( QFile::encodeName(dest.path()));
  dPath.replace('/', "\\");
  WCHAR* dest_path = synce::wstr_from_ascii(dPath.ascii());

  if (overwrite) {
    if (!CeDeleteFile(dest_path)) {
      error(KIO::ERR_CANNOT_DELETE, dest.path());
      goto exit;
    }
  }

  if (!CeCopyFile(src_path, dest_path, true)) {
    error(KIO::ERR_FILE_ALREADY_EXIST, dPath);
    goto exit;
  }

  finished();

exit:
  if (dest_path)
    synce::wstr_free_string(dest_path);

  if (src_path)
    synce::wstr_free_string(src_path);
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
