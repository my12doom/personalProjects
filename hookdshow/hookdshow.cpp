// hookdshow.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include "detours/detours.h"
#include <DShow.h>
#include <atlbase.h>
#include <conio.h>
#include "InternetFile.h"
#include "CCritSec.h"
#include <map>
#include <assert.h>

#pragma comment(lib, "detours/detours.lib")
#pragma comment(lib, "detours/detoured.lib")
#pragma comment(lib, "strmiids.lib")
HRESULT debug_list_filters(IGraphBuilder *gb);

FILE * f = fopen("Z:\\debug.txt", "wb");
// #define OutputDebugStringA(x) {fprintf(f, "%s\r\n", x); fflush(f);}

static HANDLE (WINAPI * TrueCreateFileA)(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
) = CreateFileA;
static HANDLE (WINAPI * TrueCreateFileW)(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile
	) = CreateFileW;


static BOOL (WINAPI *TrueReadFile)(
   HANDLE hFile,
   LPVOID lpBuffer,
   DWORD nNumberOfBytesToRead,
   LPDWORD lpNumberOfBytesRead,
   LPOVERLAPPED lpOverlapped
) = ReadFile;

static BOOL (WINAPI *TrueCloseHandle)(_In_  HANDLE hObject) = CloseHandle;

static BOOL (WINAPI *TrueGetFileSizeEx)(HANDLE h, PLARGE_INTEGER lpFileSize) = GetFileSizeEx;
static DWORD (WINAPI *TrueGetFileSize)(_In_ HANDLE hFile,
						 _Out_opt_  LPDWORD lpFileSizeHigh) = GetFileSize;

static BOOL (WINAPI *TrueSetFilePointerEx)(HANDLE h, __in LARGE_INTEGER liDistanceToMove,
									   __out_opt PLARGE_INTEGER lpNewFilePointer,
									   __in DWORD dwMoveMethod
) = SetFilePointerEx;

static DWORD (WINAPI *TrueSetFilePointer)(__in        HANDLE hFile,
										 __in        LONG lDistanceToMove,
										 __inout_opt PLONG lpDistanceToMoveHigh,
										 __in        DWORD dwMoveMethod
									   ) = SetFilePointer;

typedef struct
{
	DWORD dummy;
	InternetFile ifile;
	myCCritSec cs;
} dummy_handle;
myCCritSec cs;
std::map<HANDLE, dummy_handle*> handle_map;
const DWORD dummy_value = 'bo3d';
dummy_handle *get_dummy(HANDLE h)
{
	myCAutoLock lck(&cs);
	return handle_map[h];
}
static HANDLE WINAPI MineCreateFileA(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	bool b = strstr(lpFileName, "X:\\DWindow\\")==lpFileName;

	HANDLE o = TrueCreateFileA( b ? lpFileName+11 : lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (b)
	{
		USES_CONVERSION;

		wchar_t exe_path[MAX_PATH] = {0};
		GetModuleFileNameW(NULL, exe_path, MAX_PATH-1);
		o =  TrueCreateFileW( exe_path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		dummy_handle *p = new dummy_handle;
		p->dummy = dummy_value;
		p->ifile.Open(A2W(lpFileName+11), 1024);

		myCAutoLock lck(&cs);
		handle_map[o] = p;

	}

	return o;
}


static HANDLE WINAPI MineCreateFileW(
						 LPCWSTR lpFileName,
						 DWORD dwDesiredAccess,
						 DWORD dwShareMode,
						 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
						 DWORD dwCreationDisposition,
						 DWORD dwFlagsAndAttributes,
						 HANDLE hTemplateFile)
{
	bool b = wcsstr(lpFileName, L"X:\\DWindow\\")==lpFileName;

	HANDLE o = TrueCreateFileW( b ? lpFileName+11 : lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (b)
	{
		wchar_t exe_path[MAX_PATH] = {0};
 		GetModuleFileNameW(NULL, exe_path, MAX_PATH-1);
		o =  TrueCreateFileW( exe_path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

		dummy_handle *p = new dummy_handle;
		p->dummy = dummy_value;
		p->ifile.Open(lpFileName+11, 1024);
		
		myCAutoLock lck(&cs);
		handle_map[o] = p;

	}

	return o;
}


static BOOL WINAPI MineReadFile(
					   HANDLE hFile,
					   LPVOID lpBuffer,
					   DWORD nNumberOfBytesToRead,
					   LPDWORD lpNumberOfBytesRead,
					   LPOVERLAPPED lpOverlapped)
{
	dummy_handle *p = handle_map[hFile];
	if (p && p->dummy == dummy_value)
	{

		char tmp[80000];
		char tmp2[80000];

		OVERLAPPED ov = {0};
		if (lpOverlapped)
			ov = *lpOverlapped;

// 		DWORD nGot = 0;
// 		DWORD pos = TrueSetFilePointer(hFile, 0, NULL, SEEK_CUR);
// 		BOOL o2 = TrueReadFile(hFile, tmp2, nNumberOfBytesToRead, &nGot, lpOverlapped);
// 		DWORD pos22 = TrueSetFilePointer(hFile, 0, NULL, SEEK_CUR);

		if (lpOverlapped)
			*lpOverlapped = ov;


		myCAutoLock lck(&p->cs);

		DWORD pos1, pos2;
		BOOL o;

		if (lpOverlapped)
		{
			if (lpOverlapped->hEvent)
				ResetEvent(lpOverlapped->hEvent);

			LARGE_INTEGER li;
			li.HighPart = lpOverlapped->OffsetHigh;
			li.LowPart = lpOverlapped->Offset;
			p->ifile.SetFilePointerEx(li, &li, SEEK_SET);
			o = p->ifile.ReadFile(lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

			lpOverlapped->Internal = 0;
			lpOverlapped->InternalHigh = *lpNumberOfBytesRead;
			lpOverlapped->Offset = 0;
			lpOverlapped->OffsetHigh = 0;

			if (lpOverlapped->hEvent)
				SetEvent(lpOverlapped->hEvent);
		}
		else
		{

			pos1 = SetFilePointer(hFile, 0, NULL, SEEK_CUR);
			o = p->ifile.ReadFile(lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
			pos2 = SetFilePointer(hFile, 0, NULL, SEEK_CUR);
		}

// 		memcpy(tmp, lpBuffer, *lpNumberOfBytesRead);
// 		int c = memcmp(tmp2, tmp, *lpNumberOfBytesRead);

		sprintf(tmp, "(H-%08x), read %d bytes, got %d bytes, pos: %d->%d\n", hFile, nNumberOfBytesToRead, *lpNumberOfBytesRead, pos1, pos2);
		OutputDebugStringA(tmp);

// 		strcpy(tmp, "content: ");
// 		for(int i=0; i<*lpNumberOfBytesRead; i++)
// 		{
// 			char tmp2[200];
// 			sprintf(tmp2, "%02x,", ((BYTE*)lpBuffer)[i]);
// 			strcat(tmp, tmp2);
// 		}
// 		strcat(tmp, "\n");
// 		OutputDebugStringA(tmp);


		return o;
	}

	return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}
static BOOL WINAPI MineGetFileSizeEx(HANDLE h, PLARGE_INTEGER lpFileSize)
{
	dummy_handle *p = get_dummy(h);
	if (p && p->dummy == dummy_value)
		return p->ifile.GetFileSizeEx(lpFileSize);
	else
		return TrueGetFileSizeEx(h, lpFileSize);
}

static DWORD WINAPI MineGetFileSize(_In_ HANDLE hFile,_Out_opt_ LPDWORD lpFileSizeHigh)
{
	dummy_handle *p = get_dummy(hFile);
	if (p && p->dummy == dummy_value)
	{
		LARGE_INTEGER li = {0};
		p->ifile.GetFileSizeEx(&li);

		if (lpFileSizeHigh)
			*lpFileSizeHigh = li.HighPart;
		return li.LowPart;
	}
	else
		return TrueGetFileSize(hFile, lpFileSizeHigh);

}

static BOOL WINAPI MineSetFilePointerEx(HANDLE h, __in LARGE_INTEGER liDistanceToMove,
										   __out_opt PLARGE_INTEGER lpNewFilePointer,
										   __in DWORD dwMoveMethod)
{
	dummy_handle *p = get_dummy(h);
	if (p && p->dummy == dummy_value)
	{
		myCAutoLock lck(&p->cs);
		return p->ifile.SetFilePointerEx(liDistanceToMove, lpNewFilePointer, dwMoveMethod);
	}
	else
		return TrueSetFilePointerEx(h, liDistanceToMove, lpNewFilePointer, dwMoveMethod);
}
static DWORD WINAPI MineSetFilePointer(__in        HANDLE hFile,
										  __in        LONG lDistanceToMove,
										  __inout_opt PLONG lpDistanceToMoveHigh,
										  __in        DWORD dwMoveMethod)
{
	dummy_handle *p = get_dummy(hFile);
	if (p && p->dummy == dummy_value)
	{
		LARGE_INTEGER li, li2;
		li.HighPart = lpDistanceToMoveHigh ? *lpDistanceToMoveHigh : (lDistanceToMove>=0 ? 0 : 0xffffffff) ;
		li.LowPart = lDistanceToMove;

		char tmp[2048];
		sprintf(tmp, "(H-%08x), seek to %d method %d\n", hFile, (int)li.QuadPart, dwMoveMethod);
		OutputDebugStringA(tmp);

		myCAutoLock lck(&p->cs);
		p->ifile.SetFilePointerEx(li, &li2, dwMoveMethod);
		if (lpDistanceToMoveHigh)
			*lpDistanceToMoveHigh = li2.HighPart;

// 		assert(li.QuadPart == li2.QuadPart);

		return li2.LowPart;
	}

	return TrueSetFilePointer(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}
BOOL WINAPI MineCloseHandle(_In_  HANDLE hObject)
{
	dummy_handle *p = get_dummy(hObject);
	if (p && p->dummy == dummy_value)
	{
		delete p;
		handle_map[hObject] = NULL;
	}

	return TrueCloseHandle(hObject);
}
int _tmain(int argc, _TCHAR* argv[])
{
	DetourRestoreAfterWith();

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourAttach(&(PVOID&)TrueCreateFileA, MineCreateFileA);
	DetourAttach(&(PVOID&)TrueCreateFileW, MineCreateFileW);
	DetourAttach(&(PVOID&)TrueGetFileSizeEx, MineGetFileSizeEx);
	DetourAttach(&(PVOID&)TrueGetFileSize, MineGetFileSize);
	DetourAttach(&(PVOID&)TrueReadFile, MineReadFile);
	DetourAttach(&(PVOID&)TrueSetFilePointerEx, MineSetFilePointerEx);
	DetourAttach(&(PVOID&)TrueSetFilePointer, MineSetFilePointer);
	DetourAttach(&(PVOID&)TrueCloseHandle, MineCloseHandle);
	LONG error = DetourTransactionCommit();

	CoInitialize(NULL);

	CComPtr<IGraphBuilder> gb;
	gb.CoCreateInstance(CLSID_FilterGraph);

	FILE * f = fopen("D:\\my12doom\\doc\\left720.mp4", "rb");
	FILE * f2 = _wfopen(L"X:\\DWindow\\http://127.0.0.1/left720.mp4", L"rb");

	srand(12346);
	for(int i=0; i<0; i++)
	{
		int method = rand()%2;
		int target = abs(((rand()%32768)<<15+rand()%32768)%62034922);
		target = method == SEEK_END ? -target : target;
		target = method == SEEK_CUR ? rand()-16384 : target;

		fseek(f, target, method);
		fseek(f2, target, method);

		char buf1[32768];
		char buf2[32768];
		int v1 = fread(buf1, 1, 32768, f);
		int v2 = fread(buf2, 1, 32768, f2);

		int p1 = ftell(f);
		int p2 = ftell(f2);
		int c = memcmp(buf1, buf2, 32768);

		printf("v1, v2, method, target, c, pos1, pos2 = %d, %d, %d, %d, %d, %d, %d\n", v1, v2, method, target, c, p1, p2);

		if (v1 != v2 || c !=0 || p1 != p2)
			break;
	}
// 	fclose(f);
// 	fclose(f2);



//	HRESULT hr = gb->RenderFile(L"X:\\DWindow\\http://127.0.0.1/MBAFF.ts", NULL);
//   	HRESULT hr = gb->RenderFile(L"X:\\DWindow\\http://bo3d.net/test/flv.flv", NULL);
// 	HRESULT hr = gb->RenderFile(L"D:\\my12doom\\doc\\left720.mkv", NULL);
//	HRESULT hr = gb->RenderFile(L"X:\\DWindow\\http://127.0.0.1/flv.flv", NULL);
//    	HRESULT hr = gb->RenderFile(L"X:\\DWindow\\http://192.168.1.209/logintest/flv.flv", NULL);
    	HRESULT hr = gb->RenderFile(L"X:\\DWindow\\http://bo3d.net/test/flv.flv", NULL);
	debug_list_filters(gb);
	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
	mc->Run();

	while(true)
	{
		OAFilterState fs;
		mc->GetState(500, &fs);
		char*  tbl[3] = {"State_Stopped", "State_Paused", "State_Running"};
		printf("\r%s", tbl[fs]);
		Sleep(1);
	}
	getch();

	return 0;
}



HRESULT debug_list_filters(IGraphBuilder *gb)
{
	// debug: list filters
	wprintf(L"Listing filters.\n");
	CComPtr<IEnumFilters> pEnum;
	CComPtr<IBaseFilter> filter;
	gb->EnumFilters(&pEnum);
	while(pEnum->Next(1, &filter, NULL) == S_OK)
	{

		FILTER_INFO filter_info;
		filter->QueryFilterInfo(&filter_info);
		if (filter_info.pGraph) filter_info.pGraph->Release();
		wchar_t tmp[10240];
		wchar_t tmp2[1024];
		wchar_t friendly_name[200] = L"Unknown";
		//GetFilterFriedlyName(filter, friendly_name, 200);
		wcscpy(tmp, filter_info.achName);
		wcscat(tmp, L"(");
		wcscat(tmp, friendly_name);
		wcscat(tmp, L")");

		CComPtr<IEnumPins> ep;
		CComPtr<IPin> pin;
		filter->EnumPins(&ep);
		while (ep->Next(1, &pin, NULL) == S_OK)
		{
			PIN_DIRECTION dir;
			PIN_INFO pi;
			pin->QueryDirection(&dir);
			pin->QueryPinInfo(&pi);
			if (pi.pFilter) pi.pFilter->Release();

			CComPtr<IPin> connected;
			PIN_INFO pi2;
			FILTER_INFO fi;
			pin->ConnectedTo(&connected);
			pi2.pFilter = NULL;
			if (connected) connected->QueryPinInfo(&pi2);
			if (pi2.pFilter)
			{
				pi2.pFilter->QueryFilterInfo(&fi);
				if (fi.pGraph) fi.pGraph->Release();
				pi2.pFilter->Release();
			}

			wsprintfW(tmp2, L", %s %s", pi.achName, connected?L"Connected to ":L"Unconnected");
			if (connected) wcscat(tmp2, fi.achName);

			wcscat(tmp, tmp2);
			pin = NULL;
		}


		wprintf(L"%s\n", tmp);

		filter = NULL;
	}
	wprintf(L"\n");

	return S_OK;
}