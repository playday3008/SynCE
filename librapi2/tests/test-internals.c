/* $Id$ */
#include <check.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rapi_buffer.h"
#include "rapi_wstr.h"
#include "rapi_filetime.h"
#include "config/config.h"

#if HAVE_DMALLOC_H
#include "dmalloc.h"
#endif

#define VALUE_16BIT 0x1234
#define VALUE_32BIT 0x6789abcd

START_TEST(test_buffer_uint32)/*{{{*/
{
	bool success;
	RapiBuffer* buffer;
	uint32_t value;

	buffer = rapi_buffer_new();
	fail_unless(buffer != NULL, "rapi_buffer_new");
	
	value = VALUE_32BIT;
	success = rapi_buffer_write_uint32(buffer, value);
	fail_unless(success, "rapi_buffer_write_uint32");
	
	value = 0;
	success = rapi_buffer_read_uint32(buffer, &value);
	fail_unless(success, "rapi_buffer_write_uint32");

	fail_unless(VALUE_32BIT == value, "Write/read sequence failed");
	
	rapi_buffer_free(buffer);
}
END_TEST/*}}}*/

START_TEST(test_buffer_uint16)/*{{{*/
{
	bool success;
	RapiBuffer* buffer;
	uint16_t value;

	buffer = rapi_buffer_new();
	fail_unless(buffer != NULL, "rapi_buffer_new");
	
	value = VALUE_16BIT;
	success = rapi_buffer_write_uint16(buffer, value);
	fail_unless(success, "rapi_buffer_write_uint16");
	
	value = 0;
	success = rapi_buffer_read_uint16(buffer, &value);
	fail_unless(success, "rapi_buffer_read_uint16");

	fail_unless(VALUE_16BIT == value, "Write/read sequence failed");
	
	rapi_buffer_free(buffer);
}
END_TEST/*}}}*/

/*
 * This is not an array of WCHARS, beacuse then it would not be little endian
 * on all platforms
 */

static const char UNICODE_VALUE[] = 
{
  '2',0,'G',0,'o',0,'o',0,'D',0,' ',0,'P',0,'r',0,'o',0,'d',0,'u',0,'c',0,'t',0,'i',0,'o',0,'n',0,'s',0,0,0
};

static const char ASCII_VALUE[] = { "2GooD Productions" };

START_TEST(test_unicode_strlen)/*{{{*/
{
	size_t length;

	length = rapi_wstr_strlen((WCHAR*)UNICODE_VALUE);
	
	printf("Length: ascii=%i, unicode=%i\n", strlen(ASCII_VALUE), length);
	fail_unless( length == strlen(ASCII_VALUE), "rapi_wstr_strlen failed" );
}
END_TEST/*}}}*/

START_TEST(test_unicode_from_ascii)/*{{{*/
{
	void* unicode;

	unicode = rapi_wstr_from_ascii(ASCII_VALUE);

	fail_unless( unicode != 0, "rapi_wstr_from_ascii returned NULL" );

	if (unicode)
	{
		fail_unless( memcmp(unicode, (WCHAR*)UNICODE_VALUE, sizeof(UNICODE_VALUE)) == 0, "rapi_wstr_from_ascii failed" );
	}

	rapi_wstr_free_string(unicode);
}
END_TEST/*}}}*/

START_TEST(test_unicode_to_ascii)/*{{{*/
{
	char* ascii;

	ascii = rapi_wstr_to_ascii((WCHAR*)UNICODE_VALUE);

	fail_unless( ascii != 0, "rapi_wstr_to_ascii returned NULL" );

	if (ascii)
	{
		fail_unless( memcmp(ascii, ASCII_VALUE, sizeof(ASCII_VALUE)) == 0, "rapi_wstr_to_ascii failed" );
	}

	rapi_wstr_free_string(ascii);
}
END_TEST/*}}}*/

START_TEST(test_config_general)/*{{{*/
{
	struct configFile* config;
	int number;
	char* string;

	config = readConfigFile("test.conf");
	fail_unless( config != NULL, "readConfigFile failed" );

	if (config)
	{
		number = getConfigInt(config, "mysection", "mynumber");
		fail_unless(222 == number, "getConfigInt failed" );

		string = getConfigString(config, "mysection", "mystring");
		fail_unless(0 == strcmp(string, "2GooD Productions"), "getConfigString");
	}

	unloadConfigFile(config);
}
END_TEST/*}}}*/

START_TEST(test_filetime)/*{{{*/
{
	FILETIME ft;
	time_t time1 = time(NULL);
	time_t time2;

	memset(&ft, 0, sizeof(ft));

	rapi_filetime_from_unix_time(time1, &ft);

	time2 = rapi_filetime_to_unix_time(&ft);

	fail_unless(time1 == time2, "times differ");
	
}
END_TEST/*}}}*/



Suite *internals_suite (void) 
{ 
	Suite *s = suite_create ("librapi internals\n"); 
	TCase *tc_buffer  = tcase_create ("rapi_buffer");
	TCase *tc_unicode = tcase_create ("rapi_wstr");
	TCase *tc_config = tcase_create ("config");
	TCase *tc_filetime = tcase_create ("filetime");

	suite_add_tcase (s, tc_buffer);
	tcase_add_test (tc_buffer, test_buffer_uint32); 
	tcase_add_test (tc_buffer, test_buffer_uint16); 

	suite_add_tcase (s, tc_unicode);
	tcase_add_test (tc_unicode, test_unicode_strlen);
	tcase_add_test (tc_unicode, test_unicode_from_ascii);
	tcase_add_test (tc_unicode, test_unicode_to_ascii);

	suite_add_tcase (s, tc_config);
	tcase_add_test (tc_config, test_config_general); 
	
	suite_add_tcase (s, tc_filetime);
	tcase_add_test (tc_filetime, test_filetime); 

	return s; 
}

int main (void) 
{ 
	int nf; 
	Suite *s = internals_suite (); 
	SRunner *sr = srunner_create (s); 
	srunner_run_all (sr, CK_NORMAL); 
	nf = srunner_ntests_failed (sr); 
	srunner_free (sr); 
	suite_free (s); 
	return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE; 
}


