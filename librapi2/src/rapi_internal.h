/* $Id$ */
#ifndef __rapi_internal_h__
#define __rapi_internal_h__

#include "rapi_types.h"

#if HAVE_CONFIG_H
#include "rapi_config.h"
#else
#error HAVE_CONFIG_H is not defined
#endif

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef __cplusplus
#if HAVE_STDBOOL_H
#include <stdbool.h>
#endif
#endif

#include "rapi_log.h"

#endif

