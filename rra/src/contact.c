/* $Id$ */
#define _BSD_SOURCE 1
#include "contact.h"
#include "contact_ids.h"
#include "strbuf.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>

bool contact_to_vcard(
		uint32_t oid, 
		CEPROPVAL* pFields, 
		uint32_t count, 
		char** ppVcard)
{
	int i;
	StrBuf* vcard = strbuf_new(NULL);
	bool have_fn; /* the FN property must be present! */

	/* name parts */
	WCHAR* first_name = NULL;
	WCHAR* last_name  = NULL;
	WCHAR* title = NULL;
	WCHAR* suffix = NULL;
	WCHAR* middle_name = NULL;

	/* home address parts */
	WCHAR* home_street = NULL;
	WCHAR* home_locality = NULL;
	WCHAR* home_postal_code = NULL;
	WCHAR* home_country = NULL;
	
	/* work address parts */
	WCHAR* work_street = NULL;
	WCHAR* work_locality = NULL;
	WCHAR* work_postal_code = NULL;
	WCHAR* work_country = NULL;
	
	
	strbuf_append(vcard, "BEGIN:vCard\n");
	strbuf_append(vcard, "VERSION:3.0\n");
	strbuf_append(vcard, "PRODID:-//SYNCE RRA//NONSGML Version 1//EN\n");

	if (oid != CONTACT_OID_UNKNOWN)
	{
		char id_str[32];
		snprintf(id_str, sizeof(id_str), "UID:RRA-ID-%08x\n", oid);
		strbuf_append(vcard, id_str);
	}


	for (i = 0; i < count; i++)
	{
		/* TODO: validate data types */
		switch (pFields[i].propid >> 16)
		{
#if 0
			case ID_NOTE:
				{
					unsigned j;
					for (j = 0; j < pFields[i].val.blob.dwCount && pFields[i].val.blob.lpb[i]; j++)
						;

					if (j == pFields[i].val.blob.dwCount)
					{
						strbuf_append(vcard, "NOTE:");
						/* XXX: need to handle newlines! */
						strbuf_append(vcard, (const char*)pFields[i].val.blob.lpb);
						strbuf_append_c(vcard, '\n');
					}
					else
					{
						synce_warning("Note is not an ASCII string.\n");
					}
				}
				break;
#endif

			case ID_SUFFIX:
				suffix = pFields[i].val.lpwstr;
				break;

			case ID_FIRST_NAME:
				first_name = pFields[i].val.lpwstr;
				break;

			case ID_WORK_TEL:
				strbuf_append(vcard, "TEL;TYPE=WORK,VOICE,PREF:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_HOME_TEL:
				strbuf_append(vcard, "TEL;TYPE=HOME,VOICE,PREF:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_LAST_NAME:
				last_name = pFields[i].val.lpwstr;
				break;

			case ID_COMPANY:
				strbuf_append(vcard, "ORG:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_JOB_TITLE:
				strbuf_append(vcard, "TITLE:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			/*case ID_DEPARTMENT:*/

			case ID_MOBILE_TEL:
				strbuf_append(vcard, "TEL;TYPE=cell:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_CAR_TEL:
				strbuf_append(vcard, "TEL;TYPE=car:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_WORK_FAX:
				strbuf_append(vcard, "TEL;TYPE=work,fax:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_HOME_FAX:
				strbuf_append(vcard, "TEL;TYPE=home,fax:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_HOME2_TEL:
				strbuf_append(vcard, "TEL;TYPE=home,voice:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_CATEGORY:
				strbuf_append(vcard, "CATEGORIES:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_WORK2_TEL:
				strbuf_append(vcard, "TEL;TYPE=work,voice:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_WEB_PAGE:
				strbuf_append(vcard, "URL:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_PAGER:
				strbuf_append(vcard, "TEL;TYPE=pager:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_FULL_NAME:
				strbuf_append(vcard, "FN:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				have_fn = true;
				break;

			case ID_TITLE:
				title = pFields[i].val.lpwstr;
				break;

			case ID_MIDDLE_NAME:
				suffix = pFields[i].val.lpwstr;
				break;

			case ID_WORK_STREET:
				work_street = pFields[i].val.lpwstr;
				break;

			case ID_WORK_LOCALITY:
				work_locality = pFields[i].val.lpwstr;
				break;

			case ID_WORK_POSTAL_CODE:
				work_postal_code = pFields[i].val.lpwstr;
				break;

			case ID_WORK_COUNTRY:
				work_country = pFields[i].val.lpwstr;
				break;

			case ID_EMAIL:
				strbuf_append(vcard, "EMAIL;TYPE=internet,pref:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_EMAIL2:
			case ID_EMAIL3:
				strbuf_append(vcard, "EMAIL;TYPE=internet:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			default:
				synce_warning("Did not handle field with ID %04x", pFields[i].propid >> 16);
				break;
		}
	}

	if (first_name || last_name || middle_name || title || suffix)
	{
		strbuf_append      (vcard, "N:");
		strbuf_append_wstr (vcard, last_name);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, first_name);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, middle_name);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, title);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, suffix);
		strbuf_append_c    (vcard, '\n');
	}

	/* 
	 * The structured type value corresponds, in sequence, to the post office
	 * box; the extended address; the street address; the locality (e.g., city);
	 * the region (e.g., state or province); the postal code; the country name.
	 */

	if (home_street || home_locality || home_postal_code || home_country)
	{
		strbuf_append      (vcard, "ADR:TYPE=home:");
		strbuf_append_wstr (vcard, NULL); /* post office box */
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, NULL); /* extended address */
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, home_street);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, home_locality);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, NULL); /* region */
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, home_postal_code);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, home_country);
		strbuf_append_c    (vcard, '\n');
	}

	if (work_street || work_locality || work_postal_code || work_country)
	{
		strbuf_append      (vcard, "ADR:TYPE=work:");
		strbuf_append_wstr (vcard, NULL); /* post office box */
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, NULL); /* extended address */
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, work_street);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, work_locality);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, NULL); /* region */
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, work_postal_code);
		strbuf_append_c    (vcard, ';');
		strbuf_append_wstr (vcard, work_country);
		strbuf_append_c    (vcard, '\n');
	}


	if (!have_fn)
	{
		/* TODO: make up a value for this property! */
	}

	strbuf_append(vcard, "END:vCard\n");

	*ppVcard = vcard->buffer;
	strbuf_free(vcard, false);
	return true;
}

void contact_free_vcard(char* vcard)
{
	if (vcard)
		free(vcard);
}

