//------------------------------------------------------------------------------
// File: Dump.h
//
// Desc: DirectShow sample code - definitions for dump renderer.
//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------------------------

#include <windows.h>
#include <streams.h>
#include <initguid.h>

class CYV12DumpInputPin;
class CYV12Dump;
class CYV12DumpFilter;

// {59079D04-60E9-4a9d-9B25-3E2746E8F1BA}
DEFINE_GUID(CLSID_YV12Dump, 
			0x59079d04, 0x60e9, 0x4a9d, 0x9b, 0x25, 0x3e, 0x27, 0x46, 0xe8, 0xf1, 0xba);

//
class IYV12CB
{
public:
	virtual HRESULT SampleCB(int width, int height, IMediaSample *sample)=0;
};

// Main filter object
class CYV12DumpFilter : public CBaseFilter
{
	friend CYV12Dump;
	CYV12Dump * const m_pDump;

public:

	// Constructor
	CYV12DumpFilter(CYV12Dump *pDump,
		LPUNKNOWN pUnk,
		CCritSec *pLock,
		HRESULT *phr);

	// Pin enumeration
	CBasePin * GetPin(int n);
	int GetPinCount();

	// Open and close the file as necessary
	STDMETHODIMP Run(REFERENCE_TIME tStart);
	STDMETHODIMP Pause();
	STDMETHODIMP Stop();
};


//  Pin object

class CYV12DumpInputPin : public CRenderedInputPin
{
	friend class CYV12Dump;
	CYV12Dump    * const m_pDump;           // Main renderer object
	CCritSec * const m_pReceiveLock;    // Sample critical section
	REFERENCE_TIME m_tLast;             // Last sample receive time

public:

	CYV12DumpInputPin(CYV12Dump *pDump,
		LPUNKNOWN pUnk,
		CBaseFilter *pFilter,
		CCritSec *pLock,
		CCritSec *pReceiveLock,
		HRESULT *phr);

	// Do something with this media sample
	STDMETHODIMP Receive(IMediaSample *pSample);
	STDMETHODIMP EndOfStream(void);
	STDMETHODIMP ReceiveCanBlock();

	// Check if the pin can support this specific proposed type and format
	HRESULT CheckMediaType(const CMediaType *inType);

	// Break connection
	HRESULT BreakConnect();

	// Track NewSegment
	STDMETHODIMP NewSegment(REFERENCE_TIME tStart,
		REFERENCE_TIME tStop,
		double dRate);
};


//  CYV12Dump object which has filter and pin members

class CYV12Dump : public CUnknown//, public IFileSinkFilter
{
	friend class CYV12DumpFilter;
	friend class CYV12DumpInputPin;

	CYV12DumpFilter   *m_pFilter;       // Methods for filter interfaces
	CYV12DumpInputPin *m_pPin;          // A simple rendered input pin

	CCritSec m_Lock;                // Main renderer critical section
	CCritSec m_ReceiveLock;         // Sublock for received samples

	/*
	CPosPassThru *m_pPosition;      // Renderer position controls

	HANDLE   m_hFile;               // Handle to file for dumping
	LPOLESTR m_pFileName;           // The filename where we dump
	BOOL     m_fWriteError;
	*/
public:

	DECLARE_IUNKNOWN

	CYV12Dump(LPUNKNOWN pUnk, HRESULT *phr);
	~CYV12Dump();

	HRESULT SetCallback(IYV12CB *cb);
	static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

	/*
	// Write string, followed by CR/LF, to a file
	void WriteString(TCHAR *pString);

	// Write raw data stream to a file
	HRESULT Write(PBYTE pbData, LONG lDataLength);

	// Implements the IFileSinkFilter interface
	STDMETHODIMP SetFileName(LPCOLESTR pszFileName,const AM_MEDIA_TYPE *pmt);
	STDMETHODIMP GetCurFile(LPOLESTR * ppszFileName,AM_MEDIA_TYPE *pmt);
	*/
private:

	IYV12CB *m_cb;
	int m_width;
	int m_height;

	// Overriden to say what interfaces we support where
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

	/*
	// Open and write to the file
	HRESULT OpenFile();
	HRESULT CloseFile();
	HRESULT HandleWriteFailure();
	*/
};

