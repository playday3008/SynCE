// huffmanencoder.h: interface for the HuffmanEncoder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_HUFFMANENCODER_H__968FD74C_36D4_4C75_B864_171C3B432935__INCLUDED_)
#define AFX_HUFFMANENCODER_H__968FD74C_36D4_4C75_B864_171C3B432935__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "encoder.h"

class HuffmanEncoder : public Encoder  
{
public:
	HuffmanEncoder();
	HuffmanEncoder(Encoder *chain);
	virtual size_t encode(unsigned char **encData,
			unsigned char *rawData, size_t rawSize);
	void cleanUp();
	virtual ~HuffmanEncoder();
};

#endif // !defined(AFX_HUFFMANENCODER_H__968FD74C_36D4_4C75_B864_171C3B432935__INCLUDED_)
