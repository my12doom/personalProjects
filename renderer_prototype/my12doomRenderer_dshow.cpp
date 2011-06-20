// DirectShow part of my12doom renderer

#include "my12doomRenderer.h"
#include <dvdmedia.h>

bool my12doom_queue_enable = true;

// base
STDMETHODIMP DRendererInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	((DBaseVideoRenderer*)m_pFilter)->NewSegment(tStart, tStop, dRate);
	return __super::NewSegment(tStart, tStop, dRate);
}
STDMETHODIMP DRendererInputPin::BeginFlush()
{
	((DBaseVideoRenderer*)m_pFilter)->BeginFlush();
	return __super::BeginFlush();
}

CBasePin * DBaseVideoRenderer::GetPin(int n)
{
	// copied from __super, and changed only the input pin creation class
	CAutoLock cObjectCreationLock(&m_ObjectCreationLock);

	// Should only ever be called with zero
	ASSERT(n == 0);

	if (n != 0) {
		return NULL;
	}

	// Create the input pin if not already done so

	if (m_pInputPin == NULL) {

		// hr must be initialized to NOERROR because
		// CRendererInputPin's constructor only changes
		// hr's value if an error occurs.
		HRESULT hr = NOERROR;

		m_pInputPin = new DRendererInputPin(this,&hr,L"In");
		if (NULL == m_pInputPin) {
			return NULL;
		}

		if (FAILED(hr)) {
			delete m_pInputPin;
			m_pInputPin = NULL;
			return NULL;
		}
	}
	return m_pInputPin;
}


HRESULT DBaseVideoRenderer::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	m_thisstream = tStart;
	return S_OK;
}

HRESULT DBaseVideoRenderer::BeginFlush()
{
	return __super::BeginFlush();
}

// renderer dshow part
my12doomRendererDShow::my12doomRendererDShow(LPUNKNOWN pUnk,HRESULT *phr, my12doomRenderer *owner, int id)
: DBaseVideoRenderer(__uuidof(CLSID_my12doomRenderer),NAME("Texture Renderer"), pUnk, phr),
m_owner(owner),
m_id(id)
{
    if (phr)
        *phr = S_OK;
	m_format = CLSID_NULL;

	HRESULT hr;
	CMemAllocator * allocator = new CMemAllocator(_T("PumpAllocator"), NULL, &hr);
	ALLOCATOR_PROPERTIES allocator_properties = {1, 32, 32, 0}, pro_got;
	allocator->SetProperties(&allocator_properties, &pro_got);
	allocator->Commit();
	allocator->QueryInterface(IID_IMemAllocator, (void**)&m_allocator);


	m_time = m_thisstream = 0;
	m_queue_count = 0;
	m_queue_exit = false;
	if (my12doom_queue_enable) m_queue_thread = CreateThread(NULL, NULL, queue_thread, this, NULL, NULL);

	timeBeginPeriod(1);
}

my12doomRendererDShow::~my12doomRendererDShow()
{
	timeEndPeriod(1);

	m_queue_exit = true;
	WaitForSingleObject(m_queue_thread, INFINITE);

	m_allocator->Decommit();
}

HRESULT my12doomRendererDShow::CheckMediaType(const CMediaType *pmt)
{
    HRESULT   hr = E_FAIL;
    VIDEOINFO *pvi=0;

    CheckPointer(pmt,E_POINTER);

    if( *pmt->FormatType() != FORMAT_VideoInfo && *pmt->FormatType() != FORMAT_VideoInfo2)
        return E_INVALIDARG;

    pvi = (VIDEOINFO *)pmt->Format();

	GUID subtype = *pmt->Subtype();
    if(*pmt->Type() == MEDIATYPE_Video  &&
       (subtype == MEDIASUBTYPE_YV12 || subtype ==  MEDIASUBTYPE_NV12 || subtype == MEDIASUBTYPE_YUY2 || subtype == MEDIASUBTYPE_RGB32))
    {
        hr = m_owner->CheckMediaType(pmt, m_id);
    }

    return hr;
}

HRESULT my12doomRendererDShow::SetMediaType(const CMediaType *pmt)
{
	int width = m_owner->m_lVidWidth;
	int height= m_owner->m_lVidHeight;

	m_format = *pmt->Subtype();

    return S_OK;
}

HRESULT	my12doomRendererDShow::BreakConnect()
{
	m_owner->BreakConnect(m_id);
	return __super::BreakConnect();
}

HRESULT my12doomRendererDShow::CompleteConnect(IPin *pRecievePin)
{
	m_owner->CompleteConnect(pRecievePin, m_id);
	return __super::CompleteConnect(pRecievePin);
}
HRESULT my12doomRendererDShow::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate)
{
	{
		CAutoLock lck(&m_queue_lock);
		m_queue_count = 0;
	}

	{
		CAutoLock lck(&m_owner->m_queue_lock);
		gpu_sample *sample = NULL;
		while (sample = m_owner->m_left_queue.RemoveHead())
			delete sample;
		while (sample = m_owner->m_right_queue.RemoveHead())
			delete sample;
	}

	return __super::NewSegment(tStart, tStop, dRate);
}

bool my12doomRendererDShow::is_connected()
{
	if (!m_pInputPin)
		return false;
	return m_pInputPin->IsConnected() ? true : false;
}

HRESULT my12doomRendererDShow::Receive(IMediaSample *sample)
{
	if (!my12doom_queue_enable)
		return SuperReceive(sample);
retry:
	m_queue_lock.Lock();

	if (m_queue_count >= my12doom_queue_size)
	{
		m_queue_lock.Unlock();
		Sleep(1);
		goto retry;
	}

	REFERENCE_TIME start, end;
	sample->GetTime(&start, &end);
	m_queue[m_queue_count].start = start;
	m_queue[m_queue_count++].end = end;

	m_owner->DataPreroll(m_id, sample);

	m_queue_lock.Unlock();
	return S_OK;
}

//  Helper function for clamping time differences
int inline TimeDiff(REFERENCE_TIME rt)
{
	if (rt < - (50 * UNITS)) {
		return -(50 * UNITS);
	} else
		if (rt > 50 * UNITS) {
			return 50 * UNITS;
		} else return (int)rt;
}

HRESULT my12doomRendererDShow::ShouldDrawSampleNow(IMediaSample *pMediaSample,
												__inout REFERENCE_TIME *ptrStart,
												__inout REFERENCE_TIME *ptrEnd)
{

	// Don't call us unless there's a clock interface to synchronise with
	ASSERT(m_pClock);

	MSR_INTEGER(m_idTimeStamp, (int)((*ptrStart)>>32));   // high order 32 bits
	MSR_INTEGER(m_idTimeStamp, (int)(*ptrStart));         // low order 32 bits

	// We lose a bit of time depending on the monitor type waiting for the next
	// screen refresh.  On average this might be about 8mSec - so it will be
	// later than we think when the picture appears.  To compensate a bit
	// we bias the media samples by -8mSec i.e. 80000 UNITs.
	// We don't ever make a stream time negative (call it paranoia)
	if (*ptrStart>=80000) {
		*ptrStart -= 80000;
		*ptrEnd -= 80000;       // bias stop to to retain valid frame duration
	}

	// Cache the time stamp now.  We will want to compare what we did with what
	// we started with (after making the monitor allowance).
	m_trRememberStampForPerf = *ptrStart;

	// Get reference times (current and late)
	REFERENCE_TIME trRealStream;     // the real time now expressed as stream time.
	m_pClock->GetTime(&trRealStream);
#ifdef PERF
	// While the reference clock is expensive:
	// Remember the offset from timeGetTime and use that.
	// This overflows all over the place, but when we subtract to get
	// differences the overflows all cancel out.
	m_llTimeOffset = trRealStream-timeGetTime()*10000;
#endif
	trRealStream -= m_tStart;     // convert to stream time (this is a reftime)

	// We have to wory about two versions of "lateness".  The truth, which we
	// try to work out here and the one measured against m_trTarget which
	// includes long term feedback.  We report statistics against the truth
	// but for operational decisions we work to the target.
	// We use TimeDiff to make sure we get an integer because we
	// may actually be late (or more likely early if there is a big time
	// gap) by a very long time.
	const int trTrueLate = TimeDiff(trRealStream - *ptrStart);
	const int trLate = trTrueLate;

	MSR_INTEGER(m_idSchLateTime, trTrueLate/10000);

	// Send quality control messages upstream, measured against target
	// my12doom: only when queue is low

	HRESULT hr = S_OK;
	if (m_queue_count <= 1)
		hr = SendQuality(trLate, trRealStream);
	// Note: the filter upstream is allowed to this FAIL meaning "you do it".
	m_bSupplierHandlingQuality = (hr==S_OK);

	// Decision time!  Do we drop, draw when ready or draw immediately?

	const int trDuration = (int)(*ptrEnd - *ptrStart);
	{
		// We need to see if the frame rate of the file has just changed.
		// This would make comparing our previous frame rate with the current
		// frame rate inefficent.  Hang on a moment though.  I've seen files
		// where the frames vary between 33 and 34 mSec so as to average
		// 30fps.  A minor variation like that won't hurt us.
		int t = m_trDuration/32;
		if (  trDuration > m_trDuration+t
			|| trDuration < m_trDuration-t
			) {
				// There's a major variation.  Reset the average frame rate to
				// exactly the current rate to disable decision 9002 for this frame,
				// and remember the new rate.
				m_trFrameAvg = trDuration;
				m_trDuration = trDuration;
		}
	}

	MSR_INTEGER(m_idEarliness, m_trEarliness/10000);
	MSR_INTEGER(m_idRenderAvg, m_trRenderAvg/10000);
	MSR_INTEGER(m_idFrameAvg, m_trFrameAvg/10000);
	MSR_INTEGER(m_idWaitAvg, m_trWaitAvg/10000);
	MSR_INTEGER(m_idDuration, trDuration/10000);

#ifdef PERF
	if (S_OK==pMediaSample->IsDiscontinuity()) {
		MSR_INTEGER(m_idDecision, 9000);
	}
#endif

	// Control the graceful slide back from slow to fast machine mode.
	// After a frame drop accept an early frame and set the earliness to here
	// If this frame is already later than the earliness then slide it to here
	// otherwise do the standard slide (reduce by about 12% per frame).
	// Note: earliness is normally NEGATIVE
	BOOL bJustDroppedFrame
		= (  m_bSupplierHandlingQuality
		//  Can't use the pin sample properties because we might
		//  not be in Receive when we call this
		&& (S_OK == pMediaSample->IsDiscontinuity())          // he just dropped one
		)
		|| (m_nNormal==-1);                          // we just dropped one


	// Set m_trEarliness (slide back from slow to fast machine mode)
	if (trLate>0) {
		m_trEarliness = 0;   // we are no longer in fast machine mode at all!
	} else if (  (trLate>=m_trEarliness) || bJustDroppedFrame) {
		m_trEarliness = trLate;  // Things have slipped of their own accord
	} else {
		m_trEarliness = m_trEarliness - m_trEarliness/8;  // graceful slide
	}

	// prepare the new wait average - but don't pollute the old one until
	// we have finished with it.
	int trWaitAvg;
	{
		// We never mix in a negative wait.  This causes us to believe in fast machines
		// slightly more.
		int trL = trLate<0 ? -trLate : 0;
		trWaitAvg = (trL + m_trWaitAvg*(AVGPERIOD-1))/AVGPERIOD;
	}


	int trFrame;
	{
		REFERENCE_TIME tr = trRealStream - m_trLastDraw; // Cd be large - 4 min pause!
		if (tr>10000000) {
			tr = 10000000;   // 1 second - arbitrarily.
		}
		trFrame = int(tr);
	}

	// We will DRAW this frame IF...
	if (
		// ...the time we are spending drawing is a small fraction of the total
		// observed inter-frame time so that dropping it won't help much.
		(3*m_trRenderAvg <= m_trFrameAvg)

		// ...or our supplier is NOT handling things and the next frame would
		// be less timely than this one or our supplier CLAIMS to be handling
		// things, and is now less than a full FOUR frames late.
		|| ( m_bSupplierHandlingQuality
		? (trLate <= trDuration*4)
		: (trLate+trLate < trDuration)
		)

		// ...or we are on average waiting for over eight milliseconds then
		// this may be just a glitch.  Draw it and we'll hope to catch up.
		|| (m_trWaitAvg > 80000)

		// ...or we haven't drawn an image for over a second.  We will update
		// the display, which stops the video looking hung.
		// Do this regardless of how late this media sample is.
		|| ((trRealStream - m_trLastDraw) > UNITS)

		) {
			HRESULT Result;

			// We are going to play this frame.  We may want to play it early.
			// We will play it early if we think we are in slow machine mode.
			// If we think we are NOT in slow machine mode, we will still play
			// it early by m_trEarliness as this controls the graceful slide back.
			// and in addition we aim at being m_trTarget late rather than "on time".

			BOOL bPlayASAP = FALSE;

			// we will play it AT ONCE (slow machine mode) if...

			// ...we are playing catch-up
			if ( bJustDroppedFrame) {
				bPlayASAP = TRUE;
				MSR_INTEGER(m_idDecision, 9001);
			}

			// ...or if we are running below the true frame rate
			// exact comparisons are glitchy, for these measurements,
			// so add an extra 5% or so
			else if (  (m_trFrameAvg > trDuration + trDuration/16)

				// It's possible to get into a state where we are losing ground, but
				// are a very long way ahead.  To avoid this or recover from it
				// we refuse to play early by more than 10 frames.
				&& (trLate > - trDuration*10)
				){
					bPlayASAP = TRUE;
					MSR_INTEGER(m_idDecision, 9002);
			}
#if 0
			// ...or if we have been late and are less than one frame early
			else if (  (trLate + trDuration > 0)
				&& (m_trWaitAvg<=20000)
				) {
					bPlayASAP = TRUE;
					MSR_INTEGER(m_idDecision, 9003);
			}
#endif
			// We will NOT play it at once if we are grossly early.  On very slow frame
			// rate movies - e.g. clock.avi - it is not a good idea to leap ahead just
			// because we got starved (for instance by the net) and dropped one frame
			// some time or other.  If we are more than 900mSec early, then wait.
			if (trLate<-9000000) {
				bPlayASAP = FALSE;
			}

			if (bPlayASAP) {

				m_nNormal = 0;
				MSR_INTEGER(m_idDecision, 0);
				// When we are here, we are in slow-machine mode.  trLate may well
				// oscillate between negative and positive when the supplier is
				// dropping frames to keep sync.  We should not let that mislead
				// us into thinking that we have as much as zero spare time!
				// We just update with a zero wait.
				m_trWaitAvg = (m_trWaitAvg*(AVGPERIOD-1))/AVGPERIOD;

				// Assume that we draw it immediately.  Update inter-frame stats
				m_trFrameAvg = (trFrame + m_trFrameAvg*(AVGPERIOD-1))/AVGPERIOD;
#ifndef PERF
				// If this is NOT a perf build, then report what we know so far
				// without looking at the clock any more.  This assumes that we
				// actually wait for exactly the time we hope to.  It also reports
				// how close we get to the manipulated time stamps that we now have
				// rather than the ones we originally started with.  It will
				// therefore be a little optimistic.  However it's fast.
				PreparePerformanceData(trTrueLate, trFrame);
#endif
				m_trLastDraw = trRealStream;
				if (m_trEarliness > trLate) {
					m_trEarliness = trLate;  // if we are actually early, this is neg
				}
				Result = S_OK;                   // Draw it now

			} else {
				++m_nNormal;
				// Set the average frame rate to EXACTLY the ideal rate.
				// If we are exiting slow-machine mode then we will have caught up
				// and be running ahead, so as we slide back to exact timing we will
				// have a longer than usual gap at this point.  If we record this
				// real gap then we'll think that we're running slow and go back
				// into slow-machine mode and vever get it straight.
				m_trFrameAvg = trDuration;
				MSR_INTEGER(m_idDecision, 1);

				// Play it early by m_trEarliness and by m_trTarget

				{
					int trE = m_trEarliness;
					if (trE < -m_trFrameAvg) {
						trE = -m_trFrameAvg;
					}
					*ptrStart += trE;           // N.B. earliness is negative
				}

				int Delay = -trTrueLate;
				Result = Delay<=0 ? S_OK : S_FALSE;     // OK = draw now, FALSE = wait

				m_trWaitAvg = trWaitAvg;

				// Predict when it will actually be drawn and update frame stats

				if (Result==S_FALSE) {   // We are going to wait
					trFrame = TimeDiff(*ptrStart-m_trLastDraw);
					m_trLastDraw = *ptrStart;
				} else {
					// trFrame is already = trRealStream-m_trLastDraw;
					m_trLastDraw = trRealStream;
				}
#ifndef PERF
				int iAccuracy;
				if (Delay>0) {
					// Report lateness based on when we intend to play it
					iAccuracy = TimeDiff(*ptrStart-m_trRememberStampForPerf);
				} else {
					// Report lateness based on playing it *now*.
					iAccuracy = trTrueLate;     // trRealStream-RememberStampForPerf;
				}
				PreparePerformanceData(iAccuracy, trFrame);
#endif
			}
			return Result;
	}

	// We are going to drop this frame!
	// Of course in DirectDraw mode the guy upstream may draw it anyway.

	// This will probably give a large negative wack to the wait avg.
	m_trWaitAvg = trWaitAvg;

#ifdef PERF
	// Respect registry setting - debug only!
	if (m_bDrawLateFrames) {
		return S_OK;                        // draw it when it's ready
	}                                      // even though it's late.
#endif

	// We are going to drop this frame so draw the next one early
	// n.b. if the supplier is doing direct draw then he may draw it anyway
	// but he's doing something funny to arrive here in that case.

	MSR_INTEGER(m_idDecision, 2);
	m_nNormal = -1;
	return E_FAIL;                         // drop it

} // ShouldDrawSampleNow



HRESULT my12doomRendererDShow::DoRenderSample( IMediaSample * pSample )
{
	if (!my12doom_queue_enable)
		m_owner->DataPreroll(m_id, pSample);
	REFERENCE_TIME end;
	pSample->GetTime(&m_time, &end);
	m_owner->DoRender(m_id, pSample);
	return S_OK;
}

DWORD WINAPI my12doomRendererDShow::queue_thread(LPVOID param)
{
	my12doomRendererDShow * _this = (my12doomRendererDShow*)param;

	SetThreadName(GetCurrentThreadId(), _this->m_id==0?"queue thread 0":"queue thread 1");
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
	while (!_this->m_queue_exit)
	{
		CComPtr<IMediaSample> dummy_sample;
		dummy_sample = NULL;
		{
			CAutoLock lck(&_this->m_queue_lock);
			if (_this->m_queue_count)
			{
				_this->m_allocator->GetBuffer(&dummy_sample, &_this->m_queue->start, &_this->m_queue->end, NULL);
				dummy_sample->SetTime(&_this->m_queue->start, &_this->m_queue->end);
				memmove(_this->m_queue, _this->m_queue+1, sizeof(dummy_packet)*4);
				_this->m_queue_count--;
			}
		}

		if(dummy_sample)
			_this->SuperReceive(dummy_sample);
		else
			Sleep(1);
	}

	return 0;
}
