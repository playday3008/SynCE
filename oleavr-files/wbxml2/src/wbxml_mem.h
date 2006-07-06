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
 * @file wbxml_mem.h
 * @ingroup wbxml_mem
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 02/07/01
 *
 * @brief Memory Functions
 */

#ifndef WBXML_MEM_H
#define WBXML_MEM_H

#include <stdlib.h>


#ifdef WBXML_USE_LEAKTRACKER
#include "leaktrack.h"
#include "lt_log.h"
#define wbxml_mem_cleam(ptr) (lt_claim_area(ptr))
#else  /* WBXML_USE_LEAKTRACKER */
#define wbxml_mem_cleam(ptr) (ptr)
#endif /* WBXML_USE_LEAKTRACKER */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup wbxml_mem  
 *  @{ 
 */

/**
 * @brief Alloc a Memory Block
 * @param size Size of Memory to alloc
 * @return The newly mlloced Memory Block, or NULL if not enought memory
 */
WBXML_DECLARE(void *) wbxml_malloc(size_t size);

/**
 * @brief Free a Memory Block
 * @param memblock The Memory Block to free
 */
WBXML_DECLARE(void) wbxml_free(void *memblock);

/**
 * @brief Realloc a Memory Block
 * @param memblock The Memory Block to realloc
 * @param size Size of Memory to realloc
 * @return The newly realloced Memory Block, or NULL if not enought memory
 */
WBXML_DECLARE(void *) wbxml_realloc(void *memblock, size_t size);

/**
 * @brief Duplicate a C String
 * @param str The C String to duplicate
 * @return The newly duplicated C String, or NULL if not enought memory
 */
WBXML_DECLARE(char *) wbxml_strdup(const char *str);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WBXML_MEM_H */
