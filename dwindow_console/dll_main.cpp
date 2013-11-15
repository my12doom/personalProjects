#include "..\lua\lua.hpp"
#include "windows.h"
#include "resource.h"

lua_State *L;
int old_print;
char *buf = new char[65536];	// 64k ought to be enough for everybody
wchar_t *bufw = new wchar_t[65536];
char *buf2 = new char[65536];
wchar_t *bufw2 = new wchar_t[65536];
#pragma comment(lib, "lua.lib")
HWND g_lb;

static INT_PTR CALLBACK window_proc( HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg ) 
	{
	case WM_COMMAND:
		{
			int id = LOWORD(wParam);
			if (id == IDC_BUTTON1)
			{
				GetDlgItemTextW(hDlg, IDC_EDIT1, bufw2, 65536);
				int len = WideCharToMultiByte(CP_UTF8, NULL, bufw2, -1, NULL, 0, NULL, NULL);
				if (len<0)
					break;

				WideCharToMultiByte(CP_UTF8, NULL, bufw2, -1, buf2, len, NULL, NULL);
				luaL_dostring(L, buf2);
			}
		}
		break;

	case WM_INITDIALOG:
		g_lb = GetDlgItem(hDlg, IDC_LIST1);
		break;

	case WM_CLOSE:
		EndDialog(hDlg, -1);
		break;

	default:
		return FALSE;
	}

	return TRUE; // Handled message
}

static int myprint(lua_State *L)
{
	int n = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, old_print);
	buf[0] = NULL;
	for(int i=0; i<n; i++)
	{
		if (i>0)
			strcat(buf, "    ");
		lua_getglobal(L, "tostring");
		lua_pushvalue(L, i+1);
		lua_call(L, 1, 1);
		strcat(buf, lua_tostring(L, -1));
	}
	lua_pcall(L, n, 0, 0);


	int len = MultiByteToWideChar(CP_UTF8, NULL, buf, -1, NULL, 0);
	MultiByteToWideChar(CP_UTF8, NULL, buf, -1, bufw, len);
	SendMessageW(g_lb, LB_ADDSTRING, NULL, (LPARAM)bufw);


	return 0;
}

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
DWORD WINAPI thread(LPVOID p)
{
	lua_getglobal(L, "print");
	lua_pushstring(L, "HelloWorld from a thread in dwindow_console.dll!");
	lua_pcall(L, 1, 0, 0);


	DialogBoxW((HINSTANCE)&__ImageBase, MAKEINTRESOURCE(IDD_DIALOG1), NULL, window_proc);

	return 0;
}

extern "C" __declspec(dllexport)  int dwindow_dll_go(lua_State *g_L)
{
	L = lua_newthread(g_L);
	int ref = luaL_ref(g_L, LUA_REGISTRYINDEX);		// won't free it
	lua_getglobal(L, "print");
	int type = lua_type(L, -1);
	old_print = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushcfunction(L, myprint);
	lua_setglobal(L, "print");

	CreateThread(NULL, NULL, thread, NULL, NULL, NULL);
	return 0;
}