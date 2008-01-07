#include "frontend.h"

static uint32_t rra_frontend = ID_FRONTEND_UNKNOWN;

void rra_frontend_set(uint32_t frontend)
{
	rra_frontend = frontend;
}

uint32_t rra_frontend_get()
{
	return rra_frontend;
}
