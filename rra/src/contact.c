#include "contact.h"
#include "contact_ids.h"
#include "strbuf.h"
#include <rapi.h>
#include <synce_log.h>

static void strbuf_append_wstr(StrBuf* strbuf, WCHAR* wstr)
{
	if (wstr)
	{
		char* ascii_str = wstr_to_ascii(wstr);
		strbuf_append(strbuf, ascii_str);
		wstr_free_string(ascii_str);
	}
}

bool contact_to_vcard(
		uint32_t oid, 
		CEPROPVAL* pFields, 
		uint32_t count, 
		char** ppVcard)
{
	int i;
	StrBuf* vcard = strbuf_new(NULL);
	bool have_fn; /* the FN property must be present! */
	WCHAR* first_name = NULL;
	WCHAR* last_name  = NULL;
	WCHAR* title = NULL;
	WCHAR* suffix = NULL;
	WCHAR* middle_name = NULL;
	
	strbuf_append(vcard, "BEGIN:vCard\n");
	strbuf_append(vcard, "VERSION:3.0\n");

	for (i = 0; i < count; i++)
	{
		/* TODO: validate data types */
		switch (pFields[i].propid >> 16)
		{
			case ID_SUFFIX:
				suffix = pFields[i].val.lpwstr;
				break;

			case ID_FIRST_NAME:
				first_name = pFields[i].val.lpwstr;
				break;

			case ID_WORK_TEL:
				strbuf_append(vcard, "TEL;TYPE=work,voice,pref:");
				strbuf_append_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_c(vcard, '\n');
				break;

			case ID_HOME_TEL:
				strbuf_append(vcard, "TEL;TYPE=home,voice,pref:");
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

	if (!have_fn)
	{
		/* TODO: make up a value for this property! */
	}

	strbuf_append(vcard, "END:vCard\n");

	*ppVcard = vcard->buffer;
	strbuf_free(vcard, false);
	return true;
}

bool contact_free_vcard(char* vcard)
{
	if (vcard)
		free(vcard);
}

