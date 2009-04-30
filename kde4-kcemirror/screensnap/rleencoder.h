// rleencoder.h: interface for the RleEncoder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RLEENCODER_H__C5B32689_AF5A_4D17_98F9_B8186960E862__INCLUDED_)
#define AFX_RLEENCODER_H__C5B32689_AF5A_4D17_98F9_B8186960E862__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "encoder.h"

class RleEncoder : public Encoder  
{
public:
	RleEncoder();
	RleEncoder::RleEncoder(Encoder *chain);
	virtual size_t encode(unsigned char **target, unsigned char *pixels, size_t size);
	virtual ~RleEncoder();

private:
	size_t oldSize;
};

#endif // !defined(AFX_RLEENCODER_H__C5B32689_AF5A_4D17_98F9_B8186960E862__INCLUDED_)
