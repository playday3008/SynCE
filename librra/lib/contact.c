/* $Id$ */
#define _GNU_SOURCE 1
#include "contact.h"
#include "contact_ids.h"
#include "synce_ids.h"
#include "frontend.h"
#include "strbuf.h"
#include "dbstream.h"
#include "strv.h"
#include <rapi.h>
#include <synce_log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "internal.h"

extern char* convert_to_utf8(const char* inbuf);
extern char* convert_from_utf8(const char* source);

#define VERBOSE 0

static const char* product_id = "-//SYNCE RRA//NONSGML Version 1//EN";

/* 
 * the theoretical field count maximum is about 50
 * plus about 60 additional fields,
 * and we add 10 to be safe
 */
#define MAX_FIELD_COUNT 120

/* 
   ESCAPED-CHAR = "\\" / "\;" / "\," / "\n" / "\N")
        ; \\ encodes \, \n or \N encodes newline
        ; \; encodes ;, \, encodes ,
*/
static void strbuf_append_escaped(StrBuf* result, char* source, uint32_t flags)/*{{{*/
{
	char* p;

	if (!source)
		return;

	for (p = source; *p; p++)
	{
		switch (*p)
		{
			case '\r': 				/* CR */
				/* ignore */
				break;		

			case '\n':				/* LF */
				strbuf_append_c(result, '\\');
				strbuf_append_c(result, 'n');
				break;
	
			case '\\':
				strbuf_append_c(result, '\\');
				strbuf_append_c(result, *p);
				break;
				
			case ';':
			case ',':
				if (flags & RRA_CONTACT_VERSION_3_0)
					strbuf_append_c(result, '\\');
				/* fall through */

			default:
				strbuf_append_c(result, *p);
				break;
		}
	}
}/*}}}*/

/* 
   More or less the same as strbuf_append_escaped(), but specialized for comma separated lists
*/
static void strbuf_append_comma_separated(StrBuf* result, char* source, uint32_t flags)/*{{{*/
{
	char* p;

	if (!source)
		return;

	for (p = source; *p; p++)
	{
		switch (*p)
		{
			case '\r': 				/* CR */
				/* ignore */
				break;		

			case '\n':				/* LF */
				strbuf_append_c(result, '\\');
				strbuf_append_c(result, 'n');
				break;
	
			case '\\':
				strbuf_append_c(result, '\\');
				strbuf_append_c(result, *p);
				break;

			case ',':
				strbuf_append_c(result, ',');
				while (*(p+1)==' ') /* skip blanks after comma */
				  p++;
				break;

			case ';':
				if (flags & RRA_CONTACT_VERSION_3_0)
					strbuf_append_c(result, '\\');
				/* fall through */

			default:
				strbuf_append_c(result, *p);
				break;
		}
	}
}/*}}}*/

void strbuf_append_escaped_wstr(StrBuf* strbuf, WCHAR* wstr, uint32_t flags)/*{{{*/
{
	if (wstr)
	{
		char* str = NULL;
	 
		if (flags & RRA_CONTACT_UTF8)
			str = wstr_to_utf8(wstr);
		else
			str = wstr_to_ascii(wstr);

		strbuf_append_escaped(strbuf, str, flags);
		wstr_free_string(str);
	}
}/*}}}*/

void strbuf_append_comma_separated_wstr(StrBuf* strbuf, WCHAR* wstr, uint32_t flags)/*{{{*/
{
	if (wstr)
	{
		char* str = NULL;
	 
		if (flags & RRA_CONTACT_UTF8)
			str = wstr_to_utf8(wstr);
		else
			str = wstr_to_ascii(wstr);

		strbuf_append_comma_separated(strbuf, str, flags);
		wstr_free_string(str);
	}
}/*}}}*/

/*
 * value should be a comma-separated list of types
 */
static void strbuf_append_type(StrBuf* strbuf, const char* name, const char* value, uint32_t flags)
{
	strbuf_append(strbuf, name);
	strbuf_append_c(strbuf, ';');

	if (flags & RRA_CONTACT_VERSION_2_1)
	{
		char* copy = strdup(value);
		unsigned i;
		
		/* replace commas with semi-colons */
		for (i = 0; i < strlen(copy); i++)
			if (',' == copy[i])
				copy[i] = ';';

		strbuf_append(strbuf, copy);

		free(copy);
	}
	else if (flags & RRA_CONTACT_VERSION_3_0) 
	{
		strbuf_append(strbuf, "TYPE=");
		strbuf_append(strbuf, value);
	}
	else
	{
		synce_error("Unknown version");
	}
	
	strbuf_append_c(strbuf, ':');
}

static void strbuf_append_tel_type(StrBuf* strbuf, const char* value, uint32_t flags)
{
	strbuf_append_type(strbuf, "TEL", value, flags);
}

static void strbuf_append_date(StrBuf* strbuf, const char* name, FILETIME* filetime)
{
  char buffer[12];
  TIME_FIELDS time_fields;

  time_fields_from_filetime(filetime, &time_fields);
  snprintf(buffer, sizeof(buffer), "%04i-%02i-%02i", time_fields.Year, time_fields.Month, time_fields.Day);

  strbuf_append(strbuf, name);
  strbuf_append_c(strbuf, ':');
  strbuf_append(strbuf, buffer);
  strbuf_append_crlf(strbuf);
}

void rra_contact_to_vcard2_email(StrBuf* vcard, WCHAR* email, WCHAR* meta, bool pref, uint32_t flags)
{
  if (email)
  {
    if (meta)
    {
      strbuf_append(vcard, "EMAIL;");
      strbuf_append_wstr(vcard, meta);
      strbuf_append_c(vcard, ':');
    } else
      if (pref)
        strbuf_append_type(vcard, "EMAIL", "INTERNET,PREF", flags);
      else
        strbuf_append_type(vcard, "EMAIL", "INTERNET", flags);
    strbuf_append_escaped_wstr(vcard, email,flags);
    strbuf_append_crlf(vcard);
  }
};

void rra_contact_to_vcard2_tel(StrBuf* vcard, WCHAR* tel, const char* type, WCHAR* meta, uint32_t flags)
{
  if (tel)
  {
    if (meta)
    {
      strbuf_append(vcard, "TEL;");
      strbuf_append_wstr(vcard, meta);
      strbuf_append_c(vcard, ':');
    } else
      strbuf_append_tel_type(vcard, type, flags);
    strbuf_append_escaped_wstr(vcard, tel,flags);
    strbuf_append_crlf(vcard);
  }
};

#define set_count(count,value) if (count == 0) count=value;

static bool rra_contact_to_vcard2(/*{{{*/
		uint32_t id, 
		CEPROPVAL* pFields, 
		uint32_t count, 
		char** ppVcard,
		uint32_t flags)
{
  unsigned i, messaging_count, messaging_meta;
	StrBuf* vcard = strbuf_new(NULL);
	bool have_fn = false; /* the FN property must be present! */
	bool success = false;

	/* name parts */
	WCHAR* first_name = NULL;
	WCHAR* last_name  = NULL;
	WCHAR* title = NULL;
	WCHAR* suffix = NULL;
	WCHAR* middle_name = NULL;

	/* organization parts */
	WCHAR* company = NULL;
	WCHAR* department = NULL;
  WCHAR* office = NULL;

	/* home address parts */
	WCHAR* home_street = NULL;
	WCHAR* home_locality = NULL;
	WCHAR* home_region = NULL;
	WCHAR* home_postal_code = NULL;
	WCHAR* home_country = NULL;
  WCHAR* home_post_office = NULL;
	
	/* work address parts */
	WCHAR* work_street = NULL;
	WCHAR* work_locality = NULL;
	WCHAR* work_region = NULL;
	WCHAR* work_postal_code = NULL;
	WCHAR* work_country = NULL;
  WCHAR* work_post_office = NULL;

	/* other address parts */
	WCHAR* other_street = NULL;
	WCHAR* other_locality = NULL;
	WCHAR* other_region = NULL;
	WCHAR* other_postal_code = NULL;
	WCHAR* other_country = NULL;
  WCHAR* other_post_office = NULL;

  /* email */
  WCHAR* email = NULL;
  WCHAR* email2 = NULL;
  WCHAR* email3 = NULL;
  WCHAR* email4 = NULL;
  WCHAR* email_evolution_meta = NULL;
  WCHAR* email2_evolution_meta = NULL;
  WCHAR* email3_evolution_meta = NULL;
  WCHAR* email4_evolution_meta = NULL;

  /* evolution messaging */
  WCHAR* evolution_messaging[4][2] = {{NULL, NULL},
                                      {NULL, NULL},
                                      {NULL, NULL},
                                      {NULL, NULL}};

  /* telephone */
  WCHAR* tel_work = NULL;
  WCHAR* tel_home = NULL;
  WCHAR* tel_mobile = NULL;
  WCHAR* tel_radio = NULL;
  WCHAR* tel_car = NULL;
  WCHAR* fax_work = NULL;
  WCHAR* fax_home = NULL;
  WCHAR* tel_home2 = NULL;
  WCHAR* tel_assistant = NULL;
  WCHAR* tel_work2 = NULL;
  WCHAR* pager = NULL;

  WCHAR* tel_others[8] = { NULL, NULL, NULL, NULL,
                           NULL, NULL, NULL, NULL };

  WCHAR* tel_work_evolution_meta = NULL;
  WCHAR* tel_home_evolution_meta = NULL;
  WCHAR* tel_mobile_evolution_meta = NULL;
  WCHAR* tel_radio_evolution_meta = NULL;
  WCHAR* tel_car_evolution_meta = NULL;
  WCHAR* fax_work_evolution_meta = NULL;
  WCHAR* fax_home_evolution_meta = NULL;
  WCHAR* tel_home2_evolution_meta = NULL;
  WCHAR* tel_assistant_evolution_meta = NULL;
  WCHAR* tel_work2_evolution_meta = NULL;
  WCHAR* pager_evolution_meta = NULL;

  WCHAR* tel_others_evolution_meta[8] = { NULL, NULL, NULL, NULL,
                                          NULL, NULL, NULL, NULL };

	strbuf_append(vcard, "BEGIN:vCard\r\n");

	strbuf_append(vcard, "VERSION:");
	if (flags & RRA_CONTACT_VERSION_2_1)
		strbuf_append(vcard, "2.1");
	else if (flags & RRA_CONTACT_VERSION_3_0)
		strbuf_append(vcard, "3.0");
	else
	{
		synce_error("Unknown vCard version");
		goto exit;
	}

	strbuf_append_crlf(vcard);

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
    messaging_count = messaging_meta = 0;
		/* TODO: validate data types */
		switch (pFields[i].propid >> 16)
		{
      case ID_ANNIVERSARY:
        strbuf_append_date(vcard, "X-EVOLUTION-ANNIVERSARY", &pFields[i].val.filetime);
        break;

      case ID_BIRTHDAY:
        strbuf_append_date(vcard, "BDAY", &pFields[i].val.filetime);
        break;
      
			case ID_NOTE:
				{
					unsigned j;
					char *lpb_terminated;

          for (j = 0; j < pFields[i].val.blob.dwCount && pFields[i].val.blob.lpb[j]; j++)
						;

					if (j == pFields[i].val.blob.dwCount)
					{
						lpb_terminated=malloc(pFields[i].val.blob.dwCount+1);
						assert(lpb_terminated);
						memcpy(lpb_terminated, pFields[i].val.blob.lpb, pFields[i].val.blob.dwCount);
						lpb_terminated[pFields[i].val.blob.dwCount]='\0';

						strbuf_append(vcard, "NOTE:");

            if (flags & RRA_CONTACT_UTF8)
            {
              char *lpb_converted = convert_to_utf8(lpb_terminated);
              free (lpb_terminated);
              lpb_terminated = lpb_converted;
            }

            /* Windows CE require that NOTE is pair
             * if not we add a "End of text" character (0x3)
             * at end of NOTE before send it to pda.
             * We remove that character when we receive it
             * from pda.
             */

            if (lpb_terminated[strlen(lpb_terminated) - 1] == 0x3)
              lpb_terminated[strlen(lpb_terminated) - 1] = 0x0;

            strbuf_append_escaped(vcard, lpb_terminated, flags);
						strbuf_append_crlf(vcard);

						free(lpb_terminated);
					}
					else
					{
						synce_warning("Note is not an ASCII string.\n");
					}
				}
				break;

			case ID_SUFFIX:
				suffix = pFields[i].val.lpwstr;
				break;

			case ID_FIRST_NAME:
				first_name = pFields[i].val.lpwstr;
				break;

			case ID_WORK_TEL:
        tel_work = pFields[i].val.lpwstr;
				break;

			case ID_HOME_TEL:
        tel_home = pFields[i].val.lpwstr;
				break;

			case ID_LAST_NAME:
				last_name = pFields[i].val.lpwstr;
				break;

			case ID_COMPANY:
				company = pFields[i].val.lpwstr;
				break;

      case ID_JOB_ROLE:
        strbuf_append(vcard, "ROLE:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
				strbuf_append_crlf(vcard);
				break;

			case ID_DEPARTMENT:
				department = pFields[i].val.lpwstr;
				break;

			case ID_MOBILE_TEL:
        tel_mobile = pFields[i].val.lpwstr;
				break;

			case ID_CAR_TEL:
        tel_car = pFields[i].val.lpwstr;
				break;

			case ID_WORK_FAX:
        fax_work = pFields[i].val.lpwstr;
				break;

			case ID_HOME_FAX:
        fax_home = pFields[i].val.lpwstr;
				break;

			case ID_HOME2_TEL:
        tel_home2 = pFields[i].val.lpwstr;
				break;

			case ID_CATEGORY:
				strbuf_append(vcard, "CATEGORIES:");
				strbuf_append_comma_separated_wstr(vcard, pFields[i].val.lpwstr, flags);
				strbuf_append_crlf(vcard);
				break;

			case ID_WORK2_TEL:
        tel_work2 = pFields[i].val.lpwstr;
				break;

			case ID_WEB_PAGE:
				strbuf_append(vcard, "URL:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
				strbuf_append_crlf(vcard);
				break;

			case ID_PAGER:
        pager = pFields[i].val.lpwstr;
				break;

			case ID_FULL_NAME:
				strbuf_append(vcard, "FN:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
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

      case ID_HOME_POST_OFFICE:
        home_post_office = pFields[i].val.lpwstr;
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

      case ID_WORK_POST_OFFICE:
        work_post_office = pFields[i].val.lpwstr;
        break;

			case ID_OTHER_STREET:
				other_street = pFields[i].val.lpwstr;
				break;

			case ID_OTHER_LOCALITY:
				other_locality = pFields[i].val.lpwstr;
				break;

			case ID_OTHER_REGION:
				other_region = pFields[i].val.lpwstr;
				break;

			case ID_OTHER_POSTAL_CODE:
				other_postal_code = pFields[i].val.lpwstr;
				break;

			case ID_OTHER_COUNTRY:
				other_country = pFields[i].val.lpwstr;
				break;

      case ID_OTHER_POST_OFFICE:
        other_post_office = pFields[i].val.lpwstr;
        break;

			case ID_EMAIL:
        email = pFields[i].val.lpwstr;
				break;

			case ID_EMAIL2:
        email2 = pFields[i].val.lpwstr;
        break;

			case ID_EMAIL3:
        email3 = pFields[i].val.lpwstr;
        break;

      case ID_EMAIL4:
        email4 = pFields[i].val.lpwstr;
				break;

      case ID_EMAIL_EVOLUTION_META:
        email_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_EMAIL2_EVOLUTION_META:
        email2_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_EMAIL3_EVOLUTION_META:
        email3_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_EMAIL4_EVOLUTION_META:
        email4_evolution_meta = pFields[i].val.lpwstr;
        break;

			case ID_SPOUSE:
				strbuf_append(vcard, "X-EVOLUTION-SPOUSE:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
				strbuf_append_crlf(vcard);
				break;

			case ID_ASSISTANT:
				strbuf_append(vcard, "X-EVOLUTION-ASSISTANT:");
				strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
				strbuf_append_crlf(vcard);
				break;

			case ID_ASSISTANT_TEL:
        tel_assistant = pFields[i].val.lpwstr;
				break;

			case ID_OFFICE_LOC:
        office = pFields[i].val.lpwstr;
				break;

			case ID_RADIO_TEL:
        tel_radio = pFields[i].val.lpwstr;
				break;

      case ID_CHILDREN:
        strbuf_append(vcard, "X-SYNCE-CHILDREN:");
        strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
        strbuf_append_crlf(vcard);
        break;

      case ID_OTHER_TEL1:
        tel_others[0] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL2:
        tel_others[1] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL3:
        tel_others[2] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL4:
        tel_others[3] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL5:
        tel_others[4] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL6:
        tel_others[5] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL7:
        tel_others[6] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL8:
        tel_others[7] = pFields[i].val.lpwstr;
        break;

      case ID_WORK_TEL_EVOLUTION_META:
        tel_work_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_HOME_TEL_EVOLUTION_META:
        tel_home_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_MOBILE_TEL_EVOLUTION_META:
        tel_mobile_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_RADIO_TEL_EVOLUTION_META:
        tel_radio_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_CAR_TEL_EVOLUTION_META:
        tel_car_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_WORK_FAX_EVOLUTION_META:
        fax_work_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_HOME_FAX_EVOLUTION_META:
        fax_home_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_HOME2_TEL_EVOLUTION_META:
        tel_home2_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_ASSISTANT_TEL_EVOLUTION_META:
        tel_assistant_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_WORK2_TEL_EVOLUTION_META:
        tel_work2_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_PAGER_EVOLUTION_META:
        pager_evolution_meta = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL1_EVOLUTION_META:
        tel_others_evolution_meta[0] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL2_EVOLUTION_META:
        tel_others_evolution_meta[1] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL3_EVOLUTION_META:
        tel_others_evolution_meta[2] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL4_EVOLUTION_META:
        tel_others_evolution_meta[3] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL5_EVOLUTION_META:
        tel_others_evolution_meta[4] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL6_EVOLUTION_META:
        tel_others_evolution_meta[5] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL7_EVOLUTION_META:
        tel_others_evolution_meta[6] = pFields[i].val.lpwstr;
        break;

      case ID_OTHER_TEL8_EVOLUTION_META:
        tel_others_evolution_meta[7] = pFields[i].val.lpwstr;
        break;

/* FOOBAR */
			case ID_IMADDRESS:
				switch(rra_frontend_get())
				{
					case ID_FRONTEND_KDEPIM:
						synce_warning("ID_IMADDRESS");
						strbuf_append(vcard, "X-KADDRESSBOOK-X-IMAddress:");
						strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
						strbuf_append_crlf(vcard);
						break;
					default:
						synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
						break;
				}
				break;

			case ID_MESSAGING_ICQ:
				switch(rra_frontend_get())
				{
					case ID_FRONTEND_KDEPIM:
						synce_warning("ID_MESSAGING_ICQ");
						strbuf_append(vcard, "X-messaging/icq-All:");
						strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
						strbuf_append_crlf(vcard);
            break;
					default:
						synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
						break;
				}
				break;

			case ID_MESSAGING_XMPP:
				switch(rra_frontend_get())
				{
					case ID_FRONTEND_KDEPIM:
						synce_warning("ID_MESSAGING_XMPP");
						strbuf_append(vcard, "X-messaging/xmpp-All:");
						strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
						strbuf_append_crlf(vcard);
            break;
					default:
						synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
						break;
				}
				break;

			case ID_MESSAGING_MSN:
				switch(rra_frontend_get())
				{
					case ID_FRONTEND_KDEPIM:
						synce_warning("ID_MESSAGING_MSN");
						strbuf_append(vcard, "X-messaging/msn-All:");
						strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
						strbuf_append_crlf(vcard);
            break;
					default:
						synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
						break;
				}
				break;

			case ID_MESSAGING_GADU:
				switch(rra_frontend_get())
				{
					case ID_FRONTEND_KDEPIM:
						synce_warning("ID_MESSAGING_GADU");
						strbuf_append(vcard, "X-messaging/gadu-All:");
						strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
						strbuf_append_crlf(vcard);
            break;
					default:
						synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
						break;
				}
				break;

			case ID_NICKNAME:
				switch(rra_frontend_get())
				{
					case ID_FRONTEND_KDEPIM:
          case ID_FRONTEND_EVOLUTION:
						strbuf_append(vcard, "NICKNAME:");
						strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
						strbuf_append_crlf(vcard);
            break;
					default:
						synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
						break;
				}
				break;

      case ID_JOB_TITLE:
        strbuf_append(vcard, "TITLE:");
        strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
        strbuf_append_crlf(vcard);
        break;

      case ID_X_MOZILLA_HTML:
        switch(rra_frontend_get())
        {
          case ID_FRONTEND_EVOLUTION:
            strbuf_append(vcard, "X-MOZILLA-HTML:");
            strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
            strbuf_append_crlf(vcard);
            break;
          default:
            synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
            break;
        }
        break;

      case ID_X_EVOLUTION_BLOG_URL:
        switch(rra_frontend_get())
        {
          case ID_FRONTEND_EVOLUTION:
            strbuf_append(vcard, "X-EVOLUTION-BLOG-URL:");
            strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
            strbuf_append_crlf(vcard);
            break;
          default:
            synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
            break;
        }
        break;

      case ID_X_EVOLUTION_CALURI:
        switch(rra_frontend_get())
        {
          case ID_FRONTEND_EVOLUTION:
            strbuf_append(vcard, "CALURI:");
            strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
            strbuf_append_crlf(vcard);
            break;
          default:
            synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
            break;
        }
        break;

      case ID_X_EVOLUTION_FBURL:
        switch(rra_frontend_get())
        {
          case ID_FRONTEND_EVOLUTION:
            strbuf_append(vcard, "FBURL:");
            strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
            strbuf_append_crlf(vcard);
            break;
          default:
            synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
            break;
        }
        break;

      case ID_X_EVOLUTION_VIDEOURL:
        switch(rra_frontend_get())
        {
          case ID_FRONTEND_EVOLUTION:
            strbuf_append(vcard, "X-EVOLUTION-VIDEO-URL:");
            strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
            strbuf_append_crlf(vcard);
            break;
          default:
            synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
            break;
        }
        break;

      case ID_X_EVOLUTION_MANAGER:
        switch(rra_frontend_get())
        {
          case ID_FRONTEND_EVOLUTION:
            strbuf_append(vcard, "X-EVOLUTION-MANAGER:");
            strbuf_append_escaped_wstr(vcard, pFields[i].val.lpwstr, flags);
            strbuf_append_crlf(vcard);
            break;
          default:
            synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
            break;
        }
        break;

      case ID_X_EVOLUTION_MESSAGING1_META:
        set_count(messaging_count, 1);
      case ID_X_EVOLUTION_MESSAGING2_META:
        set_count(messaging_count, 2);
      case ID_X_EVOLUTION_MESSAGING3_META:
        set_count(messaging_count, 3);
      case ID_X_EVOLUTION_MESSAGING4_META:
        set_count(messaging_count, 4);
        messaging_meta = 1;
      case ID_X_EVOLUTION_MESSAGING1:
        set_count(messaging_count, 1);
      case ID_X_EVOLUTION_MESSAGING2:
        set_count(messaging_count, 2);
      case ID_X_EVOLUTION_MESSAGING3:
        set_count(messaging_count, 3);
      case ID_X_EVOLUTION_MESSAGING4:
        set_count(messaging_count, 4);
        switch(rra_frontend_get())
        {
          case ID_FRONTEND_EVOLUTION:
            evolution_messaging[messaging_count -1][messaging_meta] = pFields[i].val.lpwstr;
            break;
          default:
            synce_warning("Field with ID %04x not supported by frontend %u", pFields[i].propid >> 16, rra_frontend_get());
            break;
        }
        break;

/* FOOBAR */
			default:
				synce_warning("Did not handle field with ID %04x", pFields[i].propid >> 16);
				break;
		}
	}

	if (first_name || last_name || middle_name || title || suffix)
	{
		strbuf_append              (vcard, "N:");
		strbuf_append_escaped_wstr (vcard, last_name, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, first_name, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, middle_name, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, title, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, suffix, flags);
		strbuf_append_crlf      (vcard);
	}

	/*
	 * The type value is a structured type consisting of the organization name,
	 * followed by one or more levels of organizational unit names.
	 */
	 
  if (company || department || office)
	{
		strbuf_append(vcard, "ORG:");
		strbuf_append_escaped_wstr (vcard, company, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, department, flags);
    strbuf_append_c            (vcard, ';');
    strbuf_append_escaped_wstr (vcard, office, flags);
		strbuf_append_crlf      (vcard);
	}

	/* 
	 * The structured type value corresponds, in sequence, to the post office
	 * box; the extended address; the street address; the locality (e.g., city);
	 * the region (e.g., state or province); the postal code; the country name.
	 */

  if (home_street || home_locality || home_postal_code || home_country || home_post_office)
	{
    char *street = NULL;
    char *extended = NULL;

    if (home_street)
    {
      if (flags & RRA_CONTACT_UTF8)
        street = wstr_to_utf8(home_street);
      else
        street = wstr_to_ascii(home_street);

      extended = strstr(street, "\n");

      if (extended)
        extended++[-1] = '\0';
    }

		strbuf_append_type(vcard, "ADR", "HOME", flags);
    strbuf_append_escaped_wstr (vcard, home_post_office, flags); /* post office box */
		strbuf_append_c            (vcard, ';');
    strbuf_append_escaped      (vcard, extended, flags); /* extended address */
		strbuf_append_c            (vcard, ';');
    strbuf_append_escaped      (vcard, street, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_locality, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_region, flags); /* region */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_postal_code, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, home_country, flags);
		strbuf_append_crlf      (vcard);

    if (street)
      wstr_free_string(street);
	}

  if (work_street || work_locality || work_postal_code || work_country || work_post_office)
	{
    char *street = NULL;
    char *extended = NULL;

    if (work_street)
    {
      if (flags & RRA_CONTACT_UTF8)
        street = wstr_to_utf8(work_street);
      else
        street = wstr_to_ascii(work_street);

      extended = strstr(street, "\n");

      if (extended)
        *extended++ = '\0';
    }

		strbuf_append_type(vcard, "ADR", "WORK", flags);
    strbuf_append_escaped_wstr (vcard, work_post_office, flags); /* post office box */
		strbuf_append_c            (vcard, ';');
    strbuf_append_escaped      (vcard, extended, flags); /* extended address */
		strbuf_append_c            (vcard, ';');
    strbuf_append_escaped      (vcard, street, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_locality, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_region, flags); /* region */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_postal_code, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, work_country, flags);
		strbuf_append_crlf      (vcard);

    if (street)
      wstr_free_string(street);
	}

  if (other_street || other_locality || other_postal_code || other_country || other_post_office)
	{
    char *street = NULL;
    char *extended = NULL;

    if (other_street)
    {
      if (flags & RRA_CONTACT_UTF8)
        street = wstr_to_utf8(other_street);
      else
        street = wstr_to_ascii(other_street);

      extended = strstr(street, "\n");

      if (extended)
        extended++[-1] = '\0';
    }

    switch(rra_frontend_get())
    {
      case ID_FRONTEND_EVOLUTION:
        strbuf_append_type(vcard, "ADR", "OTHER", flags);
        break;
      default:
        strbuf_append_type(vcard, "ADR", "POSTAL", flags);
        break;
    }
    strbuf_append_escaped_wstr (vcard, other_post_office, flags); /* post office box */
		strbuf_append_c            (vcard, ';');
    strbuf_append_escaped      (vcard, extended, flags); /* extended address */
		strbuf_append_c            (vcard, ';');
    strbuf_append_escaped      (vcard, street, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, other_locality, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, other_region, flags); /* region */
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, other_postal_code, flags);
		strbuf_append_c            (vcard, ';');
		strbuf_append_escaped_wstr (vcard, other_country, flags);
		strbuf_append_crlf      (vcard);

    if (street)
      wstr_free_string(street);
	}

  switch(rra_frontend_get())
  {
    case ID_FRONTEND_EVOLUTION:
      rra_contact_to_vcard2_email(vcard, email, email_evolution_meta, true, flags);
      rra_contact_to_vcard2_email(vcard, email2, email2_evolution_meta, false, flags);
      rra_contact_to_vcard2_email(vcard, email3, email3_evolution_meta, false, flags);
      rra_contact_to_vcard2_email(vcard, email4, email4_evolution_meta, false, flags);

      rra_contact_to_vcard2_tel(vcard, tel_work, "WORK,VOICE,PREF", tel_work_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, tel_home, "HOME,VOICE,PREF", tel_home_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, tel_mobile, "CELL", tel_mobile_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, tel_radio, "X-EVOLUTION-RADIO", tel_radio_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, tel_car, "CAR", tel_car_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, fax_work, "WORK,FAX", fax_work_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, fax_home, "HOME,FAX", fax_home_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, tel_home2, "HOME,VOICE", tel_home2_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, tel_assistant, "X-EVOLUTION-ASSISTANT", tel_assistant_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, tel_work2, "WORK,VOICE", tel_work2_evolution_meta, flags);
      rra_contact_to_vcard2_tel(vcard, pager, "PAGER", pager_evolution_meta, flags);

      for (i = 0; i < 8; i++)
        rra_contact_to_vcard2_tel(vcard, tel_others[i], "VOICE", tel_others_evolution_meta[i], flags);
      break;
    default:
      rra_contact_to_vcard2_email(vcard, email, NULL, true, flags);
      rra_contact_to_vcard2_email(vcard, email2, NULL, false, flags);
      rra_contact_to_vcard2_email(vcard, email3, NULL,  false, flags);
      rra_contact_to_vcard2_email(vcard, email4, NULL,  false, flags);

      rra_contact_to_vcard2_tel(vcard, tel_work, "WORK,VOICE,PREF", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, tel_home, "HOME,VOICE,PREF", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, tel_mobile, "CELL", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, tel_radio, "X-EVOLUTION-RADIO", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, tel_car, "CAR", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, fax_work, "WORK,FAX", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, fax_home, "HOME,FAX", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, tel_home2, "HOME,VOICE", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, tel_assistant, "X-EVOLUTION-ASSISTANT", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, tel_work2, "WORK,VOICE", NULL, flags);
      rra_contact_to_vcard2_tel(vcard, pager, "PAGER", NULL, flags);

      for (i = 0; i < 8; i++)
        rra_contact_to_vcard2_tel(vcard, tel_others[i], "VOICE", NULL, flags);
      break;
  }

  for (messaging_count=0; messaging_count < 4; messaging_count++)
  {
    if (evolution_messaging[messaging_count][0] && evolution_messaging[messaging_count][1])
    {
      strbuf_append_wstr(vcard, evolution_messaging[messaging_count][1]);
      strbuf_append_c(vcard, ':');
      strbuf_append_escaped_wstr(vcard, evolution_messaging[messaging_count][0],flags);
      strbuf_append_crlf(vcard);
    }
  }

	if (!have_fn)
	{
		/* TODO: make up a value for this property! */
	}

	strbuf_append(vcard, "END:vCard\n");

	*ppVcard = vcard->buffer;
	success = true;

exit:
	strbuf_destroy(vcard, !success);
	return success;
}/*}}}*/

bool rra_contact_to_vcard(/*{{{*/
		uint32_t id, 
		const uint8_t* data, 
		size_t data_size,
		char** vcard,
		uint32_t flags)
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
				vcard,
				flags))
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
  VCARD_STATE_MULTILINE,
} VcardState;

#define myisblank(c)    ((c) == ' '  || (c) == '\t')
#define myisnewline(c)  ((c) == '\n' || (c) == '\r')

#define STR_EQUAL(a,b)  (0 == strcasecmp(a,b))

#define STR_IN_STR(haystack, needle)  (0 != strcasestr(haystack, needle))

typedef struct _Parser
{
	uint32_t vcard_version;
	int level;
	CEPROPVAL* fields;
	size_t field_index;
	bool utf8;	/* default charset is utf8 */
  int count_email;
  int count_tel_work;
  int count_tel_home;
  int count_tel_other;
  int count_evolution_messaging;
} Parser;

typedef enum _field_index
{
  INDEX_NOTE,
  INDEX_SUFFIX,
  INDEX_FIRST_NAME,
  INDEX_WORK_TEL,
  INDEX_HOME_TEL,
  INDEX_LAST_NAME,
  INDEX_COMPANY,
  INDEX_JOB_ROLE,
  INDEX_DEPARTMENT,
  INDEX_OFFICE_LOC,
  INDEX_MOBILE_TEL,
  INDEX_RADIO_TEL,
  INDEX_CAR_TEL,
  INDEX_WORK_FAX,
  INDEX_HOME_FAX,
  INDEX_HOME2_TEL,
  INDEX_BIRTHDAY,
  INDEX_ANNIVERSARY,
  INDEX_CATEGORY,
  INDEX_ASSISTANT,
  INDEX_ASSISTANT_TEL,
  INDEX_CHILDREN,
  INDEX_WORK2_TEL,
  INDEX_WEB_PAGE,
  INDEX_PAGER,
  INDEX_SPOUSE,
  INDEX_FULL_NAME,
  INDEX_TITLE,
  INDEX_MIDDLE_NAME,
  INDEX_HOME_STREET,
  INDEX_HOME_LOCALITY,
  INDEX_HOME_REGION,
  INDEX_HOME_POSTAL_CODE,
  INDEX_HOME_COUNTRY,
  INDEX_WORK_STREET,
  INDEX_WORK_LOCALITY,
  INDEX_WORK_REGION,
  INDEX_WORK_POSTAL_CODE,
  INDEX_WORK_COUNTRY,
  INDEX_OTHER_STREET,
  INDEX_OTHER_LOCALITY,
  INDEX_OTHER_REGION,
  INDEX_OTHER_POSTAL_CODE,
  INDEX_OTHER_COUNTRY,
  INDEX_EMAIL,
  INDEX_EMAIL2,
  INDEX_EMAIL3,
  INDEX_IMADDRESS,
  INDEX_MESSAGING_ICQ,
  INDEX_MESSAGING_XMPP,
  INDEX_MESSAGING_MSN,
  INDEX_MESSAGING_GADU,
  INDEX_NICKNAME,
  INDEX_JOB_TITLE,
  INDEX_EMAIL4,
  INDEX_EMAIL_EVOLUTION_META,
  INDEX_EMAIL2_EVOLUTION_META,
  INDEX_EMAIL3_EVOLUTION_META,
  INDEX_EMAIL4_EVOLUTION_META,
  INDEX_HOME_POST_OFFICE,
  INDEX_WORK_POST_OFFICE,
  INDEX_OTHER_POST_OFFICE,
  INDEX_X_MOZILLA_HTML,
  INDEX_X_EVOLUTION_BLOG_URL,
  INDEX_X_EVOLUTION_CALURI,
  INDEX_X_EVOLUTION_FBURL,
  INDEX_X_EVOLUTION_VIDEOURL,
  INDEX_X_EVOLUTION_MANAGER,
  INDEX_X_EVOLUTION_MESSAGING1,
  INDEX_X_EVOLUTION_MESSAGING2,
  INDEX_X_EVOLUTION_MESSAGING3,
  INDEX_X_EVOLUTION_MESSAGING4,
  INDEX_X_EVOLUTION_MESSAGING1_META,
  INDEX_X_EVOLUTION_MESSAGING2_META,
  INDEX_X_EVOLUTION_MESSAGING3_META,
  INDEX_X_EVOLUTION_MESSAGING4_META,
  INDEX_OTHER_TEL1,
  INDEX_OTHER_TEL2,
  INDEX_OTHER_TEL3,
  INDEX_OTHER_TEL4,
  INDEX_OTHER_TEL5,
  INDEX_OTHER_TEL6,
  INDEX_OTHER_TEL7,
  INDEX_OTHER_TEL8,
  INDEX_WORK_TEL_EVOLUTION_META,
  INDEX_HOME_TEL_EVOLUTION_META,
  INDEX_MOBILE_TEL_EVOLUTION_META,
  INDEX_RADIO_TEL_EVOLUTION_META,
  INDEX_CAR_TEL_EVOLUTION_META,
  INDEX_WORK_FAX_EVOLUTION_META,
  INDEX_HOME_FAX_EVOLUTION_META,
  INDEX_HOME2_TEL_EVOLUTION_META,
  INDEX_ASSISTANT_TEL_EVOLUTION_META,
  INDEX_WORK2_TEL_EVOLUTION_META,
  INDEX_PAGER_EVOLUTION_META,
  INDEX_OTHER_TEL1_EVOLUTION_META,
  INDEX_OTHER_TEL2_EVOLUTION_META,
  INDEX_OTHER_TEL3_EVOLUTION_META,
  INDEX_OTHER_TEL4_EVOLUTION_META,
  INDEX_OTHER_TEL5_EVOLUTION_META,
  INDEX_OTHER_TEL6_EVOLUTION_META,
  INDEX_OTHER_TEL7_EVOLUTION_META,
  INDEX_OTHER_TEL8_EVOLUTION_META,
  ID_COUNT
} field_index;

#define NAME_FIELD_COUNT 5

static uint32_t name_index[NAME_FIELD_COUNT] =
{
  INDEX_LAST_NAME,
  INDEX_FIRST_NAME,
  INDEX_MIDDLE_NAME,
  INDEX_TITLE,
  INDEX_SUFFIX
};

#define HOME 0
#define WORK 1
#define OTHER 2

#define ADDRESS_FIELD_COUNT 7

static uint32_t address_index[ADDRESS_FIELD_COUNT][3] = 
{
  {INDEX_HOME_POST_OFFICE,  INDEX_WORK_POST_OFFICE,  INDEX_OTHER_POST_OFFICE}, /* post office box */
  {0, 0}, /* extended address */
  {INDEX_HOME_STREET,       INDEX_WORK_STREET,       INDEX_OTHER_STREET},      /* street */
  {INDEX_HOME_LOCALITY,     INDEX_WORK_LOCALITY,     INDEX_OTHER_LOCALITY},    /* locality */
  {INDEX_HOME_REGION,       INDEX_WORK_REGION,       INDEX_OTHER_REGION},      /* region */
  {INDEX_HOME_POSTAL_CODE,  INDEX_WORK_POSTAL_CODE,  INDEX_OTHER_POSTAL_CODE}, /* postal code */
  {INDEX_HOME_COUNTRY,      INDEX_WORK_COUNTRY,      INDEX_OTHER_COUNTRY}      /* country */
};

#define MESSAGING_FIELD_COUNT 4

static uint32_t messaging_index[MESSAGING_FIELD_COUNT][2] =
{
  {INDEX_X_EVOLUTION_MESSAGING1,  INDEX_X_EVOLUTION_MESSAGING1_META},
  {INDEX_X_EVOLUTION_MESSAGING2,  INDEX_X_EVOLUTION_MESSAGING2_META},
  {INDEX_X_EVOLUTION_MESSAGING3,  INDEX_X_EVOLUTION_MESSAGING3_META},
  {INDEX_X_EVOLUTION_MESSAGING4,  INDEX_X_EVOLUTION_MESSAGING4_META},
};

#define OTHER_TEL_COUNT 8

static uint32_t other_tel_index[OTHER_TEL_COUNT] =
{
  INDEX_OTHER_TEL1,
  INDEX_OTHER_TEL2,
  INDEX_OTHER_TEL3,
  INDEX_OTHER_TEL4,
  INDEX_OTHER_TEL5,
  INDEX_OTHER_TEL6,
  INDEX_OTHER_TEL7,
  INDEX_OTHER_TEL8,
};

static uint32_t other_tel_evolution_meta_index[OTHER_TEL_COUNT] =
{
  INDEX_OTHER_TEL1_EVOLUTION_META,
  INDEX_OTHER_TEL2_EVOLUTION_META,
  INDEX_OTHER_TEL3_EVOLUTION_META,
  INDEX_OTHER_TEL4_EVOLUTION_META,
  INDEX_OTHER_TEL5_EVOLUTION_META,
  INDEX_OTHER_TEL6_EVOLUTION_META,
  INDEX_OTHER_TEL7_EVOLUTION_META,
  INDEX_OTHER_TEL8_EVOLUTION_META,
};

static const uint32_t field_id[ID_COUNT] =
{
  ID_NOTE,
  ID_SUFFIX,
  ID_FIRST_NAME,
  ID_WORK_TEL,
  ID_HOME_TEL,
  ID_LAST_NAME,
  ID_COMPANY,
  ID_JOB_ROLE,
  ID_DEPARTMENT,
  ID_OFFICE_LOC,
  ID_MOBILE_TEL,
  ID_RADIO_TEL,
  ID_CAR_TEL,
  ID_WORK_FAX,
  ID_HOME_FAX,
  ID_HOME2_TEL,
  ID_BIRTHDAY,
  ID_ANNIVERSARY,
  ID_CATEGORY,
  ID_ASSISTANT,
  ID_ASSISTANT_TEL,
  ID_CHILDREN,
  ID_WORK2_TEL,
  ID_WEB_PAGE,
  ID_PAGER,
  ID_SPOUSE,
  ID_FULL_NAME,
  ID_TITLE,
  ID_MIDDLE_NAME,
  ID_HOME_STREET,
  ID_HOME_LOCALITY,
  ID_HOME_REGION,
  ID_HOME_POSTAL_CODE,
  ID_HOME_COUNTRY,
  ID_WORK_STREET,
  ID_WORK_LOCALITY,
  ID_WORK_REGION,
  ID_WORK_POSTAL_CODE,
  ID_WORK_COUNTRY,
  ID_OTHER_STREET,
  ID_OTHER_LOCALITY,
  ID_OTHER_REGION,
  ID_OTHER_POSTAL_CODE,
  ID_OTHER_COUNTRY,
  ID_EMAIL,
  ID_EMAIL2,
  ID_EMAIL3,
  ID_IMADDRESS,
  ID_MESSAGING_ICQ,
  ID_MESSAGING_XMPP,
  ID_MESSAGING_MSN,
  ID_MESSAGING_GADU,
  ID_NICKNAME,
  ID_JOB_TITLE,
  ID_EMAIL4,
  ID_EMAIL_EVOLUTION_META,
  ID_EMAIL2_EVOLUTION_META,
  ID_EMAIL3_EVOLUTION_META,
  ID_EMAIL4_EVOLUTION_META,
  ID_HOME_POST_OFFICE,
  ID_WORK_POST_OFFICE,
  ID_OTHER_POST_OFFICE,
  ID_X_MOZILLA_HTML,
  ID_X_EVOLUTION_BLOG_URL,
  ID_X_EVOLUTION_CALURI,
  ID_X_EVOLUTION_FBURL,
  ID_X_EVOLUTION_VIDEOURL,
  ID_X_EVOLUTION_MANAGER,
  ID_X_EVOLUTION_MESSAGING1,
  ID_X_EVOLUTION_MESSAGING2,
  ID_X_EVOLUTION_MESSAGING3,
  ID_X_EVOLUTION_MESSAGING4,
  ID_X_EVOLUTION_MESSAGING1_META,
  ID_X_EVOLUTION_MESSAGING2_META,
  ID_X_EVOLUTION_MESSAGING3_META,
  ID_X_EVOLUTION_MESSAGING4_META,
  ID_OTHER_TEL1,
  ID_OTHER_TEL2,
  ID_OTHER_TEL3,
  ID_OTHER_TEL4,
  ID_OTHER_TEL5,
  ID_OTHER_TEL6,
  ID_OTHER_TEL7,
  ID_OTHER_TEL8,
  ID_WORK_TEL_EVOLUTION_META,
  ID_HOME_TEL_EVOLUTION_META,
  ID_MOBILE_TEL_EVOLUTION_META,
  ID_RADIO_TEL_EVOLUTION_META,
  ID_CAR_TEL_EVOLUTION_META,
  ID_WORK_FAX_EVOLUTION_META,
  ID_HOME_FAX_EVOLUTION_META,
  ID_HOME2_TEL_EVOLUTION_META,
  ID_ASSISTANT_TEL_EVOLUTION_META,
  ID_WORK2_TEL_EVOLUTION_META,
  ID_PAGER_EVOLUTION_META,
  ID_OTHER_TEL1_EVOLUTION_META,
  ID_OTHER_TEL2_EVOLUTION_META,
  ID_OTHER_TEL3_EVOLUTION_META,
  ID_OTHER_TEL4_EVOLUTION_META,
  ID_OTHER_TEL5_EVOLUTION_META,
  ID_OTHER_TEL6_EVOLUTION_META,
  ID_OTHER_TEL7_EVOLUTION_META,
  ID_OTHER_TEL8_EVOLUTION_META,
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

static bool date_to_struct(const char* datetime, TIME_FIELDS* time_fields)/*{{{*/
{
  int count;
  memset(time_fields, 0, sizeof(*time_fields));

  count = sscanf(datetime, "%4hd-%2hd-%2hd", 
      &time_fields->Year,
      &time_fields->Month,
      &time_fields->Day);

  if (count != 3)
  {
    synce_error("Bad date: '%s'", datetime);
    return false;
  }

  return true;
}/*}}}*/

static void add_date(Parser* parser, uint32_t index, const char* type, char* value)
{
  TIME_FIELDS time_fields;

  assert(value);

  if (date_to_struct(value, &time_fields))
  {
    CEPROPVAL* propval = &parser->fields[index];
    if (propval->propid & CEVT_FLAG_EMPTY)
    {
      propval->propid = (field_id[index] << 16) | CEVT_FILETIME;
      time_fields_to_filetime(&time_fields, &propval->val.filetime);
    }
  }
}

static void add_string(Parser* parser, uint32_t index, const char* type, char* value)/*{{{*/
{
  char* converted = NULL;
  CEPROPVAL* field = &parser->fields[index];

  assert(value);
	
  if (field->propid & CEVT_FLAG_EMPTY)
  {
    field->propid = (field_id[index] << 16) | CEVT_LPWSTR;

    if (STR_IN_STR(type, "QUOTED-PRINTABLE"))
    {
      value = converted = strdup_quoted_printable(value);
      assert(value);
    }

    unescape_string(value);
    assert(value);
  
    if (parser->utf8 || STR_IN_STR(type, "UTF-8"))
      field->val.lpwstr = wstr_from_utf8(value);
    else
      field->val.lpwstr = wstr_from_ascii(value);
  
    assert(field->val.lpwstr);
  
    if (converted)
      free(converted);
  }
}/*}}}*/

static void add_blob(Parser* parser, uint32_t index, const char* type, char* data, size_t data_size)/*{{{*/
{
  CEPROPVAL* field = &parser->fields[index];

  assert(data);

  if (field->propid & CEVT_FLAG_EMPTY)
  {
    field->propid = (field_id[index] << 16) | CEVT_BLOB;

    field->val.blob.dwCount = data_size;
    field->val.blob.lpb = malloc(data_size);
    assert(field->val.blob.lpb);
    memcpy(field->val.blob.lpb, data, data_size);
  }
}/*}}}*/

#define is_empty(parser, index) parser->fields[index].propid & CEVT_FLAG_EMPTY

static bool parser_handle_field(/*{{{*/
		Parser* parser,
		char* name, 
		char* type, 
    char* value)
{

	bool success = false;

#if VERBOSE
	synce_trace("Found field '%s' with type '%s' and contents '%s'",
			name, type, value);
#endif

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
			parser->vcard_version = RRA_CONTACT_VERSION_2_1;
		else if (STR_EQUAL(value, "3.0"))
			parser->vcard_version = RRA_CONTACT_VERSION_3_0;
		else
			parser->vcard_version = RRA_CONTACT_VERSION_UNKNOWN;
	}/*}}}*/
	else if (STR_EQUAL(name, "FN"))/*{{{*/
	{
    if (is_empty(parser, INDEX_FULL_NAME))
      add_string(parser, INDEX_FULL_NAME, type, value);
	}/*}}}*/
	else if (STR_EQUAL(name, "N"))/*{{{*/
	{
		int i;
		char** name = strsplit(value, ';');

		for (i = 0; i < NAME_FIELD_COUNT && name[i]; i++)
		{
			if (*name[i])
			{
        add_string(parser, name_index[i], type, name[i]);
			}
		}

		strv_free(name);
	}/*}}}*/
	else if (STR_EQUAL(name, "ADR"))/*{{{*/
	{
		int i;
		char** address = strsplit(value, ';');
		int where;

    if (STR_IN_STR(type, "POSTAL") || STR_IN_STR(type, "OTHER"))
		        where = OTHER;
		else if (STR_IN_STR(type, "WORK"))
			where = WORK;
		else if (STR_IN_STR(type, "HOME"))
			where = HOME;
		else
		{
			synce_warning("Unknown address type: '%s'", type);
			goto exit;
		}
		
		strv_dump(address);
		
		for (i = 0; i < ADDRESS_FIELD_COUNT && address[i]; i++)
		{
      if (address_index[i][where] && *address[i])
			{
        if (i == 2 && *address[1])
        {
          char buf[strlen(address[1]) + strlen(address[2]) + 3];

          snprintf(buf, sizeof(buf), "%s\\n%s", address[2], address[1]);

          add_string(parser, address_index[i][where], type, buf);
        } else
          add_string(parser, address_index[i][where], type, address[i]);
			}
		}
		
		strv_free(address);
	}/*}}}*/
	else if (STR_EQUAL(name, "TEL"))/*{{{*/
	{
    bool fax = STR_IN_STR(type, "FAX");

		/* TODO: make type uppercase */
    if (STR_IN_STR(type, "HOME") && !fax && parser->count_tel_home < MAX_TEL_HOME)
    {
      switch (parser->count_tel_home++)
      {
      case 0:
        add_string(parser, INDEX_HOME_TEL, type, value);
        if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
          add_string(parser, INDEX_HOME_TEL_EVOLUTION_META, type, type);
        break;
      case 1:
        add_string(parser, INDEX_HOME2_TEL, type, value);
        if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
          add_string(parser, INDEX_HOME2_TEL_EVOLUTION_META, type, type);
        break;
      }
    }
    else if (STR_IN_STR(type, "HOME") && fax && is_empty(parser, INDEX_HOME_FAX))
    {
      add_string(parser, INDEX_HOME_FAX, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_HOME_FAX_EVOLUTION_META, type, type);
    }
    else if (STR_IN_STR(type, "WORK") && !fax && parser->count_tel_work < MAX_TEL_WORK)
    {
      switch (parser->count_tel_work++)
      {
      case 0:
        add_string(parser, INDEX_WORK_TEL, type, value);
        if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
          add_string(parser, INDEX_WORK_TEL_EVOLUTION_META, type, type);
        break;
      case 1:
        add_string(parser, INDEX_WORK2_TEL, type, value);
        if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
          add_string(parser, INDEX_WORK2_TEL_EVOLUTION_META, type, type);
        break;
      }
    }
    else if (STR_IN_STR(type, "WORK") && fax && is_empty(parser, INDEX_WORK_FAX))
    {
      add_string(parser, INDEX_WORK_FAX, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_WORK_FAX_EVOLUTION_META, type, type);
    }
    else if (STR_IN_STR(type, "CELL") && is_empty(parser, INDEX_MOBILE_TEL))
		{
      add_string(parser, INDEX_MOBILE_TEL, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_MOBILE_TEL_EVOLUTION_META, type, type);
		}
    else if (STR_IN_STR(type, "X-EVOLUTION-ASSISTANT") && is_empty(parser, INDEX_ASSISTANT_TEL))
		{
      add_string(parser, INDEX_ASSISTANT_TEL, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_ASSISTANT_TEL_EVOLUTION_META, type, type);
		}
    else if (STR_IN_STR(type, "X-EVOLUTION-RADIO") && is_empty(parser, INDEX_RADIO_TEL))
		{
      add_string(parser, INDEX_RADIO_TEL, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_RADIO_TEL_EVOLUTION_META, type, type);
		}
    else if (STR_IN_STR(type, "CAR") && is_empty(parser, INDEX_CAR_TEL))
		{
      add_string(parser, INDEX_CAR_TEL, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_CAR_TEL_EVOLUTION_META, type, type);
		}
    else if (STR_IN_STR(type, "PAGER") && is_empty(parser, INDEX_PAGER))
		{
      add_string(parser, INDEX_PAGER, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_PAGER_EVOLUTION_META, type, type);
		}
    else
		{
      if (parser->count_tel_other < OTHER_TEL_COUNT)
      {
        if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
          add_string(parser, other_tel_evolution_meta_index[parser->count_tel_other], type, type);
        add_string(parser, other_tel_index[parser->count_tel_other++], type, value);
      }
      else
        synce_trace("Type '%s' for field '%s' not recognized.",
            type, name);
		}
	}/*}}}*/
	else if (STR_EQUAL(name, "EMAIL"))/*{{{*/
	{
		/* TODO: make type uppercase */
		/* TODO: handle multiple e-mail adresses */
		if (!STR_IN_STR(type, "INTERNET"))
		{
			synce_trace("Unexpected type '%s' for field '%s', assuming 'INTERNET'",
					type, name);
		}

    switch (parser->count_email++)
    {
    case 0:
      add_string(parser, INDEX_EMAIL, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_EMAIL_EVOLUTION_META, type, type);
      break;
    case 1:
      add_string(parser, INDEX_EMAIL2, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_EMAIL2_EVOLUTION_META, type, type);
      break;
    case 2:
      add_string(parser, INDEX_EMAIL3, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_EMAIL3_EVOLUTION_META, type, type);
      break;
    case 3:
      add_string(parser, INDEX_EMAIL4, type, value);
      if (rra_frontend_get() == ID_FRONTEND_EVOLUTION)
        add_string(parser, INDEX_EMAIL4_EVOLUTION_META, type, type);
      break;
    }
	}/*}}}*/
	else if (STR_EQUAL(name, "URL"))/*{{{*/
	{
    add_string(parser, INDEX_WEB_PAGE, type, value);
	}/*}}}*/
	else if (STR_EQUAL(name, "ORG"))/*{{{*/
	{
		char* separator = strchr(value, ';');
		if (separator)
		{
			if (separator[1]) {
        char* separator2 = strchr(separator + 1, ';');
        if (separator2)
        {
          add_string(parser, INDEX_OFFICE_LOC, type, separator2 + 1);
          *separator2 = '\0';
        }
        add_string(parser, INDEX_DEPARTMENT, type, separator + 1);
			}
			*separator = '\0';
		}

		if (value[0])
		{
      add_string(parser, INDEX_COMPANY, type, value);
		}
	}/*}}}*/
  else if (STR_EQUAL(name, "ROLE"))/*{{{*/
	{
    add_string(parser, INDEX_JOB_ROLE, type, value);
	}/*}}}*/
	else if (STR_EQUAL(name, "X-EVOLUTION-FILE-AS"))/*{{{*/
	{
    if (is_empty(parser, INDEX_FULL_NAME))
      add_string(parser, INDEX_FULL_NAME, type, value);
#if VERBOSE
		synce_trace("So, your contact has been in Evolution?");
#endif
	}/*}}}*/
	else if (STR_EQUAL(name, "UID"))/*{{{*/
	{
		/* TODO: save for later */
	}/*}}}*/
	else if (STR_EQUAL(name, "PRODID"))/*{{{*/
	{
		/* TODO: something? */
	}/*}}}*/
  else if (STR_EQUAL(name, "CATEGORIES"))
  {
    add_string(parser, INDEX_CATEGORY, type, value);
  }
  else if (STR_EQUAL(name, "BDAY"))
  {
    add_date(parser, INDEX_BIRTHDAY, type, value);
  }
  else if (STR_EQUAL(name, "X-EVOLUTION-ANNIVERSARY"))
  {
    add_date(parser, INDEX_ANNIVERSARY, type, value);
  }
  else if (STR_EQUAL(name, "X-EVOLUTION-SPOUSE"))
  {
    add_string(parser, INDEX_SPOUSE, type, value);
  }
  else if (STR_EQUAL(name, "X-EVOLUTION-ASSISTANT"))
  {
    add_string(parser, INDEX_ASSISTANT, type, value);
  }
  else if (STR_EQUAL(name, "NOTE") && strlen(value) > 0)
  {
    size_t value_length;

    if (parser->utf8)
    {
      char *value_utf8 = convert_from_utf8(value);
      free(value);
      value = value_utf8;
    }

    unescape_string(value);

    /* Windows CE require that NOTE is pair
     * if not we add a "End of text" character (0x3)
     * at end of NOTE before send it to pda.
     * We remove that character when we receive it
     * from pda.
     */

    value_length = strlen(value);

    if (value_length % 2)
    {
      char *value_old = value;

      value = malloc(value_length + 2);
      memcpy(value, value_old, value_length);
      value[value_length++] = 0x3;
      value[value_length] = 0x0;
      free(value_old);
    }

    add_blob(parser, INDEX_NOTE, type, value, value_length);
  }
  else if (STR_EQUAL(name, "X-SYNCE-CHILDREN"))
  {
    add_string(parser, INDEX_CHILDREN, type, value);
  }
/* FOOBAR */
  else if (rra_frontend_get()==ID_FRONTEND_KDEPIM && STR_EQUAL(name, "X-KADDRESSBOOK-X-IMAddress"))
  {
    add_string(parser, INDEX_IMADDRESS, type, value);
  }
  else if (rra_frontend_get()==ID_FRONTEND_KDEPIM && STR_EQUAL(name, "X-messaging/icq-All"))
  {
    add_string(parser, INDEX_MESSAGING_ICQ, type, value);
  }
  else if (rra_frontend_get()==ID_FRONTEND_KDEPIM && STR_EQUAL(name, "X-messaging/xmpp-All"))
  {
    add_string(parser, INDEX_MESSAGING_XMPP, type, value);
  }
  else if (rra_frontend_get()==ID_FRONTEND_KDEPIM && STR_EQUAL(name, "X-messaging/msn-All"))
  {
    add_string(parser, INDEX_MESSAGING_MSN, type, value);
  }
  else if (rra_frontend_get()==ID_FRONTEND_KDEPIM && STR_EQUAL(name, "X-messaging/gadu-All"))
  {
    add_string(parser, INDEX_MESSAGING_GADU, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_KDEPIM || rra_frontend_get()==ID_FRONTEND_EVOLUTION) && STR_EQUAL(name, "NICKNAME"))
  {
    add_string(parser, INDEX_NICKNAME, type, value);
  }
  else if (STR_EQUAL(name, "TITLE"))/*{{{*/
  {
    add_string(parser, INDEX_JOB_TITLE, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_EVOLUTION) && STR_EQUAL(name, "X-MOZILLA-HTML"))
  {
    add_string(parser, INDEX_X_MOZILLA_HTML, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_EVOLUTION) && STR_EQUAL(name, "X-EVOLUTION-BLOG-URL"))
  {
    add_string(parser, INDEX_X_EVOLUTION_BLOG_URL, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_EVOLUTION) && STR_EQUAL(name, "CALURI"))
  {
    add_string(parser, INDEX_X_EVOLUTION_CALURI, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_EVOLUTION) && STR_EQUAL(name, "FBURL"))
  {
    add_string(parser, INDEX_X_EVOLUTION_FBURL, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_EVOLUTION) && STR_EQUAL(name, "X-EVOLUTION-VIDEO-URL"))
  {
    add_string(parser, INDEX_X_EVOLUTION_VIDEOURL, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_EVOLUTION) && STR_EQUAL(name, "X-EVOLUTION-MANAGER"))
  {
    add_string(parser, INDEX_X_EVOLUTION_MANAGER, type, value);
  }
  else if ((rra_frontend_get()==ID_FRONTEND_EVOLUTION) && ( \
    STR_EQUAL(name, "X-AIM") || \
    STR_EQUAL(name, "X-GROUPWISE") || \
    STR_EQUAL(name, "X-ICQ") || \
    STR_EQUAL(name, "X-JABBER") || \
    STR_EQUAL(name, "X-MSN") || \
    STR_EQUAL(name, "X-YAHOO")))
  {
    char meta[strlen(name) + strlen(type) + 2];

    snprintf(meta, sizeof(meta), "%s;%s", name, type);

    if (parser->count_evolution_messaging < MESSAGING_FIELD_COUNT)
    {
      add_string(parser, messaging_index[parser->count_evolution_messaging][0], type, value);
      add_string(parser, messaging_index[parser->count_evolution_messaging++][1], type, meta);
    }
  }/*}}}*/
/* FOOBAR */

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
 * we add 40 for additional fields
 * that are been handled by others application,
 * evolution have about 28 additional fields
 */
#define MAX_ENQUEUE_FIELD (MAX_FIELD_COUNT + 40)

/*
 * all tel_work, tel_home and email must be considered before
 * deciding which will get a slot, as there may be too many
 */
void enqueue_field(/*{{{*/
		struct FieldStrings *fs,
		int *count,
		struct FieldStrings *data)
{
	int slot=-1;
	int i;

	/* if there are free slots, there is no problem */
  if ((*count)<MAX_ENQUEUE_FIELD)
		slot=(*count);
	else if (data->pref) 
		/* so there is no free slot, but a(nother) preferred one is coming
		   kick out a non-preferred one, if possible
		*/
    for (i=0; i<MAX_ENQUEUE_FIELD; i++)
      if (!fs[i].pref && !strcmp(fs[i].type, data->type)) {
				slot=i;
				break;
			}
	if (slot>=0) {
		fs[slot].name  = data->name;
		fs[slot].type  = data->type;
		fs[slot].value = data->value;
		fs[slot].pref  = data->pref;
		(*count)++;
	}
}/*}}}*/

/*
 * process the queued fields.
 */
void process_queue(/*{{{*/
	Parser *parser,
	struct FieldStrings *fs,
	int count)
{
	int i;
	char *loc;
	
	/* Get the first preferred and make it the preferred one on the device */
	for (i=0; i<count; i++) {
    if (fs[i].pref || i == 0) {
			parser_handle_field(
				parser,
				fs[i].name,
				fs[i].type,
        fs[i].value);
			/* mark this as already processed */
			fs[i].name=NULL;
		}
	}
	
	/* Process all remaining */
	for (i=0; i<count; i++) {
		if (fs[i].name!=NULL) {

			/* The remaining can't be preferred. Correct me, if I'm wrong! */
			loc = strstr(fs[i].type, "TYPE=PREF;");
			if (!loc)
				loc = strstr(fs[i].type, ";TYPE=PREF");
			if (loc)
				memmove(loc, loc+10 ,strlen(fs[i].type)-(loc-fs[i].type-10));

			parser_handle_field(
				parser,
				fs[i].name,
				fs[i].type,
        fs[i].value);
		}
	}
}/*}}}*/

/*
 * Simple vCard parser
 */
static bool rra_contact_from_vcard2(/*{{{*/
		const char* vcard, 
		uint32_t* id,
		CEPROPVAL* fields,
		size_t* field_count,
		uint32_t flags)
{
	bool success = false;
	Parser parser;
	size_t max_field_count = *field_count;
	const char* p = vcard;
  const char* vcard_end = vcard + strlen(vcard);
	int state = VCARD_STATE_NEWLINE;
  int count;
	const char* name = NULL;
	const char* name_end = NULL;
	const char* type = NULL;
	const char* type_end = NULL;
	const char* value = NULL;
	const char* value_end = NULL;
  char*       value_ml = NULL;

  struct FieldStrings *queue_field  = malloc( MAX_ENQUEUE_FIELD * sizeof( struct FieldStrings ) ); 
	struct FieldStrings *tmp_field = malloc( 1 * sizeof( struct FieldStrings ) ); 

  int count_field = 0;

	parser.vcard_version  = RRA_CONTACT_VERSION_UNKNOWN;
	parser.level          = 0;
	parser.fields         = fields;
	parser.field_index    = 0;
	parser.utf8           = flags & RRA_CONTACT_UTF8;

  parser.count_email = 0;
  parser.count_tel_work = 0;
  parser.count_tel_home = 0;
  parser.count_tel_other = 0;
  parser.count_evolution_messaging = 0;

  for (count=0; count < ID_COUNT; count++)
  {
    CEPROPVAL* field = &parser.fields[parser.field_index++];

    switch(count)
    {
    case INDEX_BIRTHDAY:
    case INDEX_ANNIVERSARY:
      field->propid = (field_id[count] << 16) |
          CEVT_FLAG_EMPTY | CEVT_FILETIME;
      break;
    case INDEX_NOTE:
      field->propid = (field_id[count] << 16) |
          CEVT_FLAG_EMPTY | CEVT_BLOB;
      break;
    default:
      field->propid = (field_id[count] << 16) |
          CEVT_FLAG_EMPTY | CEVT_LPWSTR;
      break;
    }
  }

	while (*p && parser.field_index < max_field_count)
	{
		switch (state)
		{
			case VCARD_STATE_NEWLINE:/*{{{*/
				if (myisblank(*p))
				{
          synce_error("Failed to handle multiline values");
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
          value_ml  = NULL;

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

          if ((p+2) < vcard_end && myisblank(*(p+2)))
          {
            value_ml = malloc(strlen(vcard) - (value - vcard));
            value_ml[0] = '\0';
        
            strncat(value_ml, value, value_end - value);
        
            p = p + 3;
            value = p;
        
            state = VCARD_STATE_MULTILINE;
        
            break;
          }

					tmp_field->name  = strndup(name, name_end - name);
					tmp_field->type  = type ? strndup(type, type_end - type) : strdup("");
					tmp_field->value = strndup(value, value_end - value);
					tmp_field->pref  = STR_IN_STR(tmp_field->type, "PREF");

          if (rra_frontend_get() == ID_FRONTEND_EVOLUTION &&
              STR_IN_STR(tmp_field->name, "X-EVOLUTION-FILE-AS"))
            tmp_field->pref = true;

          enqueue_field(queue_field, &count_field, tmp_field);

					state = VCARD_STATE_NEWLINE;

          if (value_ml)
            free(value_ml);
				}
				p++;
				break;/*}}}*/

      case VCARD_STATE_MULTILINE:/*{{{*/
        if (myisnewline(*p))
        {
          value_end = p;
            
          strncat(value_ml, value, value_end - value);
            
          if ((p+2) < vcard_end && myisblank(*(p+2)))
          {
            p = p + 3;
            value = p;
          }
          else
          {
            value = value_ml;
            state = VCARD_STATE_VALUE;
          }
        }
        else
          p++;
        break;/*}}}*/

			default:
				synce_error("Unknown state: %i", state);
				goto exit;
		}
	}

  process_queue(&parser, queue_field, count_field);

	*field_count = parser.field_index;

	success = true;
			
exit:
	return success;
}/*}}}*/

bool rra_contact_from_vcard(/*{{{*/
		const char* vcard, 
		uint32_t* id,
		uint8_t** data, 
		size_t* data_size,
		uint32_t flags)
{
	bool success = false;
	CEPROPVAL fields[MAX_FIELD_COUNT];
	size_t field_count = MAX_FIELD_COUNT;
	
	if (!rra_contact_from_vcard2(
				vcard,
				id,
				fields,
				&field_count,
				flags))
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

