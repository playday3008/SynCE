/* $Id$ */


#define RAPI_LOG_LEVEL_LOWEST    0

#define RAPI_LOG_LEVEL_ERROR     1
#define RAPI_LOG_LEVEL_WARNING   2
#define RAPI_LOG_LEVEL_TRACE     3

#define RAPI_LOG_LEVEL_HIGHEST   4

void _rapi_log(int level, const char* file, int line, const char* format, ...);

#define rapi_log(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, format, ##args)

#define rapi_trace(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, format, ##args)

#define rapi_warning(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_WARNING,__PRETTY_FUNCTION__, __LINE__, format, ##args)

#define rapi_error(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_ERROR,__PRETTY_FUNCTION__, __LINE__, format, ##args)

