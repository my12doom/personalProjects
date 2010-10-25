//------------------------------------------------------------------------------
// File: Transfrm.cpp
//
// Desc: DirectShow base classes - implements class for simple Split
//       filters such as video decompressors.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


#include <streams.h>
#include <measure.h>

#include "splitbase.h"

#define SAFE_RELEASE(p) { if ( (p) ) { (p)->Release(); (p) = 0; } }

// =================================================================
// Implements the CSplitFilter class
// =================================================================

CSplitFilter::CSplitFilter(TCHAR     *pName,
                                   LPUNKNOWN pUnk,
                                   REFCLSID  clsid) :
    CBaseFilter(pName,pUnk,&m_csFilter, clsid),
    m_pInput(NULL),
    m_pOutput1(NULL),
	m_pOutput2(NULL),
    m_bEOSDelivered(FALSE),
    m_bQualityChanged(FALSE),
    m_bSampleSkipped(FALSE),
	m_pAllocator(NULL)
{
#ifdef PERF
    RegisterPerfId();
#endif //  PERF
}

#ifdef UNICODE
CSplitFilter::CSplitFilter(char     *pName,
                                   LPUNKNOWN pUnk,
                                   REFCLSID  clsid) :
    CBaseFilter(pName,pUnk,&m_csFilter, clsid),
    m_pInput(NULL),
    m_pOutput1(NULL),
	m_pOutput2(NULL),
    m_bEOSDelivered(FALSE),
    m_bQualityChanged(FALSE),
    m_bSampleSkipped(FALSE),
	m_pAllocator(NULL)
{
#ifdef PERF
    RegisterPerfId();
#endif //  PERF
}
#endif

// destructor

CSplitFilter::~CSplitFilter()
{
    // Delete the pins

    delete m_pInput;
    delete m_pOutput1;
	delete m_pOutput2;
}


// Split place holder - should never be called
HRESULT CSplitFilter::Split(IMediaSample * pIn, IMediaSample *pOut1, IMediaSample *pOut2)
{
    UNREFERENCED_PARAMETER(pIn);
    UNREFERENCED_PARAMETER(pOut1);
    DbgBreak("CSplitFilter::Split() should never be called");
    return E_UNEXPECTED;
}


// return the number of pins we provide

int CSplitFilter::GetPinCount()
{
    return 3;
}


// return a non-addrefed CBasePin * for the user to addref if he holds onto it
// for longer than his pointer to us. We create the pins dynamically when they
// are asked for rather than in the constructor. This is because we want to
// give the derived class an oppportunity to return different pin objects

// We return the objects as and when they are needed. If either of these fails
// then we return NULL, the assumption being that the caller will realise the
// whole deal is off and destroy us - which in turn will delete everything.

CBasePin *
CSplitFilter::GetPin(int n)
{
    HRESULT hr = S_OK;

    // Create an input pin if necessary

    if (m_pInput == NULL) {

        m_pInput = new CSplitInputPin(NAME("Split input pin"),
                                          this,              // Owner filter
                                          &hr,               // Result code
                                          L"Video In");      // Pin name


        //  Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pInput == NULL) {
            return NULL;
        }

        m_pOutput1 = (CSplitOutputPin *)
		   new CSplitOutputPin(NAME("Split output pin1"),
                                            this,            // Owner filter
                                            &hr,             // Result code
                                            L"Video Out 1");   // Pin name

		m_pOutput2 = (CSplitOutputPin *)
			new CSplitOutputPin(NAME("Split output pin2"),
			this,            // Owner filter
			&hr,             // Result code
			L"Video Out 2");   // Pin name

        // Can't fail
        ASSERT(SUCCEEDED(hr));
        if (m_pOutput2 == NULL) {
            delete m_pInput;
			delete m_pOutput1;
            m_pInput = NULL;
        }
    }

    // Return the appropriate pin

    if (n == 0) {
        return m_pInput;
    } else
    if (n == 1) {
        return m_pOutput1;
    }
	if (n == 2) {
		return m_pOutput2;
	}
	else {
        return NULL;
    }
}


//
// FindPin
//
// If Id is In or Out then return the IPin* for that pin
// creating the pin if need be.  Otherwise return NULL with an error.

STDMETHODIMP CSplitFilter::FindPin(LPCWSTR Id, IPin **ppPin)
{
    CheckPointer(ppPin,E_POINTER);
    ValidateReadWritePtr(ppPin,sizeof(IPin *));

    if (0==lstrcmpW(Id,L"In")) {
        *ppPin = GetPin(0);
    } else if (0==lstrcmpW(Id,L"Out1")) {
        *ppPin = GetPin(1);
	} else if (0==lstrcmpW(Id,L"Out2")) {
		*ppPin = GetPin(2);
	} else {
        *ppPin = NULL;
        return VFW_E_NOT_FOUND;
    }

    HRESULT hr = NOERROR;
    //  AddRef() returned pointer - but GetPin could fail if memory is low.
    if (*ppPin) {
        (*ppPin)->AddRef();
    } else {
        hr = E_OUTOFMEMORY;  // probably.  There's no pin anyway.
    }
    return hr;
}


// override these two functions if you want to inform something
// about entry to or exit from streaming state.

HRESULT
CSplitFilter::StartStreaming()
{
    return NOERROR;
}


HRESULT
CSplitFilter::StopStreaming()
{
    return NOERROR;
}


// override this to grab extra interfaces on connection

HRESULT
CSplitFilter::CheckConnect(PIN_DIRECTION dir,IPin *pPin)
{
    UNREFERENCED_PARAMETER(dir);
    UNREFERENCED_PARAMETER(pPin);
    return NOERROR;
}


// place holder to allow derived classes to release any extra interfaces

HRESULT
CSplitFilter::BreakConnect(PIN_DIRECTION dir)
{
    UNREFERENCED_PARAMETER(dir);
    return NOERROR;
}


// Let derived classes know about connection completion

HRESULT
CSplitFilter::CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin)
{
    UNREFERENCED_PARAMETER(direction);
    UNREFERENCED_PARAMETER(pReceivePin);
    return NOERROR;
}


// override this to know when the media type is really set

HRESULT
CSplitFilter::SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt)
{
    UNREFERENCED_PARAMETER(direction);
    UNREFERENCED_PARAMETER(pmt);
    return NOERROR;
}


// Set up our output sample
HRESULT
CSplitFilter::InitializeOutputSample(IMediaSample *pSample, IMediaSample **ppOutSample1, IMediaSample **ppOutSample2 )
{


    IMediaSample *pOutSample1 = NULL;
	IMediaSample *pOutSample2 = NULL;

    // default - times are the same

    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
    const DWORD dwFlags = m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0;

    // This will prevent the image renderer from switching us to DirectDraw
    // when we can't do it without skipping frames because we're not on a
    // keyframe.  If it really has to switch us, it still will, but then we
    // will have to wait for the next keyframe

	// this is ppOutSample1 generate
	if (m_pOutput1->IsConnected())
	{
		if (!(pProps->dwSampleFlags & AM_SAMPLE_SPLICEPOINT)) {
		//dwFlags |= AM_GBF_NOTASYNCPOINT;
		}

		ASSERT(m_pOutput1->m_pAllocator != NULL);

		m_pOutput1->m_pAllocator->Commit();

		HRESULT hr = m_pOutput1->m_pAllocator->GetBuffer(
				&pOutSample1
				, pProps->dwSampleFlags & AM_SAMPLE_TIMEVALID ?
					&pProps->tStart : NULL
				, pProps->dwSampleFlags & AM_SAMPLE_STOPVALID ?
					&pProps->tStop : NULL
				, dwFlags
			);
		if (FAILED(hr)) {
			return hr;
		}

		ASSERT(pOutSample1);
		IMediaSample2 *pOutSample12;
		if (SUCCEEDED(pOutSample1->QueryInterface(IID_IMediaSample2,
												(void **)&pOutSample12))) {
			/*  Modify it */
			AM_SAMPLE2_PROPERTIES OutProps;
			EXECUTE_ASSERT(SUCCEEDED(pOutSample12->GetProperties(
				FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, tStart), (PBYTE)&OutProps)
			));
			OutProps.dwTypeSpecificFlags = pProps->dwTypeSpecificFlags;
			OutProps.dwSampleFlags =
				(OutProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED) |
				(pProps->dwSampleFlags & ~AM_SAMPLE_TYPECHANGED);
			OutProps.tStart = pProps->tStart;
			OutProps.tStop  = pProps->tStop;
			OutProps.cbData = FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId);
			hr = pOutSample12->SetProperties(
				FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId),
				(PBYTE)&OutProps
			);
			if (pProps->dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
				m_bSampleSkipped = FALSE;
			}
			pOutSample12->Release();
		} 
		else {
			if (pProps->dwSampleFlags & AM_SAMPLE_TIMEVALID) {
				pOutSample1->SetTime(&pProps->tStart,
									&pProps->tStop);
			}
			if (pProps->dwSampleFlags & AM_SAMPLE_SPLICEPOINT) {
				pOutSample1->SetSyncPoint(TRUE);
			}
			if (pProps->dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
				pOutSample1->SetDiscontinuity(TRUE);
				m_bSampleSkipped = FALSE;
			}
			// Copy the media times

			LONGLONG MediaStart, MediaEnd;
			if (pSample->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
				pOutSample1->SetMediaTime(&MediaStart,&MediaEnd);
			}
		}
	}

	// this is ppOutSample2 generate
	// default - times are the same
	//dwFlags = m_bSampleSkipped ? AM_GBF_PREVFRAMESKIPPED : 0;
	if (m_pOutput2->IsConnected())
	{

		if (!(pProps->dwSampleFlags & AM_SAMPLE_SPLICEPOINT)) {
	//		dwFlags |= AM_GBF_NOTASYNCPOINT;
		}

		ASSERT(m_pOutput2->m_pAllocator != NULL);

		m_pOutput2->m_pAllocator->Commit();
		HRESULT hr = m_pOutput2->m_pAllocator->GetBuffer(
			&pOutSample2
			, pProps->dwSampleFlags & AM_SAMPLE_TIMEVALID ?
			&pProps->tStart : NULL
			, pProps->dwSampleFlags & AM_SAMPLE_STOPVALID ?
			&pProps->tStop : NULL
			, dwFlags
			);
		if (FAILED(hr)) {
			return hr;
		}

		ASSERT(pOutSample2);
		IMediaSample2 *pOutSample22;
		if (SUCCEEDED(pOutSample2->QueryInterface(IID_IMediaSample2,
			(void **)&pOutSample22))) 
		{
			/*  Modify it */
			AM_SAMPLE2_PROPERTIES OutProps;
			EXECUTE_ASSERT(SUCCEEDED(pOutSample22->GetProperties(
				FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, tStart), (PBYTE)&OutProps)
				));
			OutProps.dwTypeSpecificFlags = pProps->dwTypeSpecificFlags;
			OutProps.dwSampleFlags =
				(OutProps.dwSampleFlags & AM_SAMPLE_TYPECHANGED) |
				(pProps->dwSampleFlags & ~AM_SAMPLE_TYPECHANGED);
			OutProps.tStart = pProps->tStart;
			OutProps.tStop  = pProps->tStop;
			OutProps.cbData = FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId);
			hr = pOutSample22->SetProperties(
				FIELD_OFFSET(AM_SAMPLE2_PROPERTIES, dwStreamId),
				(PBYTE)&OutProps
				);
			if (pProps->dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
				m_bSampleSkipped = FALSE;
			}
			pOutSample22->Release();
		} else {
			if (pProps->dwSampleFlags & AM_SAMPLE_TIMEVALID) {
				pOutSample2->SetTime(&pProps->tStart,
					&pProps->tStop);
			}
			if (pProps->dwSampleFlags & AM_SAMPLE_SPLICEPOINT) {
				pOutSample2->SetSyncPoint(TRUE);
			}
			if (pProps->dwSampleFlags & AM_SAMPLE_DATADISCONTINUITY) {
				pOutSample2->SetDiscontinuity(TRUE);
				m_bSampleSkipped = FALSE;
			}
			// Copy the media times

			LONGLONG MediaStart, MediaEnd;
			if (pSample->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR) {
				pOutSample2->SetMediaTime(&MediaStart,&MediaEnd);
			}
		}
	}
	*ppOutSample1 = pOutSample1;
	*ppOutSample2 = pOutSample2;
    return S_OK;
}

// override this to customize the Split process

HRESULT
CSplitFilter::Receive(IMediaSample *pSample)
{
	HRESULT hr;
    /*  Check for other streams and pass them on */
    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
    if (pProps->dwStreamId != AM_STREAM_MEDIA) {
		hr =  m_pOutput1->Deliver(pSample);
		hr =  m_pOutput2->Deliver(pSample);
		return hr;
    }
    ASSERT(pSample);
    IMediaSample * pOutSample1 = NULL;
	IMediaSample * pOutSample2 = NULL;

    // If no output to deliver to then no point sending us data

    ASSERT (m_pOutput1 != NULL || m_pOutput2 != NULL) ;

    // Set up the output sample
    //hr = InitializeOutputSample(pSample, &pOutSample1, &pOutSample2);
	hr = m_pOutput1->GetDeliveryBuffer(&pOutSample1, NULL, NULL, 0);
	if (FAILED(hr)) {
		pOutSample1 = NULL;
	}

	m_pOutput2->GetDeliveryBuffer(&pOutSample2, NULL, NULL, 0);
	if (FAILED(hr)) {
		pOutSample2 = NULL;
	}

    // Start timing the Split (if PERF is defined)
    MSR_START(m_idSplit);

    // have the derived class Split the data

    hr = Split(pSample, pOutSample1, pOutSample2);
	
    // Stop the clock and log it (if PERF is defined)
    MSR_STOP(m_idSplit);

    if (FAILED(hr)) {
	DbgLog((LOG_TRACE,1,TEXT("Error from Split")));
    } else {
        // the Split() function can return S_FALSE to indicate that the
        // sample should not be delivered; we only deliver the sample if it's
        // really S_OK (same as NOERROR, of course.)
        if (hr == NOERROR) {
    	    if (pOutSample1) hr = m_pOutput1->Deliver(pOutSample1);
			if (pOutSample2) hr = m_pOutput2->Deliver(pOutSample2);
            m_bSampleSkipped = FALSE;	// last thing no longer dropped
        }
		else if(hr == S_LEFT)
		{
    	    if (pOutSample1) hr = m_pOutput1->Deliver(pOutSample1);
            m_bSampleSkipped = FALSE;	// last thing no longer dropped
		}
		else if(hr == S_RIGHT)
		{
    	    if (pOutSample2) hr = m_pOutput2->Deliver(pOutSample2);
            m_bSampleSkipped = FALSE;	// last thing no longer dropped
		}
		else
		{
            // S_FALSE returned from Split is a PRIVATE agreement
            // We should return NOERROR from Receive() in this cause because returning S_FALSE
            // from Receive() means that this is the end of the stream and no more data should
            // be sent.
            if (S_FALSE == hr) {

                //  Release the sample before calling notify to avoid
                //  deadlocks if the sample holds a lock on the system
                //  such as DirectDraw buffers do
				SAFE_RELEASE(pOutSample1);
				SAFE_RELEASE(pOutSample2);
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
	SAFE_RELEASE(pOutSample1);
	SAFE_RELEASE(pOutSample2);

    return hr;
}


// Return S_FALSE to mean "pass the note on upstream"
// Return NOERROR (Same as S_OK)
// to mean "I've done something about it, don't pass it on"
HRESULT CSplitFilter::AlterQuality(Quality q)
{
    UNREFERENCED_PARAMETER(q);
    return S_FALSE;
}


// EndOfStream received. Default behaviour is to deliver straight
// downstream, since we have no queued data. If you overrode Receive
// and have queue data, then you need to handle this and deliver EOS after
// all queued data is sent
HRESULT
CSplitFilter::EndOfStream(void)
{
    HRESULT hr = NOERROR;
    if (m_pOutput1 != NULL) {
        hr = m_pOutput1->DeliverEndOfStream();
    }
	if (m_pOutput2 != NULL) {
		hr = m_pOutput2->DeliverEndOfStream();
	}

    return hr;
}


// enter flush state. Receives already blocked
// must override this if you have queued data or a worker thread
HRESULT
CSplitFilter::BeginFlush(void)
{
	// block receives -- done by caller (CBaseInputPin::BeginFlush)
	// discard queued data -- we have no queued data
	// free anyone blocked on receive - not possible in this filter
	// call downstream
    HRESULT hr = NOERROR;
    if (m_pOutput1 != NULL) {
		hr = m_pOutput1->DeliverBeginFlush();
    }
	if (m_pOutput2 != NULL) {
		hr = m_pOutput2->DeliverBeginFlush();
	}

    return hr;
}


// leave flush state. must override this if you have queued data
// or a worker thread
HRESULT
CSplitFilter::EndFlush(void)
{
    // sync with pushing thread -- we have no worker thread
    // ensure no more data to go downstream -- we have no queued data
    // call EndFlush on downstream pins
    // caller (the input pin's method) will unblock Receives

    ASSERT (m_pOutput1 != NULL || m_pOutput2 != NULL);

    HRESULT hr = m_pOutput1->DeliverEndFlush();

	hr = m_pOutput2->DeliverEndFlush();

	return hr;


}


// override these so that the derived filter can catch them

STDMETHODIMP
CSplitFilter::Stop()
{
	if (m_State == State_Running)
		Pause();
    CAutoLock lck1(&m_csFilter);
	CAutoLock lock_it(m_pLock);
    if (m_State == State_Stopped) {
        return NOERROR;
    }

    // Succeed the Stop if we are not completely connected

    ASSERT(m_pInput == NULL || (m_pOutput1 != NULL || m_pOutput2 != NULL));

    if (m_pInput == NULL || m_pInput->IsConnected() == FALSE ||
        (m_pOutput1->IsConnected() == FALSE && m_pOutput2->IsConnected()) == FALSE) {			// note, ÓÃµÄOR
                m_State = State_Stopped;
                m_bEOSDelivered = FALSE;
                return NOERROR;
    }

    ASSERT(m_pInput);
    ASSERT(m_pOutput1 != NULL || m_pOutput2 != NULL);

    // decommit the input pin before locking or we can deadlock
	if (m_pInput)
		m_pInput->Inactive();

    // synchronize with Receive calls

    CAutoLock lck2(&m_csReceive);
	if(m_pOutput1)
		m_pOutput1->Inactive();
	if(m_pOutput2)
		m_pOutput2->Inactive();

    // allow a class derived from CSplitFilter
    // to know about starting and stopping streaming

    HRESULT hr = StopStreaming();
    if (SUCCEEDED(hr)) {
	// complete the state transition
	m_State = State_Stopped;
	m_bEOSDelivered = FALSE;
    }
    return hr;
}


STDMETHODIMP
CSplitFilter::Pause()
{
    CAutoLock lck(&m_csFilter);
	CAutoLock lck2(m_pLock);
    HRESULT hr = NOERROR;

    if (m_State == State_Paused) {
        // (This space left deliberately blank)
    }

    // If we have no input pin or it isn't yet connected then when we are
    // asked to pause we deliver an end of stream to the downstream filter.
    // This makes sure that it doesn't sit there forever waiting for
    // samples which we cannot ever deliver without an input connection.

    else if (m_pInput == NULL || m_pInput->IsConnected() == FALSE) {
        if ( (m_pOutput1 || m_pOutput2) && m_bEOSDelivered == FALSE) {
			if (m_pOutput1)
				m_pOutput1->DeliverEndOfStream();
			if (m_pOutput2)
				m_pOutput2->DeliverEndOfStream();
            m_bEOSDelivered = TRUE;
        }
        m_State = State_Paused;
    }

    // We may have an input connection but no output connection
    // However, if we have an input pin we do have an output pin

    else if ( (m_pOutput1->IsConnected() || m_pOutput2->IsConnected())  == FALSE) {
        m_State = State_Paused;
    }

    else {
	if (m_State == State_Stopped) {
	    // allow a class derived from CSplitFilter
	    // to know about starting and stopping streaming
		/*
		*/
		if (m_pOutput1 && m_pOutput1->m_pAllocator) m_pOutput1->m_pAllocator->Decommit();
		if (m_pOutput2 && m_pOutput2->m_pAllocator) m_pOutput2->m_pAllocator->Decommit();
		CAutoLock lck3(&m_csReceive);
		hr = StartStreaming();
		if (m_pOutput1 && m_pOutput1->m_pAllocator) m_pOutput1->m_pAllocator->Decommit();
		if (m_pOutput2 && m_pOutput2->m_pAllocator) m_pOutput2->m_pAllocator->Decommit();
	}
	if (SUCCEEDED(hr)) {
	    hr = CBaseFilter::Pause();
	}
    }

	/*
	BeginFlush();
	EndFlush();
	*/

    m_bSampleSkipped = FALSE;
    m_bQualityChanged = FALSE;
    return hr;
}

HRESULT
CSplitFilter::NewSegment(
    REFERENCE_TIME tStart,
    REFERENCE_TIME tStop,
    double dRate)
{
    if (m_pOutput1 != NULL) {
        HRESULT hr = m_pOutput1->DeliverNewSegment(tStart, tStop, dRate);
    }
	if (m_pOutput2 != NULL) {
		HRESULT hr = m_pOutput2->DeliverNewSegment(tStart, tStop, dRate);
	}
    return S_OK;
}

// Check streaming status
HRESULT
CSplitInputPin::CheckStreaming()
{
    ASSERT(m_pSplitFilter->m_pOutput1 != NULL || m_pSplitFilter->m_pOutput2 != NULL);
    if (!m_pSplitFilter->m_pOutput1->IsConnected() && !m_pSplitFilter->m_pOutput2->IsConnected()) {
        return VFW_E_NOT_CONNECTED;
    } else {
        //  Shouldn't be able to get any data if we're not connected!
        ASSERT(IsConnected());

        //  we're flushing
        if (m_bFlushing) {
            return S_FALSE;
        }
        //  Don't process stuff in Stopped state
        if (IsStopped()) {
            return VFW_E_WRONG_STATE;
        }
        if (m_bRunTimeError) {
    	    return VFW_E_RUNTIME_ERROR;
        }
        return S_OK;
    }
}


// =================================================================
// Implements the CSplitInputPin class
// =================================================================


// constructor

CSplitInputPin::CSplitInputPin(
    TCHAR *pObjectName,
    CSplitFilter *pSplitFilter,
    HRESULT * phr,
    LPCWSTR pName)
    : CBaseInputPin(pObjectName, pSplitFilter, &pSplitFilter->m_csFilter, phr, pName)
{
    DbgLog((LOG_TRACE,2,TEXT("CSplitInputPin::CSplitInputPin")));
    m_pSplitFilter = pSplitFilter;
}

#ifdef UNICODE
CSplitInputPin::CSplitInputPin(
    CHAR *pObjectName,
    CSplitFilter *pSplitFilter,
    HRESULT * phr,
    LPCWSTR pName)
    : CBaseInputPin(pObjectName, pSplitFilter, &pSplitFilter->m_csFilter, phr, pName)
{
    DbgLog((LOG_TRACE,2,TEXT("CSplitInputPin::CSplitInputPin")));
    m_pSplitFilter = pSplitFilter;
}
#endif

// provides derived filter a chance to grab extra interfaces

HRESULT
CSplitInputPin::CheckConnect(IPin *pPin)
{
    HRESULT hr = m_pSplitFilter->CheckConnect(PINDIR_INPUT,pPin);
    if (FAILED(hr)) {
    	return hr;
    }
    return CBaseInputPin::CheckConnect(pPin);
}


// provides derived filter a chance to release it's extra interfaces

HRESULT
CSplitInputPin::BreakConnect()
{
    //  Can't disconnect unless stopped
    ASSERT(IsStopped());

	// release Allocator
	if (m_pSplitFilter->m_pAllocator)
	{
		m_pSplitFilter->m_pAllocator->Release();
		m_pSplitFilter->m_pAllocator = NULL;
	}

    m_pSplitFilter->BreakConnect(PINDIR_INPUT);
    return CBaseInputPin::BreakConnect();
}

//
// NotifyAllocator
//
/*
STDMETHODIMP
CSplitInputPin::NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly)
{
	CheckPointer(pAllocator,E_FAIL);
	CAutoLock lock_it(m_pLock);

	// Free the old allocator if any
	if (m_pSplitFilter->m_pAllocator)
		m_pSplitFilter->m_pAllocator->Release();

	// Store away the new allocator
	pAllocator->AddRef();
	m_pSplitFilter->m_pAllocator = pAllocator;

	// Notify the base class about the allocator
	return CBaseInputPin::NotifyAllocator(pAllocator,bReadOnly);

} */// NotifyAllocator


// Let derived class know when the input pin is connected

HRESULT
CSplitInputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = m_pSplitFilter->CompleteConnect(PINDIR_INPUT,pReceivePin);
    if (FAILED(hr)) {
        return hr;
    }
    return CBaseInputPin::CompleteConnect(pReceivePin);
}


// check that we can support a given media type

HRESULT
CSplitInputPin::CheckMediaType(const CMediaType* pmt)
{
    // Check the input type

    HRESULT hr = m_pSplitFilter->CheckInputType(pmt);
    if (S_OK != hr) {
        return hr;
    }

    // if the output pin is still connected, then we have
    // to check the Split not just the input format

    if ((m_pSplitFilter->m_pOutput1 != NULL) &&
        (m_pSplitFilter->m_pOutput1->IsConnected())) {
            hr = m_pSplitFilter->CheckSplit(pmt,&m_pSplitFilter->m_pOutput1->CurrentMediaType());
		}
	if((m_pSplitFilter->m_pOutput2 != NULL) &&	(m_pSplitFilter->m_pOutput2->IsConnected()))
	{
		hr = m_pSplitFilter->CheckSplit(pmt,&m_pSplitFilter->m_pOutput1->CurrentMediaType());
    }
	return hr;
}


// set the media type for this connection

HRESULT
CSplitInputPin::SetMediaType(const CMediaType* mtIn)
{
    // Set the base class media type (should always succeed)
    HRESULT hr = CBasePin::SetMediaType(mtIn);
    if (FAILED(hr)) {
        return hr;
    }

    // check the Split can be done (should always succeed)
    ASSERT(SUCCEEDED(m_pSplitFilter->CheckInputType(mtIn)));

    return m_pSplitFilter->SetMediaType(PINDIR_INPUT,mtIn);
}


// =================================================================
// Implements IMemInputPin interface
// =================================================================


// provide EndOfStream that passes straight downstream
// (there is no queued data)
STDMETHODIMP
CSplitInputPin::EndOfStream(void)
{
    CAutoLock lck(&m_pSplitFilter->m_csReceive);
    HRESULT hr = CheckStreaming();
    if (S_OK == hr) {
       hr = m_pSplitFilter->EndOfStream();
    }
    return hr;
}


// enter flushing state. Call default handler to block Receives, then
// pass to overridable method in filter
STDMETHODIMP
CSplitInputPin::BeginFlush(void)
{
	CAutoLock lock_it(m_pLock);
    CAutoLock lck(&m_pSplitFilter->m_csFilter);
    //  Are we actually doing anything?
    ASSERT(m_pSplitFilter->m_pOutput1 != NULL || m_pSplitFilter->m_pOutput2 != NULL);
    if (!IsConnected() || 
		(!m_pSplitFilter->m_pOutput1->IsConnected() && !m_pSplitFilter->m_pOutput2->IsConnected())) {
        return VFW_E_NOT_CONNECTED;
    }
    HRESULT hr = CBaseInputPin::BeginFlush();
    if (FAILED(hr)) {
    	return hr;
    }
     
	hr = m_pSplitFilter->BeginFlush();
	return hr;
}


// leave flushing state.
// Pass to overridable method in filter, then call base class
// to unblock receives (finally)
STDMETHODIMP
CSplitInputPin::EndFlush(void)
{
    CAutoLock lck(&m_pSplitFilter->m_csFilter);
    //  Are we actually doing anything?
    ASSERT(m_pSplitFilter->m_pOutput1 != NULL || m_pSplitFilter->m_pOutput2 != NULL);
    if (!IsConnected() ||
        (!m_pSplitFilter->m_pOutput1->IsConnected() && !m_pSplitFilter->m_pOutput2->IsConnected())) {
        return VFW_E_NOT_CONNECTED;
    }

    HRESULT hr = m_pSplitFilter->EndFlush();
    if (FAILED(hr)) {
        return hr;
    }

    return CBaseInputPin::EndFlush();
}


// here's the next block of data from the stream.
// AddRef it yourself if you need to hold it beyond the end
// of this call.

HRESULT
CSplitInputPin::Receive(IMediaSample * pSample)
{
    HRESULT hr;
    CAutoLock lck(&m_pSplitFilter->m_csReceive);
    ASSERT(pSample);

    // check all is well with the base class
    hr = CBaseInputPin::Receive(pSample);
    if (S_OK == hr) {
        hr = m_pSplitFilter->Receive(pSample);
		//m_pSplitFilter->m_pOutput1->Deliver(pSample);
		//m_pSplitFilter->m_pOutput2->Deliver(pSample);
    }
    return hr;
}




// override to pass downstream
STDMETHODIMP
CSplitInputPin::NewSegment(
    REFERENCE_TIME tStart,
    REFERENCE_TIME tStop,
    double dRate)
{
    //  Save the values in the pin
    CBasePin::NewSegment(tStart, tStop, dRate);
    return m_pSplitFilter->NewSegment(tStart, tStop, dRate);
}




// =================================================================
// Implements the CSplitOutputPin class
// =================================================================


// constructor

CSplitOutputPin::CSplitOutputPin(
    TCHAR *pObjectName,
    CSplitFilter *pSplitFilter,
    HRESULT * phr,
    LPCWSTR pPinName)
    : CBaseOutputPin(pObjectName, pSplitFilter, &pSplitFilter->m_csFilter, phr, pPinName),
      m_pPosition(NULL)
{
    DbgLog((LOG_TRACE,2,TEXT("CSplitOutputPin::CSplitOutputPin")));
    m_pSplitFilter = pSplitFilter;
	m_pOutputQueue = NULL;
}

#ifdef UNICODE
CSplitOutputPin::CSplitOutputPin(
    CHAR *pObjectName,
    CSplitFilter *pSplitFilter,
    HRESULT * phr,
    LPCWSTR pPinName)
    : CBaseOutputPin(pObjectName, pSplitFilter, &pSplitFilter->m_csFilter, phr, pPinName),
      m_pPosition(NULL)
{
    DbgLog((LOG_TRACE,2,TEXT("CSplitOutputPin::CSplitOutputPin")));
    m_pSplitFilter = pSplitFilter;
	m_pOutputQueue = NULL;
}
#endif

// destructor

CSplitOutputPin::~CSplitOutputPin()
{
    DbgLog((LOG_TRACE,2,TEXT("CSplitOutputPin::~CSplitOutputPin")));

	if(m_pOutputQueue)
	{
		delete m_pOutputQueue;
		m_pOutputQueue = NULL;
	}
    if (m_pPosition) m_pPosition->Release();
}


HRESULT CSplitOutputPin::Active()
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
HRESULT CSplitOutputPin::Inactive()
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
HRESULT CSplitOutputPin::Deliver(IMediaSample *pMediaSample)
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
HRESULT CSplitOutputPin::DeliverEndOfStream()
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
HRESULT CSplitOutputPin::DeliverBeginFlush()
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
HRESULT CSplitOutputPin::DeliverEndFlush()
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
HRESULT CSplitOutputPin::DeliverNewSegment(REFERENCE_TIME tStart, 
										 REFERENCE_TIME tStop,  
										 double dRate)
{
	// Make sure that we have an output queue
	if(m_pOutputQueue == NULL)
		return NOERROR;

	m_pOutputQueue->NewSegment(tStart, tStop, dRate);
	return NOERROR;

} // DeliverNewSegment


// overriden to expose IMediaPosition and IMediaSeeking control interfaces

STDMETHODIMP
CSplitOutputPin::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv,E_POINTER);
    ValidateReadWritePtr(ppv,sizeof(PVOID));
    *ppv = NULL;

    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {

        // we should have an input pin by now

        ASSERT(m_pSplitFilter->m_pInput != NULL);

        if (m_pPosition == NULL) {

            HRESULT hr = CreatePosPassThru(
                             GetOwner(),
                             FALSE,
                             (IPin *)m_pSplitFilter->m_pInput,
                             &m_pPosition);
            if (FAILED(hr)) {
                return hr;
            }
        }
        return m_pPosition->QueryInterface(riid, ppv);
    } else {
        return CBaseOutputPin::NonDelegatingQueryInterface(riid, ppv);
    }
}


// provides derived filter a chance to grab extra interfaces

HRESULT
CSplitOutputPin::CheckConnect(IPin *pPin)
{
    // we should have an input connection first

    ASSERT(m_pSplitFilter->m_pInput != NULL);
    if ((m_pSplitFilter->m_pInput->IsConnected() == FALSE)) {
	    return E_UNEXPECTED;
    }

    HRESULT hr = m_pSplitFilter->CheckConnect(PINDIR_OUTPUT,pPin);
    if (FAILED(hr)) {
	    return hr;
    }
    return CBaseOutputPin::CheckConnect(pPin);
}


// provides derived filter a chance to release it's extra interfaces

HRESULT
CSplitOutputPin::BreakConnect()
{
    //  Can't disconnect unless stopped
    ASSERT(IsStopped());
    m_pSplitFilter->BreakConnect(PINDIR_OUTPUT);
    return CBaseOutputPin::BreakConnect();
}


// Let derived class know when the output pin is connected

HRESULT
CSplitOutputPin::CompleteConnect(IPin *pReceivePin)
{
    HRESULT hr = m_pSplitFilter->CompleteConnect(PINDIR_OUTPUT,pReceivePin);
    if (FAILED(hr)) {
        return hr;
    }
    return CBaseOutputPin::CompleteConnect(pReceivePin);
}


// check a given Split - must have selected input type first

HRESULT
CSplitOutputPin::CheckMediaType(const CMediaType* pmtOut)
{
    // must have selected input first
    ASSERT(m_pSplitFilter->m_pInput != NULL);
    if ((m_pSplitFilter->m_pInput->IsConnected() == FALSE)) {
	        return E_INVALIDARG;
    }

    return m_pSplitFilter->CheckSplit(
				    &m_pSplitFilter->m_pInput->CurrentMediaType(),
				    pmtOut);
}


// called after we have agreed a media type to actually set it in which case
// we run the CheckSplit function to get the output format type again

HRESULT
CSplitOutputPin::SetMediaType(const CMediaType* pmtOut)
{
    HRESULT hr = NOERROR;
    ASSERT(m_pSplitFilter->m_pInput != NULL);

    ASSERT(m_pSplitFilter->m_pInput->CurrentMediaType().IsValid());

    // Set the base class media type (should always succeed)
    hr = CBasePin::SetMediaType(pmtOut);
    if (FAILED(hr)) {
        return hr;
    }

#ifdef DEBUG
    if (FAILED(m_pSplitFilter->CheckSplit(&m_pSplitFilter->
					m_pInput->CurrentMediaType(),pmtOut))) {
	DbgLog((LOG_ERROR,0,TEXT("*** This filter is accepting an output media type")));
	DbgLog((LOG_ERROR,0,TEXT("    that it can't currently Split to.  I hope")));
	DbgLog((LOG_ERROR,0,TEXT("    it's smart enough to reconnect its input.")));
    }
#endif

    return m_pSplitFilter->SetMediaType(PINDIR_OUTPUT,pmtOut);
}

HRESULT
CSplitOutputPin::DecideBufferSize(
    IMemAllocator * pAllocator,
    ALLOCATOR_PROPERTIES* pProp)
{
    return m_pSplitFilter->DecideBufferSize(pAllocator, pProp);
}



// return a specific media type indexed by iPosition

HRESULT
CSplitOutputPin::GetMediaType(
    int iPosition,
    CMediaType *pMediaType)
{
    ASSERT(m_pSplitFilter->m_pInput != NULL);

    //  We don't have any media types if our input is not connected

    if (m_pSplitFilter->m_pInput->IsConnected()) {
        return m_pSplitFilter->GetMediaType(iPosition,pMediaType);
    } else {
        return VFW_S_NO_MORE_ITEMS;
    }
}


// Override this if you can do something constructive to act on the
// quality message.  Consider passing it upstream as well

// Pass the quality mesage on upstream.

STDMETHODIMP
CSplitOutputPin::Notify(IBaseFilter * pSender, Quality q)
{
    UNREFERENCED_PARAMETER(pSender);
    ValidateReadPtr(pSender,sizeof(IBaseFilter));

    // First see if we want to handle this ourselves
    HRESULT hr = m_pSplitFilter->AlterQuality(q);
    if (hr!=S_FALSE) {
        return hr;        // either S_OK or a failure
    }

    // S_FALSE means we pass the message on.
    // Find the quality sink for our input pin and send it there

    ASSERT(m_pSplitFilter->m_pInput != NULL);

    return m_pSplitFilter->m_pInput->PassNotify(q);

} // Notify


// the following removes a very large number of level 4 warnings from the microsoft
// compiler output, which are not useful at all in this case.
#pragma warning(disable:4514)
