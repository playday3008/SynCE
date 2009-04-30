// XorEncoder.h: interface for the XorEncoder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_XORENCODER_H__AF7FB0A3_8499_4D10_B1BE_59898AF37405__INCLUDED_)
#define AFX_XORENCODER_H__AF7FB0A3_8499_4D10_B1BE_59898AF37405__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "encoder.h"

class XorEncoder : public Encoder  
{
public:
	XorEncoder();
	XorEncoder(Encoder *chain);
	virtual size_t encode(unsigned char **encData,
			unsigned char *rawData, size_t rawSize);
	void cleanUp();
	bool chainNotChanged();
	virtual ~XorEncoder();
private:
	unsigned char *tempData;
};

#endif // !defined(AFX_XORENCODER_H__AF7FB0A3_8499_4D10_B1BE_59898AF37405__INCLUDED_)
