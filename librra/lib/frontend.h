#ifndef __frontend_h__
#define __frontend_h__

#include <synce.h>

#define ID_FRONTEND_UNKNOWN 0
#define ID_FRONTEND_KDEPIM 1
#define ID_FRONTEND_EVOLUTION 2

void rra_frontend_set(uint32_t frontend);
uint32_t rra_frontend_get();

#endif
