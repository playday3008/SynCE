/*
    Synical - Convert Pocket PC databases to iCalendar format
    Copyright (C) 2001  David Eriksson

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __rapistring_h__
#define __rapistring_h__

#include <iconv.h>

class RapiString
{
	private:
		mutable char* mUtf8;
		mutable WCHAR* mUcs2;
	
	public:
		RapiString(const char* utf8)
			: mUtf8(NULL), mUcs2(NULL)
		{
			mUtf8 = new char[strlen(utf8)+1];
			strcpy(mUtf8, utf8);
		}

		RapiString(const WCHAR* ucs2)
			: mUtf8(NULL), mUcs2(NULL)
		{
			mUcs2 = new WCHAR[wcslen(ucs2)+1];
			wcscpy(mUcs2, ucs2);
		}

		~RapiString()
		{
			if (mUtf8) delete mUtf8;
			if (mUcs2) delete mUcs2;
		}

#if 0
		operator const WCHAR*() const
		{
			if (!mUcs2)
			{
				//iconv_t cd = iconv_open("UCS-2", "UTF-8");	// from UTF-8 to UCS-2
				
				mUcs2 = new WCHAR[strlen(mUtf8)+1];
				// TODO: make nicer conversion
				for (int i = 0; i <= strlen(mUtf8); i++)
				{
					mUcs2[i] = mUtf8[i];
				}

				//iconv_close(cd);
			}

			return mUcs2;
		}
#endif

		operator const char*() const
		{
			if (!mUtf8)
			{
				iconv_t cd = iconv_open("UTF-8", "UCS-2"); // from UCS-2 to UTF-8
				
				// Make large enough buffer
				
				size_t inbytesleft = wcslen(mUcs2) * sizeof(WCHAR);
				char * inbuf = (char *)mUcs2;
				
				size_t outbytesleft = inbytesleft + 1;
				char * outbuf = mUtf8 = new char[outbytesleft];
				memset(mUtf8, 0, outbytesleft);
				
				size_t result = iconv(cd, &inbuf, &inbytesleft, &outbuf, &outbytesleft);

				if (result < 0)
				{
					strcpy(mUtf8, "(error)");
				}
				
				iconv_close(cd);
			}

			return mUtf8;
		}

};


#endif // __rapistring_h__

