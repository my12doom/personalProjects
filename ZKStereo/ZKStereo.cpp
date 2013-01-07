// ZKStereo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"

#include "stereo.h"
#include "avisynth.h"

#pragma comment(lib, "winmm.lib")


class sq2sbs : public GenericVideoFilter 
{
public:
	sq2sbs(PClip _child, IScriptEnvironment *env);
	~sq2sbs();
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

AVSValue __cdecl Create_sq2sbs(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new sq2sbs(args[0].AsClip(), env);
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->AddFunction("LR", "c", Create_sq2sbs, 0);

	return "ZKStereo";
}