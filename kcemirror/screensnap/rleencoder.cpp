// rleencoder.cpp: implementation of the RleEncoder class.
//
//////////////////////////////////////////////////////////////////////

#include "rleencoder.h"


RleEncoder::RleEncoder() : Encoder()
{
	oldSize = 0;
}


RleEncoder::RleEncoder(Encoder *chain) : Encoder(chain)
{
	oldSize = 0;
}


size_t RleEncoder::encode(unsigned char **encData, unsigned char *rawData, size_t size)
{
	cleanUp();

	if (oldSize != size) {
		if (this->encData != NULL) {
			delete[] encData;
		}
		this->encData = new unsigned char[size * 7 / 6];
		oldSize = size;
	}
	*encData = this->encData;

	unsigned char *act1 = rawData;
	unsigned char *act2 = rawData + 1;
	unsigned char *act3 = rawData + 2;

	unsigned char val1 = *act1;
	unsigned char val2 = *act2;
	unsigned char val3 = *act3;

	unsigned char *tmp_target = this->encData;

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

	return tmp_target - this->encData;
}


RleEncoder::~RleEncoder()
{
	if (this->encData != NULL) {
		delete[] encData;
	}
}
