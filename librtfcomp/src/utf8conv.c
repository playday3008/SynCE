///////////////////////////////////////////////////////////////////////////////
// UTF8CONV.C
//
// Unicode UTF8-UCHAR functions. These are helpers that are internal to the
// framework (as they make some assumptions about legality of the data.
//
// Dr J A Gow : 12/2006
//
// This file is distributed under the terms and conditions of the LGPL - please
// see the file LICENCE in the package root directory.
//
//
///////////////////////////////////////////////////////////////////////////////

#include "sysincludes.h"
#include "utf8conv.h"

//-----------------------------------------------------------------------------
//- UTF converters

// Map noting the number of trailing bytes we have for a given char

const BYTE UTF8ExtraBytes[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

// Offsets used during UTF8 conversion

static const DWORD offsetsFromUTF8[6] = { 
						0x00000000UL, 
						0x00003080UL,
						0x000E2080UL, 
		     				0x03C82080UL
					};

// See the Unicode-provided algorithm...
// Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
// into the first byte, depending on how many bytes follow.  There are
// as many entries in this table as there are UTF-8 sequence types.
// (I.e., one byte sequence, two byte... etc.). Remember that sequencs
// for *legal* UTF-8 will be 4 or fewer bytes total.
//

static const BYTE firstByteMark[7] = { 
					0x00, 0x00, 0xC0, 
					0xE0, 0xF0, 0xF8, 
					0xFC 
				     };

//*
//*
//* UTF-8 conversion helpers
//*
//*

///////////////////////////////////////////////////////////////////////////////
// CV_UTF32FromUTF8
//
// EXPORTED
//
// Generates a Unicode code point from a UTF8 byte array.
//
///////////////////////////////////////////////////////////////////////////////

DWORD CV_UTF32FromUTF8(BYTE * utfchar)
{
	DWORD ch = 0;

	WORD extraBytesToRead = UTF8ExtraBytes[*utfchar];

	switch (extraBytesToRead) {
	    		
		case 5: ch += *utfchar++; ch <<= 6;
    		case 4: ch += *utfchar++; ch <<= 6;
    		case 3: ch += *utfchar++; ch <<= 6;
    		case 2: ch += *utfchar++; ch <<= 6;
    		case 1: ch += *utfchar++; ch <<= 6;
    		case 0: ch += *utfchar++;

	}
	ch -= offsetsFromUTF8[extraBytesToRead];

	// We now have a unicode character. UTF32 legality checking takes place
	// when we convert.

	return ch;
}

///////////////////////////////////////////////////////////////////////////////
// CV_UTF8FromUTF32
//
// EXPORTED
//
// Fills the array with code point specified by the Unicode DWORD
//
///////////////////////////////////////////////////////////////////////////////

int CV_UTF8FromUTF32(DWORD ch, BYTE * data)
{
	const DWORD byteMask = 0xBF;
	const DWORD byteMark = 0x80; 
	int len = 0;
	BYTE * to = data;

	// We know ch will be legal - but check it anyway.
	// Figure out how many bytes the result will require. 

	if (ch < 0x80) {
		len = 1;
	} else { 
		if (ch < 0x800) {     
			len = 2;
		} else { 
			if (ch < 0x10000) {   
				len = 3;
			} else { 
				if (ch <= XU_UNICODE_CHARMAX) { 
					len = 4;
				} else {
					len = -1;
					return -1;
				}
			}
		}
	}
	
	to += len;

	switch (len) { 

		case 4:	*--to = (BYTE)((ch | byteMark) & byteMask); ch >>= 6;
		case 3:	*--to = (BYTE)((ch | byteMark) & byteMask); ch >>= 6;
		case 2:	*--to = (BYTE)((ch | byteMark) & byteMask); ch >>= 6;
		case 1:	*--to = (BYTE)(ch | firstByteMark[len]);
	}
	return len;
}

///////////////////////////////////////////////////////////////////////////////
// CV_SizeOfUTF8
//
// EXPORTED
//
// Returns the size in bytes of a given character when encoded in UTF-8
//
///////////////////////////////////////////////////////////////////////////////

int CV_SizeOfUTF8(const DWORD ch)
{
	int len=0;
	
	if (ch < 0x80) {
		len = 1;
	} else { 
		if (ch < 0x800) {     
			len = 2;
		} else { 
			if (ch < 0x10000) {   
				len = 3;
			} else { 
				if (ch <= XU_UNICODE_CHARMAX) { 
					len = 4;
				} else {
					len = 0;
				}
			}
		}
	}
	return len;
}

///////////////////////////////////////////////////////////////////////////////
// CV_SizeOfUTF8Data
//
// EXPORTED
//
// Returns the number of BYTEs in the character pointed to.
//
///////////////////////////////////////////////////////////////////////////////

int CV_SizeOfUTF8Data(const BYTE * data)
{
	return UTF8ExtraBytes[*data]+1;
}

