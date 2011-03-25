#include "my12doomSource.h"

DEFINE_GUID(MEDIATYPE_XVID, 0x64697678, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
DEFINE_GUID(SUBTYPE_AAC, 0x000000FF, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);
DEFINE_GUID(SUBTYPE_MP3, 0x00000055, 0x0000, 0x0010, 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71);

// my12doomSource
my12doomSource::my12doomSource(LPUNKNOWN lpunk, HRESULT *phr)
:CSource(NAME("my12doom Source"), lpunk, CLSID_my12doomSource),
m_demuxer(NULL)
{
	m_curfile[0] = NULL;
	ASSERT(phr);
}
my12doomSource::~my12doomSource()
{
	if (m_demuxer)
		delete m_demuxer;
}

STDMETHODIMP my12doomSource::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	if (riid == IID_IFileSourceFilter) 
		return GetInterface((IFileSourceFilter *) this, ppv);

	return __super::NonDelegatingQueryInterface(riid, ppv);
}

CUnknown * WINAPI my12doomSource::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
	ASSERT(phr);

	CUnknown *punk = new my12doomSource(lpunk, phr);
	if(punk == NULL)
	{
		if(phr)
			*phr = E_OUTOFMEMORY;
	}
	return punk;
}

STDMETHODIMP my12doomSource::Load(LPCOLESTR pszFileName, __in_opt const AM_MEDIA_TYPE *pmt)
{
	if (m_curfile[0])
		return E_FAIL;

	wcscpy(m_curfile, pszFileName);

	// demux file
	m_demuxer = new C3dvDemuxer(pszFileName);

	// create pins
	CAutoLock cAutoLock(&m_cStateLock);
	if (m_demuxer->m_stream_count)
	{
		m_paStreams = (CSourceStream **) new my12doomSourceStream*[m_demuxer->m_stream_count];
		HRESULT hr = S_OK;
		for (int i=0; i<m_demuxer->m_stream_count; i++)
		{
			wchar_t *pinname= L"XVID Video";
			if (m_demuxer->m_streams[i].type == 1)
				pinname = L"MP3 Audio";
			else if (m_demuxer->m_streams[i].type == 2)
				pinname = L"AAC Audio";

			m_paStreams[i] = new my12doomSourceStream(&hr, this, pinname, m_demuxer->m_streams+i);
			if(m_paStreams[i] == NULL)
				return E_OUTOFMEMORY;
		}
	}
	return S_OK;
}

STDMETHODIMP my12doomSource::GetCurFile(__out LPOLESTR *ppszFileName, __out_opt AM_MEDIA_TYPE *pmt)
{
	if (m_curfile[0] == NULL)
		return E_FAIL;

	if (!ppszFileName)
		return E_POINTER;

	*ppszFileName = (LPOLESTR)CoTaskMemAlloc(MAX_PATH * sizeof(OLECHAR));
	wcscpy(*ppszFileName, m_curfile);

	return S_OK;
}


// pins
my12doomSourceStream::my12doomSourceStream(HRESULT *phr, my12doomSource *pParent, LPCWSTR pPinName, stream_3dv *stream):
CSourceSeeking(NAME("SeekBall"), (IPin*)this, phr, &m_cSharedState),
CSourceStream(NAME("Bouncing Ball"),phr, pParent, pPinName),
m_max_buffer_size(m_max_buffer_size),
m_stream(stream),
m_stream_start(0)
{
	m_need_I_frame = true;
	m_rtStart = 0;
	m_rtDuration = m_rtStop = (REFERENCE_TIME)m_stream->length * 10000;
 	m_frame_number = 0;
}
my12doomSourceStream::~my12doomSourceStream()
{

}
STDMETHODIMP my12doomSourceStream::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	if( riid == IID_IMediaSeeking ) 
	{
		return CSourceSeeking::NonDelegatingQueryInterface( riid, ppv );
	}
	return CSourceStream::NonDelegatingQueryInterface(riid, ppv);
}

// plots a ball into the supplied video frame
HRESULT my12doomSourceStream::FillBuffer(IMediaSample *pms)
{
	if (m_frame_number >= m_stream->packet_count)
		return S_FALSE;

	DWORD frame_size = m_stream->CBR;
	if (frame_size==0)
		frame_size = m_stream->packet_size[m_frame_number];

	pms->SetActualDataLength(frame_size);

	BYTE *buf = NULL;
	pms->GetPointer(&buf);

	{
		CAutoLock lck(m_stream->file_lock);
		fseek(m_stream->f, m_stream->packet_index[m_frame_number], SEEK_SET);
		fread(buf, 1, frame_size, m_stream->f);
	}

	REFERENCE_TIME rtStart, rtStop;
	rtStart = (REFERENCE_TIME)(m_frame_number+1) * m_stream->length * 10000 / m_stream->packet_count - m_stream_start;
	rtStop = (REFERENCE_TIME)(m_frame_number+2) * m_stream->length * 10000 / m_stream->packet_count - m_stream_start;

	pms->SetTime(&rtStart, &rtStop);

	m_frame_number ++;

	/*
	if (m_need_I_frame)
	{
		for(int i=0; i<frame_size-4; i++)
			if (buf[i] == 0 && buf[i+1] == 0 && buf[i+2]== 1 && buf[i+3] == 0xB6)
			{
				int frame_type = (buf[i+4] >> 6) & 0x3;
				// type: 0=I, 1=P, 2=B, 3=S
				if (frame_type != 0)
				{
					//pms->SetDiscontinuity(TRUE);
					pms->SetActualDataLength(4);
					return S_OK;
				}
			}
	}

	m_need_I_frame = false;
	*/
	return S_OK;
}

// Ask for buffers of the size appropriate to the agreed media type
HRESULT my12doomSourceStream::DecideBufferSize(IMemAllocator *pIMemAlloc,	ALLOCATOR_PROPERTIES *pProperties)
{
	if (pIMemAlloc == NULL)
		return E_POINTER;

	if (pProperties == NULL)
		return E_POINTER;
	CAutoLock cAutoLock(m_pFilter->pStateLock());

	pProperties->cBuffers = 1;
	if (m_stream->CBR)
		pProperties->cbBuffer = m_stream->CBR;
	else
		pProperties->cbBuffer = m_stream->max_packet_size;

	ASSERT(pProperties->cbBuffer);

	// Ask the allocator to reserve us some sample memory, NOTE the function
	// can succeed (that is return NOERROR) but still not have allocated the
	// memory that we requested, so we must check we got whatever we wanted

	ALLOCATOR_PROPERTIES Actual;
	HRESULT hr = pIMemAlloc->SetProperties(pProperties,&Actual);
	if(FAILED(hr))
	{
		return hr;
	}

	// Is this allocator unsuitable

	if(Actual.cbBuffer < pProperties->cbBuffer)
	{
		return E_FAIL;
	}

	// Make sure that we have only 1 buffer (we erase the ball in the
	// old buffer to save having to zero a 200k+ buffer every time
	// we draw a frame)

	ASSERT(Actual.cBuffers == 1);
	return NOERROR;
}

// Set the agreed media type, and set up the necessary ball parameters
HRESULT my12doomSourceStream::SetMediaType(const CMediaType *pMediaType)
{
	// TODO
	return __super::SetMediaType(pMediaType);
}

// Because we calculate the ball there is no reason why we
// can't calculate it in any one of a set of formats...
HRESULT my12doomSourceStream::CheckMediaType(const CMediaType *pMediaType)
{
	// TODO
	return S_OK;
}
HRESULT my12doomSourceStream::GetMediaType(int iPosition, CMediaType *pmt)
{
	if (pmt == NULL)
		return E_POINTER;

	if (iPosition<0)
		return E_INVALIDARG;

	if (iPosition>0)
		return VFW_S_NO_MORE_ITEMS;

	if (m_stream->type == 0)
	{
		pmt->SetType(&MEDIATYPE_Video);
		pmt->SetSubtype(&MEDIATYPE_XVID);
		pmt->SetFormatType(&FORMAT_VideoInfo);
		pmt->bFixedSizeSamples = FALSE;
		pmt->SetTemporalCompression(FALSE);
		pmt->lSampleSize = 1;

		VIDEOINFO *pvi = (VIDEOINFO*) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
		memset(pvi, 0, sizeof(VIDEOINFO));
		pvi->dwBitRate = 0;
		pvi->dwBitErrorRate = 0;
		pvi->AvgTimePerFrame = (REFERENCE_TIME)10000000*m_stream->time_in_units
								/m_stream->time_units/m_stream->packet_count;

		pvi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		pvi->bmiHeader.biWidth = 800;
		pvi->bmiHeader.biHeight = 480;
		pvi->bmiHeader.biPlanes = 1;
		pvi->bmiHeader.biBitCount = 24;
		pvi->bmiHeader.biCompression = MAKEFOURCC('X', 'V', 'I', 'D');
		pvi->bmiHeader.biSizeImage = 2304000;
		pvi->bmiHeader.biXPelsPerMeter = 1;
		pvi->bmiHeader.biYPelsPerMeter = 1;
	}
	else
	{
		pmt->SetType(&MEDIATYPE_Audio);
		pmt->SetFormatType(&FORMAT_WaveFormatEx);
		pmt->SetTemporalCompression(FALSE);
		pmt->lSampleSize = 1;

		int extra_byte = 0;
		if (m_stream->type == 1) extra_byte = 12;
		if (m_stream->type == 2) extra_byte = 2;
		WAVEFORMATEX *pai = (WAVEFORMATEX*) pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX)+extra_byte);
		BYTE* extra_data = ((BYTE*)pai) + sizeof(WAVEFORMATEX);
		memset(pai, 0, sizeof(WAVEFORMATEX)+extra_byte);
		pai->cbSize = extra_byte;
		pai->nAvgBytesPerSec = m_stream->audio_channel * m_stream->audio_bit_depth * m_stream->audio_sample_rate / 8;
		pai->nChannels = m_stream->audio_channel;
		pai->nSamplesPerSec = m_stream->audio_sample_rate;
		pai->wBitsPerSample = m_stream->audio_bit_depth;
		if (m_stream->type == 1)
		{
			pmt->bFixedSizeSamples = FALSE;
			pai->nBlockAlign = 1;
			pmt->SetSubtype(&SUBTYPE_MP3);
			pai->wFormatTag = 0x55;

			extra_data[0] = 0x01;
			extra_data[1] = 0x00;
			extra_data[2] = 0x02;
			extra_data[3] = 0x00;
			extra_data[4] = 0x00;
			extra_data[5] = 0x00;
			extra_data[6] = 0x80;
			extra_data[7] = 0x01;
			extra_data[8] = 0x01;
			extra_data[9] = 0x00;
			extra_data[10] = 0x00;
			extra_data[11] = 0x00;
		}

		else if (m_stream->type == 2)
		{
			pmt->bFixedSizeSamples = FALSE;
			pai->nBlockAlign = 4;
			pmt->SetSubtype(&SUBTYPE_AAC);
			pai->wFormatTag = 0xff;
			extra_data[0] = 0x12;
			extra_data[1] = 0x90;
		}
	}
	return S_OK;
}

// Resets the stream time to zero
HRESULT my12doomSourceStream::OnThreadCreate(void)
{
	// TODO
	return S_OK;
}
// Quality control notifications sent to us
STDMETHODIMP my12doomSourceStream::Notify(IBaseFilter * pSender, Quality q)
{
	return NOERROR;
}

// CSourceSeeking
HRESULT my12doomSourceStream::OnThreadStartPlay()
{
	m_bDiscontinuity = TRUE;
	return DeliverNewSegment(m_rtStart, m_rtStop, m_dRateSeeking);
}
HRESULT my12doomSourceStream::ChangeStart()
{
	{
		CAutoLock lock(CSourceSeeking::m_pLock);
		m_stream_start = m_rtStart;
		m_frame_number = m_rtStart * m_stream->packet_count / m_stream->length / 10000 ;

		// seek to key frame for video
		if (m_stream->type == 0 && m_stream->keyframe_count>0)
		{
			for(int i=0; i<m_stream->keyframe_count; i++)
			{
				if (m_stream->keyframes[i] == m_frame_number)
					goto flush;
			}

			for(int i=0; i<m_stream->keyframe_count-1; i++)
			{
				if (m_stream->keyframes[i] < m_frame_number && 
					m_frame_number < m_stream->keyframes[i+1])
				{
					m_frame_number = m_stream->keyframes[i];
					goto flush;
				}
			}

			m_frame_number = m_stream->keyframes[m_stream->keyframe_count-1];
		}
	}

flush:
	UpdateFromSeek();
	return S_OK;
}
HRESULT my12doomSourceStream::ChangeStop()
{
	{
		CAutoLock lock(CSourceSeeking::m_pLock);
		if ((REFERENCE_TIME)(m_frame_number+1) * m_stream->length * 10000 / m_stream->packet_count < m_rtStop)
		{
			return S_OK;
		}
	}

	// We're already past the new stop time. Flush the graph.
	UpdateFromSeek();
	return S_OK;
}
HRESULT my12doomSourceStream::ChangeRate()
{
	return S_OK;
}
STDMETHODIMP my12doomSourceStream::SetRate(double)
{
	return E_NOTIMPL;
}

void my12doomSourceStream::UpdateFromSeek()
{
	if (ThreadExists()) 
	{
		DeliverBeginFlush();
		// Shut down the thread and stop pushing data.
		Stop();
		DeliverEndFlush();
		m_need_I_frame = true;
		// Restart the thread and start pushing data again.
		Pause();
	}
}