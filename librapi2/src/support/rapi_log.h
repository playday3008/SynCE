/* $Id$ */


void _rapi_log(const char* file, int line, const char* format, ...);

#define rapi_log(format, args...) \
	_rapi_log(__PRETTY_FUNCTION__, __LINE__, format, ##args)

