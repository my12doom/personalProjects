//------------------------------------------------------------------------------
// File: Transfrm.h
//
// Desc: DirectShow base classes - defines classes from which simple 
//       Split codecs may be derived.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------


// It assumes the codec has one input and one output stream, and has no
// interest in memory management, interface negotiation or anything else.
//
// derive your class from this, and supply Split and the media type/format
// negotiation functions. Implement that class, compile and link and
// you're done.


#ifndef __SPLITBASE_H
#define __SPLITBASE_H

#define S_LEFT ((HRESULT)0x00000002L)
#define S_RIGHT ((HRESULT)0x00000004L)


// ======================================================================
// This is the com object that represents a simple Split filter. It
// supports IBaseFilter, IMediaFilter and two pins through nested interfaces
// ======================================================================

class CSplitFilter;

// ==================================================
// Implements the input pin
// ==================================================

class CSplitInputPin : public CBaseInputPin
{
    friend class CSplitFilter;

protected:
    CSplitFilter *m_pSplitFilter;


public:

    CSplitInputPin(
        TCHAR *pObjectName,
        CSplitFilter *pSplitFilter,
        HRESULT * phr,
        LPCWSTR pName);
#ifdef UNICODE
    CSplitInputPin(
        char *pObjectName,
        CSplitFilter *pSplitFilter,
        HRESULT * phr,
        LPCWSTR pName);
#endif

    STDMETHODIMP QueryId(LPWSTR * Id)
    {
        return AMGetWideString(L"In", Id);
    }

    // Grab and release extra interfaces if required

    HRESULT CheckConnect(IPin *pPin);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin *pReceivePin);

	//STDMETHODIMP NotifyAllocator(IMemAllocator *pAllocator, BOOL bReadOnly);

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtIn);

    // set the connection media type
    HRESULT SetMediaType(const CMediaType* mt);

    // --- IMemInputPin -----

    // here's the next block of data from the stream.
    // AddRef it yourself if you need to hold it beyond the end
    // of this call.
    STDMETHODIMP Receive(IMediaSample * pSample);

    // provide EndOfStream that passes straight downstream
    // (there is no queued data)
    STDMETHODIMP EndOfStream(void);

    // passes it to CSplitFilter::BeginFlush
    STDMETHODIMP BeginFlush(void);

    // passes it to CSplitFilter::EndFlush
    STDMETHODIMP EndFlush(void);

    STDMETHODIMP NewSegment(
                        REFERENCE_TIME tStart,
                        REFERENCE_TIME tStop,
                        double dRate);

    // Check if it's OK to process samples
    virtual HRESULT CheckStreaming();

    // Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };

};

// ==================================================
// Implements the output pin
// ==================================================

class CSplitOutputPin : public CBaseOutputPin
{
    friend class CSplitFilter;

protected:
    CSplitFilter *m_pSplitFilter;

public:

    // implement IMediaPosition by passing upstream
    IUnknown * m_pPosition;
	COutputQueue *m_pOutputQueue;  // Streams data to the peer pin

    CSplitOutputPin(
        TCHAR *pObjectName,
        CSplitFilter *pSplitFilter,
        HRESULT * phr,
        LPCWSTR pName);
#ifdef UNICODE
    CSplitOutputPin(
        CHAR *pObjectName,
        CSplitFilter *pSplitFilter,
        HRESULT * phr,
        LPCWSTR pName);
#endif
    ~CSplitOutputPin();

	// Used to create output queue objects
	HRESULT Active();
	HRESULT Inactive();

	HRESULT Deliver(IMediaSample *pMediaSample);
	HRESULT DeliverEndOfStream();
	HRESULT DeliverBeginFlush();
	HRESULT DeliverEndFlush();
	HRESULT DeliverNewSegment(
		REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate);

    // override to expose IMediaPosition
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);

    // --- CBaseOutputPin ------------

    STDMETHODIMP QueryId(LPWSTR * Id)
    {
        return AMGetWideString(L"Out", Id);
    }

    // Grab and release extra interfaces if required

    HRESULT CheckConnect(IPin *pPin);
    HRESULT BreakConnect();
    HRESULT CompleteConnect(IPin *pReceivePin);

    // check that we can support this output type
    HRESULT CheckMediaType(const CMediaType* mtOut);

    // set the connection media type
    HRESULT SetMediaType(const CMediaType *pmt);

    // called from CBaseOutputPin during connection to ask for
    // the count and size of buffers we need.
	//HRESULT DecideAllocator(IMemInputPin *pPin, IMemAllocator **ppAlloc);
    HRESULT DecideBufferSize(
                IMemAllocator * pAlloc,
                ALLOCATOR_PROPERTIES *pProp);

    // returns the preferred formats for a pin
    HRESULT GetMediaType(int iPosition,CMediaType *pMediaType);

    // inherited from IQualityControl via CBasePin
    STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

    // Media type
public:
    CMediaType& CurrentMediaType() { return m_mt; };
};


class AM_NOVTABLE CSplitFilter : public CBaseFilter
{

public:

    // map getpin/getpincount for base enum of pins to owner
    // override this to return more specialised pin objects

    virtual int GetPinCount();
    virtual CBasePin * GetPin(int n);
    STDMETHODIMP FindPin(LPCWSTR Id, IPin **ppPin);

    // override state changes to allow derived Split filter
    // to control streaming start/stop
    STDMETHODIMP Stop();
    STDMETHODIMP Pause();

public:

    CSplitFilter(TCHAR *, LPUNKNOWN, REFCLSID clsid);
#ifdef UNICODE
    CSplitFilter(CHAR *, LPUNKNOWN, REFCLSID clsid);
#endif
    ~CSplitFilter();

    // =================================================================
    // ----- override these bits ---------------------------------------
    // =================================================================

    // These must be supplied in a derived class

    virtual HRESULT Split(IMediaSample * pIn, IMediaSample *pOut1, IMediaSample *pOut2);

    // check if you can support mtIn
    virtual HRESULT CheckInputType(const CMediaType* mtIn) PURE;

    // check if you can support the Split from this input to this output
    virtual HRESULT CheckSplit(const CMediaType* mtIn, const CMediaType* mtOut) PURE;

    // this goes in the factory template table to create new instances
    // static CCOMObject * CreateInstance(LPUNKNOWN, HRESULT *);

    // call the SetProperties function with appropriate arguments
    virtual HRESULT DecideBufferSize(
                        IMemAllocator * pAllocator,
                        ALLOCATOR_PROPERTIES *pprop) PURE;

    // override to suggest OUTPUT pin media types
    virtual HRESULT GetMediaType(int iPosition, CMediaType *pMediaType) PURE;



    // =================================================================
    // ----- Optional Override Methods           -----------------------
    // =================================================================

    // you can also override these if you want to know about streaming
    virtual HRESULT StartStreaming();
    virtual HRESULT StopStreaming();

    // override if you can do anything constructive with quality notifications
    virtual HRESULT AlterQuality(Quality q);

    // override this to know when the media type is actually set
    virtual HRESULT SetMediaType(PIN_DIRECTION direction,const CMediaType *pmt);

    // chance to grab extra interfaces on connection
    virtual HRESULT CheckConnect(PIN_DIRECTION dir,IPin *pPin);
    virtual HRESULT BreakConnect(PIN_DIRECTION dir);
    virtual HRESULT CompleteConnect(PIN_DIRECTION direction,IPin *pReceivePin);

    // chance to customize the Split process
    virtual HRESULT Receive(IMediaSample *pSample);

    // Standard setup for output sample
    HRESULT InitializeOutputSample(IMediaSample *pSample, IMediaSample **ppOutSample1, IMediaSample **ppOutSample2);

    // if you override Receive, you may need to override these three too
    virtual HRESULT EndOfStream(void);
    virtual HRESULT BeginFlush(void);
    virtual HRESULT EndFlush(void);
    virtual HRESULT NewSegment(
                        REFERENCE_TIME tStart,
                        REFERENCE_TIME tStop,
                        double dRate);

#ifdef PERF
    // Override to register performance measurement with a less generic string
    // You should do this to avoid confusion with other filters
    virtual void RegisterPerfId()
         {m_idSplit = MSR_REGISTER(TEXT("Split"));}
#endif // PERF


// implementation details

protected:

#ifdef PERF
    int m_idSplit;                 // performance measuring id
#endif
    BOOL m_bEOSDelivered;              // have we sent EndOfStream
    BOOL m_bSampleSkipped;             // Did we just skip a frame
    BOOL m_bQualityChanged;            // Have we degraded?

    // critical section protecting filter state.

    CCritSec m_csFilter;

    // critical section stopping state changes (ie Stop) while we're
    // processing a sample.
    //
    // This critical section is held when processing
    // events that occur on the receive thread - Receive() and EndOfStream().
    //
    // If you want to hold both m_csReceive and m_csFilter then grab
    // m_csFilter FIRST - like CSplitFilter::Stop() does.

    CCritSec m_csReceive;

    // these hold our input and output pins

    friend class CSplitInputPin;
    friend class CSplitOutputPin;
    CSplitInputPin *m_pInput;
    CSplitOutputPin *m_pOutput1;
	CSplitOutputPin *m_pOutput2;
	IMemAllocator *m_pAllocator;    // Allocator from our input pin
};

#endif /* __TRANSFRM__ */


