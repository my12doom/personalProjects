#include "InternetFile.h"
#include <assert.h>

#define safe_close(x) {if(x){InternetCloseHandle(x);x=NULL;}}
#define rtn_null(x) {if ((x) == NULL){Close();return FALSE}}

InternetFile::InternetFile()
{
	m_hFile = NULL;
	m_hInternet = NULL;
	m_hConnect = NULL;
	m_hRequest = NULL;

	Close();
}

InternetFile::~InternetFile()
{
	Close();
}

BOOL InternetFile::Open(const wchar_t *URL, int max_buffer, __int64 startpos /*=0*/)
{
	Close();

	wchar_t tmp[8] = {0};
	for(unsigned int i=0; i<7 && i<wcslen(URL); i++)
		tmp[i] = towlower(URL[i]);

	if (wcscmp(tmp, L"http://") != 0)
		return FALSE;


	m_hInternet = InternetOpenW(L"Testing",INTERNET_OPEN_TYPE_PRECONFIG,NULL,NULL,0);
	if (!m_hInternet)
		return FALSE;

	if (startpos == 0)
	{
		m_hFile = InternetOpenUrlW(m_hInternet, URL,NULL,0,0,0);
		if (!m_hFile)
		{
			Close();
			return FALSE;
		}

		LARGE_INTEGER li = {0};
		li.LowPart = InternetSetFilePointer(m_hFile, 0, &li.HighPart, SEEK_END, 0);
		if (li.LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		{
			Close();
			return FALSE;
		}

		InternetSetFilePointer(m_hFile, 0, 0, SEEK_SET, 0);

		m_size = li.QuadPart;
	}
	else
	{
		m_hConnect = InternetConnectW(m_hInternet, L"dwindow.bo3d.net", INTERNET_DEFAULT_HTTP_PORT,
							NULL, NULL, INTERNET_SERVICE_HTTP, NULL, NULL);
		if (!m_hConnect)
		{
			Close();
			return FALSE;
		}

		const wchar_t* rgpszAcceptTypes[] = {L"text/*", NULL};
		m_hRequest = HttpOpenRequestW(m_hConnect, NULL, L"/test/hrag.mp4", NULL, NULL, rgpszAcceptTypes, NULL, NULL);
		if (!m_hRequest)
		{
			Close();
			return FALSE;
		}

		wchar_t tmp[200] = {0};
		if (startpos>0)
			wsprintfW(tmp, L"Range :bytes=%I64d-", startpos);
		if (!HttpSendRequestW(m_hRequest, startpos>0?tmp:NULL, wcslen(tmp), NULL, NULL))
		{
			Close();
			return FALSE;
		}
	}
	wcscpy_s(m_URL, URL);
	m_ready = true;
	m_cur = 0;

	return TRUE;
}

BOOL InternetFile::Close()
{
	m_ready = false;
	m_URL[0] = NULL;
	m_size = 0;
	m_cur = 0;

	safe_close(m_hRequest);
	safe_close(m_hConnect);
	safe_close(m_hFile);
	safe_close(m_hInternet);


	if (m_hFile)
	{
		InternetCloseHandle(m_hFile);
		m_hFile = NULL;
	}

	if (m_hInternet)
	{
		InternetCloseHandle(m_hInternet);
		m_hInternet = NULL;
	}

	return TRUE;
}

BOOL InternetFile::ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap)
{
	BOOL succ =  InternetReadFile(m_hRequest ? m_hRequest : m_hFile, lpBuffer, nToRead, nRead);

	assert(succ);

	m_cur += *nRead;

	return *nRead > 0 ? succ : FALSE;
}

BOOL InternetFile::GetFileSizeEx(PLARGE_INTEGER lpFileSize)
{
	lpFileSize->QuadPart = m_size;
	return TRUE;
}
BOOL InternetFile::SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod)
{
	assert(dwMoveMethod != SEEK_END);

// 	lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;
// 	lpNewFilePointer->LowPart = InternetSetFilePointer(m_hFile, lpNewFilePointer->LowPart, &lpNewFilePointer->HighPart, dwMoveMethod, NULL);
// 
// 	if (lpNewFilePointer->LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
// 		return FALSE;


	switch(dwMoveMethod)
	{
		case SEEK_SET:
			lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;
			break;
		case SEEK_CUR:
			lpNewFilePointer->QuadPart += liDistanceToMove.QuadPart;
			break;
		case SEEK_END:
			lpNewFilePointer->QuadPart = m_size + liDistanceToMove.QuadPart;
			break;
	}

	if (lpNewFilePointer->QuadPart != m_cur)
	{
		__int64 size = m_size;
		wchar_t URL[MAX_PATH];
		wcscpy(URL, m_URL);
		Close();
		Open(URL, 1024, liDistanceToMove.QuadPart);
		m_size = size;
		m_cur = lpNewFilePointer->QuadPart;
	}

	return TRUE;
}

DWORD InternetFile::downloading_thread()
{
	return 0;
}