/***************************************************************************
 *   Copyright (C) 2001 by Ludovic LANGE                                   *
 *   ludovic.lange@free.fr                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 ***************************************************************************/
#include <windows.h>
#define CP_ACP 0
#define MB_PRECOMPOSED 0

int WideCharToMultiByte( UINT CodePage, DWORD dwFlags, LPCWSTR lpWideChar, int cchWideChar, LPSTR lpMultiByteStr, int cchMultiByte, LPCSTR lpDefaultChar, LPBOOL lpUsedDefaultChar )
{
        return 1;
}

int MultiByteToWideChar( UINT CodePage, DWORD dwFlags, LPSTR lpMultiByteStr, int cchMultiByte, LPCWSTR lpWideChar, int cchWideChar )
{
        return 1;
}

