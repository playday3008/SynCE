/* $Id$ */
#include <check.h>
#include <stdlib.h>

#include "rapi_buffer.h"

#define VALUE_16BIT 0x1234
#define VALUE_32BIT 0x6789abcd

START_TEST(test_buffer_uint32)
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
END_TEST

START_TEST(test_buffer_uint16)
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
END_TEST


Suite *internals_suite (void) 
{ 
	Suite *s = suite_create ("librapi internals"); 
	TCase *tc_buffer = tcase_create ("rapi_buffer");

	suite_add_tcase (s, tc_buffer);
	tcase_add_test (tc_buffer, test_buffer_uint32); 
	tcase_add_test (tc_buffer, test_buffer_uint16); 

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


