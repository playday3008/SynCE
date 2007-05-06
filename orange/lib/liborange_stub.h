#ifndef __liborange_stub_h__
#define __liborange_stub_h__

/* Define required types, etc. normally defined by libsynce */

#include <sys/types.h>
#include <stdint.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

typedef uint8_t   BYTE;
typedef uint16_t  WORD;
typedef uint32_t  DWORD;

#if !defined(htole32) || !defined(htole16) || !defined(letoh16) || !defined(letoh32)
#undef htole16
#undef htole32
#undef letoh16
#undef letoh32

#if 0

#define htole16(x) ((((x) >> 8) & 0xFF) | (((x) << 8) & 0xFF00))
#define htole32(x) ((((x) >> 24) & 0xFF) | (((x) >> 8) & 0xFF00) | \
                    (((x) << 8) & 0xFF0000) | (((x) << 24) & 0xFF000000))
#define letoh16(x) htole16(x)
#define letoh32(x) htole32(x)

#else

#define htole16(x) (x)
#define htole32(x) (x)
#define letoh16(x) (x)
#define letoh32(x) (x)

#endif
#endif

#endif
