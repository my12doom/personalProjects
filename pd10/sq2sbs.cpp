#include <stdio.h>
#include <windows.h>
#include <dshow.h>
#include <ATLBASE.h>
#include <initguid.h>
#include "avisynth.h"

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
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
};

sq2sbs ::sq2sbs(PClip _child)
:GenericVideoFilter(_child)
{
	vi.width *= 2;
	vi.num_frames /= 2;
}

PVideoFrame __stdcall sq2sbs::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src1 = child->GetFrame(n*2, env);
	PVideoFrame src2 = child->GetFrame(n*2+1, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    const unsigned char* srcpY1 = src1->GetReadPtr(PLANAR_Y);
    const unsigned char* srcpV1 = src1->GetReadPtr(PLANAR_V);    
    const unsigned char* srcpU1 = src1->GetReadPtr(PLANAR_U);
    const unsigned char* srcpY2 = src2->GetReadPtr(PLANAR_Y);
    const unsigned char* srcpV2 = src2->GetReadPtr(PLANAR_V);    
    const unsigned char* srcpU2 = src2->GetReadPtr(PLANAR_U);

    unsigned char* dstpY = dst->GetWritePtr(PLANAR_Y);
    unsigned char* dstpV = dst->GetWritePtr(PLANAR_V);    
    unsigned char* dstpU = dst->GetWritePtr(PLANAR_U);

    const int src_pitchY = src1->GetPitch(PLANAR_Y);
    const int src_pitchUV = src1->GetPitch(PLANAR_V);
    const int dst_pitchY = dst->GetPitch(PLANAR_Y);
    const int dst_pitchUV = dst->GetPitch(PLANAR_U);

    const int row_sizeY = dst->GetRowSize(PLANAR_Y); 
    // Could also be PLANAR_Y_ALIGNED which would return a mod16 rowsize
    const int row_sizeUV = dst->GetRowSize(PLANAR_U); 
    // Could also be PLANAR_U_ALIGNED which would return a mod8 rowsize    
                        
    const int heightY = dst->GetHeight(PLANAR_Y);
    const int heightUV = dst->GetHeight(PLANAR_U);

    for (int y = 0; y < heightY; y++) 
	{
		memcpy(dstpY, srcpY1, row_sizeY/2);
		memcpy(dstpY + row_sizeY/2, srcpY2, row_sizeY/2);
        srcpY1 += src_pitchY;
        srcpY2 += src_pitchY;
        dstpY += dst_pitchY;
    }
                         
    for (int y = 0; y < heightUV; y++) 
	{
 		memcpy(dstpU, srcpU1, row_sizeUV/2);
		memcpy(dstpU + row_sizeUV/2, srcpU2, row_sizeUV/2);

		memcpy(dstpV, srcpV1, row_sizeUV/2);
		memcpy(dstpV + row_sizeUV/2, srcpV2, row_sizeUV/2);

		srcpU1 += src_pitchUV;
		srcpU2 += src_pitchUV;
        dstpU += dst_pitchUV;

        srcpV1 += src_pitchUV;
        srcpV2 += src_pitchUV;
        dstpV += dst_pitchUV;
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
