/* $Id$ */


#define RAPI_LOG_LEVEL_NONE      0
#define RAPI_LOG_LEVEL_ERROR     1
#define RAPI_LOG_LEVEL_WARNING   2
#define RAPI_LOG_LEVEL_TRACE     3


void _rapi_log(int level, const char* file, int line, const char* format, ...);

#define rapi_log(format, args...) \
	_rapi_log(RAPI_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, format, ##args)

