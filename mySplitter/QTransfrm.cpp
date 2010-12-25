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
    if (m_pPosition) m_pPosition->Release();
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
