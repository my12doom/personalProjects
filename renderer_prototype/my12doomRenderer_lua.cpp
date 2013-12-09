#include "..\lua\my12doom_lua.h"
#include "my12doomRenderer_lua.h"
#include "my12doomRenderer.h"
#include <windows.h>
#include <atlbase.h>
#include "..\dwindow\dx_player.h"
#include "..\hookdshow\hookdshow.h"
#include "..\dwindow\dwindow_log.h"

extern my12doomRenderer *g_renderer;
lua_manager *g_lua_dx9_manager = NULL;

int my12doomRenderer_lua_loadscript();

static int release_resource_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+0);

	if (resource == NULL) 
		return 0;

	if (resource->pointer == NULL || resource->managed)
		return 0;

	int type = resource->resource_type;
	resource->resource_type = resource_userdata::RESOURCE_TYPE_RELEASED;

	if (type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		delete ((gpu_sample*)resource->pointer);

	return 0;
}

static bool setup_gpu_sample_gc(lua_State *L)
{
	return setup_gc(L, &release_resource_core);
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
		((gpu_sample*)resource->pointer)->decommit();

	return 0;
}


static int paint_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int left = lua_tointeger(L, parameter_count+0);
	int top = lua_tointeger(L, parameter_count+1);
	int right = lua_tointeger(L, parameter_count+2);
	int bottom = lua_tointeger(L, parameter_count+3);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+4);

	int s_left = lua_tointeger(L, parameter_count+5);
	int s_top = lua_tointeger(L, parameter_count+6);
	int s_right = lua_tointeger(L, parameter_count+7);
	int s_bottom = lua_tointeger(L, parameter_count+8);
	double alpha = lua_tonumber(L, parameter_count+9);
	resampling_method method = (resampling_method)(int)lua_tointeger(L, parameter_count+10);
	resource_userdata *rrt = NULL; 
	if (-parameter_count>=12)
		rrt = (resource_userdata*)lua_touserdata(L, parameter_count+11);
	gpu_sample *rt = NULL;
	if (rrt && rrt->resource_type == resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		rt = (gpu_sample*)rrt->pointer;



	RECTF dst_rect = {left, top, right, bottom};
	RECTF src_rect = {s_left, s_top, s_right, s_bottom};
	bool hasROI = s_left > 0 || s_top > 0 || s_right > 0 || s_bottom > 0;

	g_renderer->paint(&dst_rect, resource, hasROI ? &src_rect : NULL, alpha, method, rt);

	lua_pushboolean(L, 1);
	return 1;
}

static int clear_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int left = lua_tointeger(L, parameter_count+0);
	int top = lua_tointeger(L, parameter_count+1);
	int right = lua_tointeger(L, parameter_count+2);
	int bottom = lua_tointeger(L, parameter_count+3);
	resource_userdata *resource = (resource_userdata*)lua_touserdata(L, parameter_count+4);

	if (!resource || resource->resource_type != resource_userdata::RESOURCE_TYPE_GPU_SAMPLE)
		return 0;

	RECT rect = {left, top, right, bottom};
	HRESULT hr = g_renderer->clear((gpu_sample*)resource->pointer, rect);

	lua_pushboolean(L, 1);
	return 1;
}

static int lock_frame(lua_State *L)
{
	g_renderer->m_frame_lock.Lock();
	g_renderer->m_rendered_packet_lock.Lock();
	return 0;
}
static int unlock_frame(lua_State *L)
{
	g_renderer->m_frame_lock.Unlock();
	g_renderer->m_rendered_packet_lock.Unlock();
	return 0;
}

static int set_clip_rect_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int left = lua_tointeger(L, parameter_count+0);
	int top = lua_tointeger(L, parameter_count+1);
	int right = lua_tointeger(L, parameter_count+2);
	int bottom = lua_tointeger(L, parameter_count+3);

	g_renderer->set_clip_rect(left, top, right, bottom);

	lua_pushboolean(L, 1);
	return 1;
}
static int get_resource(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int arg = lua_tointeger(L, parameter_count+0);

	resource_userdata *resource = (resource_userdata*)lua_newuserdata(L, sizeof(resource_userdata));
	g_renderer->get_resource(0, resource);
	return 1;
}

static int create_rt(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	int width = lua_tointeger(L, parameter_count+0);
	int height = lua_tointeger(L, parameter_count+1);

	resource_userdata *resource = (resource_userdata*)lua_newuserdata(L, sizeof(resource_userdata));
	HRESULT hr = g_renderer->create_rt(width, height, resource);

	setup_gpu_sample_gc(L);

	return 1;
}
static int load_bitmap_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	const char *filename = lua_tostring(L, parameter_count+0);
	UTF82W filenamew(filename);

	gpu_sample *sample = NULL;
	if (FAILED(g_renderer->loadBitmap(&sample, wcsstr(filenamew, L"http://") == filenamew ? URL2Token(filenamew) :filenamew)) || sample == NULL)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "failed loading bitmap file");
		return 2;
	}

	resource_userdata *resource = (resource_userdata*)lua_newuserdata(L, sizeof(resource_userdata));
	resource->resource_type = resource_userdata::RESOURCE_TYPE_GPU_SAMPLE;
	resource->pointer = sample;
	resource->managed = false;
	setup_gpu_sample_gc(L);
	lua_pushinteger(L, sample->m_width);
	lua_pushinteger(L, sample->m_height);
	return 3;
}

HFONT create_font(const wchar_t *facename = L"ºÚÌå", int font_height = 14)
{
	LOGFONTW lf={0};
	lf.lfHeight = -font_height;
	lf.lfCharSet = GB2312_CHARSET;
	lf.lfOutPrecision =  OUT_STROKE_PRECIS;
	lf.lfClipPrecision = CLIP_STROKE_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = VARIABLE_PITCH;
	lf.lfWeight = FW_BOLD*3;
	lstrcpynW(lf.lfFaceName, facename, 32);

	HFONT rtn = CreateFontIndirectW(&lf); 

	return rtn;
}

static int luaCreateFont(lua_State *L)
{
	if(!lua_istable(L, -1))
		return 0;

	LOGFONTW lf=
	{
		-14,
		0,
		0,
		0,
		FW_BOLD,
		0,
		0,
		0,
		0,
		OUT_STROKE_PRECIS,
		CLIP_STROKE_PRECIS,
		DEFAULT_QUALITY,
		VARIABLE_PITCH,
		L"ºÚÌå",
	};

	char *key_table[] =
	{
		"height",
		"width",
		"escapement",
		"orientation",
		"weight",
		"italic",
		"underline",
		"strikeout",
		"charset",
		"outprecision",
		"clipprecision",
		"quality",
		"pitch",
		"name",
	};

	// the first 5 elements is LONG, then 8 BYTE, the one WCHAR[32]
	for(int i=0; i<sizeof(key_table)/sizeof(char*); i++)
	{
		lua_getfield(L, -1, key_table[i]);
		if (lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			continue;;
		}

		if (i<5)
			((DWORD*)&lf)[i] = lua_tointeger(L, -1);
		else if (i<13)
			((BYTE*)&lf)[5*4+i] = lua_tointeger(L, -1);
		else if (lua_tostring(L, -1))
			lstrcpynW(lf.lfFaceName, UTF82W(lua_tostring(L, -1)), 32);

		lua_pop(L,1);
	}

	lua_pushlightuserdata(L, CreateFontIndirectW(&lf));
	return 1;
}

static int luaReleaseFont(lua_State *L)
{
	HFONT f = (HFONT)lua_touserdata(L, -1);

	if (f)
		DeleteObject(f);

	return 0;
}

static int draw_font_core(lua_State *L)
{
	static HFONT default_font = create_font();
	static RGBQUAD default_color = {255,255,255,255};

	int parameter_count = -lua_gettop(L);
	const char *text = lua_tostring(L, parameter_count+0);
	HFONT font = (HFONT)lua_touserdata(L, parameter_count+1);
	if (!font) font = default_font;
	RGBQUAD color = default_color;
	if (parameter_count == -3)
	{
		DWORD color_dw = lua_tointeger(L, parameter_count+2);
		color = *(RGBQUAD*)&color_dw;
	}

	gpu_sample *sample = NULL;

	if (FAILED(g_renderer->drawFont(&sample, font, UTF82W(text), color)) || sample == NULL)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "failed loading bitmap file");
		return 2;
	}

	resource_userdata *resource = (resource_userdata*)lua_newuserdata(L, sizeof(resource_userdata));
	resource->resource_type = resource_userdata::RESOURCE_TYPE_GPU_SAMPLE;
	resource->pointer = sample;
	resource->managed = false;
	setup_gpu_sample_gc(L);
	lua_pushinteger(L, sample->m_width);
	lua_pushinteger(L, sample->m_height);
	return 3;
}

int old_print = -1;
static int myprint(lua_State *L)
{
	int n = lua_gettop(L);
	lua_rawgeti(L, LUA_REGISTRYINDEX, old_print);
	char *buf = new char[200*1024];
	buf[0] = NULL;
	for(int i=0; i<n; i++)
	{
		if (i>0)
			strcat(buf, "    ");
		lua_getglobal(L, "tostring");
		lua_pushvalue(L, i+1);
		lua_call(L, 1, 1);
		const char * str = lua_tostring(L, -1);
		int len = lua_rawlen(L, -1);
		len = len > 1024? 1024:len;
		*(buf+strlen(buf)+len) = NULL;
		strncpy(buf+strlen(buf), str, len);
	}
	lua_pcall(L, n, 0, 0);


	int len = MultiByteToWideChar(CP_UTF8, NULL, buf, -1, NULL, 0);
	wchar_t *bufw = new wchar_t[len];
	MultiByteToWideChar(CP_UTF8, NULL, buf, -1, bufw, len);

	dwindow_log_line(L"(lua)%s", bufw);

	delete bufw;
	delete buf;

	return 0;
}

static int render(lua_State *L)
{
	g_renderer->repaint_video();
	return 0;
}
static int is2DRendering(lua_State *L)
{
	bool b = g_renderer->is2DRendering();
	lua_pushboolean(L, b);
	return 1;
}

static int is2DMovie(lua_State *L)
{
	bool b = g_renderer->is2DMovie();
	lua_pushboolean(L, b);
	return 1;
}


// renderer things
static int set_movie_rect(lua_State *L)
{
	if (lua_gettop(L) >= 1 && lua_isnil(L, -1))
	{
		g_renderer->set_movie_scissor_rect(NULL);
		lua_pushboolean(L, TRUE);
		return 1;
	}


	int parameter_count = -lua_gettop(L);
	if (parameter_count > -4)
		return 0;

	float left = lua_tonumber(L, parameter_count+0);
	float top = lua_tonumber(L, parameter_count+1);
	float right = lua_tonumber(L, parameter_count+2);
	float bottom = lua_tonumber(L, parameter_count+3);

	RECTF rect = {left, top, right, bottom};
	g_renderer->set_movie_scissor_rect(&rect);
	lua_pushboolean(L, TRUE);
	return 1;
}

int my12doomRenderer_lua_init()
{
	// print hook
	luaState L;
	lua_getglobal(L, "print");
	old_print = luaL_ref(L, LUA_REGISTRYINDEX);
	lua_pushcfunction(L, &myprint);
	lua_setglobal(L, "print");

	// dx9
	g_lua_dx9_manager = new lua_manager("dx9");
	g_lua_dx9_manager->get_variable("paint_core") = &paint_core;
	g_lua_dx9_manager->get_variable("clear_core") = &clear_core;
	g_lua_dx9_manager->get_variable("set_clip_rect_core") = &set_clip_rect_core;
	g_lua_dx9_manager->get_variable("get_resource") = &get_resource;
	g_lua_dx9_manager->get_variable("create_rt") = &create_rt;
	g_lua_dx9_manager->get_variable("load_bitmap_core") = &load_bitmap_core;
	g_lua_dx9_manager->get_variable("draw_font_core") = &draw_font_core;
	g_lua_dx9_manager->get_variable("release_resource_core") = &release_resource_core;
	g_lua_dx9_manager->get_variable("commit_resource_core") = &commit_resource_core;
	g_lua_dx9_manager->get_variable("decommit_resource_core") = &decommit_resource_core;
	g_lua_dx9_manager->get_variable("CreateFont") = &luaCreateFont;
	g_lua_dx9_manager->get_variable("ReleaseFont") = &luaReleaseFont;
	g_lua_dx9_manager->get_variable("set_movie_rect") = &set_movie_rect;
	g_lua_dx9_manager->get_variable("is2DRendering") = &is2DRendering;
	g_lua_dx9_manager->get_variable("is2DMovie") = &is2DMovie;
	g_lua_dx9_manager->get_variable("lock_frame") = &lock_frame;
	g_lua_dx9_manager->get_variable("unlock_frame") = &unlock_frame;
	g_lua_dx9_manager->get_variable("render") = &render;

	return 0;
}

#ifdef DEBUG
	#define BASE_FRAME "..\\..\\dwindow_UI\\dwindow.lua"
	#define LUA_UI "..\\..\\dwindow_UI\\3dvplayer\\render.lua"
#else
	#define BASE_FRAME "UI\\dwindow.lua"
	#ifdef VSTAR
		#define LUA_UI "UI\\3dvplayer\\render.lua"
	#else
		#define LUA_UI "UI\\classic\\render.lua"
	#endif
#endif

int my12doomRenderer_lua_loadscript()
{
	luaState lua_state;
	char apppath[MAX_PATH];
	GetModuleFileNameA(NULL, apppath, MAX_PATH);
	*((char*)strrchr(apppath, '\\')+1) = NULL;
	char tmp[MAX_PATH];
	strcpy(tmp, apppath);
	strcat(tmp, BASE_FRAME);
	USES_CONVERSION;
	g_lua_core_manager->get_variable("loading_file") = A2W(tmp);
	if (luaL_loadfile(lua_state, tmp) || lua_mypcall(lua_state, 0, 0, 0))
	{
		const char * result = lua_tostring(lua_state, -1);
		printf("failed loading renderer lua script : %s\n", result);
		lua_settop(lua_state, 0);
	}

	return 0;
}

