#pragma once

#include <streams.h>
#include <list>

extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}

int dwindow_lua_init();
int dwindow_lua_exit();
extern lua_State *g_L;
extern CCritSec g_csL;

class lua_global_variable;

class lua_manager
{
public:
	lua_manager(lua_State *L, CCritSec *cs);
	~lua_manager();
	
	int refresh();
	lua_global_variable & get_variable(const char*name);
	int delete_variable(const char*name);

protected:
	friend class lua_global_variable;
	lua_State *m_L;
	CCritSec *m_cs;
	std::list<lua_global_variable*> m_variables;
};

class lua_global_variable
{
public:
	operator int();
	operator double();
	operator const char*();
	operator const wchar_t*();
	operator bool();
	void operator=(const int in);
	void operator=(const double in);
	void operator=(const char* in);
	void operator=(const wchar_t* in);
	void operator=(const bool in);
	void operator=(lua_CFunction func);		// write only
protected:
	friend class lua_manager;
	lua_global_variable(const char*name, lua_manager *manager);
	~lua_global_variable();
	lua_manager *m_manager;
	char *m_name;
};

extern lua_manager *g_lua_manager;