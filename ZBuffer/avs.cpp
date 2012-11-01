#include "avs.h"

AVSValue __cdecl Create_sq2sbs(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new sq2sbs(args[0].AsClip());
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->SetMemoryMax(32);
	env->AddFunction("sq2sbs", "c", Create_sq2sbs, 0);
	return 0;
}

sq2sbs ::sq2sbs(PClip _child)
:GenericVideoFilter(_child)
{
	timeBeginPeriod(1);

	YDataPadded = (BYTE*)malloc((3840+32) * (1080+32));

	vi.pixel_type = VideoInfo::CS_BGR32;
}
sq2sbs ::~sq2sbs()
{
	timeEndPeriod(1);

	free(YDataPadded);
}

PVideoFrame __stdcall sq2sbs::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame dst = env->NewVideoFrame(vi);
	if (n >= vi.num_frames)
		return dst;

	memset(YDataPadded, 0, (3840+32)*(1080+32));


	PVideoFrame src1 = child->GetFrame(n, env);
	PVideoFrame src2 = child->GetFrame(n+1, env);

	const unsigned char* srcpY1 = src1->GetReadPtr(PLANAR_Y);
	const unsigned char* srcpY2 = src2->GetReadPtr(PLANAR_Y);
	const int src_pitchY = src1->GetPitch(PLANAR_Y);

	for(int y=0; y<1080; y++)
	{
		memcpy(YDataPadded+(3840+32)*(y+16)+16+1920, srcpY1 + src_pitchY*y, 1920*sizeof(BYTE));
		memcpy(YDataPadded+(3840+32)*(y+16)+16, srcpY2 + src_pitchY*y, 1920*sizeof(BYTE));
	}

	const int thread_count = 4;
	DWORD *out[thread_count*2+1] = {(DWORD*)dst->GetWritePtr()};
	HANDLE handles[thread_count];
	thread_parameter parameters[thread_count];
	for(int i=0; i<thread_count; i++)
	{
		thread_parameter &para = parameters[i];
		para.left = i*270;
		para.right = (i+1)*270;
		para.out = out[0];
		para._this = this;

		handles[i] = CreateThread(NULL, NULL, core_thread_entry, &para, NULL, NULL);
	}

	for(int i=0; i<thread_count; i++)
	{
		WaitForSingleObject(handles[i], INFINITE);
	}



	unsigned char* pdst = (unsigned char*) dst->GetReadPtr();

	return dst;
}

extern int SAD16x16(void *pdata1, void *pdata2, int stride);		// assume same stride

DWORD sq2sbs::core_thread(thread_parameter *para)
{
	DWORD *out = para->out;
	const int range = 160;
	const int throat = 0x7ffff;

	for(int y=para->left; y<para->right; y++)
	{
// 		printf("\r%d/1080", y-para->left);
		for(int x=0; x<1920; x++)
		{
			int min_delta = throat;
			int offset = -99999;
			BYTE *p1 = YDataPadded + (3840+32) * (y-8) + x - 8;
			//min_delta = min(min_delta, gen_mask((BYTE*)mask, (BYTE*)p1, YDataPadded [(3840+32) * y + x], (3840+32))*10 );
			for(int j=0; j<=0; j+=5)
				for(int i=-range; i<=range; i+=5)
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
					c = min_delta * 255 / (throat/32);
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