#include "audio_downmix.h"
#include <stdio.h>

CDWindowAudioDownmix::CDWindowAudioDownmix(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_DWindowAudioDownmix)
{
} 

CDWindowAudioDownmix::~CDWindowAudioDownmix() 
{
}

// CreateInstance
// Provide the way for COM to create a DWindowExtenderMono object
CUnknown *CDWindowAudioDownmix::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	CDWindowAudioDownmix *pNewObject = new CDWindowAudioDownmix(NAME("DWindow Extender Mono"), punk, phr);
	// Waste to check for NULL, If new fails, the app is screwed anyway
	return pNewObject;
}

STDMETHODIMP CDWindowAudioDownmix::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	//if (riid == __uuidof(IDWindowExtender)) 
	//	return GetInterface((IDWindowExtender *) this, ppv);

	return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CDWindowAudioDownmix::CheckInputType(const CMediaType *mtIn)
{

	if (mtIn->majortype != MEDIATYPE_Audio)
		return VFW_E_INVALID_MEDIA_TYPE;
	if (mtIn->subtype != MEDIASUBTYPE_PCM && mtIn->subtype != MEDIASUBTYPE_IEEE_FLOAT)
		return VFW_E_INVALID_MEDIA_TYPE;
	if (mtIn->formattype != FORMAT_WaveFormatEx)
		return VFW_E_INVALID_MEDIA_TYPE;

	memcpy(&m_in_fmt, mtIn->pbFormat, min(mtIn->cbFormat, sizeof(WAVEFORMATEX)));
	m_in_subtype = mtIn->subtype;

	printf("wFormatTag = %04x\n", m_in_fmt.wFormatTag);

	if (m_in_fmt.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
	{
		WAVEFORMATEXTENSIBLE *wext = (WAVEFORMATEXTENSIBLE*)mtIn->pbFormat;
		printf("WAVE_FORMAT_EXTENSIBLE:\n");
		if (wext->SubFormat != MEDIASUBTYPE_PCM && wext->SubFormat != MEDIASUBTYPE_IEEE_FLOAT)
			return VFW_E_INVALID_MEDIA_TYPE;
	}

	m_out_fmt.wFormatTag = WAVE_FORMAT_PCM;
	m_out_fmt.nChannels = 2;
	m_out_fmt.nSamplesPerSec = m_in_fmt.nSamplesPerSec;
	m_out_fmt.nAvgBytesPerSec = 4*m_out_fmt.nSamplesPerSec;
	m_out_fmt.nBlockAlign = 4;
	m_out_fmt.wBitsPerSample = 16;
	m_out_fmt.cbSize = 0;
	//vihOut->pExtraBytes;

	return NOERROR;
}

HRESULT CDWindowAudioDownmix::CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut)
{
	GUID subtypeIn = *mtIn->Subtype();
	GUID subtypeOut = *mtOut->Subtype();


	return NOERROR;
}

HRESULT CDWindowAudioDownmix::GetMediaType(int iPosition, CMediaType *pMediaType)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	if (iPosition < 0)
		return E_INVALIDARG;

	// Do we have more items to offer
	if (iPosition > 0)
		return VFW_S_NO_MORE_ITEMS;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();
	pMediaType->SetType(&MEDIATYPE_Audio);
	pMediaType->SetSubtype(&MEDIASUBTYPE_PCM);
	pMediaType->SetFormatType(&FORMAT_WaveFormatEx);
	pMediaType->bFixedSizeSamples = TRUE;
	pMediaType->SetTemporalCompression(FALSE);
	pMediaType->SetSampleSize(16);

	WAVEFORMATEX *vihOut = (WAVEFORMATEX *)pMediaType->ReallocFormatBuffer(sizeof(WAVEFORMATEX));
	memcpy(vihOut, &m_out_fmt, sizeof(WAVEFORMATEX));


	return NOERROR;
}

HRESULT CDWindowAudioDownmix::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
	// Is the input pin connected
	if (!m_pInput->IsConnected()) 
		return E_UNEXPECTED;

	HRESULT hr = NOERROR;

	CMediaType *inMediaType = &m_pInput->CurrentMediaType();

	pProperties->cBuffers = 4;
	pProperties->cbBuffer = 307200;

	ALLOCATOR_PROPERTIES Actual;
	hr = pAlloc->SetProperties(pProperties,&Actual);
	if (FAILED(hr)) return hr;
	if (pProperties->cBuffers > Actual.cBuffers || pProperties->cbBuffer > Actual.cbBuffer) 
		return E_FAIL;


	return NOERROR;
}

inline void CDWindowAudioDownmix::mix_a_sample_PCM(void* in, void *out)
{
	short* src = (short*)in;
	short L = src[0];
	short R = src[1];
	short C = src[2];
	short LFE = src[3];
	short BL = src[4];
	short BR = src[5];

	short *dst = (short*)out;
	dst[0] = (short) ((L*0.2929 + C*0.2071 + BL*0.2929 + LFE*0.2071));
	dst[1] = (short) ((R*0.2929 + C*0.2071 + BR*0.2929 + LFE*0.2071));
}

inline void CDWindowAudioDownmix::copy_a_sample_PCM(void* in, void *out)
{
	short * src = (short*)in;
	short *dst = (short*)out;

	dst[0] = src[0];
	dst[1] = src[1];
}

inline void CDWindowAudioDownmix::mix_a_sample_PCM_24bit(void* in, void *out)
{
	short L = (*(short*)((BYTE*)in+1+0*3));
	short R = (*(short*)((BYTE*)in+1+1*3));
	short C = (*(short*)((BYTE*)in+1+2*3));
	short LFE = (*(short*)((BYTE*)in+1+3*3));
	short BL = (*(short*)((BYTE*)in+1+4*3));
	short BR = (*(short*)((BYTE*)in+1+5*3));

	short *dst = (short*)out;
	dst[0] = (short) ((L*0.2929 + C*0.2071 + BL*0.2929 + LFE*0.2071));
	dst[1] = (short) ((R*0.2929 + C*0.2071 + BR*0.2929 + LFE*0.2071));
}

inline void CDWindowAudioDownmix::copy_a_sample_PCM_24bit(void* in, void *out)
{
	short *dst = (short*)out;
	dst[0] = (*(short*)((BYTE*)in+1+0*3));
	dst[1] = (*(short*)((BYTE*)in+1+1*3));
}

inline void CDWindowAudioDownmix::copy_a_sample_PCM_8bit(void* in, void *out)
{
	BYTE * src = (BYTE*)in;
	short *dst = (short*)out;

	dst[0] = (((int)src[0])-128) * 256;
	dst[1] = (((int)src[1])-128) * 256;
}

inline float clip_float(float in)
{
	if (in<-1)
		return -1;
	if (in>1)
		return 1;
	return in;
}

inline void CDWindowAudioDownmix::mix_a_sample_FLOAT(void* in, void *out)
{
	float * src = (float*)in;
	float L = clip_float(src[0]);
	float R = clip_float(src[1]);
	float C = clip_float(src[2]);
	float LFE = clip_float(src[3]);
	float BL = clip_float(src[4]);
	float BR = clip_float(src[5]);

	signed short *dst = (signed short*)out;
	dst[0] = (signed short) (32767*(L*0.2929 + C*0.2071 + BL*0.2929 + LFE*0.2071));
	dst[1] = (signed short) (32767*(R*0.2929 + C*0.2071 + BR*0.2929 + LFE*0.2071));
}

inline void CDWindowAudioDownmix::copy_a_sample_FLOAT(void* in, void *out)
{
	float * src = (float*)in;
	signed short *dst = (signed short*)out;

	dst[0] = (signed short) (clip_float(src[0]) * 32767);
	dst[1] = (signed short) (clip_float(src[1]) * 32767);
}


#define do_convert(src, dst, func) \
	for(int i=0; i<sample_count; i++)\
	{\
		func(src, dst);\
		src += m_in_fmt.nBlockAlign;\
		dst += m_out_fmt.nBlockAlign;\
	}\

HRESULT CDWindowAudioDownmix::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
	BYTE *src;
	BYTE *dst;

	pIn->GetPointer(&src);
	pOut->GetPointer(&dst);

	int sample_count = pIn->GetActualDataLength() / m_in_fmt.nBlockAlign;
	pOut->SetActualDataLength(sample_count * m_out_fmt.nBlockAlign);

	if (m_in_subtype == MEDIASUBTYPE_IEEE_FLOAT)
	{
		if (m_in_fmt.nChannels >= 6)
		{
			do_convert(src, dst, mix_a_sample_FLOAT);
		}
		else
		{
			do_convert(src, dst, copy_a_sample_FLOAT);
		}
	}
	else
	{
		if (m_in_fmt.nChannels >= 6)
		{
			if (m_in_fmt.wBitsPerSample == 16)
			{
				do_convert(src, dst, mix_a_sample_PCM);
			}
			else if (m_in_fmt.wBitsPerSample == 24)
			{
				do_convert(src, dst, mix_a_sample_PCM_24bit);
			}
		}
		else
		{
			if (m_in_fmt.wBitsPerSample == 16)
			{
				do_convert(src, dst, copy_a_sample_PCM);
			}
			else if (m_in_fmt.wBitsPerSample == 24)
			{
				do_convert(src, dst, copy_a_sample_PCM_24bit);
			}
			else if (m_in_fmt.wBitsPerSample == 8)
			{
				do_convert(src, dst, copy_a_sample_PCM_8bit);
			}
		}
	}
		

	REFERENCE_TIME TimeStart, TimeEnd;
	LONGLONG MediaStart, MediaEnd;
	if (SUCCEEDED(pIn->GetMediaTime(&MediaStart,&MediaEnd)))
		pOut->SetMediaTime(&MediaStart,&MediaEnd);
	if (SUCCEEDED(pIn->GetTime(&TimeStart, &TimeEnd)))
		pOut->SetTime(&TimeStart, &TimeEnd);
	pOut->SetSyncPoint(pIn->IsSyncPoint() == S_OK);
	pOut->SetPreroll(pIn->IsPreroll() == S_OK);
	pOut->SetDiscontinuity(pIn->IsDiscontinuity() == S_OK);

	return S_OK;
}