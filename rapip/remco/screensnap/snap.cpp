#include "snap.h"
#include <math.h>
#include "huffman.h"

#define SIZE_MESSAGE	1
#define XOR_IMAGE		2
#define KEY_IMAGE		3

Snap::Snap(WORD cClrBits)
{
	this->cClrBits = cClrBits;

	hDC = GetDC(NULL);
	if (!hDC) {
		MessageBox(NULL, L"Could not getDC", L"Snap", MB_OK);
	}

	if (GetClipBox(hDC, &lprc) == ERROR) {
		MessageBox(NULL, L"Could not get clip box", L"Snap", MB_OK);
	}

	target = NULL;
}


Snap::~Snap()
{
	ReleaseDC(NULL, hDC);
}


Snap::SnapImage Snap::createSnapImage()
{
	Snap::SnapImage newImage;

    newImage.pbmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	newImage.pbmi.bmiHeader.biWidth = lprc.right;
	newImage.pbmi.bmiHeader.biHeight = lprc.bottom;
    newImage.pbmi.bmiHeader.biPlanes = 1;
	newImage.pbmi.bmiHeader.biBitCount = cClrBits;
    newImage.pbmi.bmiHeader.biCompression = BI_RGB;
	newImage.pbmi.bmiHeader.biSizeImage = 
			((newImage.pbmi.bmiHeader.biWidth * cClrBits + 31) & ~31) / 8 
			* newImage.pbmi.bmiHeader.biHeight;
	newImage.pbmi.bmiHeader.biXPelsPerMeter = 3780;
    newImage.pbmi.bmiHeader.biYPelsPerMeter = 3780;
    newImage.pbmi.bmiHeader.biClrUsed = 0;
    newImage.pbmi.bmiHeader.biClrImportant = 0;
	newImage.pbmi.bmiColors[0].rgbBlue = 0;
	newImage.pbmi.bmiColors[0].rgbGreen = 0;
	newImage.pbmi.bmiColors[0].rgbRed = 0;
	newImage.pbmi.bmiColors[0].rgbReserved = 0;

	newImage.blackSize = (size_t) ceil(newImage.pbmi.bmiHeader.biSizeImage / 771.0) * 7;

	newImage.bmp = CreateDIBSection(hDC, &newImage.pbmi, DIB_RGB_COLORS, 
			(void **) &newImage.usPixels, NULL, 0);

	if (newImage.bmp == NULL) {
		MessageBox(NULL, L"Could not create bitmap", L"Snap", MB_OK);
	}

	newImage.targetDC = CreateCompatibleDC(NULL);

	newImage.oo = SelectObject(newImage.targetDC, newImage.bmp);

	if (target == NULL) {
		target = new unsigned char[newImage.pbmi.bmiHeader.biSizeImage * 7 / 6];
	}

	return newImage;
}


BOOL Snap::snap(Snap::SnapImage image)
{
	return BitBlt(image.targetDC, 0, 0, lprc.right, lprc.bottom, hDC, 0, 0, SRCCOPY);
}


BOOL Snap::xorBits(Snap::SnapImage image1, Snap::SnapImage image2)
{
	return BitBlt(image1.targetDC, 0, 0, lprc.right, lprc.bottom, image2.targetDC, 0, 0, SRCINVERT);
}


BOOL Snap::exchangeImages(Snap::SnapImage image1, Snap::SnapImage image2)
{
	return BitBlt(image1.targetDC, 0, 0, lprc.right, lprc.bottom, image2.targetDC, 0, 0, SRCCOPY);
/*
	unsigned char *usPixels;

	HBITMAP tmp;
	SelectObject(image1.targetDC, image1.oo);
	SelectObject(image2.targetDC, image2.oo);
	tmp = image2.bmp;
	image2.bmp = image1.bmp;
	image1.bmp = tmp;
	usPixels = image2.usPixels;
	image2.usPixels = image1.usPixels;
	image1.usPixels = usPixels;
	SelectObject(image1.targetDC, image1.bmp);
	SelectObject(image2.targetDC, image2.bmp);
*/
}


void Snap::writeFile(Snap::SnapImage image, char *path, char *fileName)
{
    FILE *fBitmap;
	BITMAPFILEHEADER bmfHeader;
	
    bmfHeader.bfType = 0x4d42;
    bmfHeader.bfSize = sizeof(bmfHeader) + sizeof(image.pbmi.bmiHeader) + 
		image.pbmi.bmiHeader.biSizeImage;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(bmfHeader) + sizeof(image.pbmi.bmiHeader);

	char file[256];
	strcpy(file, path);
	strcat(file, "/");
	strcat(file, fileName);
  
	if ( fBitmap = fopen( file, "wb" ) ) {
		fwrite( (void*) &bmfHeader, sizeof(BITMAPFILEHEADER), 1, fBitmap);
		fwrite( (void*) &image.pbmi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, fBitmap);
		fwrite (image.usPixels, image.pbmi.bmiHeader.biSizeImage, 1, fBitmap);
		fclose( fBitmap );
	}
}


bool Snap::writeSocket(SOCKET socket, Snap::SnapImage image)
{
	BITMAPFILEHEADER bmfHeader;

    bmfHeader.bfType = 0x4d42;
    bmfHeader.bfSize = sizeof(bmfHeader) + sizeof(image.pbmi.bmiHeader) + 
		image.pbmi.bmiHeader.biSizeImage;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(bmfHeader) + sizeof(image.pbmi.bmiHeader);

	u_long size = htonl(sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
			image.pbmi.bmiHeader.biSizeImage);

	if (send(socket, (const char *) &size, sizeof(u_long), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(socket, (const char *) &bmfHeader, sizeof(BITMAPFILEHEADER), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(socket, (const char *) &image.pbmi.bmiHeader, sizeof(BITMAPINFOHEADER), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(socket, (const char *) image.usPixels, image.pbmi.bmiHeader.biSizeImage, 0) == SOCKET_ERROR) {
		return false;
	}

	return true;
}


size_t Snap::rle_encode(unsigned char *target, unsigned char *pixels, size_t size)
{
	unsigned char *act1 = pixels;
	unsigned char *act2 = pixels + 1;
	unsigned char *act3 = pixels + 2;

	unsigned char val1 = *act1;
	unsigned char val2 = *act2;
	unsigned char val3 = *act3;

	unsigned char *tmp_target = target;

	size_t count = 0;
	size_t samcount = 0;

	while (count < size) {
		if ((*act1 == val1) && (*act2 == val2) && (*act3 == val3)) {
			samcount++;
			*tmp_target++ = val1;
			*tmp_target++ = val2;
			*tmp_target++ = val3;
			act1 += 3;
			act2 += 3;
			act3 += 3;
			count += 3;
			if (samcount == 2) {
				samcount = 0;
				unsigned char samruncount = 0;
				while (count < size && samruncount < 255) {
					if ((*act1 == val1) && (*act2 == val2) && (*act3 == val3)) {
						samruncount++;
						act1 += 3;
						act2 += 3;
						act3 += 3;
						count += 3;
					} else {
                        break;
					}
				}
				if (count < size) {
					val1 = *act1;
					val2 = *act2;
					val3 = *act3;
				}
				*tmp_target++ = samruncount;
			}
		} else {
			samcount = 0;
			*tmp_target++ = val1 = *act1;
			*tmp_target++ = val2 = *act2;
			*tmp_target++ = val3 = *act3;
			act1 += 3;
			act2 += 3;
			act3 += 3;
			count += 3;
		}
	}

	return tmp_target - target;
}


bool Snap::writeGeometry(SOCKET socket)
{
	u_long x = htonl(lprc.right);
	u_long y = htonl(lprc.bottom);
	unsigned char packageType = SIZE_MESSAGE;

	if (send(socket, (const char *) &packageType, sizeof(unsigned char), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(socket, (const char *) &x, sizeof(u_long), 0) == SOCKET_ERROR) {
		return false;
	}

	if (send(socket, (const char *) &y, sizeof(u_long), 0) == SOCKET_ERROR) {
		return false;
	}

	return true;
}


bool Snap::writeSocketRLE(SOCKET socket, Snap::SnapImage image, bool *written)
{
	BITMAPFILEHEADER bmfHeader;

    bmfHeader.bfType = 0x4d42;
    bmfHeader.bfSize = sizeof(bmfHeader) + sizeof(image.pbmi.bmiHeader) + 
		image.pbmi.bmiHeader.biSizeImage;
    bmfHeader.bfReserved1 = 0;
    bmfHeader.bfReserved2 = 0;
    bmfHeader.bfOffBits = sizeof(bmfHeader) + sizeof(image.pbmi.bmiHeader);

	u_long headerSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	u_long headerSizeN = htonl(headerSize);
	size_t rleSize = rle_encode(target, image.usPixels, image.pbmi.bmiHeader.biSizeImage);
	unsigned char *buf;
	size_t buflen;

	if (rleSize != image.blackSize || target[0] != 0) {

		u_long rleSizeN = htonl((u_long) rleSize);
		u_long bmpSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) +
				image.pbmi.bmiHeader.biSizeImage;
		u_long bmpSizeN = htonl(bmpSize);

		huffman_encode_memory(target, rleSize, &buf, &buflen);
		rleSizeN = htonl((u_long) buflen);
		unsigned char packageType = XOR_IMAGE;

		if (send(socket, (const char *) &packageType, sizeof(unsigned char), 0) == SOCKET_ERROR) {
			return false;
		}

		if (send(socket, (const char *) &headerSizeN, sizeof(u_long), 0) == SOCKET_ERROR) {
			return false;
		}	
	

		if (send(socket, (const char *) &rleSizeN, sizeof(u_long), 0) == SOCKET_ERROR) {
			return false;
		}


		if (send(socket, (const char *) &bmpSizeN, sizeof(u_long), 0) == SOCKET_ERROR) {
			return false;
		}

		if (send(socket, (const char *) &bmfHeader, sizeof(BITMAPFILEHEADER), 0) == SOCKET_ERROR) {
			return false;
		}

		if (send(socket, (const char *) &image.pbmi.bmiHeader, sizeof(BITMAPINFOHEADER), 0) == SOCKET_ERROR) {
			return false;
		}

/*
		if (send(socket, (const char *) target, rleSize, 0) == SOCKET_ERROR) {
			return false;
		}
*/
		if (send(socket, (const char *) buf, buflen, 0) == SOCKET_ERROR) {
			free(buf);
			return false;
		}

		free(buf);
		*written = true;
	} else {
		*written = false;
	}

	return true;
}
