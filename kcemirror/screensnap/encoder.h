// encoder.h: interface for the Encoder class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENCODER_H__687994DC_D8BA_42CD_B0F7_0F15D40DF5D4__INCLUDED_)
#define AFX_ENCODER_H__687994DC_D8BA_42CD_B0F7_0F15D40DF5D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Winsock.h>

class Encoder  
{
public:
	Encoder();
	Encoder(Encoder *chain);
	virtual size_t encode(unsigned char **encData, // The real encoding takes place here
			unsigned char *rawData, size_t rawSize);
	bool write(SOCKET s);			
	size_t chainEncode(unsigned char **encData, unsigned char *rawData, size_t rawSize);
	bool chainWrite(SOCKET s);
	virtual bool chainNotChanged();
	void chainCleanUp();
	virtual void cleanUp();	// Should be reemplemented to cleanup resources 
							// after every encoding - mostly calls cleanUpEncData();
	virtual ~Encoder();

private:
	Encoder *chain;
	bool writeSize(SOCKET s);
	bool chainWriteSize(SOCKET s);
	bool writeData(SOCKET s);

protected:
	size_t encSize;
	size_t rawSize;
	unsigned char *encData;
	void cleanUpEncData();  // Does a delete[] on encData;
};

#endif // !defined(AFX_ENCODER_H__687994DC_D8BA_42CD_B0F7_0F15D40DF5D4__INCLUDED_)
