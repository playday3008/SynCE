/* $Id$ */
#include <check.h>
#include <stdlib.h>

#include "rapi_buffer.h"


START_TEST(test_buffer)
{

}
END_TEST


Suite *internals_suite (void) 
{ 
	Suite *s = suite_create ("librapi internals"); 
	TCase *tc_buffer = tcase_create ("rapi_buffer");

	suite_add_tcase (s, tc_buffer);
	tcase_add_test (tc_buffer, test_buffer); 

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


