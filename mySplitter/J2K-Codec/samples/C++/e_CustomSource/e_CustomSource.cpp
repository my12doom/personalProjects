
// This sample shows you how to implement a custom data source.

// For simplicity, we will just make a wrapper for standard file operations.

#include "..\common.h"
//_____________________________________________________________________________

// Define callback functions

int  __stdcall MySeek(void *data_source, int offset)
{
	return -1; // Our source won't be seekable

	// For seekable source you would write:
	return (fseek((FILE*)data_source, offset, SEEK_SET)==0 ? 0 : -1);
}

int  __stdcall MyRead(void *ptr, int size, void *data_source)
{
	return fread(ptr, 1, size, (FILE*)data_source);
}

void __stdcall MyClose(void *data_source)
{
	if(data_source) fclose((FILE*)data_source);
}
//_____________________________________________________________________________

void main(int ArgC, char **ArgV)
{
	if(ArgC<2){ printf("\n Please, specify the J2K image file name!\n\n"); system("pause"); return; }

	J2K_Unlock("Your registration key");

	// Prepare callbacks structure

	J2K_Callbacks cbs; 

	cbs.seek  = MySeek;
	cbs.read  = MyRead;
	cbs.close = MyClose;

	// Open J2K image file

	FILE *f = fopen(ArgV[1], "rb"); if(!f){ printf("\n File '%s' opening error.\n", ArgV[1]); return; }

	// Open custom data source

	J2K_CImage img;

	if(img.open(f, &cbs) != J2K_SUCCESS){ printf("\n  Custom source opening error: %s.\n", img.errorStr); return; }

	// Decode the image

	if(img.decode() != J2K_SUCCESS){ printf("\n Decoding error: %s.\n", img.errorStr); return; }

	printf("\n Image info: %ux%u, %u component(s).\n", img.Width, img.Height, img.Components);

	// Finally save the decoded image as BMP file

	SaveAsBMP("decoded.bmp", img);

	printf("\n Saved as \"decoded.bmp\".\n\n");  system("pause");
}
