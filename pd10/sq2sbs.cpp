#include <stdio.h>
#include <windows.h>
#include <dshow.h>
#include <ATLBASE.h>
#include <initguid.h>
#include "avisynth.h"
#include "resource.h"

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

protected:
	char *m_YUY2_buffer;
};

sq2sbs ::sq2sbs(PClip _child)
:GenericVideoFilter(_child)
{
	vi.width *= 2;
	vi.num_frames /= 2;

	if (vi.height == 1088)
		vi.height = 1080;

	vi.pixel_type = vi.CS_YV12;
	m_YUY2_buffer = (char*) malloc(vi.width * vi.height *2);
}

sq2sbs::~sq2sbs()
{
	free(m_YUY2_buffer);
}

PVideoFrame __stdcall sq2sbs::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src1 = child->GetFrame(n*2, env);
	PVideoFrame src2 = child->GetFrame(n*2+1, env);

    const unsigned char* srcpY1 = src1->GetReadPtr(PLANAR_Y);
    const unsigned char* srcpY2 = src2->GetReadPtr(PLANAR_Y);
    const int src_pitchY = src1->GetPitch(PLANAR_Y);

	int skip_line[8] = {1, 136, 272, 408, 544, 680, 816, 952};
	int line_source = 0;
	int row_sizeY = vi.width * 2;
	char *dstpY = m_YUY2_buffer;
	for (int y = 0; y < vi.height; y++) 
	{
		memcpy(dstpY, srcpY1, row_sizeY/2);
		memcpy(dstpY + row_sizeY/2, srcpY2, row_sizeY/2);

        srcpY1 += src_pitchY;
        srcpY2 += src_pitchY;
		line_source ++;

		// skip those lines
		for(int i=0; i<8; i++)
			if (line_source == skip_line[i] && vi.height == 1080)
			{
				srcpY1 += src_pitchY;
				srcpY2 += src_pitchY;
				line_source ++;
			}

        dstpY += row_sizeY;
    }

	// add mask here
	// assume file = pd10.dll 
	// and colorspace = YUY2
	if (n==0)
	{
		HINSTANCE hdll = LoadLibrary(_T("pd10.dll"));
		HBITMAP hbmp = LoadBitmap(hdll, MAKEINTRESOURCE(IDB_BITMAP1));
		
		SIZE size = {200, 136};

		// get data and close bitmap
		unsigned char *m_mask = (unsigned char*)malloc(size.cx * size.cy * 4);
		GetBitmapBits(hbmp, size.cx * size.cy * 4, m_mask);
		DeleteObject(hbmp);

		// convert to YUY2
		// ARGB32 [0-3] = [BGRA]
		for(int i=0; i<size.cx * size.cy; i++)
		{
			m_mask[i*2] = m_mask[i*4];
			m_mask[i*2+1] = 128;
		}

		// add mask
		char *pdst = m_YUY2_buffer;
		int width = vi.width;
		int height = vi.height;
		for(int y=0; y<size.cy; y++)
		{
			memcpy(pdst+(y+height/2-size.cy/2)*width*2 +width/2-size.cx, 
				m_mask + size.cx*y*2, size.cx*2);
			memcpy(pdst+(y+height/2-size.cy/2)*width*2 +width/2-size.cx + width,
				m_mask + size.cx*y*2, size.cx*2);
		}

		FreeLibrary(hdll);
	}


	// my special convert to YV12
    PVideoFrame dst = env->NewVideoFrame(vi);
	unsigned char* pdst = (unsigned char*) dst->GetReadPtr();
	unsigned char* psrc = (unsigned char*)m_YUY2_buffer;

	// copy Y
	for(int y=0; y<vi.height; y++)
	{
		for(int x=0; x<vi.width; x++)
		{
			pdst[x] = psrc[x*2];

		}
		pdst += vi.width;
		psrc += vi.width*2;
	}

	// copy UV
	psrc = (unsigned char*)m_YUY2_buffer;
	unsigned char *pdst2 = pdst+vi.width*vi.height/4;
	for(int y=0; y<vi.height/2; y++)
	{
		for(int x=0; x<vi.width/2; x++)
		{
			pdst[x]  = psrc[x*4 +3];
			pdst2[x] = psrc[x*4 +1]; 
		}

		psrc += vi.width*4;// 2 line
		pdst += vi.width/2;// 1 line (half width)
		pdst2+= vi.width/2;
	}

	// debug output
	if(false)
	{
		char tmp[256];
		sprintf(tmp, "E:\\test_folder\\%03d.yuy2", n);
		FILE * f = fopen(tmp, "wb");
		fwrite(dst->GetReadPtr(), 3840, 1088*2, f);
		fclose(f);
	}

    return dst;
}

AVSValue __cdecl Create_sq2sbs(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new sq2sbs(args[0].AsClip());
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
    env->AddFunction("sq2sbs", "c", Create_sq2sbs, 0);
    env->AddFunction("CreateGRF", "s", func_create_grf, 0);
    return 0;
}
