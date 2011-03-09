#include <atlbase.h>
#include <windows.h>
#include <stdio.h>
#include <streams.h>
#include <dvdmedia.h>
#include <initguid.h>
#include "..\mysplitter\QTransfrm.h"

// {D00E73D7-06F5-44F9-8BE4-B7DB191E9E7E}
DEFINE_GUID(CLSID_PD10_DECODER, 
                        0xD00E73D7, 0x06f5, 0x44F9, 0x8B, 0xE4, 0xB7, 0xDB, 0x19, 0x1E, 0x9E, 0x7E);

// {F07E981B-0EC4-4665-A671-C24955D11A38}
DEFINE_GUID(CLSID_PD10_DEMUXER, 
                        0xF07E981B, 0x0EC4, 0x4665, 0xA6, 0x71, 0xC2, 0x49, 0x55, 0xD1, 0x1A, 0x38);

// {998BB643-150D-4af5-80D0-0416BA066F9B}
DEFINE_GUID(CLSID_sq2sbs, 
0x998bb643, 0x150d, 0x4af5, 0x80, 0xd0, 0x4, 0x16, 0xba, 0x6, 0x6f, 0x9b);

// {419832C4-7813-4b90-A262-12496691E82E}
DEFINE_GUID(CLSID_DWindowSSP, 
0x419832c4, 0x7813, 0x4b90, 0xa2, 0x62, 0x12, 0x49, 0x66, 0x91, 0xe8, 0x2e);

HRESULT ActiveMVC(IBaseFilter *filter);
HRESULT AddGraphToRot(IUnknown *pUnkGraph, DWORD *pdwRegister);
void RemoveGraphFromRot(DWORD pdwRegister);
HRESULT GetUnconnectedPin(IBaseFilter *pFilter,PIN_DIRECTION PinDir, IPin **ppPin);

class MPacket
{
public:
	MPacket(IMediaSample *sample);
	~MPacket();
	REFERENCE_TIME rtStart, rtStop;
	BYTE *m_data;
	int m_datasize;
};


class sq2sbs : public CTransformFilter
{
public:
	DECLARE_IUNKNOWN;

	// CTransformFilter Bug FIX
	STDMETHODIMP Pause(){CAutoLock lock_it(m_pLock);return CTransformFilter::Pause();}
	STDMETHODIMP Stop(){CAutoLock lock_it(m_pLock);return CTransformFilter::Stop();}

	// Overrriden from CTransformFilter base class
	HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
	HRESULT CheckInputType(const CMediaType *mtIn);
	HRESULT CheckTransform(const CMediaType *mtIn, const CMediaType *mtOut);
	HRESULT DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties);
	HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
	HRESULT NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate);


	sq2sbs(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
	~sq2sbs();

	// debug function
	FILE *m_dbg;
	void debug_print(const char *tszErr, ...);
	void debug_print(const wchar_t *tszErr, ...);

private:

	// for pd10 fixing
	enum
	{
		packet_state_normal,
		packet_state_flushing_r1,
		packet_state_flushing_r2,
		packet_state_flushing_r3,
		packet_state_flushing_r4,
		packet_state_flushing_l1,
		packet_state_flushing_l2,
		packet_state_flushing_l3,
		packet_state_jump,
	} m_packet_state;
	int m_right_eye_flushing_fn;

	int m_last_fn;
	bool m_resuming_left_eye;		// true = waiting for first resuming left eye packet, to reverse a right eye packet from tail to head;

	// image control
	int m_1088fix;
	int m_in_x;
	int m_in_y;
	int m_out_x;	// not sbs, is 1920 or 1280
	int m_out_y;
	CGenericList<MPacket> m_left_queue;
	CGenericList<MPacket> m_right_queue;

	// stream time
	REFERENCE_TIME m_this_stream_start;

};