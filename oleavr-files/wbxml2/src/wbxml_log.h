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
 * @file wbxml_log.h
 * @ingroup wbxml_log
 *
 * @author Aymerick Jéhanne <libwbxml@aymerick.com>
 * @date 02/12/04
 *
 * @brief Log Functions
 */

#ifndef WBXML_LOG_H
#define WBXML_LOG_H

#include <stdlib.h>


#ifdef WBXML_USE_LEAKTRACKER
#include "leaktrack.h"
#include "lt_log.h"
#endif /* WBXML_USE_LEAKTRACKER */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @addtogroup wbxml_log  
 *  @{ 
 */
 
/* Compilation Flags: WBXML_LIB_VERBOSE / WBXML_USE_LEAKTRACKER */

/* Log Macros */
#if defined( WBXML_LIB_VERBOSE )
#if defined( WBXML_USE_LEAKTRACKER )
#define WBXML_DEBUG(msg)   lt_log msg
#define WBXML_WARNING(msg) lt_log msg
#define WBXML_ERROR(msg)   lt_log msg
#define WBXML_PARSER 0x00
#define WBXML_ENCODER 0x00
#define WBXML_CONV 0x00
#else  /* !WBXML_USE_LEAKTRACKER */
#define WBXML_DEBUG(msg)   wbxml_log_error msg
#define WBXML_WARNING(msg) wbxml_log_debug msg
#define WBXML_ERROR(msg)   wbxml_log_warning msg
#define WBXML_PARSER 0x01
#define WBXML_ENCODER 0x02
#define WBXML_CONV 0x03
#endif /* WBXML_USE_LEAKTRACKER */
#else  /* !WBXML_LIB_VERBOSE */
#define WBXML_DEBUG(msg)
#define WBXML_WARNING(msg)
#define WBXML_ERROR(msg)
#endif /* WBXML_LIB_VERBOSE */


/**
 * @brief Log a DEBUG message
 * @param type Type of Message
 * @param fmt Message to log
 * @note Do NOT use this function directly, use WBXML_DEBUG() macro instead
 */
WBXML_DECLARE(void) wbxml_log_debug(WB_UTINY type, const WB_TINY *fmt, ...);

/**
 * @brief Log a WARNING message
 * @param type Type of Message
 * @param fmt Message to log
 * @note Do NOT use this function directly, use WBXML_WARNING() macro instead
 */
WBXML_DECLARE(void) wbxml_log_warning(WB_UTINY type, const WB_TINY *fmt, ...);

/**
 * @brief Log an ERROR message
 * @param type Type of Message
 * @param fmt Message to log
 * @note Do NOT use this function directly, use WBXML_ERROR() macro instead
 */
WBXML_DECLARE(void) wbxml_log_error(WB_UTINY type, const WB_TINY *fmt, ...);

/** @} */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WBXML_LOG_H */
