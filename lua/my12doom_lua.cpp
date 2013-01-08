#include <Windows.h>
#include "my12doom_lua.h"

lua_State *L = NULL;

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
  int status, result;
  L = luaL_newstate();  /* create state */
  if (L == NULL) {
	  return EXIT_FAILURE;
  }

  /* open standard libraries */
  luaL_checkversion(L);
  lua_gc(L, LUA_GCSTOP, 0);  /* stop collector during initialization */
  luaL_openlibs(L);  /* open libraries */
  lua_gc(L, LUA_GCRESTART, 0);

  lua_pushcfunction(L, &lua_GetTickCount);
  lua_setglobal(L, "GetTickCount");
  lua_pushcfunction(L, &execute_luafile);
  lua_setglobal(L, "execute_luafile");

  result = lua_toboolean(L, -1);  /* get result */

  return 0;
}

int dwindow_lua_simple_test()
{
	luaL_loadfile(L, "D:\\private\\render.lua");
  int status = lua_pcall(L, 0, 0, 0);
  lua_getglobal(L, "main");
  if (lua_isfunction(L, -1))
  {
	  lua_pushinteger(L, 0);
	  lua_pushinteger(L, 0);
	  lua_pcall(L, 2, 3, 0);
	  lua_getglobal(L, "print");
	  lua_insert(L, -4);
	  lua_pcall(L, 3, 0, 0);
  }

  return status;
}

int dwindow_lua_exit()
{
	if (L == NULL)
		return 0;

	lua_close(L);
	L = NULL;
	return 0;
}
