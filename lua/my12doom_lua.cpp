#include <Windows.h>
#include "my12doom_lua.h"
#include <list>
#include <list>
#include "..\dwindow\global_funcs.h"

lua_State *g_L = NULL;
CCritSec g_csL;
lua_manager *g_lua_manager = NULL;
std::list<lua_State*> free_threads;

static int lua_GetTickCount (lua_State *L) 
{
	lua_pushinteger(L, timeGetTime());
	return 1;
}

static int lua_Sleep(lua_State *L)
{
	DWORD time = lua_tointeger(L, -1);
	Sleep(time);

	return 0;
}

static int execute_luafile(lua_State *L)
{
// 	int parameter_count = lua_gettop(L);
	const char *filename = NULL;
	filename = luaL_checkstring(L, 1);
	USES_CONVERSION;
	g_lua_manager->get_variable("loading_file") = W2UTF8(A2W(filename));
	if(luaL_loadfile(L, W2A(UTF82W(filename))) || lua_pcall(L, 0, 0, 0))
	{
		const char * result;
		result = lua_tostring(L, -1);
		lua_pushboolean(L, 0);
		lua_pushstring(L, W2UTF8(A2W(result)));

		printf("execute_luafile(%s)failed: %s\n", filename, result);

		return 2;
	}

	lua_pushboolean(L, 1);
	return 1;
}

static int http_request(lua_State *L)
{
	const char *url = NULL;
	url = luaL_checkstring(L, 1);

	char *buffer = new char[1024*1024];
	int size = 1024*1024;
	download_url((char*)url, buffer, &size);

	lua_pushlstring(L, buffer, size);
	delete buffer;
	lua_pushnumber(L, 200);
	return 2;
}

static int track_back(lua_State *L)
{
	return 0;
}


// threads

int luaResumeThread(lua_State *L)
{
	HANDLE h = (HANDLE)lua_touserdata(L, -1);
	if (h == NULL)
		return 0;

	ResumeThread(h);
	return 0;
}
int luaSuspendThread(lua_State *L)
{
	HANDLE h = (HANDLE)lua_touserdata(L, -1);
	if (h == NULL)
		return 0;
	SuspendThread(h);
	return 0;
}
int luaTerminateThread(lua_State *L)
{
	int n = -lua_gettop(L);
	HANDLE h = (HANDLE)lua_touserdata(L, n+0);
	DWORD exitcode = lua_tointeger(L, n+1);
	if (h == NULL)
		return 0;
	TerminateThread(h, exitcode);
	return 0;
}int luaWaitForSingleObject(lua_State *L)
{
	int n = -lua_gettop(L);
	HANDLE h = (HANDLE)lua_touserdata(L, n+0);
	if (h == NULL)
		return 0;
	DWORD timeout = lua_tointeger(L, n+2);
	DWORD o = WaitForSingleObject(h, timeout);

	lua_pushinteger(L, o);
	return 1;
}

DWORD WINAPI luaCreateThreadEntry(LPVOID parameter)
{
	luaState L;

	int *p = (int*)parameter;
	for(int i=p[0]; i>=0; i--)
	{
		lua_rawgeti(L, LUA_REGISTRYINDEX, p[i+1]);
		luaL_unref(L, LUA_REGISTRYINDEX, p[i+1]);
	}

	if (!lua_isfunction(L, -p[0]))
	{
		lua_pop(L, p[0]);
		delete p;
		return -2;					// no entry function found, clear the stack and return;
	}

	lua_mypcall(L, p[0]-1, 0, 0);

	delete p;

	return 0;
}

int luaCreateThread(lua_State *L)
{
	int n = lua_gettop(L);

	int * p = new int[n+1];
	p[0] = n;
	for(int i=0; i<n; i++)
		p[i+1] = luaL_ref(L, LUA_REGISTRYINDEX);

	lua_pushlightuserdata(L, CreateThread(NULL, NULL, luaCreateThreadEntry, p, NULL, NULL));
	return 1;
}


int luaFAILED(lua_State *L)
{
	HRESULT hr = lua_tointeger(L, -1);
	lua_pushboolean(L, FAILED(hr));
	return 1;
}
extern "C" int luaopen_cjson_safe(lua_State *l);

int dwindow_lua_init () 
{
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

  g_lua_manager = new lua_manager("dwindow");
  g_lua_manager->get_variable("FAILED") = &luaFAILED;
  g_lua_manager->get_variable("GetTickCount") = &lua_GetTickCount;
  g_lua_manager->get_variable("execute_luafile") = &execute_luafile;
  g_lua_manager->get_variable("track_back") = &track_back;
  g_lua_manager->get_variable("http_request") = &http_request;
  g_lua_manager->get_variable("Sleep") = &lua_Sleep;

  // threads
  g_lua_manager->get_variable("ResumeThread") = &luaResumeThread;
  g_lua_manager->get_variable("SuspendThread") = &luaSuspendThread;
  g_lua_manager->get_variable("WaitForSingleObject") = &luaWaitForSingleObject;
  g_lua_manager->get_variable("CreateThread") = &luaCreateThread;
  g_lua_manager->get_variable("TerminateThread") = &luaTerminateThread;

  g_lua_manager->get_variable("cjson") = &luaopen_cjson_safe;

  return 0;
}

lua_State * dwindow_lua_get_thread()
{
	CAutoLock lck(&g_csL);
	lua_State *rtn = NULL;
	if (!free_threads.empty())
	{
		rtn = free_threads.front();
		free_threads.pop_front();
	}
	else
	{
		rtn = lua_newthread(g_L);
	}
	return rtn;
}

void dwindow_lua_release_thread(lua_State * p)
{
	CAutoLock lck(&g_csL);
	free_threads.push_back(p);
}

luaState::luaState()
{
	L =dwindow_lua_get_thread();
}

luaState::~luaState()
{
	dwindow_lua_release_thread(L);
}
luaState::operator lua_State*()
{
	return L;
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

lua_manager::lua_manager(const char* table_name)
{
	m_table_name = new char[strlen(table_name)+1];
	strcpy(m_table_name, table_name);

	CAutoLock lck(&m_cs);

	luaState L;

	lua_getglobal(L, table_name);
	if (!lua_istable(L, -1))
	{
		lua_newtable(L);
		lua_setglobal(L, table_name);
	}
	lua_settop(L,0);
}

lua_manager::~lua_manager()
{
	std::list<lua_global_variable*>::iterator it;
	for(it = m_variables.begin(); it != m_variables.end(); it++)
		delete *it;
	delete [] m_table_name;
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
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);

	int o = lua_tointeger(L, -1);

	lua_pop(L, 2);
	
	return o;
}

void lua_global_variable::operator=(const int in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushinteger(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}


lua_global_variable::operator bool()
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);

	int o = lua_toboolean(L, -1);

	lua_pop(L, 2);

	return o == 0;
}

void lua_global_variable::operator=(const bool in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushboolean(L, in ? 1 : 0);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

lua_global_variable::operator double()
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);

	double o = lua_tonumber(L, -1);

	lua_pop(L, 2);

	return o;
}

void lua_global_variable::operator=(const double in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushnumber(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}


lua_global_variable::operator const char*()
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_getfield(L, -1, m_name);

	const char* o = lua_tostring(L, -1);

	lua_pop(L, 2);

	return o;
}

void lua_global_variable::operator=(const char *in)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushstring(L, in);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

void lua_global_variable::operator=(lua_CFunction func)
{
	CAutoLock lck(&m_manager->m_cs);
	luaState L;

	lua_getglobal(L, m_manager->m_table_name);
	lua_pushcfunction(L, func);
	lua_setfield(L, -2, m_name);
	lua_pop(L, 1);
}

int lua_track_back(lua_State *L)
{
	const char* err = lua_tostring(L, -1);

#ifdef DEBUG
	char tmp[10240];
	lua_Debug debug;
	for(int level = 1; lua_getstack(L, level, &debug); level++)
	{
		int suc = lua_getinfo(L, "Sl", &debug);
		sprintf(tmp, "%s(%d,1) : %s \n", debug.source+1, debug.currentline, level == 1 ? strrchr(err, ':')+1 : "");
		OutputDebugStringA(tmp);
	}
	DebugBreak();
#endif

	lua_pushstring(L, err);
	return 1;
}

int lua_mypcall(lua_State *L, int n, int r, int flag)
{
	lua_pushcfunction(L, &lua_track_back);
	lua_insert(L, lua_gettop(L) - n - 1);
	int o = lua_pcall(L, n, r, lua_gettop(L) - n - 1);
	lua_pop(L, 1);
	return o;
}
