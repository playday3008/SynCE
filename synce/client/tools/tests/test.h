// $Id$
#include <stdio.h>
#include <iconv.h>
#include <rapi.h>
#include <unistd.h>
#include <sys/mman.h>
#include <errno.h>

//
// Return values from main()
//
#define TEST_SUCCEEDED 0
#define TEST_FAILED 1

//
// HRESULT tests
// 
#define VERIFY_HRESULT(call) \
if (FAILED((call))) { printf("FAIL.\n"); return TEST_FAILED; }

#define TEST_HRESULT(call) \
printf("Testing %s...", #call); \
VERIFY_HRESULT(call) else printf("ok.\n");

//
// Test return value
//
#define VERIFY_EQUAL(value, call) \
if ((value) != (call)) { printf("FAIL.\n"); return TEST_FAILED; }

#define TEST_EQUAL(value, call) \
printf("Testing %s...", #call); \
VERIFY_EQUAL((value), (call)) else printf("ok.\n");


//
// Test return value
//
#define VERIFY_NOT_EQUAL(value, call) \
if ((value) == (call)) { printf("FAIL.\n"); return TEST_FAILED; }

#define TEST_NOT_EQUAL(value, call) \
printf("Testing %s...", #call); \
VERIFY_NOT_EQUAL((value), (call)) else printf("ok.\n");


//
// Test to verify that the return value is zero
//
#define VERIFY_NOT_FALSE(call) VERIFY_NOT_EQUAL(0, (call))
#define TEST_NOT_FALSE(call) TEST_NOT_EQUAL(0, call)


//
// Convert text
//
char* from_unicode(const WCHAR* inbuf)
{
	size_t length = wcslen(inbuf);
	size_t inbytesleft = length * 2, outbytesleft = length;
	char* outbuf = new char[outbytesleft+1];
	char* outbuf_iterator = outbuf;
	char* inbuf_iterator = (char*)inbuf;
	
	iconv_t cd = iconv_open("iso8859-1", "UCS-2");
	size_t result = iconv(cd, &inbuf_iterator, &inbytesleft, &outbuf_iterator, &outbytesleft);
	iconv_close(cd);

	if ((size_t)-1 == result)
		strcpy(outbuf, "(failed)");
	else
		outbuf[length] = 0;
	
	return outbuf;
}

WCHAR* to_unicode(const char* inbuf)
{
	size_t length = strlen(inbuf);
	size_t inbytesleft = length, outbytesleft = (length+1)* 2;
	char * inbuf_iterator = const_cast<char*>(inbuf);
	WCHAR* outbuf = new WCHAR[inbytesleft+1];
	WCHAR* outbuf_iterator = outbuf;
	
	iconv_t cd = iconv_open("UCS-2", "UTF-8");
	size_t result = iconv(cd, &inbuf_iterator, &inbytesleft, (char**)&outbuf_iterator, &outbytesleft);
	iconv_close(cd);

	if ((size_t)-1 == result)
		outbuf[0] = 0;
	else
		outbuf[length] = 0;
	
	return outbuf;
}

// This does not work with Linux kernel 2.2 and earlier
bool is_valid_ptr(void * ptr)
{
	size_t pagesize = getpagesize();
	unsigned char vec;
	
	// align pointer
	ptr = (void*)((unsigned)ptr & ~(pagesize-1));

	int result = mincore(ptr, 1, &vec);
	if (0 != result)
	{
		printf("mincore failed: %s\n", strerror(errno));
		return false;
	}

	return true;
}

