#include <stdlib.h>
#include <ctype.h>
#include "test.h"

int handle_property(PCEPROPVAL value)
{
	unsigned k;
	unsigned id = value->propid >> 16;
	unsigned type = value->propid & 0xffff;

	if (0x0067 != id)
		return TEST_SUCCEEDED;

	if (CEVT_BLOB != type)
	{
		printf("Property not a BLOB!\n");
		return TEST_FAILED;
	}

	if (52 != value->val.blob.dwCount)
	{
		printf("Warning! BLOB size not 52!\n");
	}

	// the first 36 are always the same, on my system:
	// 04 00 00 00 82 00 e0 00 74 c5 b7 10 1a 82 e0 08 00 00 00 00 
	// 83 54 2d 00 90 00 75 50 72 27 14 79 56 8b 58 5d
	// the last 16 varies in a very random way, proably a GUID

	printf("BLOB (size=%lu) \"", value->val.blob.dwCount);
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
	
		printf("Row %u (oid=0x%lx): ", i, oid);
	
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

