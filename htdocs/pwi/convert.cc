// falta que busque varios parrafos

#include <iostream>
#include <fstream>
#include <cstring>
using namespace std;

int findParagraph(FILE *pf)
{
	
	unsigned char c[4];
	unsigned char out = 0xff;
	while(!feof(pf))
	{	
		fread(c,sizeof(char), 1, pf);
		if(c[0] == out) 
		{
			fread(c,sizeof(char), 1, pf);
			if(c[0] == out) return 0;
		}
	}
	return feof(pf);
}


int findStartTag(FILE *pf)
{
	unsigned char FORMATTING_TAG = 0xE0;
	unsigned char tag[2];
	do 
	{	
		fread(tag, sizeof(char), 1, pf);
//		printf("%X  == %X\n", c[0], (c[0] & 0xF0));
	} while(!((tag[0] & 0xF0) == FORMATTING_TAG) && !feof(pf));
   	if(tag[0] == 0xE5 || tag[0] == 0xE6 || tag[0] == 0xE7 || tag[0] == 0xE8)
	{	
		fread(&tag[1], sizeof(char), 2, pf);
	} else {
		fread(&tag[1], sizeof(char), 1, pf);		
	}	
}

int findNextTag(FILE *pf)
{
	unsigned char FORMATTING_TAG = 0xE0;
	unsigned char tag[2];
	int avc;
	do 
	{	
		fread(tag, sizeof(char), 1, pf);

     	if(tag[0] == 0xE5 || tag[0] == 0xE6 || tag[0] == 0xE7 || tag[0] == 0xE8)
		{	
			fread(&tag[1], sizeof(char), 2, pf);
			avc = 3;
		} else {
			fread(&tag[1], sizeof(char), 1, pf);
			avc = 2;
		}	
//		printf("--> %X - %X - %X \n", tag[0], tag[1], tag[2]);
	} while(((tag[0] & 0xF0) == FORMATTING_TAG) && !feof(pf));
	
    fseek(pf, ftell(pf) - avc , SEEK_SET);
}

int findStartText(FILE *pf)
{
	unsigned char c[1];
	char tag[4];
	unsigned char FORMATTING_TAG = 0xE0;
	char car[2];
	string texto;
	// Buscamos el comienzo de las marcas de formateo.
	findStartTag(pf);
	findNextTag(pf);

	unsigned char TEXT_TAG_END = 0xC4;
	for(;;) {
		fread(car, sizeof(char), 1, pf);
//		cout << ".";
//		printf("%X", car[0]);
		if((unsigned char)car[0] == TEXT_TAG_END) {
			fread(car, sizeof(char), 1, pf);
			if((unsigned char)car[0] == 0x00 ) {
	//			cout << "out";
				break;
			}
		}
		car[1] = '\0';
		texto += car;
	} 
	
	cout << endl << "-" << texto << "-" << endl;
	return 1;
}

int main(int argc, char *argv[])
{
	string file = argv[1];
	cout << "Abriendo " << file << endl;

	FILE *pf;
	if(!(pf = fopen(file.c_str(), "rb")))	{
		return 1;
	}


	unsigned char data[80];
	fread(data, sizeof(char), 52, pf);	// Preambulo
	do {
            fread(data, sizeof(char), 80, pf);                          
    } while (!(data[76] == 5 && data[77] == 0 
                            && data[78] == 1 && data[79] == 0)); 
	while(!findParagraph(pf))
	{
		findStartText(pf);
	}
	return 0;

}
