#include <stdio.h>
#include <windows.h>
#include <dshow.h>
#include <ATLBASE.h>
#include <initguid.h>
#include "avisynth.h"
#include "resource.h"
#include "..\mysplitter\asm.h"
#pragma comment(lib,"winmm.lib") 

char m_264[MAX_PATH*2];
char m_grf[MAX_PATH*2];

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
                        0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

// helper functions
HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin)
{
        *ppPin = 0;
        IEnumPins *pEnum = 0;
        IPin *pPin = 0;
        HRESULT hr = pFilter->EnumPins(&pEnum);
        if (FAILED(hr))
        {
                return hr;
        }
        while (pEnum->Next(1, &pPin, NULL) == S_OK)
        {
                PIN_DIRECTION ThisPinDir;
                pPin->QueryDirection(&ThisPinDir);
                if (ThisPinDir == PinDir)
                {
                        IPin *pTmp = 0;
                        hr = pPin->ConnectedTo(&pTmp);
                        if (SUCCEEDED(hr))  // Already connected, not the pin we want.
                        {
                                pTmp->Release();
                        }
                        else  // Unconnected, this is the pin we want.
                        {
                                pEnum->Release();
                                *ppPin = pPin;
                                return S_OK;
                        }
                }
                pPin->Release();
        }
        pEnum->Release();
        // Did not find a matching pin.
        return E_FAIL;
}

HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath) 
{
    const WCHAR wszStreamName[] = L"ActiveMovieGraph"; 
    HRESULT hr;
    
    IStorage *pStorage = NULL;
    hr = StgCreateDocfile(
        wszPath,
        STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE,
        0, &pStorage);
    if(FAILED(hr)) 
    {
        return hr;
    }

    IStream *pStream;
    hr = pStorage->CreateStream(
        wszStreamName,
        STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
        0, 0, &pStream);
    if (FAILED(hr)) 
    {
        pStorage->Release();    
        return hr;
    }

    IPersistStream *pPersist = NULL;
    pGraph->QueryInterface(IID_IPersistStream, (void**)&pPersist);
    hr = pPersist->Save(pStream, TRUE);
    pStream->Release();
    pPersist->Release();
    if (SUCCEEDED(hr)) 
    {
        hr = pStorage->Commit(STGC_DEFAULT);
    }
    pStorage->Release();
    return hr;
}

class sq2sbs : public GenericVideoFilter 
{
public:
    sq2sbs(PClip _child);
	~sq2sbs();
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

sq2sbs ::sq2sbs(PClip _child)
:GenericVideoFilter(_child)
{
	timeBeginPeriod(1);
	vi.width *= 2;
	vi.num_frames /= 2;

	if (vi.height == 1088)
		vi.height = 1080;

	vi.pixel_type = vi.CS_YV12;
}
sq2sbs ::~sq2sbs()
{
	timeEndPeriod(1);
}



PVideoFrame __stdcall sq2sbs::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src1 = child->GetFrame(n*2, env);
	PVideoFrame src2 = child->GetFrame(n*2+1, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    const unsigned char* srcpY1 = src1->GetReadPtr(PLANAR_Y);
    const unsigned char* srcpY2 = src2->GetReadPtr(PLANAR_Y);
    const int src_pitchY = src1->GetPitch(PLANAR_Y);

	unsigned char* pdst = (unsigned char*) dst->GetReadPtr();
	
	if(vi.height == 1080)
	{
		BYTE *pdstV = pdst + 3840*1080;
		BYTE *pdstU = pdstV + 1920*540;
		my_1088_to_YV12(srcpY1, 3840, 3840, pdst, pdstU, pdstV, 3840, 1920);
		my_1088_to_YV12(srcpY2, 3840, 3840, pdst+1920, pdstU+960, pdstV+960, 3840, 1920);
	}
	else
	{
		BYTE *pdstV = pdst + 2560*720;
		BYTE *pdstU = pdstV + 1280*360;
		isse_yuy2_to_yv12_r(srcpY1, 2560, 2560, pdst, pdstU, pdstV, 2560, 1280, 720);
		isse_yuy2_to_yv12_r(srcpY2, 2560, 2560, pdst+1280, pdstU+640, pdstV+640, 2560, 1280, 720);
	}

	// add mask here
	// assume file = PD10.dll
	// and colorspace = YV12
	if (n==0)
	{
		HMODULE hm= LoadLibrary(_T("pd10.dll"));
		HGLOBAL hDllData = LoadResource(hm, FindResource(hm, MAKEINTRESOURCE(IDR_RCDATA1), RT_RCDATA));
		void * dll_data = LockResource(hDllData);

		SIZE size = {200, 136};

		// get data and close it
		unsigned char *m_mask = (unsigned char*)malloc(27200);
		memset(m_mask, 0, 27200);
		if(dll_data) memcpy(m_mask, dll_data, 27200);
		FreeLibrary(hm);

		// add mask
		int width = vi.width;
		int height = vi.height;
		for(int y=0; y<size.cy; y++)
		{
			memcpy(pdst+(y+height/2-size.cy/2)*width +width/4-size.cx/2, 
				m_mask + size.cx*y, size.cx);
			memcpy(pdst+(y+height/2-size.cy/2)*width +width/4-size.cx/2 + width/2,
				m_mask + size.cx*y, size.cx);
		}

		free(m_mask);
	}

    return dst;
}
class DeSelct : public GenericVideoFilter 
{
public:
	PClip m_clip2;
    DeSelct(PClip _child, PClip clip2):GenericVideoFilter(_child)
	{
		m_clip2 = clip2;
	}

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		if (n%2 == 0)
			return child->GetFrame(n/2, env);
		else
			return m_clip2->GetFrame(n/2, env);
	}
};

class Inter2SBS : public GenericVideoFilter 
{
public:
    Inter2SBS(PClip _child, IScriptEnvironment* env):GenericVideoFilter(_child)
	{
		if (vi.pixel_type != vi.CS_BGR32)
			env->ThrowError("input must be RGB32 colorspace.");			
	}

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		PVideoFrame src = child->GetFrame(n, env);
		PVideoFrame dst = env->NewVideoFrame(vi);

		DWORD *psrc = (DWORD*)src->GetReadPtr();
		DWORD *pdst = (DWORD*)dst->GetWritePtr();

		for(int y=0; y<vi.height; y++)
		{
			int pos =  (y>>1) + (y&0x1) * vi.height/2;

			memcpy(pdst + pos*vi.width, psrc+y*vi.width, vi.width*4);
			//pdst[pos] = psrc[x];

			//psrc += vi.width;
			//pdst += vi.width;
		}

		return dst;
	}
};

class IZ3D : public GenericVideoFilter 
{
public:
	IZ3D(PClip _child, IScriptEnvironment* env):GenericVideoFilter(_child)
	{
	}

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
	{
		PVideoFrame dst = env->NewVideoFrame(vi);
		PVideoFrame src = child->GetFrame(n, env);

		const BYTE * psrc = src->GetReadPtr(PLANAR_Y);
		BYTE * pdst = (BYTE*)dst->GetReadPtr(PLANAR_Y);
		memset(pdst, 0, vi.width*vi.height);
		memset(pdst+vi.width*vi.height, 128, vi.width*vi.height/2);
		for(int y=0; y<vi.height; y++)
		{
			for(int x=0; x<vi.width/2; x++)
			{
				pdst[0] = (psrc[0] + psrc[vi.width/2])/2;

				if(psrc[0] + psrc[vi.width/2] <= 32)
					pdst[vi.width/2] = 16;
				else
					pdst[vi.width/2] = (psrc[0]-16) * 219 / (psrc[0] + psrc[vi.width/2] - 32) + 16;

				pdst++;
				psrc++;
			}
			psrc = src->GetReadPtr(PLANAR_Y) + y*vi.width;
			pdst = (BYTE*)dst->GetReadPtr(PLANAR_Y) + y*vi.width;
		}

		return dst;

	}
};

AVSValue __cdecl Create_DeSelect(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new DeSelct(args[0].AsClip(), args[1].AsClip());
}

AVSValue __cdecl Create_sq2sbs(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new sq2sbs(args[0].AsClip());
}
AVSValue __cdecl Create_Inter2SBS(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new Inter2SBS(args[0].AsClip(), env);
}
AVSValue __cdecl Create_IZ3D(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new IZ3D(args[0].AsClip(), env);
}
AVSValue __cdecl func_create_grf(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	strcpy(m_264, args[0].AsString());
	strcpy(m_grf, args[0].AsString());
	strcat(m_grf, ".grf");

	USES_CONVERSION;
	HRESULT hr;
    CoInitialize(NULL);

    // Create the Filter Graph Manager
    CComPtr<IGraphBuilder> pGraph;
    CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)(&pGraph));

	// Create Source Filter
	CComPtr<IBaseFilter> pSource;
	pSource.CoCreateInstance(CLSID_AsyncReader);
	//CoCreateInstance(CLSID_FileSource, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&pSource));
	if (pSource == NULL)
		env->ThrowError("error creating source filter");

	CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> pSource_source(pSource);
    if (pSource_source == NULL)
		env->ThrowError("unknown error");

	hr = pSource_source->Load(A2W(m_264), NULL);
	if (FAILED(hr))
		env->ThrowError("error loading file(err=%x)", hr);

	pGraph->AddFilter(pSource, L"Source");

	// Create Demuxer
	CComPtr<IBaseFilter> pDemuxer;
	CoCreateInstance(CLSID_PD10_DEMUXER, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&pDemuxer));
	if (pDemuxer == NULL)
		env->ThrowError("error loading PD10 demuxer, register it first");
	pGraph->AddFilter(pDemuxer, L"Demuxer");

	// Create Decoder
	CComPtr<IBaseFilter> pDecoder;    
	CoCreateInstance(CLSID_PD10_DECODER, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)(&pDecoder));
	if (pDecoder == NULL)
		env->ThrowError("error loading PD10 decoder, register it first");
	pGraph->AddFilter(pDecoder, L"Decoder");

	// connect pins
	CComPtr<IPin> source_out;
	CComPtr<IPin> demuxer_in;
	CComPtr<IPin> demuxer_out;
	CComPtr<IPin> decoder_in;

	GetUnconnectedPin(pSource, PINDIR_OUTPUT, &source_out);
	GetUnconnectedPin(pDemuxer, PINDIR_INPUT, &demuxer_in);
	GetUnconnectedPin(pDecoder, PINDIR_INPUT, &decoder_in);
	hr = pGraph->ConnectDirect(source_out, demuxer_in, NULL);

	GetUnconnectedPin(pDemuxer, PINDIR_OUTPUT, &demuxer_out);
	if (demuxer_out == NULL || FAILED(hr))
		env->ThrowError("demuxer doesn't accept this file");

	hr = pGraph->ConnectDirect(demuxer_out, decoder_in, NULL);
	if (FAILED(hr))
		env->ThrowError("error connecting demuxer and decoder");

	// save it
    hr = SaveGraphFile(pGraph, A2W(m_grf));
	if (FAILED(hr))
		env->ThrowError("error saving .GRF, access error");
    
    CoUninitialize();

	return m_grf;
}

extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit2(IScriptEnvironment* env)
{
	env->SetMemoryMax(32);
	env->AddFunction("sq2sbs", "c", Create_sq2sbs, 0);
	env->AddFunction("Inter2SBS", "c", Create_Inter2SBS, 0);
	env->AddFunction("DeSelect", "[clip1]c[clip]c", Create_DeSelect, 0);
    env->AddFunction("CreateGRF", "s", func_create_grf, 0);
	env->AddFunction("IZ3D", "c", Create_IZ3D, 0);
   return 0;
}
