/* $Id: contact.h 3713 2009-03-17 14:32:11Z mark_ellis $ */
#ifndef __file_h__
#define __file_h__

#include <synce.h>

/* flags for file type */

#define RRA_FILE_TYPE_UNKNOWN    0x00
#define RRA_FILE_TYPE_DIRECTORY  0x10
#define RRA_FILE_TYPE_FILE       0x20

/*
 * Convert file data
 */

#ifndef SWIG
bool rra_file_unpack(
		const uint8_t *data, 
		size_t data_size,
		DWORD *ftype,
		char **path,
		uint8_t **file_content,
		size_t *file_size);

bool rra_file_pack(
		DWORD ftype,
		const char *path, 
		const uint8_t *file_content,
		size_t file_size,
		uint8_t **data,
		size_t *data_size);
#endif /* SWIG */

#define rra_file_free_data(p)  if (p) free(p)

#endif
