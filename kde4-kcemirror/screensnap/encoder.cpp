// encoder.cpp: implementation of the Encoder class.
//
//////////////////////////////////////////////////////////////////////

#include "encoder.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Encoder::Encoder()
{
	chain = NULL;
	encData = NULL;
}


Encoder::Encoder(Encoder *chain)
{
	this->chain = chain;
	encData = NULL;
}


size_t Encoder::encode(unsigned char **encData, unsigned char *rawData, 
					   size_t rawSize)
{
	*encData = rawData;

	return rawSize;
}


size_t Encoder::chainEncode(unsigned char **encData, unsigned char *rawData, 
							size_t rawSize)
{
	unsigned char *chainEncData;
	size_t chainEncSize;

	if (chain != NULL) {
		chainEncSize = chain->chainEncode(&chainEncData, rawData, rawSize);
	} else {
		chainEncData = rawData;
		chainEncSize = rawSize;
	}

	encSize = encode(&this->encData, chainEncData, chainEncSize);

	if (encData != NULL) {
		*encData = this->encData;
	}

	this->rawSize = rawSize;

	return encSize;
}


bool Encoder::writeSize(SOCKET s)
{
	u_long encSizeN = htonl(encSize);

	return send(s, (char *) &encSizeN, sizeof(u_long), 0) > 0;
}


bool Encoder::chainWriteSize(SOCKET s)
{
	bool ret = true;

	if (chain != NULL) {
		ret = chain->chainWriteSize(s);
	}

	if (ret) {
		ret = writeSize(s);
	}

	return ret;
}


bool Encoder::writeData(SOCKET s)
{
	bool ret;

	ret = send(s, (char *) encData, encSize, 0) > 0;

	return ret;
}



bool Encoder::chainNotChanged()
{
	bool ret = false;

	if (chain!= NULL) {
		ret = chain->chainNotChanged();
	}

	return ret;
}


bool Encoder::chainWrite(SOCKET s)
{
	bool ret = false;
	
	u_long rawSizeN = htonl(rawSize);
	if ((ret = (send(s, (char *) &rawSizeN, sizeof(u_long), 0) > 0))) {
		if ((ret = chainWriteSize(s))) {
			ret = writeData(s);
		}
	}

	return ret;
}


bool Encoder::write(SOCKET s)
{
	bool ret = false;

	if ((ret = writeSize(s))) {
		ret = writeData(s);
	}

	cleanUp();

	return ret;
}


void Encoder::cleanUpEncData()
{
	if (encData != NULL) {
		delete[] encData;
		encData = NULL;
	}
}


void Encoder::chainCleanUp()
{
	if (chain != NULL) {
		chain->chainCleanUp();
	}

	cleanUp();
}


void Encoder::cleanUp()
{
}


Encoder::~Encoder()
{
	if (chain != NULL) {
		delete chain;
	}
}
