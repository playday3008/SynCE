/* $Id$ */
#ifndef __rapi_log_h__
#define __rapi_log_h__

#include "rapi_types.h"

#define RAPI_LOG_LEVEL_LOWEST    0

#define RAPI_LOG_LEVEL_ERROR     1
#define RAPI_LOG_LEVEL_WARNING   2
#define RAPI_LOG_LEVEL_TRACE     3

#define RAPI_LOG_LEVEL_HIGHEST   4

#ifdef __cplusplus
extern "C"
{
#endif

void rapi_log_set_level(int level);

void _rapi_log(int level, const char* file, int line, const char* format, ...);

#define rapi_log(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, format, ##args)

#define rapi_trace(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, format, ##args)

#define rapi_warning(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_WARNING,__PRETTY_FUNCTION__, __LINE__, format, ##args)

#define rapi_error(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_ERROR,__PRETTY_FUNCTION__, __LINE__, format, ##args)

void _rapi_log_wstr(int level, const char* file, int line, const char* name, const WCHAR* wstr);

#define rapi_trace_wstr(wstr) \
	_rapi_log_wstr(RAPI_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, #wstr, wstr)

#ifdef __cplusplus
}
#endif


#endif
	
