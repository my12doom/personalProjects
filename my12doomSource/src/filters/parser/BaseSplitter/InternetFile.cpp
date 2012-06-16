#include "InternetFile.h"
#include <assert.h>

InternetFile::InternetFile()
{
	m_hFile = NULL;
	m_hInternet = NULL;
	Close();
}

InternetFile::~InternetFile()
{
	Close();
}

BOOL InternetFile::Open(const wchar_t *URL, int max_buffer)
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
	wcscpy_s(m_URL, URL);
	m_ready = true;

	return TRUE;
}

BOOL InternetFile::Close()
{
	m_ready = false;
	m_URL[0] = NULL;
	m_size = 0;

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
	return InternetReadFile(m_hFile, lpBuffer, nToRead, nRead);

}

BOOL InternetFile::GetFileSizeEx(PLARGE_INTEGER lpFileSize)
{
	lpFileSize->QuadPart = m_size;
	return TRUE;
}
BOOL InternetFile::SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod)
{
	assert(liDistanceToMove.LowPart == 0);

	lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;
	lpNewFilePointer->LowPart = InternetSetFilePointer(m_hFile, lpNewFilePointer->LowPart, &lpNewFilePointer->HighPart, dwMoveMethod, NULL);

	if (lpNewFilePointer->LowPart == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
		return FALSE;

	return TRUE;
}

DWORD InternetFile::downloading_thread()
{
	return 0;
}