// screensnap.cpp: implementation of the ScreenSnap class.
//
//////////////////////////////////////////////////////////////////////

#include "snap.h"

#include "huffmanencoder.h"
#include "rleencoder.h"
#include "XorEncoder.h"

#define SIZE_MESSAGE	1
#define XOR_IMAGE		2
#define KEY_IMAGE		3
#define BMP_HEADER		4


static DWORD WINAPI snapProc(LPVOID lpParameter)
{
	Snap *snap = (Snap *) lpParameter;
    bool written = false;
	
	if (!snap->writeGeometry()) {
		return 0;
	}

	if (!snap->writeBmpHeader()) {
		return 0;
	}

	while (snap->snap(&written)) {
		if (written) {
			Sleep(50);
		} else {
			snap->waitOnEvent();
		}
	}
	return 0;
}


Snap::Snap(SOCKET cs, HANDLE clientEvent)
{
	this->cs = cs;
	this->clientEvent = clientEvent;
	hDC = NULL;
	
	this->encoderChain = NULL;
	this->encoderChain = new XorEncoder(this->encoderChain);
	this->encoderChain = new RleEncoder(this->encoderChain);
	this->encoderChain = new HuffmanEncoder(this->encoderChain);
}


bool Snap::init()
{	
	hDC = GetDC(NULL);
	if (!hDC) {
		MessageBox(NULL, L"Could not getDC", L"Snap", MB_OK);
		return false;
	}

	if (GetClipBox(hDC, &lprc) == ERROR) {
		MessageBox(NULL, L"Could not get clip box", L"Snap", MB_OK);
		return false;
	}

	screenImage = Snap::createImage(hDC, lprc);
	size_t imgSize = ((lprc.right * 24 + 31) & ~31) / 8 * lprc.bottom;

	return true;
}


bool Snap::start()
{
	HANDLE threadHandle;
	DWORD threadId;

	if (init()) {

		threadHandle = CreateThread(NULL, 0, snapProc, this, 0, &threadId);
		if (threadHandle == NULL) {
			MessageBox(NULL, L"Could not create Thread", L"Snap", 
					MB_OK | MB_ICONERROR);
			return false;
		}
	}
	
	return true;
}


void Snap::waitOnEvent()
{
	WaitForSingleObject(clientEvent, 50);
}


Image Snap::createImage(HDC hDC, RECT lprc)
{
	Image image;

    image.pbmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	image.pbmi.bmiHeader.biWidth = lprc.right;
	image.pbmi.bmiHeader.biHeight = lprc.bottom;
    image.pbmi.bmiHeader.biPlanes = 1;
	image.pbmi.bmiHeader.biBitCount = 24;
    image.pbmi.bmiHeader.biCompression = BI_RGB;
	image.pbmi.bmiHeader.biSizeImage = 
			((image.pbmi.bmiHeader.biWidth * 24 + 31) & ~31) / 8 
			* image.pbmi.bmiHeader.biHeight;
	image.pbmi.bmiHeader.biXPelsPerMeter = 3780;
    image.pbmi.bmiHeader.biYPelsPerMeter = 3780;
    image.pbmi.bmiHeader.biClrUsed = 0;
    image.pbmi.bmiHeader.biClrImportant = 0;
	image.pbmi.bmiColors[0].rgbBlue = 0;
	image.pbmi.bmiColors[0].rgbGreen = 0;
	image.pbmi.bmiColors[0].rgbRed = 0;
	image.pbmi.bmiColors[0].rgbReserved = 0;

	image.blackSize = 
			(size_t) ceil(image.pbmi.bmiHeader.biSizeImage / 771.0) * 7;

	image.bmp = CreateDIBSection(hDC, &image.pbmi, DIB_RGB_COLORS, 
			(void **) &image.usPixels, NULL, 0);

	if (image.bmp == NULL) {
		MessageBox(NULL, L"Could not create bitmap", L"Snap", MB_OK);
	}

	image.targetDC = CreateCompatibleDC(NULL);

	image.oo = SelectObject(image.targetDC, image.bmp);

	return image;
}


BOOL Snap::snap(bool *written)
{
    BitBlt(screenImage.targetDC, 0, 0, lprc.right, lprc.bottom, 
			hDC, 0, 0, SRCCOPY);
	return writeImage(written);
}


bool Snap::writeGeometry()
{
	u_long x = htonl(lprc.right);
	u_long y = htonl(lprc.bottom);
	unsigned char packageType = SIZE_MESSAGE;

	if (send(cs, (const char *) &packageType, sizeof(unsigned char), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(cs, (const char *) &x, sizeof(u_long), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(cs, (const char *) &y, sizeof(u_long), 0) == SOCKET_ERROR) {
		return false;
	}

	return true;
}


bool Snap::writeBmpHeader()
{
	BITMAPFILEHEADER bmfHeader;

    bmfHeader.bfType = 0x4d42;
    bmfHeader.bfSize = sizeof(bmfHeader) + sizeof(screenImage.pbmi.bmiHeader) + 
		screenImage.pbmi.bmiHeader.biSizeImage;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(bmfHeader) + sizeof(screenImage.pbmi.bmiHeader);

	u_long headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	u_long bmpSize = headerSize + screenImage.pbmi.bmiHeader.biSizeImage;
	u_long headerSizeN = htonl(headerSize);
	u_long bmpSizeN = htonl(bmpSize);
	u_long imgSizeN = htonl(screenImage.pbmi.bmiHeader.biSizeImage);

	unsigned char packageType = BMP_HEADER;

	if (send(cs, (const char *) &packageType, sizeof(unsigned char), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(cs, (const char *) &bmpSizeN, sizeof(u_long), 0) == SOCKET_ERROR) {
		return false;
	}
	
	if (send(cs, (const char *) &headerSizeN, sizeof(u_long), 0) == SOCKET_ERROR) {
		return false;
	}	
	
	if (send(cs, (const char *) &bmfHeader, sizeof(BITMAPFILEHEADER), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(cs, (const char *) &screenImage.pbmi.bmiHeader, sizeof(BITMAPINFOHEADER), 0) == SOCKET_ERROR) {
		return false;
	}

	return true;
}


bool Snap::writeImage(bool *written)
{
	bool ret = true;

	encoderChain->chainEncode(NULL, screenImage.usPixels, 
			screenImage.pbmi.bmiHeader.biSizeImage);

	if (!encoderChain->chainNotChanged()) {
		unsigned char packageType = XOR_IMAGE;
		if ((ret = (send(cs, (const char *) &packageType, sizeof(unsigned char), 0) 
				!= SOCKET_ERROR))) {
			ret = encoderChain->chainWrite(cs);
		}
	}

	encoderChain->chainCleanUp();
		
	return ret;
}


Snap::~Snap()
{
	if (hDC) {
		ReleaseDC(NULL, hDC);
	}
}
