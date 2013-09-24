#include <Windows.h>
#include "lua.hpp"

#pragma comment(lib, "lua.lib")

static int helloworld(lua_State *L)
{
	MessageBoxA(NULL, lua_tostring(L, -1), "HelloWorld!", MB_OK);

	return 0;
}
extern "C" __declspec(dllexport)  int dwindow_dll_go(lua_State *L)
{
	lua_pushcfunction(L, &helloworld);
	lua_setglobal(L, "helloworld");

	return 0;
}