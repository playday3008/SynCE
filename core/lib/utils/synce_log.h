/* $Id$ */
#ifndef __synce_log_h__
#define __synce_log_h__

#include "synce.h"

/** 
 * @defgroup SynceLog Logging utilities
 * @ingroup SynceUtils
 * @brief Facilities to log to terminal or syslog
 *
 * @{ 
 */ 

/** Log at lowest level */
#define SYNCE_LOG_LEVEL_LOWEST    0


/** Error logging level */
#define SYNCE_LOG_LEVEL_ERROR     1
/** Warning logging level */
#define SYNCE_LOG_LEVEL_WARNING   2
/** Information logging level */
#define SYNCE_LOG_LEVEL_INFO      3
/** Debug logging level */
#define SYNCE_LOG_LEVEL_DEBUG     4
/** Trace logging level, equivalent to debug */
#define SYNCE_LOG_LEVEL_TRACE     SYNCE_LOG_LEVEL_DEBUG

/** Log at highest level */
#define SYNCE_LOG_LEVEL_HIGHEST   5
/** Default logging level, equivalent to warning */
#define SYNCE_LOG_LEVEL_DEFAULT   2

#ifdef __cplusplus
extern "C"
{
#endif

void synce_log_set_level(int level);
void synce_log_use_syslog();

void _synce_log(int level, const char* file, int line, const char* format, ...);

/** @brief Log a trace level message
 * 
 * @param format printf style format string
 * @param ... arguments to the printf style string
 */
#define synce_trace(format, args...) \
	_synce_log(SYNCE_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, format, ##args)

/** @brief Log a debug level message
 * 
 * @param format printf style format string
 * @param ... arguments to the printf style string
 */
#define synce_debug(format, args...) \
	_synce_log(SYNCE_LOG_LEVEL_DEBUG,__PRETTY_FUNCTION__, __LINE__, format, ##args)

/** @brief Log an information level message
 * 
 * @param format printf style format string
 * @param ... arguments to the printf style string
 */
#define synce_info(format, args...) \
	_synce_log(SYNCE_LOG_LEVEL_INFO,__PRETTY_FUNCTION__, __LINE__, format, ##args)

/** @brief Log a warning level message
 * 
 * @param format printf style format string
 * @param ... arguments to the printf style string
 */
#define synce_warning(format, args...) \
	_synce_log(SYNCE_LOG_LEVEL_WARNING,__PRETTY_FUNCTION__, __LINE__, format, ##args)

/** @brief Conditionally log a warning level message
 * 
 * @param cond condition to check, log the message if this is not true
 * @param format printf style format string
 * @param ... arguments to the printf style string
 */
#define synce_warning_unless(cond, format, args...) \
	if (!(cond)) \
	_synce_log(SYNCE_LOG_LEVEL_WARNING,__PRETTY_FUNCTION__, __LINE__, format, ##args)

/** @brief Log an error level message
 * 
 * @param format printf style format string
 * @param ... arguments to the printf style string
 */
#define synce_error(format, args...) \
	_synce_log(SYNCE_LOG_LEVEL_ERROR,__PRETTY_FUNCTION__, __LINE__, format, ##args)

void _synce_log_wstr(int level, const char* file, int line, const char* name, const WCHAR* wstr);

/** @brief Log a UCS2 variable at trace level
 *
 * Log a trace level message with the name and contents of
 * a variable containing a UCS2 encoded string.
 * 
 * @param wstr name of the variable to log
 */
#define synce_trace_wstr(wstr) \
	_synce_log_wstr(SYNCE_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, #wstr, wstr)

#ifdef __cplusplus
}
#endif

/** @} */

#endif

