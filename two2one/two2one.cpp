#include "two2one.h"


// Input Pin
// mostly copied from CTransformInputPin
HRESULT C2to1InputPin::Receive(IMediaSample * pSample)
{
	HRESULT hr;
	CAutoLock lck(&((C2to1Filter*)m_pTransformFilter)->m_csReceive);
	ASSERT(pSample);

	// check all is well with the base class
	hr = CBaseInputPin::Receive(pSample);
	if (S_OK == hr) 
	{
		hr = ((C2to1Filter*)m_pTransformFilter)->Receive(pSample, m_id);
	}
	return hr;
}


// Filter
int C2to1Filter::GetPinCount()
{
	return 3;
}
CBasePin *C2to1Filter::GetPin(int n)
{
	HRESULT hr = S_OK;

	// Create input pin if necessary
	if (m_pInput == NULL) 
	{
		m_pInput = new C2to1InputPin(NAME("Transform input pin"),
			this,              // Owner filter
			&hr,               // Result code
			L"XForm In", 1);      // Pin name

		m_pInput2 = new C2to1InputPin(NAME("Transform input pin"),
			this,              // Owner filter
			&hr,               // Result code
			L"XForm In2", 2);      // Pin name

		m_pOutput = (CTransformOutputPin *)
			new CQTransformOutputPin(NAME("Transform output pin"),
			this,            // Owner filter
			&hr,             // Result code
			L"XForm Out");   // Pin name
	}

	// Return the appropriate pin

	if (n == 0) 
	{
		return m_pInput;
	} 
	else if (n == 1) 
	{
		return m_pInput2;
	} 
	else if (n == 2)
	{
		return m_pOutput;
	}
	else
	{
		return NULL;
	}
}


STDMETHODIMP C2to1Filter::FindPin(LPCWSTR Id, __deref_out IPin **ppPin)
{
	CheckPointer(ppPin,E_POINTER);
	ValidateReadWritePtr(ppPin,sizeof(IPin *));

	if (0==lstrcmpW(Id,L"In2"))
	{
		*ppPin = GetPin(2);
	} 
	if (0==lstrcmpW(Id,L"In"))
	{
		*ppPin = GetPin(0);
	} 
	else if (0==lstrcmpW(Id,L"Out"))
	{
		*ppPin = GetPin(1);
	}
	else 
	{
		*ppPin = NULL;
		return VFW_E_NOT_FOUND;
	}

	HRESULT hr = NOERROR;
	//  AddRef() returned pointer - but GetPin could fail if memory is low.
	if (*ppPin) 
	{
		(*ppPin)->AddRef();
	} 
	else 
	{
		hr = E_OUTOFMEMORY;  // probably.  There's no pin anyway.
	}
	return hr;
}

// mostly copied from CTransformFilter
HRESULT C2to1Filter::Receive(IMediaSample *pSample, int id)
{
	/*  Check for other streams and pass them on */
	AM_SAMPLE2_PROPERTIES * pProps;
	if (id == 1)
		pProps = m_pInput->SampleProps();
	else
		pProps = m_pInput2->SampleProps();

	if (pProps->dwStreamId != AM_STREAM_MEDIA) 
	{
		return m_pOutput->Deliver(pSample);
	}
	HRESULT hr;
	ASSERT(pSample);
	IMediaSample * pOutSample;

	// If no output to deliver to then no point sending us data

	ASSERT (m_pOutput != NULL) ;

	// Set up the output sample
	hr = InitializeOutputSample(pSample, &pOutSample, id);

	if (FAILED(hr)) {
		return hr;
	}

	// Start timing the transform (if PERF is defined)
	MSR_START(m_idTransform);

	// have the derived class transform the data

	hr = Transform(pSample, pOutSample, id);

	// Stop the clock and log it (if PERF is defined)
	MSR_STOP(m_idTransform);

	if (FAILED(hr)) {
		DbgLog((LOG_TRACE,1,TEXT("Error from transform")));
	} else {
		// the Transform() function can return S_FALSE to indicate that the
		// sample should not be delivered; we only deliver the sample if it's
		// really S_OK (same as NOERROR, of course.)
		if (hr == NOERROR) {
			hr = m_pOutput->Deliver(pOutSample);
			m_bSampleSkipped = FALSE;	// last thing no longer dropped
		} else {
			// S_FALSE returned from Transform is a PRIVATE agreement
			// We should return NOERROR from Receive() in this cause because returning S_FALSE
			// from Receive() means that this is the end of the stream and no more data should
			// be sent.
			if (S_FALSE == hr) {

				//  Release the sample before calling notify to avoid
				//  deadlocks if the sample holds a lock on the system
				//  such as DirectDraw buffers do
				pOutSample->Release();
				m_bSampleSkipped = TRUE;
				if (!m_bQualityChanged) {
					NotifyEvent(EC_QUALITY_CHANGE,0,0);
					m_bQualityChanged = TRUE;
				}
				return NOERROR;
			}
		}
	}

	// release the output buffer. If the connected pin still needs it,
	// it will have addrefed it itself.
	pOutSample->Release();

	return hr;
}

// Set up our output sample
HRESULT C2to1Filter::InitializeOutputSample(IMediaSample *pSample, __deref_out IMediaSample **ppOutSample, int id)
{
	if (id == 1)
	{
		return __super::InitializeOutputSample(pSample, ppOutSample);
	}

	if (id == 2)
	{
		CTransformInputPin *tmp = m_pInput;
		m_pInput = m_pInput2;

		HRESULT hr = __super::InitializeOutputSample(pSample, ppOutSample);
		m_pInput = tmp;
		return hr;
	}
}

// Transform place holder - should never be called
HRESULT C2to1Filter::Transform(IMediaSample * pIn, IMediaSample *pOut, int id)
{
	UNREFERENCED_PARAMETER(pIn);
	UNREFERENCED_PARAMETER(pOut);
	DbgBreak("CTransformFilter::Transform() should never be called");
	return E_UNEXPECTED;
}