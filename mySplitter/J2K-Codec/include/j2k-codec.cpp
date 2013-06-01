
#include "j2k-codec.h"

#ifdef __APPLE__

	#define WORD  unsigned short
	#define DWORD unsigned int
	#define LONG  int

	#define BI_RGB  0

	#pragma pack(2)

	typedef struct tagBITMAPFILEHEADER {
			WORD    bfType;
			DWORD   bfSize;
			WORD    bfReserved1;
			WORD    bfReserved2;
			DWORD   bfOffBits;
	} BITMAPFILEHEADER;

	#pragma pack()

	typedef struct tagBITMAPINFOHEADER{
			DWORD      biSize;
			LONG       biWidth;
			LONG       biHeight;
			WORD       biPlanes;
			WORD       biBitCount;
			DWORD      biCompression;
			DWORD      biSizeImage;
			LONG       biXPelsPerMeter;
			LONG       biYPelsPerMeter;
			DWORD      biClrUsed;
			DWORD      biClrImportant;
	} BITMAPINFOHEADER;

#endif


TCHAR* J2K_GetErrStr(int code)
{
	static TCHAR err_str[J2K_MAX_ERROR_LEN];

	J2K_GetErrorStr(code, err_str);

	return err_str;
}

/*/////////////////////////////////////////////////////////////////////////////////////////
// BMP Saving
/////////////////////////////////////////////////////////////////////////////////////////*/

int SaveAsBMP(char *filename, J2K_Image *img)
{
	int w, w2, h, width = img->Width, height = img->Height, bpp, prec, tmp, color_map_size; FILE *f;

	unsigned char   *row;
	unsigned short *srow;

	BITMAPFILEHEADER bmpHeader; BITMAPINFOHEADER bmpInfoHeader; 

	prec = img->Precision;

	if(prec > 8) bpp = img->buffer_bpp / 2; else bpp = img->buffer_bpp;

	if(bpp == 2) return 0; // not supported

	color_map_size = (bpp == 1 ? 256*4 : 0);

	bmpHeader.bfType    = 'MB';
	bmpHeader.bfSize    = 14 + 40 + color_map_size; //sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	bmpHeader.bfOffBits = bmpHeader.bfSize;
	bmpHeader.bfSize   += width * height * bpp;

	bmpHeader.bfReserved1 = bmpHeader.bfReserved2 = 0;

	bmpInfoHeader.biSize     = 40;//sizeof(BITMAPINFOHEADER); 
	bmpInfoHeader.biWidth    = width;
	bmpInfoHeader.biHeight   = height; 
	bmpInfoHeader.biPlanes   = 1; 
	bmpInfoHeader.biBitCount = (bpp == 1) ? 8 : (bpp==3 ? 24 : 32);
	bmpInfoHeader.biCompression   = BI_RGB;
	bmpInfoHeader.biSizeImage     = 0; 
	bmpInfoHeader.biXPelsPerMeter = 0; 
	bmpInfoHeader.biYPelsPerMeter = 0; 
	bmpInfoHeader.biClrUsed       = (bpp==1 ? 256 : 0); 
	bmpInfoHeader.biClrImportant  = 0; 

	f = fopen(filename, "wb"); if(!f) return 0;

	// fwrite(&bmpHeader, 14, 1, f);
	fwrite(&bmpHeader.bfType, 2, 1, f);
	fwrite(&bmpHeader.bfSize, 4, 1, f);
	fwrite(&bmpHeader.bfReserved1, 2, 1, f);
	fwrite(&bmpHeader.bfReserved2, 2, 1, f);
	fwrite(&bmpHeader.bfOffBits, 4, 1, f);

	// fwrite(&bmpInfoHeader, sizeof(BITMAPINFOHEADER), 1, f);

	fwrite(&bmpInfoHeader.biSize, 4, 1, f);
	fwrite(&bmpInfoHeader.biWidth, 4, 1, f);
	fwrite(&bmpInfoHeader.biHeight, 4, 1, f);
	fwrite(&bmpInfoHeader.biPlanes, 2, 1, f);
	fwrite(&bmpInfoHeader.biBitCount, 2, 1, f);
	fwrite(&bmpInfoHeader.biCompression, 4, 1, f);
	fwrite(&bmpInfoHeader.biSizeImage, 4, 1, f);
	fwrite(&bmpInfoHeader.biXPelsPerMeter, 4, 1, f);
	fwrite(&bmpInfoHeader.biYPelsPerMeter, 4, 1, f);
	fwrite(&bmpInfoHeader.biClrUsed, 4, 1, f);
	fwrite(&bmpInfoHeader.biClrImportant, 4, 1, f);

	 row = img->buffer + width * (height-1) * bpp;
	srow = (unsigned short*)(img->buffer + width * (height-1) * bpp * 2);

	if(bpp==1)
	{
		for(w = 0; w < 256; w++){ tmp = (w<<16) | (w<<8) | w; fwrite(&tmp, 4, 1, f); }

		for(h = 0; h < height; h++)
		{
			if(prec > 8) // 16-bit
			{
				for(w = 0; w < width; w++){ fputc((srow[w] >> (prec-8)), f); } // we need to scale it to 8 bits in BMP
			}
			else fwrite(row, width, 1, f); // 8-bit

			if(width&3) for(w = 0; w < 4-(width&3); w++) fputc(0, f);

			 row -= img->buffer_pitch;
			srow -= img->buffer_pitch / 2;
		}
	}
	else
	{
		w2 = width * bpp;

		if(prec > 8) // 16-bit
		{
			for(h = 0; h < height; h++, srow -= img->buffer_pitch / 2)
			{
				for(w = 0; w < w2; w++) fputc((srow[w] >> (prec-8)), f); // we need to scale it to 8 bits in BMP

				if(w2&3) for(w = 0; w < 4-(w2&3); w++) fputc(0, f);
			}
		}
		else // 8-bit
		{
			for(h = 0; h < height; h++, row -= img->buffer_pitch)
			{
				fwrite(row, w2, 1, f);
				
				if(w2&3) for(w = 0; w < 4-(w2&3); w++) fputc(0, f);
			}
		}
	}

	fclose(f);

	return 1;
}//_____________________________________________________________________________

#ifdef __cplusplus

int SaveAsBMP(char *filename, J2K_Image  &img){ return SaveAsBMP(filename, &img); }
int SaveAsBMP(char *filename, J2K_CImage &img){ return SaveAsBMP(filename, (J2K_Image*)&img); }

#endif
