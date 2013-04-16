// picture_sort.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
//#include "Fourier.h"
#include "dct.h"
#include <Windows.h>
#include "..\png2raw\include\il\il.h"
#include "..\png2raw\include\il\ilu.h"
#include <streams.h>
#include <assert.h>
#include <math.h>
#include <locale.h>

#pragma comment(lib, "../png2raw/lib/DevIL.lib")
#pragma comment(lib, "../png2raw/lib/ILU.lib")

double tmp1[2][512][512];
double tmp2[2][512][512];
double tmp3[2][512][512];

double tmp4[2][512][512];
double tmp5[2][512][512];

CRITICAL_SECTION cs;

typedef struct  
{
	wchar_t filename[MAX_PATH];
	double sig[4];
} IMAGE_ENTRY;

HRESULT save_bitmap(RGBQUAD *data, const wchar_t *filename, int width, int height);
int compare_entry(const void *v1, const void*v2);
int compare_entry2(const void *v1, const void*v2);
DWORD sig(const wchar_t *filename, double *o);
template<typename pixel_type>
void resize(const pixel_type *src, pixel_type *dst, int src_width, int src_height, int dst_width, int dst_height, int src_stride=-1, int dst_stride=-1)
{
	int channelcount = sizeof(pixel_type);
	if (src_stride<0)
		src_stride = src_width * sizeof(pixel_type);
	if (dst_stride<0)
		dst_stride = dst_width * sizeof(pixel_type);

#define resize_access(x,y) ((pixel_type*)((BYTE*)src) + y*src_stride)[x]

	for(int i=0; i<channelcount; i++)
	{
		for(int y=0; y<dst_height; y++)
			for(int x=0; x<dst_width; x++)
			{
				float xx = (float)x * src_width / dst_width;
				float yy = (float)y * src_height / dst_height;

				int l = (int)xx;
				int r = l+1;
				float fx = xx - l;
				int t = (int)yy;
				int b = t+1;
				float fy = yy - t;
				l = max(0,l);
				r = min(r,src_width-1);
				t = max(0,t);
				t = min(t,src_height-1);
				b = max(0,b);
				b = min(b,src_height-1);


				BYTE*p = ((pixel_type*)((BYTE*)dst) + y*dst_stride)+x;
				float xt = resize_access(r,t)*fx + resize_access(l,t)*(1-fx);
				float xb = resize_access(r,b)*fx + resize_access(l,b)*(1-fx);
				float xc = xt*(1-fy) + xb * fy;
				xc = max(0, xc);
				xc = min(255, xc);

				*p = (BYTE)(xc+0.5f);
			}

			src = (pixel_type*)(((BYTE*)src)+1);
			dst = (pixel_type*)(((BYTE*)dst)+1);
	}
}

void fft_double(unsigned int p_nSamples, bool p_bInverseTransform, double *p_lpRealIn, double *p_lpImagIn, double *p_lpRealOut, double *p_lpImagOut)
{
	if (p_bInverseTransform)
		idct(p_lpRealIn, p_lpRealOut, p_nSamples);
	else
		dct(p_lpRealIn, p_lpRealOut, p_nSamples);
}

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LOCALE_ALL, "chs");
	InitializeCriticalSection(&cs);

	double test[8] = {0.34, 0.33, 0.45, 0.56, 
					 0.78, 0.99, 1.23, 55.0};

	double p1[8], p2[8];

	dct_init(8);
	dct(test, p1, 8);
	idct(p1, p2, 8);

	dct_init(512);

	RGBQUAD *data = new RGBQUAD[512*512];
	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\512.bmp", IMAGE_BITMAP, 512, 512, LR_LOADFROMFILE);
	GetBitmapBits(bm, 512*512*4, data);
	DeleteObject(bm);

	// use only green channel

	// line FFT
	#pragma omp parallel for
	for(int y=0; y<512; y++)
	{
		for(int x=0; x<512; x++)
		{
			RGBQUAD *p = data + 512*y;
			tmp1[0][y][x] = p[x].rgbRed;
			tmp1[1][y][x] = 0;
		}

		fft_double(512, false, tmp1[0][y], tmp1[1][y], tmp2[0][y], tmp2[1][y]);
	}


	// row FFT
	#pragma omp parallel for
	for(int x=0; x<512; x++)
	{
		double tmp_r[512];
		double tmp_i[512];

		for(int y=0; y<512; y++)
		{
			tmp_r[y] = tmp2[0][y][x];
			tmp_i[y] = tmp2[1][y][x];
		}

		double tmp2_r[512];
		double tmp2_i[512];
		fft_double(512, false, tmp_r, tmp_i, tmp2_r, tmp2_i);

		for(int y=0; y<512; y++)
		{
			tmp3[0][y][x] = tmp2_r[y];
			tmp3[1][y][x] = tmp2_i[y];
		}
	}


	// FFT result peak scan
	double peak = 0.0;
	for(int y=0; y<512; y++)
	{
		for(int x=0; x<512; x++)
		{
			peak = max(peak, abs(tmp3[0][y][x]));
		}
	}

	// filter:
	float s = 30;
	float e = 180;
	int range = 32767;
	int num_left = 0;
	for(int y=0; y<512; y++)
	{
		for(int x=0; x<512; x++)
		{
			int m = max(x,y);

			float f = 1- (m-s)/(e-s);
			f = max(0.001, min(f,1));

			tmp3[0][y][x] *= f;
			tmp3[1][y][x] *= f;

			if (tmp3[0][y][x] >= 1.0)
				num_left ++;
// 			else
// 				tmp3[0][y][x] = 0;
		}
	}

	printf("%d/%d left, %.2f%%\n", num_left, 512*512, 100*(double)num_left/512/512);

	// store

	// Inverse row FFT
	#pragma omp parallel for
	for(int x=0; x<512; x++)
	{
		double tmp_r[512];
		double tmp_i[512];

		for(int y=0; y<512; y++)
		{
			tmp_r[y] = tmp3[0][y][x];
			tmp_i[y] = tmp3[1][y][x];
		}

		double tmp2_r[512];
		double tmp2_i[512];
		fft_double(512, true, tmp_r, tmp_i, tmp2_r, tmp2_i);

		for(int y=0; y<512; y++)
		{
			tmp4[0][y][x] = tmp2_r[y];
			tmp4[1][y][x] = tmp2_i[y];
		}
	}

	// Inverse line FFT
	#pragma omp parallel for
	for(int y=0; y<512; y++)
	{
		fft_double(512, true, tmp4[0][y], tmp4[1][y], tmp5[0][y], tmp5[1][y]);
	}

	RGBQUAD *o = new RGBQUAD[512*512];
	for(int y=0; y<512; y++)
	{
		for(int x=0; x<512; x++)
		{
			memset(&o[512*y+x], 0, 4);
			o[512*y+x].rgbGreen = tmp3[0][y][x]/peak*255 + 0.5;
		}
	}

	save_bitmap(o, L"Z:\\fft.bmp", 512, -512);

	// reconstruct
	peak = 255;
	for(int y=0; y<512; y++)
	{
		for(int x=0; x<512; x++)
		{
			memset(&o[512*y+x], 0, 4);
			double p = tmp5[0][y][x]/peak*255 + 0.5;
			o[512*y+x].rgbGreen = p<0?0:(p>255?255:p);
		}
	}

	save_bitmap(o, L"Z:\\reconstruct.bmp", 512, -512);


	// scanning test
	WIN32_FIND_DATAW find_data;
	HANDLE find_handle = FindFirstFileW(L"Z:\\tmp\\*.*", &find_data);

	IMAGE_ENTRY *entrys = new IMAGE_ENTRY[1024];
	int count = 0;

	if (find_handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0
				&& wcscmp(L".",find_data.cFileName ) !=0
				&& wcscmp(L"..", find_data.cFileName) !=0
				)
			{
			}
			else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)

			{
				wcscpy(entrys[count].filename, L"Z:\\tmp\\");
				wcscat(entrys[count].filename, find_data.cFileName);

				count ++;
			}

		}
		while( FindNextFile(find_handle, &find_data ) );
	}

	CRITICAL_SECTION cs;
	int done = 0;
	InitializeCriticalSection(&cs);
#pragma omp parallel for
	for(int i=0; i<count; i++)
	{
		sig(entrys[i].filename, entrys[i].sig);

		EnterCriticalSection(&cs);

		done ++;
		printf("\r%d/%d done", done, count);
		LeaveCriticalSection(&cs);
	}

	//des cs

	for(int i=0; i<count; i++)
	{
		wprintf(L"%s - %.2f,%.2f,%.2f,%.2f\n", entrys[i].filename, entrys[i].sig[0], entrys[i].sig[1], entrys[i].sig[2], entrys[i].sig[3]);
	}

	wprintf(L"\n\n\n");

	qsort(entrys, count, sizeof(IMAGE_ENTRY), compare_entry);
// 	qsort(entrys, count, sizeof(IMAGE_ENTRY), compare_entry2);

	for(int i=0; i<count; i++)
	{
		wchar_t tmp[MAX_PATH];
		swprintf(tmp, L"Z:\\sorted\\%d.jpg", i);
		wprintf(L"%d - %s - %.2f,%.2f,%.2f,%.2f\n", i, entrys[i].filename, entrys[i].sig[0], entrys[i].sig[1], entrys[i].sig[2], entrys[i].sig[3]);
		CopyFile(entrys[i].filename, tmp, FALSE);
	}

	return 0;
}

int compare_entry(const void *v1, const void*v2)
{
	IMAGE_ENTRY *p1 = (IMAGE_ENTRY*)v1;
	IMAGE_ENTRY *p2 = (IMAGE_ENTRY*)v2;

	return p1->sig[0] > p2->sig[0] ? 1 : -1;

	return 0;
}

int compare_entry2(const void *v1, const void*v2)
{
	IMAGE_ENTRY *p1 = (IMAGE_ENTRY*)v1;
	IMAGE_ENTRY *p2 = (IMAGE_ENTRY*)v2;

	if (abs(p1->sig[0] - p2->sig[0]) < min(abs(p1->sig[0]), abs(p2->sig[0])) * 0.1)
		return p1->sig[1] > p2->sig[1] ? 1 : -1;

	return 0;
}

DWORD sig(const wchar_t *filename, double *o)
{

	// read
	FILE * f = _wfopen(filename, L"rb");
	if (!f)
		return 0;

	fseek(f, 0, SEEK_END);
	int size = ftell(f);
	fseek(f, 0, SEEK_SET);

	char *data = (char*)malloc(size);
	fread(data, 1, size, f);
	fclose(f);

	// decode and resize
	int rtn = -1;
	unsigned char *decoded = (unsigned char*)malloc(512*512);
	double *m1 = new double[512*512];
	double *m2 = new double[512*512];
	double *m3 = new double[512*512];

	{
		EnterCriticalSection(&cs);
		ilInit();
		ilOriginFunc(IL_ORIGIN_UPPER_LEFT);
		ilEnable(IL_ORIGIN_SET);
		ILenum type = IL_TYPE_UNKNOWN;
		ILboolean result = ilLoadL(type, data, size );
		if (!result)
		{
			ILenum err = ilGetError() ;
			printf( "the error %d\n", err );
			printf( "string is %s\n", ilGetString( err ) );
			LeaveCriticalSection(&cs);
			goto end;
		}

		ilConvertImage(IL_LUMINANCE, IL_UNSIGNED_BYTE);
		int width = ilGetInteger(IL_IMAGE_WIDTH);
		int height = ilGetInteger(IL_IMAGE_HEIGHT);
		int datasize = ilGetInteger(IL_IMAGE_SIZE_OF_DATA);

		assert(width * height == datasize);
		

		resize<unsigned char>(ilGetData(), decoded, width, height, 512, 512);

		LeaveCriticalSection(&cs);
	}

	// calc DCT
	for(int i=0; i<512*512; i++)
		m1[i] = decoded[i];

#pragma omp parallel for
	for(int y=0; y<512; y++)
		dct(m1+y*512, m2+y*512, 512);


	// row FFT
#pragma omp parallel for
	for(int x=0; x<512; x++)
	{
		double tmp[512];
		double tmp2[512];

		for(int y=0; y<512; y++)
			tmp[y] = m2[y*512+x];

		dct(tmp, tmp2, 512);

		for(int y=0; y<512; y++)
			m3[y*512+x] = tmp2[y];
	}

	o[0] = m3[0*512+0];
	o[1] = m3[0*512+1] + m3[1*512+0];
	o[2] = m3[0*512+2] + m3[1*512+1] + m3[2*512+0];
	o[3] = m3[0*512+3] + m3[1*512+2] + m3[2*512+1] + m3[3*512+0];


	// return
end:
	free(decoded);
	free(data);
	delete [] m1;
	delete [] m2;
	delete [] m3;

	return rtn;
}



HRESULT save_bitmap(RGBQUAD *data, const wchar_t *filename, int width, int height) 
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
	BMIH.biSizeImage = ((((abs(BMIH.biWidth) * BMIH.biBitCount) 
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