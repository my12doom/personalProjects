#include "yv12dump.h"
#include <dvdmedia.h>

// Setup data
/*
const AMOVIESETUP_MEDIATYPE sudPinTypes =
{
	&MEDIATYPE_NULL,            // Major type
	&MEDIASUBTYPE_NULL          // Minor type
};

const AMOVIESETUP_PIN sudPins =
{
	L"Input",                   // Pin string name
	FALSE,                      // Is it rendered
	FALSE,                      // Is it an output
	FALSE,                      // Allowed none
	FALSE,                      // Likewise many
	&CLSID_NULL,                // Connects to filter
	L"Output",                  // Connects to pin
	1,                          // Number of types
	&sudPinTypes                // Pin information
};

const AMOVIESETUP_FILTER sudDump =
{
	&CLSID_YV12Dump,                // Filter CLSID
	L"Dump",                    // String name
	MERIT_DO_NOT_USE,           // Filter merit
	1,                          // Number pins
	&sudPins,                    // Pin details
	CLSID_LegacyAmFilterCategory
};


//
//  Object creation stuff
//
CFactoryTemplate g_Templates[]= {
	L"Dump", &CLSID_YV12Dump, CYV12Dump::CreateInstance, NULL, &sudDump
};
int g_cTemplates = 1;
*/

// Constructor
CYV12DumpFilter::CYV12DumpFilter(CYV12Dump *pDump,
						 LPUNKNOWN pUnk,
						 CCritSec *pLock,
						 HRESULT *phr) :
CBaseFilter(NAME("CYV12DumpFilter"), pUnk, pLock, CLSID_YV12Dump),
m_pDump(pDump)
{
}


//
// GetPin
//
CBasePin * CYV12DumpFilter::GetPin(int n)
{
	if (n == 0) {
		return m_pDump->m_pPin;
	} else {
		return NULL;
	}
}


//
// GetPinCount
//
int CYV12DumpFilter::GetPinCount()
{
	return 1;
}


//
// Stop
//
// Overriden to close the dump file
//
STDMETHODIMP CYV12DumpFilter::Stop()
{
	CAutoLock cObjectLock(m_pLock);

	return CBaseFilter::Stop();
}


//
// Pause
//
// Overriden to open the dump file
//
STDMETHODIMP CYV12DumpFilter::Pause()
{
	CAutoLock cObjectLock(m_pLock);

	return CBaseFilter::Pause();
}


//
// Run
//
// Overriden to open the dump file
//
STDMETHODIMP CYV12DumpFilter::Run(REFERENCE_TIME tStart)
{
	CAutoLock cObjectLock(m_pLock);

	return CBaseFilter::Run(tStart);
}


//
//  Definition of CYV12DumpInputPin
//
CYV12DumpInputPin::CYV12DumpInputPin(CYV12Dump *pDump,
							 LPUNKNOWN pUnk,
							 CBaseFilter *pFilter,
							 CCritSec *pLock,
							 CCritSec *pReceiveLock,
							 HRESULT *phr) :

CRenderedInputPin(NAME("CYV12DumpInputPin"),
				  pFilter,                   // Filter
				  pLock,                     // Locking
				  phr,                       // Return code
				  L"Input"),                 // Pin name
				  m_pReceiveLock(pReceiveLock),
				  m_pDump(pDump),
				  m_tLast(0)
{
}


//
// CheckMediaType
//
// Check if the pin can support this specific proposed type and format
//
HRESULT CYV12DumpInputPin::CheckMediaType(const CMediaType *inType)
{
	if (*inType->Type() != MEDIATYPE_Video)
		return E_FAIL;
	if (*inType->FormatType() != FORMAT_VideoInfo && *inType->FormatType() != FORMAT_VideoInfo2)
		return E_FAIL;
	if (*inType->Subtype() != MEDIASUBTYPE_YV12)
		return E_FAIL;

	if( *inType->FormatType() == FORMAT_VideoInfo)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER*)inType->Format())->bmiHeader;
		m_pDump->m_width = pbih->biWidth;
		m_pDump->m_height = pbih->biHeight;
	}
	else if( *inType->FormatType() == FORMAT_VideoInfo2)
	{
		BITMAPINFOHEADER *pbih = &((VIDEOINFOHEADER2*)inType->Format())->bmiHeader;
		m_pDump->m_width = pbih->biWidth;
		m_pDump->m_height = pbih->biHeight;
	}

	return S_OK;
}


//
// BreakConnect
//
// Break a connection
//
HRESULT CYV12DumpInputPin::BreakConnect()
{
	return CRenderedInputPin::BreakConnect();
}


//
// ReceiveCanBlock
//
// We don't hold up source threads on Receive
//
STDMETHODIMP CYV12DumpInputPin::ReceiveCanBlock()
{
	return S_FALSE;
}


//
// Receive
//
// Do something with this media sample
//
STDMETHODIMP CYV12DumpInputPin::Receive(IMediaSample *pSample)
{
	CheckPointer(pSample,E_POINTER);

	CAutoLock lock(m_pReceiveLock);

	if (m_pDump->m_cb)
		m_pDump->m_cb->SampleCB(m_pDump->m_width, m_pDump->m_height, pSample);
	return S_OK;
}

//
// EndOfStream
//
STDMETHODIMP CYV12DumpInputPin::EndOfStream(void)
{
	CAutoLock lock(m_pReceiveLock);
	return CRenderedInputPin::EndOfStream();

} // EndOfStream


//
// NewSegment
//
// Called when we are seeked
//
STDMETHODIMP CYV12DumpInputPin::NewSegment(REFERENCE_TIME tStart,
									   REFERENCE_TIME tStop,
									   double dRate)
{
	m_tLast = 0;
	return S_OK;

} // NewSegment


//
//  CYV12Dump class
//
CYV12Dump::CYV12Dump(LPUNKNOWN pUnk, HRESULT *phr) :
CUnknown(NAME("CYV12Dump"), pUnk),
m_pFilter(NULL),
m_pPin(NULL),
m_cb(NULL)
{
	ASSERT(phr);

	m_pFilter = new CYV12DumpFilter(this, GetOwner(), &m_Lock, phr);
	if (m_pFilter == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}

	m_pPin = new CYV12DumpInputPin(this,GetOwner(),
		m_pFilter,
		&m_Lock,
		&m_ReceiveLock,
		phr);
	if (m_pPin == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
		return;
	}
}

HRESULT CYV12Dump::SetCallback(IYV12CB *cb)
{
	CAutoLock cObjectLock(m_pPin->m_pReceiveLock);

	m_cb = cb;

	return S_OK;
}

/*
//
// SetFileName
//
// Implemented for IFileSinkFilter support
//
STDMETHODIMP CYV12Dump::SetFileName(LPCOLESTR pszFileName,const AM_MEDIA_TYPE *pmt)
{
	// Is this a valid filename supplied

	CheckPointer(pszFileName,E_POINTER);
	if(wcslen(pszFileName) > MAX_PATH)
		return ERROR_FILENAME_EXCED_RANGE;

	// Take a copy of the filename

	size_t len = 1+lstrlenW(pszFileName);
	m_pFileName = new WCHAR[len];
	if (m_pFileName == 0)
		return E_OUTOFMEMORY;

	HRESULT hr = StringCchCopyW(m_pFileName, len, pszFileName);

	// Clear the global 'write error' flag that would be set
	// if we had encountered a problem writing the previous dump file.
	// (eg. running out of disk space).
	m_fWriteError = FALSE;

	// Create the file then close it

	hr = OpenFile();
	CloseFile();

	return hr;

} // SetFileName


//
// GetCurFile
//
// Implemented for IFileSinkFilter support
//
STDMETHODIMP CYV12Dump::GetCurFile(LPOLESTR * ppszFileName,AM_MEDIA_TYPE *pmt)
{
	CheckPointer(ppszFileName, E_POINTER);
	*ppszFileName = NULL;

	if (m_pFileName != NULL) 
	{
		size_t len = 1+lstrlenW(m_pFileName);
		*ppszFileName = (LPOLESTR)
			QzTaskMemAlloc(sizeof(WCHAR) * (len));

		if (*ppszFileName != NULL) 
		{
			HRESULT hr = StringCchCopyW(*ppszFileName, len, m_pFileName);
		}
	}

	if(pmt) 
	{
		ZeroMemory(pmt, sizeof(*pmt));
		pmt->majortype = MEDIATYPE_NULL;
		pmt->subtype = MEDIASUBTYPE_NULL;
	}

	return S_OK;

} // GetCurFile
*/

// Destructor

CYV12Dump::~CYV12Dump()
{
	delete m_pPin;
	delete m_pFilter;
}


//
// CreateInstance
//
// Provide the way for COM to create a dump filter
//
CUnknown * WINAPI CYV12Dump::CreateInstance(LPUNKNOWN punk, HRESULT *phr)
{
	ASSERT(phr);

	CYV12Dump *pNewObject = new CYV12Dump(punk, phr);
	if (pNewObject == NULL) {
		if (phr)
			*phr = E_OUTOFMEMORY;
	}

	return pNewObject;

} // CreateInstance


//
// NonDelegatingQueryInterface
//
// Override this to say what interfaces we support where
//
STDMETHODIMP CYV12Dump::NonDelegatingQueryInterface(REFIID riid, void ** ppv)
{
	CheckPointer(ppv,E_POINTER);
	CAutoLock lock(&m_Lock);

	// Do we have this interface
	if (riid == IID_IBaseFilter || riid == IID_IMediaFilter || riid == IID_IPersist) {
		return m_pFilter->NonDelegatingQueryInterface(riid, ppv);
	}

	return CUnknown::NonDelegatingQueryInterface(riid, ppv);

} // NonDelegatingQueryInterface


//
// OpenFile
//
// Opens the file ready for dumping
//
/*
HRESULT CYV12Dump::OpenFile()
{
	TCHAR *pFileName = NULL;

	// Is the file already opened
	if (m_hFile != INVALID_HANDLE_VALUE) {
		return NOERROR;
	}

	// Has a filename been set yet
	if (m_pFileName == NULL) {
		return ERROR_INVALID_NAME;
	}

	// Convert the UNICODE filename if necessary

#if defined(WIN32) && !defined(UNICODE)
	char convert[MAX_PATH];

	if(!WideCharToMultiByte(CP_ACP,0,m_pFileName,-1,convert,MAX_PATH,0,0))
		return ERROR_INVALID_NAME;

	pFileName = convert;
#else
	pFileName = m_pFileName;
#endif

	// Try to open the file

	m_hFile = CreateFile((LPCTSTR) pFileName,   // The filename
		GENERIC_WRITE,         // File access
		FILE_SHARE_READ,       // Share access
		NULL,                  // Security
		CREATE_ALWAYS,         // Open flags
		(DWORD) 0,             // More flags
		NULL);                 // Template

	if (m_hFile == INVALID_HANDLE_VALUE) 
	{
		DWORD dwErr = GetLastError();
		return HRESULT_FROM_WIN32(dwErr);
	}

	return S_OK;

} // Open


//
// CloseFile
//
// Closes any dump file we have opened
//
HRESULT CYV12Dump::CloseFile()
{
	// Must lock this section to prevent problems related to
	// closing the file while still receiving data in Receive()
	CAutoLock lock(&m_Lock);

	if (m_hFile == INVALID_HANDLE_VALUE) {
		return NOERROR;
	}

	CloseHandle(m_hFile);
	m_hFile = INVALID_HANDLE_VALUE; // Invalidate the file 

	return NOERROR;

} // Open


//
// Write
//
// Write raw data to the file
//
HRESULT CYV12Dump::Write(PBYTE pbData, LONG lDataLength)
{
	DWORD dwWritten;

	// If the file has already been closed, don't continue
	if (m_hFile == INVALID_HANDLE_VALUE) {
		return S_FALSE;
	}

	if (!WriteFile(m_hFile, (PVOID)pbData, (DWORD)lDataLength,
		&dwWritten, NULL)) 
	{
		return (HandleWriteFailure());
	}

	return S_OK;
}


HRESULT CYV12Dump::HandleWriteFailure(void)
{
	DWORD dwErr = GetLastError();

	if (dwErr == ERROR_DISK_FULL)
	{
		// Close the dump file and stop the filter, 
		// which will prevent further write attempts
		m_pFilter->Stop();

		// Set a global flag to prevent accidental deletion of the dump file
		m_fWriteError = TRUE;

		// Display a message box to inform the developer of the write failure
		TCHAR szMsg[MAX_PATH + 80];
		HRESULT hr = StringCchPrintf(szMsg, MAX_PATH + 80, TEXT("The disk containing dump file has run out of space, ")
			TEXT("so the dump filter has been stopped.\r\n\r\n")
			TEXT("You must set a new dump file name or restart the graph ")
			TEXT("to clear this filter error."));
		MessageBox(NULL, szMsg, TEXT("Dump Filter failure"), MB_ICONEXCLAMATION);
	}

	return HRESULT_FROM_WIN32(dwErr);
}

//
// WriteString
//
// Writes the given string into the file
//
void CYV12Dump::WriteString(TCHAR *pString)
{
	ASSERT(pString);

	// If the file has already been closed, don't continue
	if (m_hFile == INVALID_HANDLE_VALUE) {
		return;
	}

	BOOL bSuccess;
	DWORD dwWritten = 0;
	DWORD dwToWrite = lstrlen(pString);

	// Write the requested data to the dump file
	bSuccess = WriteFile((HANDLE) m_hFile,
		(PVOID) pString, (DWORD) dwToWrite,
		&dwWritten, NULL);

	if (bSuccess == TRUE)
	{
		// Append a carriage-return and newline to the file
		const TCHAR *pEndOfLine = TEXT("\r\n\0");
		dwWritten = 0;
		dwToWrite = lstrlen(pEndOfLine);

		bSuccess = WriteFile((HANDLE) m_hFile,
			(PVOID) pEndOfLine, (DWORD) dwToWrite,
			&dwWritten, NULL);
	}

	// If either of the writes failed, stop receiving data
	if (!bSuccess || (dwWritten < dwToWrite))
	{
		HandleWriteFailure();
	}

} // WriteString
*/

////////////////////////////////////////////////////////////////////////
//
// Exported entry points for registration and unregistration 
// (in this case they only call through to default implementations).
//
////////////////////////////////////////////////////////////////////////

//
// DllRegisterSever
//
// Handle the registration of this filter
//
/*
STDAPI DllRegisterServer()
{
	return AMovieDllRegisterServer2( TRUE );

} // DllRegisterServer


//
// DllUnregisterServer
//
STDAPI DllUnregisterServer()
{
	return AMovieDllRegisterServer2( FALSE );

} // DllUnregisterServer


//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
					  DWORD  dwReason, 
					  LPVOID lpReserved)
{
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
*/
