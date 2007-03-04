///////////////////////////////////////////////////////////////////////////////
// SYSINCLUDES.H
//
// Central system includes and configuration
//
// Dr J A Gow : 25/2/2006
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SYSINCLUDES_H_
#define _SYSINCLUDES_H_

//
// DLL exports

#ifdef HAVE_GCCVISIBILITY
#define _DLLAPI   __attribute__ ((visibility("default")))
#define _INTERNAL __attribute__ ((visibility("hidden")))
#else
#define _DLLAPI
#define _INTERNAL
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//
// Standard base types

typedef unsigned char	BYTE;
typedef unsigned short	WORD;
typedef unsigned int	DWORD;
typedef unsigned long	QWORD;

//
// Useful types

typedef unsigned int	UINT;
typedef unsigned long	ULONG;



#endif
