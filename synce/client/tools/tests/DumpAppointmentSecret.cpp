#include <stdlib.h>
#include <ctype.h>
#include "test.h"

//  %s/ *\(BLOB.*}\)\(.*\)/\2 \1/

int handle_secret(PCEPROPVAL value)
{
	unsigned k;
	unsigned type = value->propid & 0xffff;

	if (CEVT_BLOB != type)
	{
		printf("Secret property not a BLOB!\n");
		return TEST_FAILED;
	}

	if (52 != value->val.blob.dwCount)
	{
		// Size 56 also found
		printf(" Warning! BLOB size not 52!");
	}

	unsigned char always_equal[20] = 
		{0x04, 0x00, 0x00, 0x00, 
		 0x82, 0x00, 0xe0, 0x00, 
		 0x74, 0xc5, 0xb7, 0x10, 
		 0x1a, 0x82, 0xe0, 0x08, 
		 0x00, 0x00, 0x00, 0x00};

	if (20 >= value->val.blob.dwCount)
	{
		if (0 != memcmp(always_equal, value->val.blob.lpb, 20))
		{
			printf(" Warning! First 20 bytes unexpected!");
		}
	}

	// the first 36 are always the same, on my system:
	// 04 00 00 00 82 00 e0 00 74 c5 b7 10 1a 82 e0 08 00 00 00 00 
	// 83 54 2d 00 90 00 75 50 72 27 14 79 56 8b 58 5d
	// the last 16 varies in a very random way, proably a GUID

	printf(" BLOB (size=%lu) \"", value->val.blob.dwCount);
	for (k = 0; k < value->val.blob.dwCount; k++)
	{
		int c = value->val.blob.lpb[k];
		printf("%c", isprint(c) ? c : '.');
	}
	printf("\"={ ");
	for (k = 0; k < value->val.blob.dwCount; k++)
	{
		printf("%02x ", value->val.blob.lpb[k]);
	}
	printf("}");

	return TEST_SUCCEEDED;
}

int handle_start(PCEPROPVAL value)
{
	unsigned type = value->propid & 0xffff;

	if (CEVT_FILETIME != type)
	{
		printf("Start property not FILETIME!\n");
		return TEST_FAILED;
	}
	
	time_t unixtime = DOSFS_FileTimeToUnixTime(&(value->val.filetime), NULL);
	struct tm *tm = localtime(&unixtime);
	char buffer[MAX_PATH];
	strftime(buffer, MAX_PATH, "%c", tm);
	printf(" %08x %08x=%s",value->val.filetime.dwHighDateTime,value->val.filetime.dwLowDateTime, buffer);

	return TEST_SUCCEEDED;
}

int handle_property(PCEPROPVAL value)
{
	unsigned id = value->propid >> 16;

	if (0x0067 == id)
		return handle_secret(value);
	else if (0x420d == id)
		return handle_start(value);
	else
		return TEST_SUCCEEDED;
}

int handle_record(PCEPROPVAL values, DWORD property_count)
{
	for (unsigned j = 0; j < property_count; j++)
	{
		if (TEST_SUCCEEDED != handle_property(values+j))
			return TEST_FAILED;

	} // for every property

	return TEST_SUCCEEDED;
}

int handle_database(HANDLE db, DWORD num_records)
{
#define MYBUFSIZE  (16*1024)
	PCEPROPVAL values = (PCEPROPVAL)malloc(MYBUFSIZE);
	DWORD buffer_size = MYBUFSIZE;

	for (unsigned i = 0; i < num_records; i++)
	{
		WORD property_count;
		CEOID oid;
		TEST_NOT_FALSE(oid = CeReadRecordProps(db, CEDB_ALLOWREALLOC, &property_count, NULL, (BYTE**)&values, &buffer_size));
	
		printf("Row %3u (oid=0x%08x):", i, oid);
	
		if (TEST_SUCCEEDED != handle_record(values, property_count))
			return TEST_FAILED;
	
		printf("\n");
		
	} // for every row


	return TEST_SUCCEEDED;
}

int main()
{
	VERIFY_HRESULT(CeRapiInit());

	CEDB_FIND_DATA* find_data = NULL;
	WORD db_count = 0;
	
	// Find database
	VERIFY_NOT_FALSE(CeFindAllDatabases(0x19, 0xffff, &db_count, &find_data));
	if (db_count != 1)
	{
		printf("Expected one database, found %u.\n", db_count);
		return TEST_FAILED;
	}

	for (unsigned i = 0; i < db_count; i++)
	{
		HANDLE db = INVALID_HANDLE_VALUE;
		TEST_NOT_EQUAL(INVALID_HANDLE_VALUE, db = CeOpenDatabase(&find_data[i].OidDb, NULL, 0, CEDB_AUTOINCREMENT, NULL));

		handle_database(db, find_data[i].DbInfo.wNumRecords);
	
		TEST_NOT_FALSE(CeCloseHandle(db));
	}
	
	//printf ("find_data is valid pointer: %i\n", is_valid_ptr(find_data));
	VERIFY_HRESULT(CeRapiFreeBuffer(find_data));

	VERIFY_HRESULT(CeRapiUninit());
	return TEST_SUCCEEDED;
}

