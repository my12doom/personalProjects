#pragma once

#include <streams.h>
extern "C"
{
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
int dwindow_lua_init();
int dwindow_lua_exit();
int dwindow_lua_simple_test();
extern lua_State *g_L;
extern CCritSec g_csL;