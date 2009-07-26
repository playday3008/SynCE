/* $Id: rapitypes2.h 3791 2009-07-12 17:27:04Z mark_ellis $ */
#ifndef __rapitypes2_h__
#define __rapitypes2_h__

#include <synce.h>

#ifdef __cplusplus
namespace synce
{
extern "C"
{
#endif


typedef struct _GUID {
        unsigned long  Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char  Data4[8];
} GUID, UUID;

typedef GUID RAPIDEVICEID;

typedef enum {
        RAPI_DEVICE_DISCONNECTED = 0,
        RAPI_DEVICE_CONNECTED = 1
} RAPI_DEVICESTATUS;

typedef enum {
        RAPI_GETDEVICE_NONBLOCKING,
        RAPI_GETDEVICE_BLOCKING
} RAPI_GETDEVICEOPCODE;

typedef enum {
        RAPI_CONNECTION_USB = 0,
        RAPI_CONNECTION_IR = 1,
        RAPI_CONNECTION_SERIAL = 2,
        RAPI_CONNECTION_NETWORK = 3
} RAPI_CONNECTIONTYPE;


/* http://msdn.microsoft.com/en-us/library/ms221069.aspx */
/* WTypes.h */
typedef WCHAR  OLECHAR;

/* OLEAuto.h */
#if 0
typedef OLECHAR FAR * BSTR;
#else
typedef WCHAR BSTR;
#endif

typedef struct {
        RAPIDEVICEID DeviceId;
        DWORD dwOsVersionMajor;
        DWORD dwOsVersionMinor;
        BSTR bstrName;
        BSTR bstrPlatform;
} RAPI_DEVICEINFO;




#define _SS_MAXSIZE 128                  // Maximum size.
#define _SS_ALIGNSIZE (sizeof(int64_t))  // Desired alignment.
#define _SS_PAD1SIZE (_SS_ALIGNSIZE - sizeof (short))
#define _SS_PAD2SIZE (_SS_MAXSIZE - (sizeof (short) + _SS_PAD1SIZE \
                                     + _SS_ALIGNSIZE))

typedef struct _sockaddr_storage {
        short ss_family;               // Address family.
        char __ss_pad1[_SS_PAD1SIZE];  // 6 byte pad, this is to make
                                       // implementation specific pad up to
                                       // alignment field that follows explicit
                                       // in the data structure.
        int64_t __ss_align;            // Field to force desired structure.
        char __ss_pad2[_SS_PAD2SIZE];  // 112 byte pad to achieve desired size;
                                       // _SS_MAXSIZE value minus size of
                                       // ss_family, __ss_pad1, and
                                       // __ss_align fields is 112.
} sockaddr_storage;

typedef sockaddr_storage SOCKADDR_STORAGE;

typedef struct {
        SOCKADDR_STORAGE ipaddr;
        SOCKADDR_STORAGE hostIpaddr;
        RAPI_CONNECTIONTYPE connectionType;
} RAPI_CONNECTIONINFO;


#ifdef __cplusplus
}
}
#endif

#endif /* __rapitypes2_h__ */

