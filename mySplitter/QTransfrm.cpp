#include "QTransfrm.h"

// destructor

CQTransformOutputPin::~CQTransformOutputPin()
{
    DbgLog((LOG_TRACE,2,TEXT("CQTransformOutputPin::~CQTransformOutputPin")));

	if(m_pOutputQueue)
	{
		delete m_pOutputQueue;
		m_pOutputQueue = NULL;
	}
}

HRESULT CQTransformOutputPin::Active()
{
	CAutoLock lock_it(m_pLock);
	HRESULT hr = NOERROR;

	// Make sure that the pin is connected
	if(m_Connected == NULL)
		return NOERROR;

	// Create the output queue if we have to
	if(m_pOutputQueue == NULL)
	{
		m_pOutputQueue = new COutputQueue(m_Connected, &hr, TRUE);
		if(m_pOutputQueue == NULL)
			return E_OUTOFMEMORY;

		// Make sure that the constructor did not return any error
		if(FAILED(hr))
		{
			delete m_pOutputQueue;
			m_pOutputQueue = NULL;
			return hr;
		}
	}

	// Pass the call on to the base class
	CBaseOutputPin::Active();
	return NOERROR;

} // Active


//
// Inactive
//
// This is called when we stop streaming
// We delete the output queue at this time
//
HRESULT CQTransformOutputPin::Inactive()
{
	CAutoLock lock_it(m_pLock);

	// Delete the output queus associated with the pin.
	if(m_pOutputQueue)
	{
		delete m_pOutputQueue;
		m_pOutputQueue = NULL;
	}

	CBaseOutputPin::Inactive();
	return NOERROR;

} // Inactive


//
// Deliver
//
HRESULT CQTransformOutputPin::Deliver(IMediaSample *pMediaSample)
{
	CheckPointer(pMediaSample,E_POINTER);

	// Make sure that we have an output queue
	if(m_pOutputQueue == NULL)
		return NOERROR;

	pMediaSample->AddRef();
	return m_pOutputQueue->Receive(pMediaSample);

} // Deliver


//
// DeliverEndOfStream
//
HRESULT CQTransformOutputPin::DeliverEndOfStream()
{
	// Make sure that we have an output queue
	if(m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->EOS();
	return NOERROR;

} // DeliverEndOfStream


//
// DeliverBeginFlush
//
HRESULT CQTransformOutputPin::DeliverBeginFlush()
{
	// Make sure that we have an output queue
	if(m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->BeginFlush();
	return NOERROR;

} // DeliverBeginFlush


//
// DeliverEndFlush
//
HRESULT CQTransformOutputPin::DeliverEndFlush()
{
	// Make sure that we have an output queue
	if(m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->EndFlush();
	return NOERROR;

} // DeliverEndFlish

//
// DeliverNewSegment
//
HRESULT CQTransformOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, 
										 REFERENCE_TIME tStop,  
										 double dRate)
{
	// Make sure that we have an output queue
	if(m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->NewSegment(tStart, tStop, dRate);
	return NOERROR;

} // DeliverNewSegment

CBasePin *
CQTransformFilter::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL) {

        m_pInput = new CTransformInputPin(NAME("Transform input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"XForm In");      // Pin name


        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pInput == NULL) {
            return NULL;
        }
        m_pOutput = (CTransformOutputPin *)
		   new CQTransformOutputPin(NAME("Transform output pin"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"XForm Out");   // Pin name


        // Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pOutput == NULL) {
            delete m_pInput;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    } else
    if (n == 1) {
        return m_pOutput;
    } else {
        return NULL;
    }
}


// mostly copied from CTransformFilter
HRESULT CQTransformFilter::Receive(IMediaSample *pSample)
{
	/*  Check for other streams and pass them on */
	AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
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
	hr = InitializeOutputSample(pSample, &pOutSample);

	if (FAILED(hr)) {
		return hr;
	}

	// Start timing the transform (if PERF is defined)
	MSR_START(m_idTransform);

	// have the derived class transform the data

	hr = Transform(pSample, pOutSample);

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