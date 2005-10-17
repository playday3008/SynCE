#ifndef __pe_h__
#define __pe_h__

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

/*
 * High level functions
 */

bool pe_rsrc_offset(FILE *input, uint32_t* fileOffset, uint32_t* virtualOffset);
bool pe_size(FILE *input, uint32_t* result);

/*
 * Low level functions
 */

uint16_t pe_read16(FILE *input);
uint32_t pe_read32(FILE *input);


#endif
