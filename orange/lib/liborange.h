/* $Id$ */
#ifndef __liborange_h__
#define __liborange_h__

#if WITH_LIBSYNCE
#include <synce.h>
#else
#include <liborange_stub.h>
#endif

typedef struct _CabInfo
{
  size_t size;
  int processor;
} CabInfo;

/**
  Examine a Microsoft Cabinet File to see if it is installable
 */

bool orange_get_installable_cab_info(
    const char* input_filename,
    CabInfo* cab_info);

bool orange_get_new_installable_cab_info(
    const char* input_filename,
    CabInfo* cab_info);


/**
  Generic callback with a filename
 */

typedef bool (*orange_filename_callback)(
    const char* filename, 
    CabInfo* info,
    void* cookie);

/**
  Generic callback with a buffer
 */

typedef bool (*orange_buffer_callback)(
    const uint8_t* buffer,
    size_t size,
    CabInfo* info,
    void* cookie);

/**
  Squeeze a file in order to find installable Microsoft Cabinet files
 */

bool orange_squeeze_file(
    const char* filename,
    orange_filename_callback callback,
    void* cookie);

/**
  Squeeze a directory in order to find installable Microsoft Cabinet files
 */

bool orange_squeeze_directory(
    const char* directory,
    orange_filename_callback callback,
    void* cookie);

/**
 * Extract resource data from PE file
 */
bool orange_extract_rsrc(
    const char* input_filename, 
    const char* output_directory);

/**
  Takes an self-extracting program that uses a function called DllInflate in a
  DLL called inflate.dll to extract the actual installation program.
 */

bool orange_dllinflate(
    const char* input_filename, 
    const char* output_filename);

/**
  Takes a Setup Factory installer and extract its contents

  Source in suf.c
 */

bool orange_extract_setup_factory(
    const char* input_filename, 
    const char* output_directory);

/**
  Source in inno.c
*/

bool orange_extract_inno(
    const char* input_filename, 
    const char* output_directory);

/**
  Source in installshield_sfx.c
*/

bool orange_extract_installshield_sfx(
    const char* input_filename, 
    const char* output_directory);

bool orange_extract_installshield_sfx2(
    const char* input_filename, 
    const char* output_directory);

/**
  Source in vise.c
*/

bool orange_extract_vise(
    const char* input_filename, 
    const char* output_directory);

/**
  Separate installable Microsoft Cabinet Files from a file
 */

bool orange_separate(
    const char* input_filename, 
    const char* output_directory);

bool orange_separate2(
    uint8_t* input_buffer,
    size_t input_size,
    orange_buffer_callback callback,
    void* cookie);

/**
   Extract a TomTom .apk file
 */

bool orange_extract_apk(
    const char* input_filename,
    const char* output_directory);

/**
   Extract a TomTom .arh file
 */

bool orange_extract_arh(
    const char* input_filename,
    const char* output_directory);

/**
   Extract an InstallShield Cabinet File to a directory
 */

bool orange_extract_is_cab(
    const char* input_filename,
    const char* output_directory);

/**
   Extract a (self-extracting) Microsoft Cabinet File to a directory
 */

bool orange_extract_ms_cab(
    const char* input_filename,
    const char* output_directory);

/**
  Extract an MSI file to a directory
 */

bool orange_extract_msi(
    const char* input_filename,
    const char* output_directory);

/**
  Extract a (self-extracting) RAR file to a directory
 */

bool orange_extract_rar(
    const char* input_filename,
    const char* output_directory);

/**
  Extract a (self-extracting) ZIP file to a directory
 */

bool orange_extract_zip(
    const char* input_filename,
    const char* output_directory);

/**
  Is this a Nullsoft Installer?
  */
bool orange_is_nullsoft_installer(const char* input_filename);

#endif
