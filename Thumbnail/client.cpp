#include <Windows.h>
#include <stdio.h>
#include "remote_thumbnail.h"

int width = 800;
int height = 480;
HRESULT save_bitmap(void *data, const wchar_t *filename, int width, int height);

int main()
{
	remote_thumbnail r(width, height);
	r.connect(L"\\\\.\\pipe\\DWindowThumbnailPipe");
	r.open_file(L"E:\\video\\video\\8bit.mkv");
	r.seek(90000);
	for(int i=0; i<100; i++)
	{
		wchar_t tmp[MAX_PATH];
		swprintf(tmp, L"E:\\video\\shots\\%d.bmp", i);
		if (r.get_one_frame() != 0)
			break;
		save_bitmap(r.recieved_data, tmp, 800, -480);
	}
	r.close();
	r.disconnect();

	return 0;
}










HRESULT save_bitmap(void *data, const wchar_t *filename, int width, int height) 
{
	FILE *pFile = _wfopen(filename, L"wb");
	if(pFile == NULL)
		return E_FAIL;

	BITMAPINFOHEADER BMIH;
	memset(&BMIH, 0, sizeof(BMIH));
	BMIH.biSize = sizeof(BITMAPINFOHEADER);
	BMIH.biBitCount = 32;
	BMIH.biPlanes = 1;
	BMIH.biCompression = BI_RGB;
	BMIH.biWidth = width;
	BMIH.biHeight = height;
	BMIH.biSizeImage = ((((BMIH.biWidth * BMIH.biBitCount) 
		+ 31) & ~31) >> 3) * abs(BMIH.biHeight);

	BITMAPFILEHEADER bmfh;
	int nBitsOffset = sizeof(BITMAPFILEHEADER) + BMIH.biSize; 
	LONG lImageSize = BMIH.biSizeImage;
	LONG lFileSize = nBitsOffset + lImageSize;
	bmfh.bfType = 'B'+('M'<<8);
	bmfh.bfOffBits = nBitsOffset;
	bmfh.bfSize = lFileSize;
	bmfh.bfReserved1 = bmfh.bfReserved2 = 0;
	//Write the bitmap file header

	UINT nWrittenFileHeaderSize = fwrite(&bmfh, 1, 
		sizeof(BITMAPFILEHEADER), pFile);
	//And then the bitmap info header

	UINT nWrittenInfoHeaderSize = fwrite(&BMIH, 
		1, sizeof(BITMAPINFOHEADER), pFile);
	//Finally, write the image data itself 

	//-- the data represents our drawing

	UINT nWrittenDIBDataSize = 
		fwrite(data, 1, lImageSize, pFile);

	fclose(pFile);

	return S_OK;
}
