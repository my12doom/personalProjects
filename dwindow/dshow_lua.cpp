#include "dshow_lua.h"
#include "global_funcs.h"
#include "dx_player.h"
#include "open_double_file.h"
#include "open_url.h"

CCritSec cs;
lua_manager *g_dshow_lua_manager = NULL;
extern dx_player *g_player;

static int luaOpenFile(lua_State *L)
{
	wchar_t out[1024] = L"";
	if (!open_file_dlg(out, g_player->get_window((int)g_lua_manager->get_variable("active_view")+1)))
		return 0;

	lua_pushstring(L, W2UTF8(out));
	return 1;
}

static int luaOpenDoubleFile(lua_State *L)
{
	wchar_t left[1024] = L"", right[1024] = L"";
	if (FAILED(open_double_file(g_player->m_hexe, g_player->get_window((int)g_lua_manager->get_variable("active_view")+1), left, right)))
		return 0;

	lua_pushstring(L, W2UTF8(left));
	lua_pushstring(L, W2UTF8(right));
	return 2;
}

static int luaOpenFolder(lua_State *L)
{
	wchar_t out[1024] = L"";
	if (!browse_folder(out, g_player->get_window((int)g_lua_manager->get_variable("active_view")+1)))
		return 0;

	lua_pushstring(L, W2UTF8(out));
	return 1;
}

static int luaOpenURL(lua_State *L)
{
	wchar_t url[1024] = L"";
	if (FAILED(open_URL(g_player->m_hexe, g_player->get_window((int)g_lua_manager->get_variable("active_view")+1), url)))
		return 0;

	lua_pushstring(L, W2UTF8(url));
	return 1;
}

int dshow_lua_init()
{
	g_dshow_lua_manager = new lua_manager("dialog");
	g_dshow_lua_manager->get_variable("OpenFile") = &luaOpenFile;
	g_dshow_lua_manager->get_variable("OpenDoubleFile") = &luaOpenDoubleFile;
	g_dshow_lua_manager->get_variable("OpenFolder") = &luaOpenFolder;
	g_dshow_lua_manager->get_variable("OpenURL") = &luaOpenURL;

	return 0;
}