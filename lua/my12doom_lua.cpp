#include <Windows.h>
#include "my12doom_lua.h"

lua_State *g_L = NULL;
CCritSec g_csL;
lua_manager *g_lua_manager = NULL;
const char *table_name = "dwindow";

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

		OutputDebugStringA(result);
		OutputDebugStringA("\nD:\\private\\bp.lua(19,1): error X3000: attempt to call global 'module' (a nil value)");

		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

static int track_back(lua_State *L)
{
	return 0;
}


int dwindow_lua_init () {
  dwindow_lua_exit();
  CAutoLock lck(&g_csL);
  int result;
  g_L = luaL_newstate();  /* create state */
  if (g_L == NULL) {
	  return EXIT_FAILURE;
  }

  /* open standard libraries */
  luaL_checkversion(g_L);
  lua_gc(g_L, LUA_GCSTOP, 0);  /* stop collector during initialization */
  luaL_openlibs(g_L);  /* open libraries */
  lua_gc(g_L, LUA_GCRESTART, 0);


  result = lua_toboolean(g_L, -1);  /* get result */

  g_lua_manager = new lua_manager(g_L, &g_csL);
  g_lua_manager->get_variable("GetTickCount") = &lua_GetTickCount;
  g_lua_manager->get_variable("execute_luafile") = &execute_luafile;
  g_lua_manager->get_variable("track_back") = &track_back;

  return 0;
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


lua_manager::lua_manager(lua_State *L, CCritSec *cs)
{
	m_L = L;
	m_cs = cs;

	CAutoLock lck(cs);

	lua_newtable(L);
	lua_setglobal(L, table_name);
}

lua_manager::~lua_manager()
{
	std::list<lua_global_variable*>::iterator it;
	for(it = m_variables.begin(); it != m_variables.end(); it++)
		delete *it;
}

int lua_manager::refresh()
{
	// TODO
	return 0;
}

int lua_manager::delete_variable(const char*name)
{
	std::list<lua_global_variable*>::iterator it;
	for(it = m_variables.begin(); it != m_variables.end(); it++)
	{
		if (strcmp((*it)->m_name, name) == 0)
		{
			lua_global_variable *tmp = *it;
			m_variables.remove(*it);
			delete tmp;
			return 1;
		}
	}
	
	return 0;
}

lua_global_variable & lua_manager::get_variable(const char*name)
{
	std::list<lua_global_variable*>::iterator it;
	for(it = m_variables.begin(); it != m_variables.end(); it++)
	{
		if (strcmp((*it)->m_name, name) == 0)
		{
			return **it;
		}
	}

	lua_global_variable * p = new lua_global_variable(name, this);
	m_variables.push_back(p);
	return *p;
}

lua_global_variable::lua_global_variable(const char*name, lua_manager *manager)
{
	m_manager = manager;
	m_name = new char [strlen(name)+1];
	strcpy(m_name, name);
}

lua_global_variable::~lua_global_variable()
{
	delete m_name;
};

lua_global_variable::operator int()
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_getfield(L, -1, m_name);

	int o = lua_tointeger(L, -1);

	lua_pop(L, 2);
	
	return o;
}

void lua_global_variable::operator=(const int in)
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_pushinteger(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}


lua_global_variable::operator bool()
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_getfield(L, -1, m_name);

	int o = lua_toboolean(L, -1);

	lua_pop(L, 2);

	return o == 0;
}

void lua_global_variable::operator=(const bool in)
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_pushboolean(L, in ? 1 : 0);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

lua_global_variable::operator double()
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_getfield(L, -1, m_name);

	double o = lua_tonumber(L, -1);

	lua_pop(L, 2);

	return o;
}

void lua_global_variable::operator=(const double in)
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_pushnumber(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}


lua_global_variable::operator const char*()
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_getfield(L, -1, m_name);

	const char* o = lua_tostring(L, -1);

	lua_pop(L, 2);

	return o;
}

void lua_global_variable::operator=(const char *in)
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_pushstring(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

void lua_global_variable::operator=(lua_CFunction func)
{
	CAutoLock lck(m_manager->m_cs);
	lua_State *L = m_manager->m_L;

	lua_getglobal(L, table_name);
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

int lua_mypcall(lua_State *L, int n, int r, int flag)
{
	int o;
	if (o = lua_pcall(L, n, r, flag))
	{
		printf("%s\n", lua_tostring(L, -1));
	}
	return o;
}
