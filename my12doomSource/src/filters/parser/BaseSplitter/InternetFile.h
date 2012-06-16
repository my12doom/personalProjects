#pragma once
#include <Windows.h>
#include <wininet.h>
#pragma comment(lib,"wininet.lib")

class InternetFile
{
public:
	InternetFile();
	~InternetFile();
	BOOL ReadFile(LPVOID lpBuffer, DWORD nToRead, LPDWORD nRead, LPOVERLAPPED lpOverlap);
	BOOL GetFileSizeEx(PLARGE_INTEGER lpFileSize);
	BOOL SetFilePointerEx(__in LARGE_INTEGER liDistanceToMove, __out_opt PLARGE_INTEGER lpNewFilePointer, __in DWORD dwMoveMethod);
	BOOL Open(const wchar_t *URL, int max_buffer);
	BOOL Close();
	bool IsReady(){return m_ready;}

protected:

	static DWORD WINAPI downloading_thread_entry(LPVOID p){return ((InternetFile*)p)->downloading_thread();}
	DWORD downloading_thread();

	bool m_ready;
	wchar_t m_URL[MAX_PATH];
	__int64 m_size;
	HINTERNET m_hInternet;
	HINTERNET m_hFile;
};