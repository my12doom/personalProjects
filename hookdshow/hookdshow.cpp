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
#include "full_cache.h"

#pragma comment(lib, "detours/detours.lib")
#pragma comment(lib, "detours/detoured.lib")
#pragma comment(lib, "strmiids.lib")
HRESULT debug_list_filters(IGraphBuilder *gb);
void test_cache();

FILE * f = fopen("Z:\\debug.txt", "wb");
wchar_t ref_file[MAX_PATH] = L"D:\\my12doom\\doc\\MBAFF.ts";
wchar_t URL[] = L"\\\\DWindow\\http://bo3d.net/test/hrag.mp4";
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
	disk_manager ifile;
	myCCritSec cs;
	__int64 pos;
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
	bool b = strstr(lpFileName, "\\\\DWindow\\")==lpFileName;

	HANDLE o = TrueCreateFileA( b ? lpFileName+10 : lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (b)
	{
		USES_CONVERSION;

		wchar_t exe_path[MAX_PATH] = {0};
		GetModuleFileNameW(NULL, exe_path, MAX_PATH-1);
		o =  TrueCreateFileW( exe_path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		dummy_handle *p = new dummy_handle;
		p->dummy = dummy_value;
		p->pos = 0;
		if (p->ifile.setURL(A2W(lpFileName+10)) < 0)
		{
			CloseHandle(o);
			return INVALID_HANDLE_VALUE;
		}

		myCAutoLock lck(&cs);
		handle_map[o] = p;

	}

	return o;
}

disk_manager *g_last_manager;

static HANDLE WINAPI MineCreateFileW(
						 LPCWSTR lpFileName,
						 DWORD dwDesiredAccess,
						 DWORD dwShareMode,
						 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
						 DWORD dwCreationDisposition,
						 DWORD dwFlagsAndAttributes,
						 HANDLE hTemplateFile)
{
	bool b = wcsstr(lpFileName, L"\\\\DWindow\\")==lpFileName;

	HANDLE o = TrueCreateFileW( b ? lpFileName+10 : lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

	if (b)
	{
		wchar_t exe_path[MAX_PATH];
 		GetModuleFileNameW(NULL, exe_path, MAX_PATH-1);
		o =  TrueCreateFileW( exe_path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);

		dummy_handle *p = new dummy_handle;
		p->dummy = dummy_value;
		p->pos = 0;
		g_last_manager = &p->ifile;
		if (p->ifile.setURL(lpFileName+10) < 0)
		{
			CloseHandle(o);
			return INVALID_HANDLE_VALUE;
		}
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

		myCAutoLock lck(&p->cs);

		if (lpOverlapped)
		{
			if (lpOverlapped->hEvent)
				ResetEvent(lpOverlapped->hEvent);

			__int64 pos =/* __int64(lpOverlapped->OffsetHigh) << 32 +*/ lpOverlapped->Offset;
			fragment frag = {pos, pos+nNumberOfBytesToRead};
			p->ifile.get(lpBuffer, frag);
			p->pos = frag.end;

			// pre reader
			for(int i=1; i<5; i++)
			{
				char buf[200];
				fragment pre_reader = {pos+ 4096*1024*i, pos+ 4096*1024*i +100};
				p->ifile.get(buf, pre_reader);
			}

			lpOverlapped->Internal = 0;
			lpOverlapped->InternalHigh = frag.end - frag.start;
			lpOverlapped->Offset = 0;
			lpOverlapped->OffsetHigh = 0;
			*lpNumberOfBytesRead = lpOverlapped->InternalHigh;

			if (lpOverlapped->hEvent)
				SetEvent(lpOverlapped->hEvent);
		}
		else
		{
			fragment frag = {p->pos, p->pos+nNumberOfBytesToRead};
			p->ifile.get(lpBuffer, frag) >= 0;
			*lpNumberOfBytesRead = frag.end - frag.start;
			p->pos += *lpNumberOfBytesRead;

			// pre reader
			for(int i=1; i<5; i++)
			{
				char buf[200];
				fragment pre_reader = {p->pos+ 4096*1024*i, p->pos+ 4096*1024*i +100};
				p->ifile.get(buf, pre_reader);
			}
		}

		return TRUE;
	}

	return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}
static BOOL WINAPI MineGetFileSizeEx(HANDLE h, PLARGE_INTEGER lpFileSize)
{
	dummy_handle *p = get_dummy(h);
	if (p && p->dummy == dummy_value)
	{
		lpFileSize->QuadPart = p->ifile.getsize();
		return TRUE;
	}
	else
		return TrueGetFileSizeEx(h, lpFileSize);
}

static DWORD WINAPI MineGetFileSize(_In_ HANDLE hFile,_Out_opt_ LPDWORD lpFileSizeHigh)
{
	dummy_handle *p = get_dummy(hFile);
	if (p && p->dummy == dummy_value)
	{
		__int64 size = p->ifile.getsize();		

		if (lpFileSizeHigh)
			*lpFileSizeHigh = (DWORD)(size>>32);
		DWORD o = size&0xffffffff;
		return o;
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
		switch(dwMoveMethod)
		{
		case SEEK_SET:
			lpNewFilePointer->QuadPart = liDistanceToMove.QuadPart;
			break;
		case SEEK_CUR:
			lpNewFilePointer->QuadPart = p->pos + liDistanceToMove.QuadPart;
			break;
		case SEEK_END:
			lpNewFilePointer->QuadPart = p->ifile.getsize() + liDistanceToMove.QuadPart;
			break;
		}

		p->pos = lpNewFilePointer->QuadPart;		

		return TRUE;
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


		myCAutoLock lck(&p->cs);

		switch(dwMoveMethod)
		{
		case SEEK_SET:
			li2.QuadPart = li.QuadPart;
			break;
		case SEEK_CUR:
			li2.QuadPart = p->pos + li.QuadPart;
			break;
		case SEEK_END:
			li2.QuadPart = p->ifile.getsize() + li.QuadPart;
			break;
		}

		p->pos = li2.QuadPart;
		if (lpDistanceToMoveHigh)
			*lpDistanceToMoveHigh = li2.HighPart;
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
		return TRUE;
	}

	return TrueCloseHandle(hObject);
}
int debug_window(disk_manager *manager);

int _tmain(int argc, _TCHAR* argv[])
{
// 	test_cache();

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

	HRESULT hr = gb->RenderFile(URL, NULL);
	debug_list_filters(gb);
	CComQIPtr<IMediaControl, &IID_IMediaControl> mc(gb);
	mc->Run();

	debug_window(g_last_manager);

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


void test_cache()
{

	disk_manager *d = new disk_manager(L"flv.flv.config");
	d->setURL(L"http://bo3d.net/test/flv.flv");

	FILE * f = fopen("Z:\\flv.flv", "rb");

	srand(123456);

	int l = GetTickCount();
	int max_response = 0;
	int last_tick = l;
	const int block_size = 99;
	for(int i=0; i<500; i++)
	{
		int pos = __int64(21008892-block_size) * rand() / RAND_MAX;

		char block[block_size] = {0};
		char block2[block_size] = {0};
		char ref_block[block_size] = {0};
		fragment frag = {pos, pos+block_size};
		d->get(block, frag);

		fseek(f, pos, SEEK_SET);
		fread(ref_block, 1, sizeof(ref_block), f);

		int c = memcmp(block, ref_block, sizeof(block)-1);

		if (c != 0)
		{
			int j;
			for(j=0; j<sizeof(block); j++)
				if (block[j] != ref_block[j])
					break;
				

			d->get(block2, frag);
			d->get(block2, frag);
			int c2 = memcmp(block2, block, sizeof(block));
			break;
		}

		max_response = max(max_response, GetTickCount() - last_tick);
		last_tick = GetTickCount();
	}

	printf("avg speed: %d KB/s\n", __int64(50000)*block_size / (GetTickCount()-l));
	printf("avg response time: %d ms, total %dms, max %dms\n\n", (GetTickCount()-l)/50000, GetTickCount()-l, max_response);

	l = GetTickCount();
	printf("exiting cache");
	delete d;
	printf("done exiting cache, %dms\n", GetTickCount()-l);
}