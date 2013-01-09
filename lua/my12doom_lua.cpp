#include <Windows.h>
#include "my12doom_lua.h"

lua_State *g_L = NULL;
CCritSec g_csL;

static int lua_GetTickCount (lua_State *L) 
{
	lua_pushinteger(L, GetTickCount());
	return 1;
}


static int execute_luafile(lua_State *L)
{
// 	int parameter_count = lua_gettop(L);
	const char *filename = NULL;
	filename = luaL_checkstring(L, 1);
	if(luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0))
	{
		const char * result;
		result = lua_tostring(L, -1);
		lua_pushboolean(L, 0);
		lua_pushstring(L, result);
		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}


int dwindow_lua_init () {
  dwindow_lua_exit();
  CAutoLock lck(&g_csL);
  int status, result;
  g_L = luaL_newstate();  /* create state */
  if (g_L == NULL) {
	  return EXIT_FAILURE;
  }

  /* open standard libraries */
  luaL_checkversion(g_L);
  lua_gc(g_L, LUA_GCSTOP, 0);  /* stop collector during initialization */
  luaL_openlibs(g_L);  /* open libraries */
  lua_gc(g_L, LUA_GCRESTART, 0);

  lua_pushcfunction(g_L, &lua_GetTickCount);
  lua_setglobal(g_L, "GetTickCount");
  lua_pushcfunction(g_L, &execute_luafile);
  lua_setglobal(g_L, "execute_luafile");

  result = lua_toboolean(g_L, -1);  /* get result */

  return 0;
}

int dwindow_lua_simple_test()
{
	CAutoLock lck(&g_csL);
	luaL_loadfile(g_L, "D:\\private\\render.lua");
  int status = lua_pcall(g_L, 0, 0, 0);
  lua_getglobal(g_L, "main");
  if (lua_isfunction(g_L, -1))
  {
	  lua_pushinteger(g_L, 0);
	  lua_pushinteger(g_L, 0);
	  lua_pcall(g_L, 2, 3, 0);
	  lua_getglobal(g_L, "print");
	  lua_insert(g_L, -4);
	  lua_pcall(g_L, 3, 0, 0);
  }

  return status;
}

int dwindow_lua_exit()
{
	CAutoLock lck(&g_csL);
	if (g_L == NULL)
		return 0;

	lua_close(g_L);
	g_L = NULL;
	return 0;
}
