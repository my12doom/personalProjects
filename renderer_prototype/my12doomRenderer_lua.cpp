#include "..\lua\my12doom_lua.h"
#include "my12doomRenderer_lua.h"
#include "my12doomRenderer.h"
#include <windows.h>
#include <atlbase.h>

extern my12doomRenderer *g_renderer;

int my12doomRenderer_lua_loadscript();

static int paint(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int left = lua_tointeger(L, parameter_count+0);
	int top = lua_tointeger(L, parameter_count+1);
	int right = lua_tointeger(L, parameter_count+2);
	int bottom = lua_tointeger(L, parameter_count+3);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+4);

	g_renderer->paint(left, top, right, bottom, resource);

	lua_pushboolean(L, 1);
	return 1;
}

static int get_resource(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int arg = lua_tointeger(L, parameter_count+0);

	resource_userdata *resource = new resource_userdata;
	g_renderer->get_resource(0, resource);
	lua_pushlightuserdata(L, resource);
	return 1;
}

static int load_bitmap_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	const char *filename = lua_tostring(L, parameter_count+0);

	USES_CONVERSION;
	gpu_sample *sample = NULL;
	if (FAILED(g_renderer->loadBitmap(&sample, A2W(filename))) || sample == NULL)
		return 0;

	resource_userdata *resource = new resource_userdata;
	resource->resource_type = resource_userdata::RESOURCE_TYPE_GPU_SAMPLE;
	resource->pointer = sample;
	resource->managed = false;
	lua_pushlightuserdata(L, resource);
	return 1;
}

static int release_resource_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+0);
	bool is_decommit = lua_toboolean(L, parameter_count+1);

	if (resource == NULL) 
		return 0;

	if (resource->pointer == NULL || resource->managed)
		return 0;

	if (resource->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		delete ((gpu_sample*)resource->pointer);

	delete resource;

	return 0;
}

static int commit_resource_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+0);

	if (resource == NULL) 
		return 0;

	if (resource->pointer == NULL || resource->managed)
		return 0;

	if (resource->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		((gpu_sample*)resource->pointer)->commit();

	return 0;
}


static int decommit_resource_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+0);

	if (resource == NULL) 
		return 0;

	if (resource->pointer == NULL || resource->managed)
		return 0;

	if (resource->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		((gpu_sample*)resource->pointer)->commit();

	return 0;
}


static int myprint(lua_State *L)
{
	return 0;
}

int my12doomRenderer_lua_init()
{
	lua_pushcfunction(L, &paint);
	lua_setglobal(L, "paint");

	lua_pushcfunction(L, &get_resource);
	lua_setglobal(L, "get_resource");

	lua_pushcfunction(L, &load_bitmap_core);
	lua_setglobal(L, "load_bitmap_core");

	lua_pushcfunction(L, &release_resource_core);
	lua_setglobal(L, "release_resource_core");

	lua_pushcfunction(L, &commit_resource_core);
	lua_setglobal(L, "commit_resource_core");

	lua_pushcfunction(L, &decommit_resource_core);
	lua_setglobal(L, "decommit_resource_core");

	lua_pushcfunction(L, &myprint);
	lua_setglobal(L, "print");

	my12doomRenderer_lua_loadscript();

	return 0;
}

int my12doomRenderer_lua_loadscript()
{
	if (LUA_OK == luaL_loadfile(L, "D:\\private\\render.lua"))
		lua_pcall(L, 0, 0, 0);
	else
	{
		const char * result = lua_tostring(L, -1);
		printf("failed loading renderer lua script : %s\n", result);
		lua_settop(L, 0);
	}

	return 0;
}