// ZKStereo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "stereo.h"
#include "avisynth.h"
#include <emmintrin.h>

#pragma comment(lib, "winmm.lib")


class sq2sbs : public GenericVideoFilter 
{
public:
	sq2sbs(PClip _child, IScriptEnvironment *env);
	~sq2sbs();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

class letterbox_detector : public GenericVideoFilter 
{
public:
	letterbox_detector(PClip _child, IScriptEnvironment *env);
	~letterbox_detector();
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

sq2sbs ::sq2sbs(PClip _child, IScriptEnvironment *env)
:GenericVideoFilter(_child)
{
	if (_child->GetVideoInfo().pixel_type != vi.CS_YV12)
		env->ThrowError("only YV12 is supported for now");

	timeBeginPeriod(1);
	vi.width;

	vi.pixel_type = vi.CS_YV12;
}
sq2sbs ::~sq2sbs()
{
	timeEndPeriod(1);
}


BYTE clip(float v)
{
	v = max(0, v);
	v = min(v, 255);
	return (BYTE)v;
}

PVideoFrame __stdcall sq2sbs::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dstFrame = env->NewVideoFrame(vi);

	int width = vi.width/2;
	int height = vi.height;

	const BYTE * p = src->GetReadPtr(PLANAR_Y);

	CImage left(width, height, 1);
	CImage right(width, height, 1);

	// loading
	for(int y=0; y<height; y++)
	{
		for(int x=0; x<width; x++)
		{
			left.setValue(y, x, 0, p[width*2*y+x]);
			right.setValue(y, x, 0, p[width*2*y+x+width]);
		}
	}


	CStereo xxxx;

	int a = GetTickCount();
	xxxx.FindDisparityMap(&left, &right, -20, 20, 1, 2, 1, 8, 0.97, TRUE);
	xxxx.DisplayConfidenceValues();
	int b = GetTickCount() - a;

	BYTE *dst = dstFrame->GetWritePtr(PLANAR_Y);
	memset(dst + width * height*2, 128, width * height);

	for(int y=0; y<height; y++)
	{
		for(int x=0; x<width; x++)
		{
			dst[(y)*width*2 +x + width] = 255 - clip(xxxx.m_DisplayImage->getValue(y, x, 0));
			dst[(y)*width*2 +x + 0    ] = clip(xxxx.m_DisplayImage->getValue(y, x, 1));
		}
	}
	xxxx.Delete();

	return dstFrame;
}

letterbox_detector::letterbox_detector(PClip _child, IScriptEnvironment *env)
:GenericVideoFilter(_child)
{

}

letterbox_detector::~letterbox_detector()
{

}


int letterbox_detectY(const BYTE *data, int width, int height, int stride)
{
 	static FILE *f = fopen("Z:\\letterbox.log", "wb");
	fseek(f, 0, SEEK_SET);
	int t = GetTickCount();
	float line_lum[2000] = {0};
	float lum_avg = 0;
	__m128i mm_zero;
	mm_zero = _mm_setzero_si128();
	for(int y=1; y<height/2; y++)
	{
		__m128i mm_total1;
		__m128i mm_total2;
		mm_total1 = _mm_setzero_si128();
		mm_total2 = _mm_setzero_si128();
		__m128i mm1;
		__m128i mm2;
		for(int x=0; x<width; x+=16)
		{
			const BYTE *p = &data[y*stride+x];
			mm1 = _mm_loadu_si128((__m128i*)p);
			mm2 = _mm_unpackhi_epi8(mm1, mm_zero);
			mm1 = _mm_unpacklo_epi8 (mm1, mm_zero);
			mm_total1 = _mm_add_epi16(mm_total1, mm1);
			mm_total2 = _mm_add_epi16(mm_total2, mm2);
		}
		WORD *p = (WORD*)&mm_total1;
		WORD *p2 = (WORD*)&mm_total2;
		float total = p[0]+p[1]+p[2]+p[3]+p[4]+p[5]+p[6]+p[7];
		total += p2[0]+p2[1]+p2[2]+p2[3]+p2[4]+p2[5]+p2[6]+p2[7];
		total /= 1920;
		line_lum[y] = max(total-16, 0);
		lum_avg += line_lum[y]/height/2;
	}
// 	printf("time cost:%d\n", GetTickCount()-t);
	if (f)
		fprintf(f, "avg=\r\n", lum_avg);
	for(int y=1; y<height/2; y++)
	{
		float t1 = line_lum[y];
		float t2 = line_lum[y-1];
		t1 = max(0.001,t1);
		t2 = max(0.001,t2);
		float times = 0;
		if (t1>0 && t2>0)
			times = t1>t2?(double)t1/t2:(double)t2/t1;

		if (f)
			fprintf(f, "%d\t%f\t%f\r\n", y, line_lum[y], times);

		if (times > 10 && line_lum[y] > lum_avg*0.5)
			return y;

 		if (f)
 			fflush(f);
	}

	return 0;
}

PVideoFrame letterbox_detector::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dstFrame = env->NewVideoFrame(vi);

	const BYTE *psrc = src->GetReadPtr();
	BYTE *pdst = dstFrame->GetWritePtr();
	int stride = src->GetPitch();

	int letterbox_height = letterbox_detectY(psrc, vi.width, vi.height, stride);

	for(int y=0; y<vi.height; y++)
	{
		memcpy(pdst+y*stride, psrc+y*stride, stride);
		memcpy(pdst+vi.height*stride+y*stride/2, psrc+vi.height*stride+y*stride/2, stride/2);
	}

	for(int y=0; y<letterbox_height; y++)
	{
		memset(pdst+y*stride, 255, stride/2);
		memset(pdst+(vi.height-1-y)*stride, 255, stride/2);
	}
	return dstFrame;
}

AVSValue __cdecl Create_sq2sbs(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new sq2sbs(args[0].AsClip(), env);
}

AVSValue __cdecl Create_letterboxdetector(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new letterbox_detector(args[0].AsClip(), env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("LR", "c", Create_sq2sbs, 0);
	env->AddFunction("ShowLetterBox", "c", Create_letterboxdetector, 0);

	return "ZKStereo";
}