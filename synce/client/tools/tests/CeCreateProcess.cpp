#include "test.h"

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	PROCESS_INFORMATION process;
	memset(&process, 0, sizeof(PROCESS_INFORMATION));
	
	// XXX: process information not properly returned
	
	TEST_NOT_FALSE(CeCreateProcess(
				to_unicode("\\Windows\\sndplay.exe"), // application
				to_unicode("\\Windows\\Exclam.wav"), // command line
				NULL, // process attributes - not supported
				NULL, // thread attributes - not supported
				FALSE, // inherit handles - not supported
				0, // creation flags
				NULL, // environment - not supported
				NULL, // current directory - not supported
				NULL, // startup info - not supported
				(LPPROCESS_INFORMATION)&process // process info
				));

	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}

