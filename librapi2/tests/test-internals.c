/* $Id$ */
#include <check.h>
#include <stdlib.h>

#include "rapi_buffer.h"
#include "rapi_wstr.h"
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
	u_int32_t value;

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
	u_int16_t value;

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

static const WCHAR UNICODE_VALUE[] = 
{
  '2','G','o','o','D',' ','P','r','o','d','u','c','t','i','o','n','s',0
};

static const char ASCII_VALUE[] = { "2GooD Productions" };

START_TEST(test_unicode_strlen)/*{{{*/
{
	size_t length;

	length = rapi_wstr_string_length(UNICODE_VALUE);
	
	fail_unless( length == strlen(ASCII_VALUE), "rapi_wstr_string_length failed" );
}
END_TEST/*}}}*/

START_TEST(test_unicode_from_ascii)/*{{{*/
{
	void* unicode;

	unicode = rapi_wstr_from_ascii(ASCII_VALUE);

	fail_unless( unicode != 0, "rapi_wstr_from_ascii returned NULL" );

	if (unicode)
	{
		fail_unless( memcmp(unicode, UNICODE_VALUE, sizeof(UNICODE_VALUE)) == 0, "rapi_wstr_from_ascii failed" );
	}

	rapi_wstr_free_string(unicode);
}
END_TEST/*}}}*/

START_TEST(test_unicode_to_ascii)/*{{{*/
{
	char* ascii;

	ascii = rapi_wstr_to_ascii(UNICODE_VALUE);

	fail_unless( ascii != 0, "rapi_wstr_to_ascii returned NULL" );

	if (ascii)
	{
		fail_unless( memcmp(ascii, ASCII_VALUE, sizeof(ASCII_VALUE)) == 0, "rapi_wstr_to_ascii failed" );
	}

	rapi_wstr_free_string(ascii);
}
END_TEST/*}}}*/

START_TEST(test_config_general)
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
END_TEST

Suite *internals_suite (void) 
{ 
	Suite *s = suite_create ("librapi internals"); 
	TCase *tc_buffer  = tcase_create ("rapi_buffer");
	TCase *tc_unicode = tcase_create ("rapi_wstr");
	TCase *tc_config = tcase_create ("config");

	suite_add_tcase (s, tc_buffer);
	tcase_add_test (tc_buffer, test_buffer_uint32); 
	tcase_add_test (tc_buffer, test_buffer_uint16); 

	suite_add_tcase (s, tc_unicode);
	tcase_add_test (tc_unicode, test_unicode_strlen);
	tcase_add_test (tc_unicode, test_unicode_from_ascii);
	tcase_add_test (tc_unicode, test_unicode_to_ascii);

	suite_add_tcase (s, tc_config);
	tcase_add_test (tc_config, test_config_general); 
	
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


