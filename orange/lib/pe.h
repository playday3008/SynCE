#ifndef __pe_h__
#define __pe_h__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

bool pe_rsrc_offset(FILE *input, uint32_t* result);
bool pe_size(FILE *input, uint32_t* result);

#endif
