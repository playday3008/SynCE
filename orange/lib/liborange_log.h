#ifndef __liborange_log_h__
#define __liborange_log_h__

#if WITH_LIBSYNCE
#include <synce_log.h>
#else

#define ORANGE_LOG_LEVEL_LOWEST    0

#define ORANGE_LOG_LEVEL_ERROR     1
#define ORANGE_LOG_LEVEL_WARNING   2
#define ORANGE_LOG_LEVEL_INFO      3
#define ORANGE_LOG_LEVEL_DEBUG     4
#define ORANGE_LOG_LEVEL_TRACE     ORANGE_LOG_LEVEL_DEBUG

#define ORANGE_LOG_LEVEL_HIGHEST   5

void _orange_log(int level, const char* file, int line, const char* format, ...);
void orange_log_set_level(int level);

#define SYNCE_LOG_LEVEL_LOWEST ORANGE_LOG_LEVEL_LOWEST
#define synce_trace(format, args...) \
	_orange_log(ORANGE_LOG_LEVEL_TRACE,__PRETTY_FUNCTION__, __LINE__, format, ##args)
#define synce_debug(format, args...) \
        _orange_log(ORANGE_LOG_LEVEL_DEBUG,__PRETTY_FUNCTION__, __LINE__, format, ##args)
#define synce_error(format, args...) \
        _orange_log(ORANGE_LOG_LEVEL_ERROR,__PRETTY_FUNCTION__, __LINE__, format, ##args)
#define synce_log_set_level(x) orange_log_set_level(x)
#endif

#endif
