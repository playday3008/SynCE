#ifndef SNAP_H
#define SNAP_H

#include <Winsock.h>

class Snap 
{
public:
	struct _snapimage {
		HBITMAP bmp;
		BITMAPINFO pbmi;
		HDC targetDC;
		HGDIOBJ oo;
		unsigned char *usPixels;
		size_t blackSize;
	};

	typedef struct _snapimage SnapImage;

	Snap(WORD cClrBits = 24);
	~Snap();
	void snap(Snap::SnapImage image);
	void writeFile(Snap::SnapImage image, char *path, char *fileName);
	bool writeSocket(SOCKET socket, Snap::SnapImage image);
	bool writeSocketRLE(SOCKET socket, Snap::SnapImage image, bool *written);
	SnapImage createSnapImage();
	void xorBits(Snap::SnapImage image1, Snap::SnapImage image2);
	void exchangeImages(Snap::SnapImage image1, Snap::SnapImage image2);

private:
	HDC screen;
	RECT lprc;
	HDC hDC;
	WORD cClrBits;	
	unsigned char *target;
	size_t rle_encode(unsigned char *target, unsigned char *pixels, size_t size);
};

#endif