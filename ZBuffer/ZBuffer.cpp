#include <time.h>
#include <stdio.h>
#include <Windows.h>
#include "stereo_test.h"
#include <assert.h>
#include <emmintrin.h>

#pragma comment(lib, "winmm.lib")

HRESULT save_bitmap(DWORD *data, const wchar_t *filename, int width, int height); 
int main();
int SAD16x16(void *pdata1, void *pdata2, int stride);		// assume same stride
int gen_mask(BYTE *out, BYTE *in, BYTE center_color, int stride);
int SAD16x16(void *pdata1, void *pdata2, void *mask, int stride);		// assume same stride
void philip();


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

BYTE *YData = new BYTE[3840*1080];
BYTE *YDataPadded = new BYTE[(3840+32) * (1080+32)];
DWORD *resource = new DWORD[3840*1080];
DWORD *padded = new DWORD[(3840+32) * (1080+32)];

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
	DWORD *mask = new DWORD[(3840+32) * (1080+32)];
	const int throat = 0x7ffff;

	for(int y=para->left; y<para->right; y++)
	{
		printf("\r%d/1080", y-para->left);
		for(int x=0; x<1920; x++)
		{
			int min_delta = throat;
			int offset = -99999;
			BYTE *p1 = YDataPadded + (3840+32) * (y-8) + x - 8;
			//min_delta = min(min_delta, gen_mask((BYTE*)mask, (BYTE*)p1, YDataPadded [(3840+32) * y + x], (3840+32))*10 );
			for(int j=-5; j<=5; j+=5)
			for(int i=-range; i<=range; i++)
			{
				BYTE *p2 = YDataPadded + (3840+32) * (y-8+j) + x + 1920 - 8 + i;

				int t = SAD16x16(p1, p2/*, mask*/, (3840+32));
				//if (x == 540) printf("%.1f, ", (float)t);
				if (t<min_delta)
				{
					min_delta = t;
					offset = i;
				}
			}
			if (offset != -99999)
			{
				int c = min((offset+range)*255/(2*range)+1, 255);
				//c = min_delta * 255 / (throat/32);
				out[(1080-1-y)*1920+x] = RGB(c, c, c);
			}
			else
			{
// 				int x1 = max(0, min(x, 1920));
// 				int y1 = max(0, min(y, 1080));
// 				out[(1080-1-y)*1920+x] = out[(1080-1-y1)*1920+x1];
				out[(1080-1-y)*1920+x] = RGB(255, 255, 255);
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

int subpixel2D()
{
	RGBQUAD *src = new RGBQUAD [1920*1080];
	RGBQUAD *dst = new RGBQUAD [1920*1080*6];
	RGBQUAD *ref = new RGBQUAD [1920*1080*3];

	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\ass01.bmp", IMAGE_BITMAP, 1920, 1080, LR_LOADFROMFILE);
	memset(src, 0x3f, 1920*1080*4);
	GetBitmapBits(bm, 1920*1080*4, src);
	DeleteObject(bm);

	for(int y=0; y<1080; y++)
	for(int x=0; x<1920; x++)
	{
		// BGRA
		RGBQUAD s = src[y*1920+x];
		RGBQUAD r = {0, 0, s.rgbRed, 0};
		RGBQUAD g = {0, s.rgbGreen, 0, 0};
		RGBQUAD b = {s.rgbBlue, 0, 0, 0};

		RGBQUAD *d = &dst[y*1920*6+ x*6];
		d[0] = g;
		d[1] = g;
		d[2] = r;
		d[3] = r;
		d[4] = b;
		d[5] = b;
	}

// 	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\ass01.bmp", IMAGE_BITMAP, 1920, 1080, LR_LOADFROMFILE);
	memset(src, 0x3f, 1920*1080*4);
	GetBitmapBits(bm, 1920*1080*4, src);
	DeleteObject(bm);
	for(int y=0; y<1080; y++)
	for(int x=0; x<1920; x++)
	{
		// BGRA
		RGBQUAD s = src[y*1920+x];
		RGBQUAD r = {0, 0, s.rgbRed, 0};
		RGBQUAD g = {0, s.rgbGreen, 0, 0};
		RGBQUAD b = {s.rgbBlue, 0, 0, 0};

		RGBQUAD *d = &ref[y*1920*3+ x*3];
		d[0] = r;
		d[1] = g;
		d[2] = b;
	}

	save_bitmap((DWORD*)dst, L"Z:\\out.bmp", 1920*6, 1080);
	save_bitmap((DWORD*)ref, L"Z:\\ref.bmp", 1920*3, 1080);

	return 0;
}

int letterbox_detect()
{
	RGBQUAD * data = new RGBQUAD[1920*1080];
	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\letterboxBW.bmp", IMAGE_BITMAP, 1920, 1080, LR_LOADFROMFILE);
	GetBitmapBits(bm, 1920*1080*4, data);
	DeleteObject(bm);

	RGBQUAD mask[4] = {{0,255,0,0},{0,255,0,0},{0,255,0,0},{0,255,0,0}};
	int t = GetTickCount();
	int line_lum[1080] = {0};
	__m128i mm_mask;
	mm_mask = _mm_loadu_si128((__m128i*)&mask);
	for(int i=0; i<1000; i++)
	{
		FILE *f = fopen("Z:\\letterbox.txt", "wb");
		for(int y=1; y<540; y++)
		{
			__m128i mm0;
			mm0 = _mm_setzero_si128();
			__m128i mm1;
			for(int x=1; x<960; x+=4)
			{
				RGBQUAD *p = &data[y*1920+x];
				mm1 = _mm_loadu_si128((__m128i*)p);
				mm1 = _mm_and_si128(mm1, mm_mask);
				mm1 = _mm_srai_epi32 (mm1, 8);
				mm0 = _mm_add_epi32(mm0, mm1);
// 				line_lum[y] += ((double)abs(p->rgbGreen));//((double)abs(p->rgbGreen - (p-1)->rgbGreen));
			}
			DWORD *p = (DWORD*)&mm0;
			int total = p[0]+p[1]+p[2]+p[3];
			line_lum[y] = total;
			int t1 = line_lum[y];
			int t2 = line_lum[y-1];
			float times = 0;
			if (t1>0 && t2>0)
				times = t1>t2?(double)t1/t2:(double)t2/t1;

 			fprintf(f, "%d\t%d\t%f\r\n", y, line_lum[y], times);
			if (times > 100)
				break;
		}
		fclose(f);
	}
	printf("time cost:%d\n", GetTickCount()-t);
	delete data;

	return 0;
}


int letterbox_detectY()
{
	BYTE * data = new BYTE[1920*1080];
	FILE * f = fopen("Z:\\letterbox.raw", "rb");
	fread(data, 1, 1920*1080, f);
	fclose(f);

	int t = GetTickCount();
	int line_lum[1080] = {0};
	__m128i mm_zero;
	mm_zero = _mm_setzero_si128();
	for(int i=0; i<1000; i++)
	{
		FILE *f = fopen("Z:\\letterbox.txt", "wb");
		for(int y=1; y<540; y++)
		{
			__m128i mm_total1;
			__m128i mm_total2;
			mm_total1 = _mm_setzero_si128();
			mm_total2 = _mm_setzero_si128();
			__m128i mm1;
			__m128i mm2;
			for(int x=0; x<1920; x+=16)
			{
				BYTE *p = &data[y*1920+x];
				mm1 = _mm_loadu_si128((__m128i*)p);
				mm2 = _mm_unpackhi_epi8(mm1, mm_zero);
				mm1 = _mm_unpacklo_epi8 (mm1, mm_zero);
				mm_total1 = _mm_add_epi16(mm_total1, mm1);
				mm_total2 = _mm_add_epi16(mm_total2, mm2);
			}
			WORD *p = (WORD*)&mm_total1;
			WORD *p2 = (WORD*)&mm_total2;
			int total = p[0]+p[1]+p[2]+p[3]+p2[0]+p2[1]+p2[2]+p2[3];
			total /= 1920;
			line_lum[y] = total;
			int t1 = line_lum[y];
			int t2 = line_lum[y-1];
			float times = 0;
			if (t1>0 && t2>0)
				times = t1>t2?(double)t1/t2:(double)t2/t1;

 			fprintf(f, "%d\t%d\t%f\r\n", y, line_lum[y], times);
			if (times > 5 && total > 10)
				break;
		}
		fclose(f);
	}
	printf("time cost:%d\n", GetTickCount()-t);
	delete data;

	return 0;
}

int main()
{
	return letterbox_detectY();
	/*
	FILE * xxxx = fopen("Z:\\y2.raw", "rb");

	BYTE *p = new BYTE[3840*1080];
	fread(p, 1, 3840*1080, xxxx);
	fclose(xxxx);
	RGBQUAD *p2 = new RGBQUAD[3840*1080];

	for(int y=0; y<1080; y++)
		for(int x=0; x<1920; x++)
	{
		BYTE &l = p[y*3840 + x];
		BYTE &r = p[y*3840 + x + 1920];

		BYTE ll = l - r/4;
		BYTE rr = r - l/4;

		l = ll;
		r = rr;
	}

	for(int i=3840*1080-1; i>=0; i--)
		memset(p2+i, ((BYTE*)p)[i], 4);

	save_bitmap((DWORD*)p2, L"Z:\\y2.bmp", 3840, 1080);
	return 0;
	*/



// 	return subpixel2D();

// 	philip();
// 	return 0;

	//return PC3DV();
	//return CV();
	const int range = 40;

	timeBeginPeriod(1);

	HANDLE h_parent_process = GetCurrentProcess();
	SetPriorityClass(h_parent_process, IDLE_PRIORITY_CLASS);			//host idle priority

	HBITMAP bm = (HBITMAP)LoadImageA(0, "Z:\\pro.bmp", IMAGE_BITMAP, 3840, 1080, LR_LOADFROMFILE);
	GetBitmapBits(bm, 3840*1080*4, resource);
	DeleteObject(bm);
	memset(padded, 0, sizeof(DWORD) * (3840+32) * (1080+32));

	FILE * f = fopen("Z:\\I.raw", "rb");
	fread(YData, 1, 3840*1080, f);
	fclose(f);
	memset(YDataPadded, 0, (3840+32)*(1080+32));



	padded += (3840+32)*16;

	for(int y=0; y<1080; y++)
	{
		memcpy(padded+(3840+32)*(y+16)+16, resource + 3840*y, 3840*sizeof(DWORD));
		memcpy(YDataPadded+(3840+32)*(y+16)+16, YData + 3840*y, 3840*sizeof(BYTE));
	}

	YDataPadded += (3840+32)*16;

	save_bitmap(padded, L"Z:\\padded.bmp", 3840+32, 1080+32);

	BYTE tmp[256];
	BYTE tmp1[256];
	BYTE tmp2[256];
	for(int x = 0; x<16; x++)
		for(int y=0; y<16; y++)
	{
		tmp1[x+16*y] = tmp[x+16*y] = (x*5+6*y + x * y) &0xff;
		tmp1[x+16*y] = tmp1[x+16*y] == 255 ? 254 : tmp1[x+16*y]+1;
	}

	gen_mask(tmp2, tmp, 50, 16);
	int sad = SAD16x16(tmp, tmp1, tmp2, 16);



	resource[0] = 0xffffffff;
	int o = 0;
	timeBeginPeriod(1);
	int l = timeGetTime();
	for(int i=0; i<1000000; i++)
		o += SAD16x16(resource, resource + 1920, i) *2+3;

	printf("SAD16x16() = %d, time = %dms", o, timeGetTime() - l);

	timeEndPeriod(1);

// 	Sleep(5000);

	//FILE * f = fopen("E:\\JM18\\bin\\test_dec.yuv", "rb");
	//fread(resource, 1, 1920*1080*1.5, f);
	//fclose(f);



	int tick = timeGetTime();

	int layout = input_layout_auto;
	for(int j=0; j<100; j++)
	{
		get_layout<BYTE>(YData, 3840, 1080, &layout);
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


int SAD16x16(void *pdata1, void *pdata2, int stride)		// assume same stride
{
	char *p1 = (char*)pdata1;
	char *p2 = (char*)pdata2;

	__m128i a,b,c;
	__m128i t2;

	t2 = _mm_setzero_si128();

	for(int i=0; i<16; i++)
	{
		a = _mm_loadu_si128((__m128i*)p1);
		b = _mm_loadu_si128((__m128i*)p2);

		a = _mm_sad_epu8(a, b);
		t2 = _mm_add_epi64(t2, a);

		p1 += stride;
		p2 += stride;
	}

	__int64 *p = (__int64*) &t2;
	return p[0] + p[1];
}

int SAD16x16(void *pdata1, void *pdata2, void *mask, int stride)		// assume same stride
{
	char *p1 = (char*)pdata1;
	char *p2 = (char*)pdata2;
	char *p3 = (char*)mask;

	__m128i a,b,c;
	__m128i t2;

	t2 = _mm_setzero_si128();

	for(int i=0; i<16; i++)
	{
		a = _mm_loadu_si128((__m128i*)p1);
		b = _mm_loadu_si128((__m128i*)p2);
		c = _mm_loadu_si128((__m128i*)p3);

		a = _mm_and_si128(a, c);
		b = _mm_and_si128(b, c);

		a = _mm_sad_epu8(a, b);
		t2 = _mm_add_epi64(t2, a);

		p1 += stride;
		p2 += stride;
		p3 += stride;
	}

	__int64 *p = (__int64*) &t2;
	return p[0] + p[1];
}

int gen_mask(BYTE *out, BYTE *in, BYTE center_color, int stride)
{
	int c = 0;
	for(int i=0; i<16; i++)
	{
		for(int x=0; x<16; x++)
		{
			bool a = abs (in[x] - center_color) < 10;
			c += a ? 1 : 0;
			out[x] = a ? 255 : 0;
		}

		in += stride;
		out += stride;
	}

	return c;
}

RGBQUAD * views[9];
RGBQUAD * comp_views[9];
RGBQUAD * out;

void comp()
{
	double matrix[] = 
	{
// 1.788,-0.682,0.259,-0.094,0.024,0.024,-0.094,0.259,-0.682,
// -0.682,1.788,-0.682,0.259,-0.094,0.024,0.024,-0.094,0.259,
// 0.259,-0.682,1.788,-0.682,0.259,-0.094,0.024,0.024,-0.094,
// -0.094,0.259,-0.682,1.788,-0.682,0.259,-0.094,0.024,0.024,
// 0.024,-0.094,0.259,-0.682,1.788,-0.682,0.259,-0.094,0.024,
// 0.024,0.024,-0.094,0.259,-0.682,1.788,-0.682,0.259,-0.094,
// -0.094,0.024,0.024,-0.094,0.259,-0.682,1.788,-0.682,0.259,
// 0.259,-0.094,0.024,0.024,-0.094,0.259,-0.682,1.788,-0.682,
// -0.682,0.259,-0.094,0.024,0.024,-0.094,0.259,-0.682,1.788,


// 		0.75,0.25,0.00,0.00,0.00,0.00,0.00,0.00,0.25,
// 		0.25,0.75,0.25,0.00,0.00,0.00,0.00,0.00,0.00,
// 		0.00,0.25,0.75,0.25,0.00,0.00,0.00,0.00,0.00,
// 		0.00,0.00,0.25,0.75,0.25,0.00,0.00,0.00,0.00,
// 		0.00,0.00,0.00,0.25,0.75,0.25,0.00,0.00,0.00,
// 		0.00,0.00,0.00,0.00,0.25,0.75,0.25,0.00,0.00,
// 		0.00,0.00,0.00,0.00,0.00,0.25,0.75,0.25,0.00,
// 		0.00,0.00,0.00,0.00,0.00,0.00,0.25,0.75,0.25,
// 		0.25,0.00,0.00,0.00,0.00,0.00,0.00,0.25,0.75,

		1,0,0.00,0.00,0.00,0.00,0.00,0.00,0,
		0,1,0,0.00,0.00,0.00,0.00,0.00,0.00,
		0.00,0,1,0,0.00,0.00,0.00,0.00,0.00,
		0.00,0.00,0,1,0,0.00,0.00,0.00,0.00,
		0.00,0.00,0.00,0,1,0,0.00,0.00,0.00,
		0.00,0.00,0.00,0.00,0,1,0,0.00,0.00,
		0.00,0.00,0.00,0.00,0.00,0,1,0,0.00,
		0.00,0.00,0.00,0.00,0.00,0.00,0,1,0,
		0,0.00,0.00,0.00,0.00,0.00,0.00,0,1,

// 0.50,0.25,0.00,0.00,0.00,0.00,0.00,0.00,0.25,
// 0.25,0.50,0.25,0.00,0.00,0.00,0.00,0.00,0.00,
// 0.00,0.25,0.50,0.25,0.00,0.00,0.00,0.00,0.00,
// 0.00,0.00,0.25,0.50,0.25,0.00,0.00,0.00,0.00,
// 0.00,0.00,0.00,0.25,0.50,0.25,0.00,0.00,0.00,
// 0.00,0.00,0.00,0.00,0.25,0.50,0.25,0.00,0.00,
// 0.00,0.00,0.00,0.00,0.00,0.25,0.50,0.25,0.00,
// 0.00,0.00,0.00,0.00,0.00,0.00,0.25,0.50,0.25,
// 0.25,0.00,0.00,0.00,0.00,0.00,0.00,0.25,0.50,

// 9.000,-7.000,5.000,-3.000,1.000,1.000,-3.000,5.000,-7.000,
// -7.000,9.000,-7.000,5.000,-3.000,1.000,1.000,-3.000,5.000,
// 5.000,-7.000,9.000,-7.000,5.000,-3.000,1.000,1.000,-3.000,
// -3.000,5.000,-7.000,9.000,-7.000,5.000,-3.000,1.000,1.000,
// 1.000,-3.000,5.000,-7.000,9.000,-7.000,5.000,-3.000,1.000,
// 1.000,1.000,-3.000,5.000,-7.000,9.000,-7.000,5.000,-3.000,
// -3.000,1.000,1.000,-3.000,5.000,-7.000,9.000,-7.000,5.000,
// 5.000,-3.000,1.000,1.000,-3.000,5.000,-7.000,9.000,-7.000,
// -7.000,5.000,-3.000,1.000,1.000,-3.000,5.000,-7.000,9.000,

	};

// 	for(int i=0; i<sizeof(matrix)/sizeof(double); i++)
// 		matrix[i] /= 6;

	for(int y=0; y<1080; y++)
	{
		for(int x=0; x<1920; x++)
		{
			RGBQUAD p[9];
			for(int i=0; i<9; i++)
				p[i] = views[i][y*1920 + x];

			for(int i=0; i<9; i++)  // target view
			{
				double r = 0;
				double g = 0;
				double b = 0;
				for(int j=0; j<9; j++) // each source view
				{
					double f = matrix[i*9+j];
					r += f * p[j].rgbRed;
					g += f * p[j].rgbGreen;
					b += f * p[j].rgbBlue;
				}

				RGBQUAD q;
				q.rgbRed = max(min(r,255),0);
				q.rgbGreen = max(min(g,255),0);
				q.rgbBlue = max(min(b,255),0);

				comp_views[i][y*1920 + x] = q;
			}
		}
	}

	printf("");
}

void interlace()
{
	int tbl[9] = {1,3,5,7,9,2,4,6,8};

	for(int y=0; y<1080; y++)
	{
		for(int x=0; x<1920; x++)
		{
			int r = x*3 + y*4;
			int g = r+1;
			int b = g+1;

			int rr = tbl[r%9];
			int gg = tbl[g%9];
			int bb = tbl[b%9];

			RGBQUAD q;
			q.rgbRed = comp_views[rr-1][y*1920 + x].rgbRed;
			q.rgbGreen = comp_views[gg-1][y*1920 + x].rgbGreen;
			q.rgbBlue = comp_views[bb-1][y*1920 + x].rgbBlue;

			out[y*1920 + x] = q;
		}
	}

}


void philip()
{
	// read 9 views
	out = new RGBQUAD[1920*1080];
	for(int i=0; i<9; i++)
	{
		views[i] = new RGBQUAD[1920*1080];
		comp_views[i] = new RGBQUAD[1920*1080];

		char tmp[MAX_PATH];
		int n = i+1;
		sprintf(tmp, "Z:\\1\\view%02d\\unknown00007.bmp", n);
		HBITMAP bm = (HBITMAP)LoadImageA(0, tmp, IMAGE_BITMAP, 1920, 1080, LR_LOADFROMFILE);
		memset(views[i], 0x3f, 1920*1080*4);
		GetBitmapBits(bm, 1920*1080*4, views[i]);
		DeleteObject(bm);

		// reverse
		RGBQUAD tmp2[1920];
		for (int y=0; y<1080/2; y++)
		{
			memcpy(tmp2, views[i] + 1920*y, 1920*4);
			memcpy(views[i] + 1920*y, views[i] + 1920*(1080-1-y), 1920*4);
			memcpy(views[i] + 1920*(1080-1-y), tmp2,1920*4);
		}
	}


	// 
	comp();

	interlace();


	for(int i=0; i<9; i++)
	{
		wchar_t tmp[MAX_PATH];
		swprintf(tmp, L"Z:\\comp%02d.bmp", i+1);
		save_bitmap((DWORD*) comp_views[i], tmp, 1920, 1080);
	}
	
	save_bitmap((DWORD*) out, L"Z:\\philip.bmp", 1920, 1080);
}




























// int SAD16x16(void *pdata1, void *pdata2, int stride)		// assume same stride
// {
// 	BYTE *p1 = (BYTE*)pdata1;
// 	BYTE *p2 = (BYTE*)pdata2;
// 
// 	int o = 0;
// 
// 	for(int i=0; i<16; i++)
// 	{
// 		for(int x=0; x<16; x++)
// 			o += abs(p1[x]-p2[x]);
// 
// 
// 		p1 += stride;
// 		p2 += stride;
// 	}
// 
// 	return o;
// }
