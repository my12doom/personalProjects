#include <Windows.h>
#include <stdio.h>
#include "remote_thumbnail.h"

int width = 800;
int height = 480;
HRESULT save_bitmap(void *data, const wchar_t *filename, int width, int height);

int main()
{
	wchar_t worker_path[MAX_PATH];
	GetModuleFileNameW(NULL, worker_path, MAX_PATH);
	wcscpy(wcsrchr(worker_path, L'\\'), L"\\Thumbnail.exe");
	SHELLEXECUTEINFO ShExecInfo = {0};
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
	ShExecInfo.hwnd = NULL;
	ShExecInfo.lpFile = worker_path;
	ShExecInfo.lpParameters = NULL;	
	ShExecInfo.lpDirectory = NULL;
	ShExecInfo.nShow = SW_SHOW;
	ShExecInfo.hInstApp = NULL;	
	ShExecInfo.lpVerb = L"open";

	ShellExecuteEx(&ShExecInfo);
	HANDLE process = ShExecInfo.hProcess;

	Sleep(500);


	remote_thumbnail r;
	r.set_out_format(30, 400, 225);
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

	TerminateProcess(process, 0);

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
