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

class RapiString
{
	private:
		mutable char* mAscii;
		mutable WCHAR* mUcs2;
	
	public:
		RapiString(const char* ascii)
			: mAscii(NULL), mUcs2(NULL)
		{
			mAscii = new char[strlen(ascii)+1];
			strcpy(mAscii, ascii);
		}

		RapiString(const WCHAR* ucs2)
			: mAscii(NULL), mUcs2(NULL)
		{
			mUcs2 = new WCHAR[wcslen(ucs2)+1];
			wcscpy(mUcs2, ucs2);
		}

		~RapiString()
		{
			if (mAscii) delete mAscii;
			if (mUcs2) delete mUcs2;
		}

		operator const WCHAR*() const
		{
			if (!mUcs2)
			{
				mUcs2 = new WCHAR[strlen(mAscii)+1];
				// TODO: make nicer conversion
				for (int i = 0; i <= strlen(mAscii); i++)
				{
					mUcs2[i] = mAscii[i];
				}
			}

			return mUcs2;
		}

		operator const char*() const
		{
			if (!mAscii)
			{
				mAscii = new char[wcslen(mUcs2)+1];
				// TODO: make nicer conversion
				for (int i = 0; i <= wcslen(mUcs2); i++)
				{
					mAscii[i] = (char)mUcs2[i];
				}
			}

			return mAscii;
		}

};


#endif // __rapistring_h__

