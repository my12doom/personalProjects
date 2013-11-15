#include <Windows.h>
#include "lua.hpp"
#include "TorrentHook.h"

#pragma comment(lib, "lua.lib")

class W2UTF8
{
public:
	W2UTF8(const wchar_t *in);
	~W2UTF8();
	operator char*();
	char *p;
};

class UTF82W
{
public:
	UTF82W(UTF82W &o);
	UTF82W(const char *in);
	~UTF82W();
	operator wchar_t*();
	wchar_t *p;
};

W2UTF8::W2UTF8(const wchar_t *in)
{
	p = NULL;

	if (!in)
		return;

	int len = WideCharToMultiByte(CP_UTF8, NULL, in, -1, NULL, 0, NULL, NULL);
	if (len<0)
		return;

	p = (char*)malloc(len*sizeof(char));
	WideCharToMultiByte(CP_UTF8, NULL, in, -1, p, len, NULL, NULL);
}

W2UTF8::~W2UTF8()
{
	if(p)free(p);
}

W2UTF8::operator char*()
{
	return p;
}

UTF82W::UTF82W(const char *in)
{
	p = NULL;

	if (!in)
		return;

	int len = MultiByteToWideChar(CP_UTF8, NULL, in, -1, NULL, 0);
	if (len<0)
		return;

	p = (wchar_t*)malloc(len*sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, NULL, in, -1, p, len);
}

UTF82W::UTF82W(UTF82W &o)
{
	p = new wchar_t[wcslen(o.p)+1];
	wcscpy(p, o.p);
}
UTF82W::~UTF82W()
{
	if(p)free(p);
}
UTF82W::operator wchar_t*()
{
	return p;
}

static int create_torrent_hooker(lua_State *L)
{
	const char *filename = lua_tostring(L, -1);
	if (!filename)
		return 0;
	UTF82W filenamew(filename);

	TorrentHook *p = TorrentHook::create(filenamew);
	if (!p)
		return 0;

	lua_pushlightuserdata(L, p);
	return 1;
}


extern "C" __declspec(dllexport)  int dwindow_dll_go(lua_State *g_L)
{
	init_torrent_hook(g_L);
	save_all_torrent_state();

	lua_getglobal(g_L, "core");
	lua_getfield(g_L, -1,"register_reader");
	luaL_loadstring(g_L, "return function (URL) return URL:lower():find(\".torrent\") end");
	lua_pcall(g_L, 0, 1, 0);
	lua_pushcfunction(g_L, &create_torrent_hooker);
	lua_pcall(g_L, 2, 1, 0);

	lua_settop(g_L, 0);

	return 0;
}