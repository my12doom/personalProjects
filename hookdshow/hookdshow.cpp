#include "hookdshow.h"

#include "..\lua\DWindowReader.h"
#include <list>
#include <Windows.h>
#include "..\dwindow\global_funcs.h"
#include "..\dwindow\dwindow_log.h"
#include "detours/detours.h"
#include <DShow.h>
#include <atlbase.h>
#include <conio.h>
#include "CCritSec.h"
#include <map>
#include <string>
#include <assert.h>
#include "full_cache.h"
#include <streams.h>			// for CCritSec
#include "archive_crc32.h"

#pragma comment(lib, "detours/detours.lib")
#pragma comment(lib, "detours/detoured.lib")
#pragma comment(lib, "strmiids.lib")
void test_cache();

class HTTPHook : public IDWindowReader
{
public:
	static HTTPHook * create(const wchar_t *URL, void *extraInfo = NULL);			// create a instance, return NULL if not supported
	~HTTPHook();			// automatically close()
	__int64 size();
	__int64 get(void *buf, __int64 offset, int size);
	void close();				// cancel all pending get() operation and reject all further get()
	std::map<__int64, __int64> buffer();
private:
	void *m_core;
	bool m_closed;
	HTTPHook(void *core);
};

IDWindowReader *OpenReader(const wchar_t *URL);
void CloseReader(IDWindowReader *p);

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
	IDWindowReader *ifile;
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
	USES_CONVERSION;

	HANDLE o;

	if (Token2URL(A2W(lpFileName)))
	{

		wchar_t exe_path[MAX_PATH] = L"Z:\\flv.flv";
 		GetModuleFileNameW(NULL, exe_path, MAX_PATH-1);
		o =  TrueCreateFileW( exe_path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		dummy_handle *p = new dummy_handle;
		p->dummy = dummy_value;
		p->pos = 0;
		p->ifile = OpenReader(Token2URL(A2W(lpFileName)));
		if (p->ifile == NULL)
		{
			CloseHandle(o);
			return INVALID_HANDLE_VALUE;
		}

		myCAutoLock lck(&cs);
		handle_map[o] = p;
	}
	else
	{
		o = TrueCreateFileA( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
	}

	return o;
}

inet_file *g_last_manager = NULL;

static HANDLE WINAPI MineCreateFileW(
						 LPCWSTR lpFileName,
						 DWORD dwDesiredAccess,
						 DWORD dwShareMode,
						 LPSECURITY_ATTRIBUTES lpSecurityAttributes,
						 DWORD dwCreationDisposition,
						 DWORD dwFlagsAndAttributes,
						 HANDLE hTemplateFile)
{
	HANDLE o;

	if (Token2URL(lpFileName))
	{
		wchar_t exe_path[MAX_PATH] = L"Z:\\flv.flv";
 		GetModuleFileNameW(NULL, exe_path, MAX_PATH-1);
		o =  TrueCreateFileW( exe_path, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		dummy_handle *p = new dummy_handle;
		p->dummy = dummy_value;
		p->pos = 0;
		p->ifile = OpenReader(Token2URL(lpFileName));
		if (p->ifile == NULL)
		{
			CloseHandle(o);
			return INVALID_HANDLE_VALUE;
		}
		myCAutoLock lck(&cs);
		handle_map[o] = p;

		//g_last_manager = p->ifile;
	}
	else
	{
		o = TrueCreateFileW( lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
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
		int pre_reader_block_size = 2048*1024;
		nNumberOfBytesToRead = min(nNumberOfBytesToRead, p->ifile->size() - p->pos);

		if (lpOverlapped)
		{
			if (lpOverlapped->hEvent)
				ResetEvent(lpOverlapped->hEvent);

			__int64 pos =/* __int64(lpOverlapped->OffsetHigh) << 32 +*/ lpOverlapped->Offset;
			//fragment frag = {pos, pos+nNumberOfBytesToRead};
			int got = p->ifile->get(lpBuffer, pos, nNumberOfBytesToRead);
			p->pos += got;

			// pre reader
			for(int i=0; i<5; i++)
			{
 				fragment pre_reader = {pos+nNumberOfBytesToRead+ pre_reader_block_size*i, pos+nNumberOfBytesToRead+ pre_reader_block_size*(i+0.5)};
 				//p->ifile->pre_read(pre_reader);
			}

			lpOverlapped->Internal = 0;
			lpOverlapped->InternalHigh = got;
			lpOverlapped->Offset = 0;
			lpOverlapped->OffsetHigh = 0;
			*lpNumberOfBytesRead = lpOverlapped->InternalHigh;

			if (lpOverlapped->hEvent)
				SetEvent(lpOverlapped->hEvent);
		}
		else
		{
			//fragment frag = {p->pos, p->pos+nNumberOfBytesToRead};
			*lpNumberOfBytesRead = p->ifile->get(lpBuffer, p->pos, nNumberOfBytesToRead);
			p->pos += *lpNumberOfBytesRead;

			// pre reader
 			for(int i=0; i<5; i++)
 			{
  				fragment pre_reader = {p->pos+nNumberOfBytesToRead+ pre_reader_block_size*i, p->pos+nNumberOfBytesToRead+ pre_reader_block_size*(i+0.5)};
  				//p->ifile->pre_read(pre_reader);
 			}
		}

		return *lpNumberOfBytesRead == nNumberOfBytesToRead ? TRUE : FALSE;
	}

	return TrueReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}
static BOOL WINAPI MineGetFileSizeEx(HANDLE h, PLARGE_INTEGER lpFileSize)
{
	dummy_handle *p = get_dummy(h);
	if (p && p->dummy == dummy_value)
	{
		lpFileSize->QuadPart = p->ifile->size();
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
		__int64 size = p->ifile->size();		

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
			p->pos = liDistanceToMove.QuadPart;
			break;
		case SEEK_CUR:
			p->pos = p->pos + liDistanceToMove.QuadPart;
			break;
		case SEEK_END:
			p->pos = p->ifile->size() + liDistanceToMove.QuadPart;
			break;
		}

		if(lpNewFilePointer)
			lpNewFilePointer->QuadPart = p->pos;

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
			li2.QuadPart = p->ifile->size() + li.QuadPart;
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
		if (p->ifile)
			CloseReader(p->ifile);
 		delete p;
		handle_map[hObject] = NULL;
		return TRUE;
	}

	return TrueCloseHandle(hObject);
}

int stop_all_handles()
{
	myCAutoLock lck(&cs);
	for(std::map<HANDLE, dummy_handle*>::iterator i = handle_map.begin(); i!= handle_map.end(); ++i)
	{
		if (i->second && i->second->ifile)
			i->second->ifile->close();
	}
	return 0;
}

int clear_all_handles()
{
	myCAutoLock lck(&cs);
	for(std::map<HANDLE, dummy_handle*>::iterator i = handle_map.begin(); i!= handle_map.end(); ++i)
	{
		CloseHandle(i->first);
	}
	return 0;

}

int lua_create_http_reader(lua_State *L)
{
	if (!lua_isstring(L, -1))
		return 0;

	lua_pushlightuserdata(L, HTTPHook::create(UTF82W(lua_tostring(L, -1))));
	return 1;
}

int enable_hookdshow()
{
	luaState L;
	lua_pushcfunction(L, &lua_create_http_reader);
	lua_setglobal(L, "create_http_reader");

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
	return DetourTransactionCommit();
}

int disable_hookdshow()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());
	DetourDetach(&(PVOID&)TrueCreateFileA, MineCreateFileA);
	DetourDetach(&(PVOID&)TrueCreateFileW, MineCreateFileW);
	DetourDetach(&(PVOID&)TrueGetFileSizeEx, MineGetFileSizeEx);
	DetourDetach(&(PVOID&)TrueGetFileSize, MineGetFileSize);
	DetourDetach(&(PVOID&)TrueReadFile, MineReadFile);
	DetourDetach(&(PVOID&)TrueSetFilePointerEx, MineSetFilePointerEx);
	DetourDetach(&(PVOID&)TrueSetFilePointer, MineSetFilePointer);
	DetourDetach(&(PVOID&)TrueCloseHandle, MineCloseHandle);
	return DetourTransactionCommit();
}

void test_cache()
{

	inet_file *d = new inet_file(L"flv.flv.config");
	d->setURL(L"http://cm.baidupcs.com/file/016e3d3c76417f363611f37cf0926694?xcode=07cd34ed4aa5d508def893a78f8f16a51da5bf4cb672b7dd&fid=3792016278-250528-992739353&time=1380090694&sign=FDTAXER-DCb740ccc5511e5e8fedcff06b081203-UYYhjqWlphz8VlfTMjeXeEoVovA%3D&to=cmb&fm=N,B,T,mn&expires=8h&rt=sh&r=928118386&logid=2684583592&sh=1&fn=%E4%B8%9C%E4%BA%AC%E5%A5%B3%E5%AD%90%E6%B5%81%20-%20Bad%20Flower%28mv%29%5Bwww.truemv.com%5D.rmvb");

	FILE * f = fopen("Z:\\a.rmvb", "rb");

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



typedef struct
{
	int ref_count;
	inet_file *manager;
	char config_file[4096];
	char URL_utf[4096];
} active_httpfile_list_entry;
std::list<active_httpfile_list_entry> g_active_httpfile_list;
std::map<std::wstring, std::wstring> g_URL2Token;
std::map<std::wstring, std::wstring> g_Token2URL;
CCritSec g_active_httpfile_list_lock;

IDWindowReader *OpenReader(const wchar_t *URL)
{
	IDWindowReader *p = NULL;
	luaState L;	
	lua_getglobal(L, "core");
	lua_getfield(L, -1,"create_reader");
	if (lua_isfunction(L, -1))
	{
		lua_pushstring(L, W2UTF8(URL));
		lua_mypcall(L, 1, 1, 0);

		p = (IDWindowReader*)lua_touserdata(L, -1);
	}

	lua_settop(L, 0);

	return p;
}

void CloseReader(IDWindowReader *p)
{
	p->release();
}

const wchar_t *URL2Token(const wchar_t *URL)
{
	CAutoLock lck(&g_active_httpfile_list_lock);

	// return the token if found
	if (g_URL2Token.find(URL) != g_URL2Token.end())
		return g_URL2Token[URL].c_str();

	// create the token and insert it
	wchar_t token[MAX_PATH];
	wsprintf(token, L"X:\\DWindow\\%d%s", g_URL2Token.size(), wcsrchr(URL, L'.'));

	g_URL2Token[URL] = token;
	g_Token2URL[token] = URL;

	return g_URL2Token[URL].c_str();
}

const wchar_t *Token2URL(const wchar_t *Token)
{
	CAutoLock lck(&g_active_httpfile_list_lock);

	if (g_Token2URL.find(Token) != g_Token2URL.end())
		return g_Token2URL[Token].c_str();

	return NULL;
}

// HTTPHook implementation
HTTPHook * HTTPHook::create(const wchar_t *URL, void *extraInfo/* = NULL*/)			// create a instance, return NULL if not supported
{
	wchar_t dwindow_path[MAX_PATH];
	wcscpy(dwindow_path, dwindow_log_get_filename());
	*(wchar_t*)(wcsrchr(dwindow_path, L'\\')+1) = NULL;
	wcscat(dwindow_path, L"cache\\");
	_wmkdir(dwindow_path);

	W2UTF8 URL_utf(URL);
	char config_file[4096];
	unsigned long name_crc = crc32(0, (unsigned char*)URL, wcslen(URL) * sizeof(wchar_t));
	USES_CONVERSION;
	sprintf(config_file, "%s%08x", W2A(dwindow_path), name_crc);

	// search for instance
	CAutoLock lck(&g_active_httpfile_list_lock);
	for(std::list<active_httpfile_list_entry>::iterator i = g_active_httpfile_list.begin(); i!= g_active_httpfile_list.end(); ++i)
	{
		if (strcmp((*i).URL_utf, URL_utf) == 0)
		{
			(*i).ref_count ++;
			inet_file *p = (*i).manager;

			return new HTTPHook(p);
		}
	}

	// no match ? create one
	active_httpfile_list_entry entry = {1};
	strcpy(entry.config_file, config_file);
	strcpy(entry.URL_utf, URL_utf);
	entry.manager = new inet_file(UTF82W(config_file));
	if (entry.manager->setURL(URL) <0)
	{
		delete entry.manager;
		return NULL;
	}
	g_active_httpfile_list.push_back(entry);

	return new HTTPHook(entry.manager);
}
HTTPHook::~HTTPHook()			// automatically close()
{
	inet_file *p = (inet_file*)m_core;
	close();
	CAutoLock lck(&g_active_httpfile_list_lock);
	for(std::list<active_httpfile_list_entry>::iterator i = g_active_httpfile_list.begin(); i!= g_active_httpfile_list.end(); ++i)
	{
		if ((*i).manager == p )
		{
			(*i).ref_count --;
			if ((*i).ref_count == 0)
			{
				delete p;
				g_active_httpfile_list.erase(i);
			}
			break;
		}
	}
}
__int64 HTTPHook::size()
{
	return ((inet_file*) m_core)->getsize();
}
__int64 HTTPHook::get(void *buf, __int64 offset, int size)
{
	fragment frag = {offset, offset+size};
	((inet_file*) m_core)->get(buf, frag, &m_closed);
	return frag.end - frag.start;
}
void HTTPHook::close()				// cancel all pending get() operation and reject all further get()
{
	m_closed = true;
}
HTTPHook::HTTPHook(void *core)
:m_core(core)
,m_closed(false)
{

}
std::map<__int64, __int64> HTTPHook::buffer()
{
	std::map<__int64, __int64> o;

	return o;
}