/*
 * libwbxml, the WBXML Library.
 * Copyright (C) 2002-2004 Aymerick Jéhanne <aymerick@jehanne.org>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *  
 * GPL v2: http://www.gnu.org/licenses/gpl.txt
 * 
 * Contact: libwbxml@aymerick.com
 * Home: http://libwbxml.aymerick.com
 */
 
/**
 * @file wbxml_mem.c
 * @ingroup wbxml_mem
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 02/11/24
 *
 * @brief Memory Functions
 */

#include "wbxml.h"


/***************************************************
 *    Public Functions
 */

WBXML_DECLARE(void *) wbxml_malloc(size_t size)
{
#ifdef WBXML_USE_LEAKTRACKER
    return lt_malloc(size);
#else
    return malloc(size);
#endif
}


WBXML_DECLARE(void) wbxml_free(void *memblock)
{
#ifdef WBXML_USE_LEAKTRACKER
    lt_free(memblock);
#else
    free(memblock);
#endif
}


WBXML_DECLARE(void *) wbxml_realloc(void *memblock, size_t size)
{
#ifdef WBXML_USE_LEAKTRACKER
    return lt_realloc(memblock, size);
#else
    return realloc(memblock, size);
#endif
}


WBXML_DECLARE(char *) wbxml_strdup(const char *str)
{
#ifdef WBXML_USE_LEAKTRACKER
    return lt_strdup(str);
#else
    return strdup(str);
#endif
}
