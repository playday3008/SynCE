/* $Id$ */
#define _GNU_SOURCE 1
#include "librra.h"
#include "contact_ids.h"
#include "strbuf.h"
#include "dbstream.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const char* product_id = "-//SYNCE RRA//NONSGML Version 1//EN";

/* 
 * the theoretical field count maximum is about 50,
 * and we add 10 to be safe
 */
#define MAX_FIELD_COUNT  60

/* 
   ESCAPED-CHAR = "\\" / "\;" / "\," / "\n" / "\N")
        ; \\ encodes \, \n or \N encodes newline
        ; \; encodes ;, \, encodes ,
*/
static void strbuf_append_escaped(StrBuf* result, char* source)/*{{{*/
{
	char* p;

	for (p = source; *p; p++)
	{
		switch (*p)
		{
			case '\\':
			case ';':
			case ',':
				strbuf_append_c(result, '\\');
				strbuf_append_c(result, *p);
				break;
				
			case '\r': 				/* CR */
				/* ignore */
				break;		

			case '\n':				/* LF */
				strbuf_append_c(result, '\\');
				strbuf_append_c(result, 'n');
				break;
	
			default:
				strbuf_append_c(result, *p);
				break;
		}
	}
}/*}}}*/

void strbuf_append_escaped_wstr(StrBuf* strbuf, WCHAR* wstr)/*{{{*/
{
	if (wstr)
	{
		char* ascii_str = wstr_to_ascii(wstr);
		strbuf_append_escaped(strbuf, ascii_str);
		wstr_free_string(ascii_str);
	}
}/*}}}*/



static bool rra_contact_to_vcard2(/*{{{*/
		uint32_t id, 
		CEPROPVAL* pFields, 
		uint32_t count, 
		uint32_t vcard_version,
		char** ppVcard)
{
	int i;
	StrBuf* vcard = strbuf_new(NULL);
	bool have_fn = false; /* the FN property must be present! */

	/* name parts */
	WCHAR* first_name = NULL;
	WCHAR* last_name  = NULL;
	WCHAR* title = NULL;
	WCHAR* suffix = NULL;
	WCHAR* middle_name = NULL;

	/* organization parts */
	WCHAR* company = NULL;
	WCHAR* department = NULL;

	/* home address parts */
	WCHAR* home_street = NULL;
	WCHAR* home_locality = NULL;
	WCHAR* home_region = NULL;
	WCHAR* home_postal_code = NULL;
	WCHAR* home_country = NULL;
	
	/* work address parts */
	WCHAR* work_street = NULL;
	WCHAR* work_locality = NULL;
	WCHAR* work_region = NULL;
	WCHAR* work_postal_code = NULL;
	WCHAR* work_country = NULL;

	if (vcard_version != RRA_CONTACT_VCARD_3_0)
	{
		synce_error("Unsupported vCard version");
		strbuf_free(vcard, true);
		return false;
	}
	
	strbuf_append(vcard, "BEGIN:vCard\n");
	strbuf_append(vcard, "VERSION:3.0\n");

	strbuf_append(vcard, "PRODID:");
	strbuf_append(vcard, product_id);
	strbuf_append_crlf(vcard);

	if (id != RRA_CONTACT_ID_UNKNOWN)
	{
		char id_str[32];
		snprintf(id_str, sizeof(id_str), "UID:RRA-ID-%08x\n", id);
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
						strbuf_append_crlf(vcard);
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
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_HOME_TEL:
				strbuf_append(vcard, "TEL;TYPE=HOME,VOICE,PREF:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_LAST_NAME:
				last_name = pFields[i].val.lpwstr;
				break;

			case ID_COMPANY:
				company = pFields[i].val.lpwstr;
				break;

			case ID_JOB_TITLE:
				strbuf_append(vcard, "TITLE:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_DEPARTMENT:
				department = pFields[i].val.lpwstr;
				break;

			case ID_MOBILE_TEL:
				strbuf_append(vcard, "TEL;TYPE=CELL:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_CAR_TEL:
				strbuf_append(vcard, "TEL;TYPE=CAR:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_WORK_FAX:
				strbuf_append(vcard, "TEL;TYPE=WORK,FAX:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_HOME_FAX:
				strbuf_append(vcard, "TEL;TYPE=HOME,FAX:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_HOME2_TEL:
				strbuf_append(vcard, "TEL;TYPE=HOME,VOICE:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_CATEGORY:
				strbuf_append(vcard, "CATEGORIES:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_WORK2_TEL:
				strbuf_append(vcard, "TEL;TYPE=WORK,VOICE:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_WEB_PAGE:
				strbuf_append(vcard, "URL:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_PAGER:
				strbuf_append(vcard, "TEL;TYPE=pager:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_FULL_NAME:
				strbuf_append(vcard, "FN:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				have_fn = true;
				break;

			case ID_TITLE:
				title = pFields[i].val.lpwstr;
				break;

			case ID_MIDDLE_NAME:
				middle_name = pFields[i].val.lpwstr;
				break;

			case ID_HOME_STREET:
				home_street = pFields[i].val.lpwstr;
				break;

			case ID_HOME_LOCALITY:
				home_locality = pFields[i].val.lpwstr;
				break;

			case ID_HOME_REGION:
				home_region = pFields[i].val.lpwstr;
				break;

			case ID_HOME_POSTAL_CODE:
				home_postal_code = pFields[i].val.lpwstr;
				break;

			case ID_HOME_COUNTRY:
				home_country = pFields[i].val.lpwstr;
				break;

			case ID_WORK_STREET:
				work_street = pFields[i].val.lpwstr;
				break;

			case ID_WORK_LOCALITY:
				work_locality = pFields[i].val.lpwstr;
				break;

			case ID_WORK_REGION:
				work_region = pFields[i].val.lpwstr;
				break;

			case ID_WORK_POSTAL_CODE:
				work_postal_code = pFields[i].val.lpwstr;
				break;

			case ID_WORK_COUNTRY:
				work_country = pFields[i].val.lpwstr;
				break;

			case ID_EMAIL:
				strbuf_append(vcard, "EMAIL;TYPE=INTERNET,PREF:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			case ID_EMAIL2:
			case ID_EMAIL3:
				strbuf_append(vcard, "EMAIL;TYPE=INTERNET:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr);
				strbuf_append_crlf(vcard);
				break;

			default:
				synce_warning("Did not handle field with ID %04x", pFields[i].propid >> 16);
				break;
		}
	}

	if (first_name || last_name || middle_name || title || suffix)
	{
		strbuf_append              (vcard, "N:");
		strbuf_append_escaped_wstr (vcard, last_name);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, first_name);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, middle_name);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, title);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, suffix);
		strbuf_append_crlf      (vcard);
	}

	/*
	 * The type value is a structured type consisting of the organization name,
	 * followed by one or more levels of organizational unit names.
	 */
	 
	if (company || department)
	{
		strbuf_append(vcard, "ORG:");
		strbuf_append_escaped_wstr (vcard, company);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, department);
		strbuf_append_crlf      (vcard);
	}

	/* 
	 * The structured type value corresponds, in sequence, to the post office
	 * box; the extended address; the street address; the locality (e.g., city);
	 * the region (e.g., state or province); the postal code; the country name.
	 */

	if (home_street || home_locality || home_postal_code || home_country)
	{
		strbuf_append              (vcard, "ADR;TYPE=HOME:");
		strbuf_append_escaped_wstr (vcard, NULL); /* post office box */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, NULL); /* extended address */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_street);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_locality);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_region); /* region */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_postal_code);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_country);
		strbuf_append_crlf      (vcard);
	}

	if (work_street || work_locality || work_postal_code || work_country)
	{
		strbuf_append              (vcard, "ADR;TYPE=WORK:");
		strbuf_append_escaped_wstr (vcard, NULL); /* post office box */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, NULL); /* extended address */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_street);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_locality);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_region); /* region */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_postal_code);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_country);
		strbuf_append_crlf      (vcard);
	}


	if (!have_fn)
	{
		/* TODO: make up a value for this property! */
	}

	strbuf_append(vcard, "END:vCard\n");

	*ppVcard = vcard->buffer;
	strbuf_free(vcard, false);
	return true;
}/*}}}*/

bool rra_contact_to_vcard(/*{{{*/
		uint32_t id, 
		const uint8_t* data, 
		size_t data_size, 
		uint32_t vcard_version,
		char** vcard)
{
	bool success = false;
	uint32_t field_count = 0;
	CEPROPVAL* fields = NULL;

	if (!data)
	{
		synce_error("Data is NULL");
		goto exit;
	}

	if (data_size < 8)
	{
		synce_error("Invalid data size");
		goto exit;
	}

	field_count = letoh32(*(uint32_t*)(data + 0));
	synce_trace("Field count: %i", field_count);

	if (0 == field_count)
	{
		synce_error("No fields!");
		goto exit;
	} 
	
	if (field_count > MAX_FIELD_COUNT)
	{
		synce_error("A contact does not have this many fields");
		goto exit;
	}

	fields = (CEPROPVAL*)malloc(sizeof(CEPROPVAL) * field_count);

	if (!dbstream_to_propvals(data + 8, field_count, fields))
	{
		fprintf(stderr, "Failed to convert database stream\n");
		goto exit;
	}

	if (!rra_contact_to_vcard2(
				id, 
				fields, 
				field_count, 
				vcard_version,
				vcard))
	{
		fprintf(stderr, "Failed to create vCard\n");
		goto exit;
	}

	success = true;

exit:
	dbstream_free_propvals(fields);
	return success;
}/*}}}*/

typedef enum _VcardState
{
	VCARD_STATE_UNKNOWN,
	VCARD_STATE_NEWLINE,
	VCARD_STATE_NAME,
	VCARD_STATE_TYPE,
	VCARD_STATE_VALUE,
} VcardState;

#define myisblank(c)    ((c) == ' '  || (c) == '\t')
#define myisnewline(c)  ((c) == '\n' || (c) == '\r')

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

typedef struct _Parser
{
	uint32_t vcard_version;
	int level;
	CEPROPVAL* fields;
	size_t field_index;
} Parser;

#define NAME_FIELD_COUNT 5

static uint32_t name_ids[NAME_FIELD_COUNT] =
{
	ID_LAST_NAME,
	ID_FIRST_NAME,
	ID_MIDDLE_NAME,
	ID_TITLE,
	ID_SUFFIX
};

#define HOME 0
#define WORK 1

#define ADDRESS_FIELD_COUNT 7

static uint32_t address_ids[ADDRESS_FIELD_COUNT][2] = 
{
	{0, 0}, /* post office box */
	{0, 0}, /* extended address */
	{ID_HOME_STREET,      ID_WORK_STREET},      /* street */
	{ID_HOME_LOCALITY,    ID_WORK_LOCALITY},    /* locality */
	{ID_HOME_REGION,      ID_WORK_REGION},      /* region */
	{ID_HOME_POSTAL_CODE, ID_WORK_POSTAL_CODE}, /* postal code */
	{ID_HOME_COUNTRY,     ID_WORK_COUNTRY}      /* country */
};

static char* strdup_quoted_printable(const char* source)/*{{{*/
{
	char* result = malloc(strlen(source) + 1);
	char* dest = result;

	while (*source)
	{
		if ('=' == source[0])
		{
			if (isxdigit(source[1]) && isxdigit(source[2]))
			{
				char hex[3] = {source[1], source[2], '\0'};
				*dest++ = strtol(hex, NULL, 16);
				source += 3;
			}
			else
			{
				break;
			}
		}
		else
		{
			*dest++ = *source++;
		}
	}

	*dest = '\0';
	return result;
}/*}}}*/

/* the length of the unescaped string is always less than or equal to the escaped string,
 * that's why we do this in place */
static void unescape_string(char* value)/*{{{*/
{
	char* source = value;
	char* dest = value;

	while (*source)
	{
		if ('\\' == *source)
		{
			switch (source[1])
			{
				case '\\':
				case ';':
				case ',':
					*dest++ = source[1];
					source += 2;
					break;

				case 'n':
					*dest++ = '\r';
					*dest++ = '\n';
					source += 2;
					break;

				default:
					synce_warning("Unexpected escape: '%c%c'", source[0], source[1]);
					break;
			}
		}
		else
			*dest++ = *source++;
	}

	*dest = '\0';
}/*}}}*/

static void set_string(CEPROPVAL* field, uint32_t id, const char* type, char* value)/*{{{*/
{
	char* converted = NULL;
	
	field->propid = (id << 16) | CEVT_LPWSTR;

	if (strstr(type, "QUOTED-PRINTABLE"))
	{
		value = converted = strdup_quoted_printable(value);
	}

	unescape_string(value);

	if (strstr(type, "UTF-8"))
		field->val.lpwstr = wstr_from_utf8(value);
	else
		field->val.lpwstr = wstr_from_ascii(value);

	if (converted)
		free(converted);
}/*}}}*/

static char** strsplit(const char* source, int separator)/*{{{*/
{
	int i;
	int count = 0;
	const char* p = NULL;
	char** result = NULL;
	size_t length = 0;

	for (p = source; *p; p++)
		if (separator == *p)
			count++;

	result = malloc((count + 2) * sizeof(char*));

	for (p = source, i = 0; i < count; i++)
	{
		length = strchr(p, separator) - p;
		result[i] = strndup(p, length);
		p += length + 1;
	}

	result[i++] = strdup(p);

	result[i] = NULL;
	return result;
}/*}}}*/

static void strv_dump(char** strv)/*{{{*/
{
	char** pp;

	for (pp = strv; *pp; pp++)
		synce_trace("'%s'", *pp);
}/*}}}*/

static void strv_free(char** strv)/*{{{*/
{
	char** pp;

	for (pp = strv; *pp; pp++)
		free(*pp);

	free(strv);
}/*}}}*/

static bool parser_handle_field(/*{{{*/
		Parser* parser,
		char* name, 
		char* type, 
		char* value)
{
	bool success = false;
	CEPROPVAL* field;

	synce_trace("Found field '%s' with type '%s' and contents '%s'",
			name, type, value);

	if (STR_EQUAL(name, "BEGIN"))/*{{{*/
	{
		if (STR_EQUAL(value, "VCARD"))
		{
			if (0 != parser->level)
			{
				synce_error("Nested vCards not supported");
				goto exit;
			}
			
			parser->level++;
			success = true;
			goto exit;
		}
		else
		{
			synce_error("Unexpected BEGIN");
			goto exit;
		}
	}/*}}}*/

	if (parser->level != 1)
	{
		synce_error("Not within BEGIN:VCARD / END:VCARD");
		goto exit;
	}
	
	if (STR_EQUAL(name, "END"))/*{{{*/
	{
		if (STR_EQUAL(value, "VCARD"))
		{
			parser->level--;
		}
		else
		{
			synce_error("Unexpected END");
			goto exit;
		}
	}/*}}}*/
	else if (STR_EQUAL(name, "VERSION"))/*{{{*/
	{
		if (STR_EQUAL(value, "2.1"))
			parser->vcard_version = RRA_CONTACT_VCARD_2_1;
		else if (STR_EQUAL(value, "3.0"))
			parser->vcard_version = RRA_CONTACT_VCARD_3_0;
		else
			parser->vcard_version = RRA_CONTACT_VCARD_UNKNOWN;
	}/*}}}*/
	else if (STR_EQUAL(name, "FN"))/*{{{*/
	{
		field = &parser->fields[parser->field_index++];
		set_string(field, ID_FULL_NAME, type, value);
	}/*}}}*/
	else if (STR_EQUAL(name, "N"))/*{{{*/
	{
		int i;
		char** name = strsplit(value, ';');

		for (i = 0; i < NAME_FIELD_COUNT && name[i]; i++)
		{
			if (name_ids[i] && *name[i])
			{
				field = &parser->fields[parser->field_index++];
				set_string(field, name_ids[i], type, name[i]);
			}
			else
				break;
		}

		strv_free(name);
	}/*}}}*/
	else if (STR_EQUAL(name, "ADR"))/*{{{*/
	{
		int i;
		char** address = strsplit(value, ';');
		int where;

		if (strstr(type, "WORK"))
			where = WORK;
		else if (strstr(type, "HOME"))
			where = HOME;
		else
		{
			synce_warning("Unknown address type: '%s'", type);
			goto exit;
		}
		
		strv_dump(address);
		
		for (i = 0; i < ADDRESS_FIELD_COUNT && address[i]; i++)
		{
			if (address_ids[i][where] && *address[i])
			{
				field = &parser->fields[parser->field_index++];
				set_string(field, address_ids[i][where], type, address[i]);
			}
		}
		
		strv_free(address);
	}/*}}}*/
	else if (STR_EQUAL(name, "TEL"))/*{{{*/
	{
		/* TODO: make type uppercase */
		if (strstr(type, "HOME"))
		{
			field = &parser->fields[parser->field_index++];
			set_string(field, ID_HOME_TEL, type, value);
		}
		else if (strstr(type, "WORK"))
		{
			field = &parser->fields[parser->field_index++];
			set_string(field, ID_WORK_TEL, type, value);
		}
		else if (strstr(type, "CELL"))
		{
			field = &parser->fields[parser->field_index++];
			set_string(field, ID_MOBILE_TEL, type, value);
		}
		else
		{
			synce_trace("Type '%s' for field '%s' not recognized.",
					type, name);
		}
	}/*}}}*/
	else if (STR_EQUAL(name, "EMAIL"))/*{{{*/
	{
		/* TODO: make type uppercase */
		/* TODO: handle multiple e-mail adresses */
		if (!strstr(type, "INTERNET"))
		{
			synce_trace("Unexpected type '%s' for field '%s', assuming 'INTERNET'",
					type, name);
		}
	
		field = &parser->fields[parser->field_index++];
		set_string(field, ID_EMAIL, type, value);
	}/*}}}*/
	else if (STR_EQUAL(name, "URL"))/*{{{*/
	{
		field = &parser->fields[parser->field_index++];
		set_string(field, ID_WEB_PAGE, type, value);
	}/*}}}*/
	else if (STR_EQUAL(name, "ORG"))/*{{{*/
	{
		char* separator = strchr(value, ';');
		if (separator && separator[1])
		{
			field = &parser->fields[parser->field_index++];
			set_string(field, ID_DEPARTMENT, type, separator + 1);
			*separator = '\0';
		}

		if (value[0])
		{
			field = &parser->fields[parser->field_index++];
			set_string(field, ID_COMPANY, type, value);
		}
	}/*}}}*/
	else if (STR_EQUAL(name, "TITLE"))/*{{{*/
	{
		field = &parser->fields[parser->field_index++];
		set_string(field, ID_JOB_TITLE, type, value);
	}/*}}}*/
	else if (STR_EQUAL(name, "X-EVOLUTION-FILE-AS"))/*{{{*/
	{
		synce_trace("So, your contact has been in Evolution?");
	}/*}}}*/
	else if (STR_EQUAL(name, "UID"))/*{{{*/
	{
		/* TODO: save for later */
	}/*}}}*/
	else if (STR_EQUAL(name, "PRODID"))/*{{{*/
	{
		/* TODO: something? */
	}/*}}}*/
#if 0
	else if (STR_EQUAL(name, ""))
	{
	}
	else if (STR_EQUAL(name, ""))
	{
	}
	else if (STR_EQUAL(name, ""))
	{
	}
	else if (STR_EQUAL(name, ""))
	{
	}
	else if (STR_EQUAL(name, ""))
	{
	}
#endif
	else
	{
		synce_trace("Field name '%s' not recognized", name);
		goto exit;
	}

	success = true;

exit:
	free(name);
	free(type);
	free(value);

	return success;
}/*}}}*/

/*
 * Simple vCard parser
 */
static bool rra_contact_from_vcard2(/*{{{*/
		const char* vcard, 
		uint32_t* id,
		CEPROPVAL* fields,
		size_t* field_count)
{
	bool success = false;
	Parser parser;
	size_t max_field_count = *field_count;
	const char* p = vcard;
	int state = VCARD_STATE_NEWLINE;
	const char* name = NULL;
	const char* name_end = NULL;
	const char* type = NULL;
	const char* type_end = NULL;
	const char* value = NULL;
	const char* value_end = NULL;

	parser.vcard_version  = RRA_CONTACT_VCARD_UNKNOWN;
	parser.level          = 0;
	parser.fields         = fields;
	parser.field_index    = 0;

	while (*p && parser.field_index < max_field_count)
	{
		switch (state)
		{
			case VCARD_STATE_NEWLINE:/*{{{*/
				if (myisblank(*p))
				{
					synce_error("Can't handle multiline values");
					goto exit;
				}

				if (myisnewline(*p))
				{
					p++;
				}
				else
				{
					name      = p++;
					name_end  = NULL;
					type      = NULL;
					type_end  = NULL;
					value     = NULL;
					value_end = NULL;

					state = VCARD_STATE_NAME;
				}
				break;/*}}}*/

			case VCARD_STATE_NAME:/*{{{*/
				if (':' == *p)
				{
					name_end = p;
					value = p + 1;
					state = VCARD_STATE_VALUE;
				}
				else if (';' == *p)
				{
					name_end = p;
					type = p + 1;
					state = VCARD_STATE_TYPE;
				}
				p++;
				break;/*}}}*/

			case VCARD_STATE_TYPE:/*{{{*/
				if (':' == *p)
				{
					type_end = p;
					value = p + 1;
					state = VCARD_STATE_VALUE;
				}
				p++;
				break;/*}}}*/

			case VCARD_STATE_VALUE:/*{{{*/
				if (myisnewline(*p))
				{
					value_end = p;

					parser_handle_field(
							&parser,
							strndup(name, name_end - name),
							type ? strndup(type, type_end - type) : strdup(""),
							strndup(value, value_end - value));

					state = VCARD_STATE_NEWLINE;
				}
				p++;
				break;/*}}}*/

			default:
				synce_error("Unknown state: %i", state);
				goto exit;
		}
	}

	*field_count = parser.field_index;

	success = true;
			
exit:
	return success;
}/*}}}*/

bool rra_contact_from_vcard(/*{{{*/
		int command, 
		const char* vcard, 
		uint32_t* id,
		uint8_t** data, 
		size_t* data_size)
{
	bool success = false;
	CEPROPVAL fields[MAX_FIELD_COUNT];
	size_t field_count = MAX_FIELD_COUNT;
	
	if (!rra_contact_from_vcard2(
				vcard,
				id,
				fields,
				&field_count))
	{
		synce_error("Failed to convert vCard to database entries");
		goto exit;
	}

	if (!dbstream_from_propvals(
				fields,
				field_count,
				data,
				data_size))
	{
		synce_error("Failed to convert database entries to stream");
		goto exit;
	}

	success = true;

exit:
	return success;
}/*}}}*/

