#include <stdio.h>
#include <Windows.h>
#include "stereo_test.h"
#include <assert.h>

#pragma comment(lib, "winmm.lib")

HRESULT save_bitmap(DWORD *data, const wchar_t *filename, int width, int height); 
int main();

HRESULT save_bitmap(DWORD *data, const wchar_t *filename, int width, int height) 
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
		+ 31) & ~31) >> 3) * BMIH.biHeight;

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

DWORD *resource = new DWORD[3840*1080];
int get_pixel(int x, int y, bool left)
{
	if (x<0 || x>=1920 || y<0 || y>=1080)
		return 255;

	DWORD *d = resource + y*3840 + x + (left?0:1920);

	//BYTE r = (*d)&0xff;
	BYTE g = ((*d)&0xff00) >> 8;
	//BYTE b = ((*d)&0xff0000) >> 24;
	return g;//(r*0.299 + g*0.587 + b*0.114);
}

double clamp(double in)
{
	if (in<0)
		return 0;
	if (in>255)
		return 255;
	return in;
}

inline int d_abs(int in)
{
	return in>0?in:-in;
}

int get_delta(int x, int y, int delta)
{
	int t2 = get_pixel(x, y, false);
	int t = get_pixel(x+delta, y, true);

	return clamp(abs(t-t2));
}

double get_delta2(int x, int y, int delta, int range)
{
	double t = 0;
	for(int xx= -range; xx<=range; xx++)
		t+= get_delta(x+xx, y, delta);

	return t;
}


typedef struct struct_thread_parameter
{
	int left;
	int right;
	DWORD *out;
} thread_parameter;

DWORD WINAPI core_thread(LPVOID parameter)
{
	thread_parameter *para = (thread_parameter*)parameter;
	DWORD *out = para->out;
	const int range = 160;

	for(int y=para->left; y<para->right; y++)
	{
		printf("\r%d/1080", y-para->left);
		for(int x=0; x<1920; x++)
		{
			double min_delta = 99999999;
			for(int i=-range; i<=range; i++)
			{
				double t = get_delta2(x, y, i, 16);
				//if (x == 540) printf("%.1f, ", (float)t);
				if (t<min_delta)
				{
					min_delta = t;
					int c = (i+range)*255/(2*range);
					out[(1080-1-y)*1920+x] = RGB(c, c, c);
				}
			}
			//if (x==540)printf("%d\n", y);
		}
	}

	return 0;
}

int count(DWORD *source, int x, int y)
{
	typedef struct struct_point
	{
		int x;
		int y;
		int color;
	} point;

	const int max_point = 4096;
	const int middle_value = 20;

	point points[max_point];
	int point_count = 0;

	return point_count;
}

int CV()
{
	RGBQUAD res[189*161];
	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\CV.bmp", IMAGE_BITMAP, 189, 161, LR_LOADFROMFILE);
	GetBitmapBits(bm, 189*161*4, res);
	DeleteObject(bm);

	int tx = 0;
	int ty = 0;
	int n = 0;

	for(int i=0; i<189*161; i++)
	{
		RGBQUAD &q = res[i];


		int t = (q.rgbRed + q.rgbGreen + q.rgbBlue)/3;
		int x = i%189;
		int y = i/189;

		if (q.rgbBlue == 255 && q.rgbGreen == 255 && q.rgbRed == 255)
		{

		}
		else if (q.rgbBlue == 0 && q.rgbGreen == 0 && q.rgbRed == 0)
		{

		}
		else
		{

			if (t>128)
				t = 255;
			else
				t = 0;
		}


		if (t == 255)
		{
			tx += x;
			ty += y;
			n ++;
		}

	}

	tx = tx / n;
	ty = ty / n;

	RGBQUAD &q = res[ty*189+tx];
	q.rgbRed = 255;
	q.rgbGreen = 0;
	q.rgbBlue = 0;

	save_bitmap((DWORD*)res, L"Z:\\CVo.bmp", 189, 161);

	return 0;
}

int PC3DV()
{
	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\3DVRCTEST.bmp", IMAGE_BITMAP, 800, 480, LR_LOADFROMFILE);
	GetBitmapBits(bm, 800*480*4, resource);
	//DeleteObject(bm);

	HDC hdc1 = CreateCompatibleDC(NULL);
	HDC hdc2 = CreateCompatibleDC(NULL);

	SelectObject(hdc1, bm);
	

	RGBQUAD *data = NULL;
	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = 800;
	bmi.bmiHeader.biHeight = 480;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	HBITMAP bm2 = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, (void**)&data, NULL, NULL);

	SelectObject(hdc2, bm2);

	StretchBlt(hdc2, 0, 0, 800, 480, hdc1, 400, 0, 400, 480, SRCCOPY);

	for(int y=0; y<480; y++)
		for(int x=0; x<800; x+=2)
		{
			assert(memcmp(&data[y*800+x], &data[y*800+x+1], sizeof(RGBQUAD)) == 0);
		}
	save_bitmap((DWORD*)data, L"Z:\\800.bmp", 800, 480);
	return 0;
}

int main()
{
	//return PC3DV();
	//return CV();
	const int range = 40;

	timeBeginPeriod(1);

	HANDLE h_parent_process = GetCurrentProcess();
	SetPriorityClass(h_parent_process, IDLE_PRIORITY_CLASS);			//host idle priority

	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\test.bmp", IMAGE_BITMAP, 3840, 1080, LR_LOADFROMFILE);
	GetBitmapBits(bm, 3840*1080*4, resource);
	DeleteObject(bm);

	//FILE * f = fopen("E:\\JM18\\bin\\test_dec.yuv", "rb");
	//fread(resource, 1, 1920*1080*1.5, f);
	//fclose(f);



	int tick = timeGetTime();

	int layout = 0;
	for(int j=0; j<1; j++)
	{
		get_layout<DWORD>(resource, 3840, 1080, &layout);
	}

	if (layout == side_by_side)
		printf("sbs.");
	else if (layout == top_bottom)
		printf("tb.");
	else
		printf("unkown");

	printf("test time:%dms.\n", timeGetTime()-tick);


	DWORD *out[range*2+1] = {new DWORD[1920*1080]};
	/*
	for(int i=-range; i<=range; i++)
	{
		DWORD*& o = out[i+range];
		o = new DWORD[1920*1080];
		wchar_t file[1024];
		swprintf(file, L"Z:\\save%02d.bmp", i+range);

		printf("\r%d/%d      ", i, range);
		for(int y=0; y<1080; y++)
		{
			for(int x=0; x<1920; x++)
			{
				int t = (int)get_delta(x, y, i);
				o[(1080-1-y)*1920+x] = RGB(t,t,t);		//t<20?RGB(255-t,0,0):
			}
		}

		save_bitmap(o, file, 1920, 1080);
	}
	*/

	/*
	for(int y=0; y<1080; y++)
	{
		printf("\r%d/1080", y);
		for(int x=0; x<1920; x++)
		{
			double min_delta = 255;
			for(int i=-range; i<=range; i++)
			{
				double t = get_delta2(x, y, i, 4);
				if (x==962 && y==540)
					printf("%02f\n", t);
				if (t<min_delta)
				{
					min_delta = t;
					int c = (i+range)*255/(2*range);
					out[(1080-1-y)*1920+x] = RGB(c, c, c);
				}
			}
		}
	}
	*/

	const int thread_count = 4;
	HANDLE handles[thread_count];
	thread_parameter parameters[thread_count];
	for(int i=0; i<thread_count; i++)
	{
		thread_parameter &para = parameters[i];
		para.left = i*270;
		para.right = (i+1)*270;
		para.out = out[0];

		handles[i] = CreateThread(NULL, NULL, core_thread, &para, NULL, NULL);
	}

	for(int i=0; i<thread_count; i++)
	{
		WaitForSingleObject(handles[i], INFINITE);
	}

	save_bitmap(out[0], L"Z:\\zbuffer.bmp", 1920, 1080);

	return 0;
}