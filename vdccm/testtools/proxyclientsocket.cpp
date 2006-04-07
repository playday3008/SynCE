//
// C++ Implementation: proxyclientsocket
//
// Description:
//
//
// Author: Volker Christian <voc@users.sourceforge.net>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "proxyclientsocket.h"
#include <iostream>
#include <iomanip>
#include <synce.h>

using namespace std;

ProxyClientSocket::ProxyClientSocket(char *path)
    : LocalClientSocket(path), ContinousNode(1, 0)
{
}


ProxyClientSocket::~ProxyClientSocket()
{
}


void ProxyClientSocket::event(Descriptor::eventType /*et*/)
{
    char buf[256];

    int n = read(getDescriptor(), buf, 256);

    if (n == 0) {
        exit(0);
    }
    cout <<  endl << "Echo: " << flush;

    for (int i = 0; i < n; i++) {
        cout << hex << "0x" << (int) buf[i] << " " << flush;
    }
}


void ProxyClientSocket::printPackage(unsigned char *buf)
{
    uint32_t length = *(uint32_t *) buf;
    char lineBuf[8];

    cout << "0x" << hex << setw(8) << setfill('0') << 0 << "  " << flush;
    for (int i = 0; i < length + 4; i++) {
        cout << "0x" << hex << setw(2) << setfill('0') <<  (int) buf[i] << " " << flush;
        lineBuf[i % 8] = buf[i];
        if ((i + 1) % 8 == 0) {
            cout << " " << flush;
            for (int n = 0; n < 8; n++) {
                char out = (isprint(lineBuf[n]) ? lineBuf[n] : '.');
                cout << out << flush;
            }
            cout << endl;
            if ((i + 1) < length + 4) {
                cout << "0x" << hex << setw(8) << setfill('0') << i + 1 << "  " << flush;
            }
        } else if (i + 1 == length + 4) {
            int count = (i + 1) % 8;
            for (int n = 1; n < (8 - count) * 5 + 2; n++) {
                std::cout << " " << std::flush;
            }
            for (int n = 0; n < count; n++) {
                char out = ( isprint( lineBuf[ n ] ) ? lineBuf[ n ] : '.' );
                std::cout << out << std::flush;
            }
            std::cout << std::endl;
        }
    }
}


bool ProxyClientSocket::writePackage(unsigned char *buf)
{
    uint32_t length = *(uint32_t *) buf;

    cout << endl << "Write package to device" << endl;
    cout << "=======================" << endl;
    printPackage(buf);
    if (write(getDescriptor(), buf, length + 4) != length + 4) {
        return false;
    }

    return true;
}



size_t ProxyClientSocket::readNumBytes(unsigned char *buffer, size_t numBytes)
{
    size_t totalBytes = 0;
    size_t nBytes = 0;
    unsigned char *bufptr = buffer;

    do {
        nBytes = read(getDescriptor(), bufptr, numBytes - totalBytes);
        /*
        int bytesToRead = (numBytes - totalBytes) < 1024 ? numBytes - totalBytes : 1024;
        nBytes = read(getDescriptor(), bufptr, bytesToRead);
        cout << "Numbytes: " << nBytes << ",  TotalBytes: " << numBytes << ", NumRead: " << totalBytes << endl;
        sleep(1);
                                                    */
        if (nBytes == 0) {
            return 0;
        }
        bufptr += nBytes;
        totalBytes +=nBytes;
    } while (totalBytes < numBytes);

    return totalBytes;
}


bool ProxyClientSocket::readPackage(unsigned char **ergBuf)
{
    uint32_t length;

    if (read(getDescriptor(), &length, 4) <= 0) {
        shutdown();
        exit(0);
    }

    unsigned char *buf;

    buf = new unsigned char[length + 4];

    if (readNumBytes(buf + 4, length) < length) {
        delete[] buf;
        shutdown();
        exit(0);
    }
    /*
    if (read(getDescriptor(), buf + 4, length) <= 0) {
        delete[] buf;
        shutdown();
        return false;
    }
    */

    memcpy(buf, (unsigned char *) &length, 4);

    cout << endl << "Read package from device" << endl;
    cout << "========================" << endl;
    printPackage(buf);

    *ergBuf = buf;

    return true;
}


void ProxyClientSocket::ceGetSpecialFolderPath(uint32_t folderType)
{
    unsigned char buffer[16] = {
        0x0c, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x04, 0x01, 0x00, 0x00};

    /* This is highly not portable :-) */
    uint32_t *bFolderType = (uint32_t*)(buffer + 8);
    *bFolderType = folderType;

    if (writePackage(buffer)) {
        unsigned char *buf;
        if (readPackage(&buf)) {
            // Work with data in buffer buf
            // The buffer is allocate in readPackage - but we have to delete it here!
            delete[] buf;
        }
    }
}


void ProxyClientSocket::ceFindAllFiles(char *path)
{
    LPWSTR lPath = synce::wstr_from_ascii(path);
    size_t pathLength = synce::wstrlen(lPath) * sizeof(WCHAR) + 2;

    /* This is highly not portable :-) */
    unsigned char* buffer = new unsigned char[pathLength + 0x10];
    uint32_t *packageLength = (uint32_t *) buffer;
    uint32_t *sequence = (uint32_t *) (buffer + 0x04);
    uint32_t *strLength = (uint32_t *) (buffer + 0x08);
    unsigned char *actString = buffer + 0x0c;
    uint32_t *flags = (uint32_t *) (buffer + 0x0c + pathLength);

    *packageLength = 0x0c + pathLength;
    *sequence = 0x1a;
    *strLength = pathLength;
    *flags = 0x000180a9;
    for (int i = 0; i < pathLength; i++) {
        *(actString + i) = *(((unsigned char *) lPath) + i);
    }

    if (writePackage(buffer)) {
        unsigned char *buf;
        if (readPackage(&buf)) {
            // Work with data in buffer buf
            // The buffer is allocate in readPackage - but we have to delete it here!
            delete[] buf;
        }
    }

    delete buffer;
}


void ProxyClientSocket::ceWhichCallSetsTheTimeOnTheDevice(int setClock)
{
        // data not relevant, should be { 01, 00, 00, 00 },
    unsigned char timePackage[ 24 ] = { 0x14, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x80, 0xed, 0x50, 0x8a,
    0xb5, 0x3e, 0xc6, 0x01,
    0x00, 0x00, 0x00, 0x00,
    0x10, 0x27, 0x00, 0x00 };

    uint32_t *setTime = ( uint32_t * ) ( timePackage + 0x04 );
    uint32_t *lowDateTime = ( uint32_t * ) ( timePackage + 0x08 );
    uint32_t *highDateTime = ( uint32_t * ) ( timePackage + 0x0c );

    time_t t = time( NULL );
    struct tm *tim = gmtime( &t );

    TIME_FIELDS tf;
    tf.Year = 1900 + tim->tm_year;
    tf.Month = 1 + tim->tm_mon;
    tf.Day = tim->tm_mday;
    tf.Hour = tim->tm_hour;
    tf.Minute = tim->tm_min;
    tf.Second = tim->tm_sec;
    tf.Milliseconds = 0;
    tf.Weekday = 0;

    FILETIME ft;
    if ( !synce::time_fields_to_filetime( &tf, &ft ) ) {
        printf( "Wired error\n" );
        exit( 0 );
    }

    *setTime = ( setClock ) ? ( uint32_t ) 0x00000001 : ( uint32_t ) 0x00000000;
    *lowDateTime = ( uint32_t ) ft.dwLowDateTime;
    *highDateTime = ( uint32_t ) ft.dwHighDateTime;

    if (writePackage(timePackage)) {
        unsigned char *buf;
        if (readPackage(&buf)) {
            // Work with data in buffer buf
            // The buffer is allocate in readPackage - but we have to delete it here!
            delete[] buf;
        }
    }

}


void ProxyClientSocket::shot()
{
//    ceGetSpecialFolderPath(0x05);
//    ceFindAllFiles("\\Temp\\*");
//    ceWhichCallSetsTheTimeOnTheDevice(0);
    ceFindAllFiles("\\My Documents\\*.*");

}

