// XorEncoder.cpp: implementation of the XorEncoder class.
//
//////////////////////////////////////////////////////////////////////

#include "XorEncoder.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

XorEncoder::XorEncoder() : Encoder()
{
	tempData = NULL;
}


XorEncoder::XorEncoder(Encoder *chain) : Encoder(chain)
{
	tempData = NULL;
}


bool XorEncoder::chainNotChanged()
{
	for (size_t i = 0; i < encSize; i++) {
		if (encData[i] != 0) {
			return false;
		}
	}

	return true;
}


size_t XorEncoder::encode(unsigned char **encData, 
								  unsigned char *rawData, size_t rawSize)
{
	if (this->encData == NULL) {
		this->encData = new unsigned char[rawSize];
		memset(this->encData, 0, rawSize);
		tempData = new unsigned char[rawSize];
		memset(tempData, 0, rawSize);
	}
	
	for (size_t i = 0; i < rawSize; i++) {
		this->encData[i] = tempData[i] ^ rawData[i];
		tempData[i] = rawData[i];
	}

	*encData = this->encData;

	return rawSize;
}


void XorEncoder::cleanUp()
{
}


XorEncoder::~XorEncoder()
{
	if (encData != NULL) {
		delete[] encData;
		delete[] tempData;
	}
}
