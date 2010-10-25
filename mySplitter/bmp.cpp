#include <Windows.h>
#include "bmp.h"

unsigned char inner_buffer[1280*720*10];
char dbg[1024];
#define append_dbg(a) sprintf(dbg, "%s.%d", dbg, a)
#define print_dbg MessageBox(0, dbg, dbg, MB_OK)
#define cls_dbg dbg[0]=NULL
typedef struct RGB_struct
{
	unsigned char B;
	unsigned char G;
	unsigned char R;
	unsigned char A;
} RGB;

typedef struct YUV_struct
{
	unsigned char V;
	unsigned char U;
	unsigned char Y;
	unsigned char A;
} YUV;
RGB mask_rgb[1280*720];
//RGB mask_rgb_half[640*360];
unsigned char mask[1280*720];
unsigned char mask_uv[640*360];
unsigned char mask_stride [1536*720];
unsigned char mask_uv_stride [768*360];
//YUV color = {240,90,82,0};
YUV color = {128,128,255,0};

void load()
{
	static l = false;
	if (l) return;

	GetBitMapData("C:\\black_text.bmp", (DWORD*)mask_rgb, 1280, 720);
	//GetBitMapData("Z:\\black_text.bmp", (DWORD*)mask_rgb_half, 640, 360);
	for(int i=0; i<1280*720; i++)
	{
		mask[i] = mask_rgb[i].R;
	}
	for(int i=0; i<720; i++)
	{
		memcpy(mask+1280*i+640, mask+1280*i, 640);
	}
	//for(int i=0; i<640*360; i++)
	//{
	//	mask_uv[i] = mask_rgb_half[i].R;
	//}


	for(int x=0; x<640; x++)
		for(int y=0; y<360; y++)
		{
			int x2 = x*2;
			int y2 = y*2;
			mask_uv[x+640*y] = (mask[x2   + y2   *1280] +
				mask[x2+1 + y2   *1280] +
				mask[x2   +(y2+1)*1280] +
				mask[x2+1 +(y2+1)*1280]) /4;			
		}


		l = true;
}

void init_stride()
{
	static l = false;
	if (l) return;

	for(int y=0; y<720; y++)
	{
		for (int x=0; x<1280; x++)
		{
			mask_stride[1280*y+x] = mask[1280*y+x];
		}
	}
	for(int y=0; y<360; y++)
	{
		for (int x=0; x<640; x++)
		{
			mask_uv_stride[640*y+x] = mask_uv[640*y+x];
			mask_uv_stride[640*y+x] = mask_uv[640*y+x];
		}
	}
	l = true;
}

unsigned char tbl[256] = {1,1};

void gen(unsigned char *pDst, int width, int height, int stride)
{
	if (tbl[0] == 1)
	{
		for (int i=0; i<256; i++)
		{
			tbl[i] = i;
		}
		tbl[17] = 255;
		tbl[236] = 0;
	}
	unsigned char *pDstV = pDst + stride*height;
	unsigned char *pDstU = pDstV+ (stride/2)*(height/2);
	for (int i=0; i<width; i++)
	{
		int c = tbl[(i/4) % 256];
		//DWORD yuv = RGB2YUV(RGB(c,c,c));
		pDst[i] = c;//(unsigned char)(yuv & 0xff00) >> 8;
	}
	for (int i=0; i<height; i++)
	{
		memcpy(pDst+stride*i, pDst, width);
	}

	for (int i=0; i<width/2; i++)
	{
		pDstU[i] = pDstV[i] = 128;//pDst[i*2];
	}
	for (int i=0; i<height/2; i++)
	{
		memcpy(pDstV+stride/2*i, pDstV, width/2);
		memcpy(pDstU+stride/2*i, pDstU, width/2);
	}

}

void mix(unsigned char *pDst, int width, int height, int stride)
{
	load();
	init_stride();
	unsigned char *pDstV = pDst + stride*height;
	unsigned char *pDstU = pDstV+ (stride/2)*(height/2);


	// mix y
	for (int y=0; y<720; y++)
	{
		int stride_y = stride * y;
		int _1280_y = 1280*y;
		for (int x=0; x<1280; x++)
		{
			pDst[stride_y+x] = (pDst[stride_y+x] * (255-mask[_1280_y+x])  + color.Y * mask[_1280_y+x]) / 255;
		}
	}

	// mix uv
	for (int y=0; y<360; y++)
	{
		int stride_y = stride / 2 * y;
		int _640_y = 640*y;
		for (int x=0; x<640; x++)
		{
			pDstV[stride_y+x] = (pDstV[stride_y+x] * (255-mask_uv[_640_y+x])  + color.V * mask_uv[_640_y+x]) / 255;
		}
		for (int x=0; x<640; x++)
		{
			pDstU[stride_y+x] = (pDstU[stride_y+x] * (255-mask_uv[_640_y+x])  + color.U * mask_uv[_640_y+x]) / 255;
		}
	}	
}

int GetBitMapData(LPTSTR filepath, DWORD*pdata, int width, int height)
{
	HBITMAP bmp = (HBITMAP)LoadImage(NULL, filepath, IMAGE_BITMAP, width, height, LR_LOADFROMFILE);
	GetBitmapBits(bmp, 1280*720*4, pdata);
	DeleteObject(bmp);

	return 0;
}

inline DWORD RGB2YUV(DWORD rgb)
{
	RGB in;
	YUV out;
	memcpy(&in, &rgb, 4);

	int R = in.R;
	int G = in.G;
	int B = in.B;

	out.A = 0;
	out.Y = (unsigned char)( (  66 * R + 129 * G +  25 * B + 128) >> 8 ) +  16;
	out.U = (unsigned char)( ( -38 * R -  74 * G + 112 * B + 128) >> 8) + 128;
	out.V = (unsigned char)( ( 112 * R -  94 * G -  18 * B + 128) >> 8) + 128;

	memcpy(&rgb, &out, 4);
	return rgb;
}
inline unsigned char clip_0_255(int in)
{
	if (in>255)
		return 255;
	if (in<0)
		return 0;
	return (unsigned char)in;
}
inline DWORD YUV2RGB(DWORD yuv)
{
	YUV in;
	RGB out;
	memcpy(&in, &yuv, 4);

	int C = (int)in.Y - 16;
	int D = (int)in.U - 128;
	int E = (int)in.V - 128;

	out.A = 0;
	out.R = clip_0_255(( 298 * C           + 409 * E + 128) >> 8);
	out.G = clip_0_255(( 298 * C - 100 * D - 208 * E + 128) >> 8);
	out.B = clip_0_255(( 298 * C + 516 * D           + 128) >> 8);

	memcpy(&yuv, &out, 4);
	return yuv;
}