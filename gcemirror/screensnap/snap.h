// screensnap.h: interface for the ScreenSnap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCREENSNAP_H__01179A3A_0FEC_45D5_AE32_25E78CAECC6D__INCLUDED_)
#define AFX_SCREENSNAP_H__01179A3A_0FEC_45D5_AE32_25E78CAECC6D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <Winsock.h>
#include "encoder.h"

struct _image {
	HBITMAP bmp;
	BITMAPINFO pbmi;
	HDC targetDC;
	HGDIOBJ oo;
	unsigned char *usPixels;
	size_t blackSize;
};

typedef struct _image Image;

class Snap {
public:
	Snap(SOCKET cs, HANDLE clientEvent);
	bool start();
	virtual ~Snap();

private:
	bool init();
	static Image createImage(HDC hDC, RECT lprc);
	BOOL snap(bool *written);
    bool writeGeometry();
	bool writeBmpHeader();
	bool writeImage(bool *written);
	void waitOnEvent();

	HANDLE clientEvent;
	SOCKET cs;
	HDC hDC;
	RECT lprc;
	Image screenImage;
	Encoder *encoderChain;

friend static DWORD WINAPI snapProc(LPVOID lpParameter);
};

#endif // !defined(AFX_SCREENSNAP_H__01179A3A_0FEC_45D5_AE32_25E78CAECC6D__INCLUDED_)
