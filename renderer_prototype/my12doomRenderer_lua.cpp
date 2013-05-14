#include "..\lua\my12doom_lua.h"
#include "my12doomRenderer_lua.h"
#include "my12doomRenderer.h"
#include <windows.h>
#include <atlbase.h>
#include "..\dwindow\dx_player.h"

extern my12doomRenderer *g_renderer;

int my12doomRenderer_lua_loadscript();

static int lua_get_splayer_subtitle(lua_State *L)
{
	int parameter_count = lua_gettop(L);
	if (parameter_count < 1 || lua_tostring(L, -parameter_count) == NULL)
		return 0;

	UTF82W filename (lua_tostring(L, -parameter_count));
	const wchar_t *langs[16] = {L"eng", L"", NULL};
	wchar_t tmp[16][200];
	for(int i=1; i<parameter_count && i<16; i++)
	{
		wcscpy(tmp[i-1], UTF82W(lua_tostring(L, -parameter_count+i)));
		langs[i-1] = tmp[i-1];
	}

	wchar_t out[5000] = {0};
	get_splayer_subtitle(filename, out, langs);

	wchar_t *outs[50] = {0};
	int result_count = wcsexplode(out, L"|", outs, 50);

	for(int i=0; i<result_count; i++)
		lua_pushstring(L, W2UTF8(outs[i]));

	for(int i=0; i<50; i++)
		if (outs[i])
			free(outs[i]);

	return result_count;
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

	RECTF dst_rect = {left, top, right, bottom};
	RECTF src_rect = {s_left, s_top, s_right, s_bottom};
	bool hasROI = s_left > 0 || s_top > 0 || s_right > 0 || s_bottom > 0;

	g_renderer->paint(&dst_rect, resource, hasROI ? &src_rect : NULL, alpha, method);

	lua_pushboolean(L, 1);
	return 1;
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

	resource_userdata *resource = new resource_userdata;
	g_renderer->get_resource(0, resource);
	lua_pushlightuserdata(L, resource);
	return 1;
}

static int load_bitmap_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	const char *filename = lua_tostring(L, parameter_count+0);

	gpu_sample *sample = NULL;
	if (FAILED(g_renderer->loadBitmap(&sample, UTF82W(filename))) || sample == NULL)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "failed loading bitmap file");
		return 2;
	}

	resource_userdata *resource = new resource_userdata;
	resource->resource_type = resource_userdata::RESOURCE_TYPE_GPU_SAMPLE;
	resource->pointer = sample;
	resource->managed = false;
	lua_pushlightuserdata(L, resource);
	lua_pushinteger(L, sample->m_width);
	lua_pushinteger(L, sample->m_height);
	return 3;
}

HFONT create_font(const wchar_t *facename = L"ºÚÌå", int font_height = 14)
{
	LOGFONTW lf={0};
	;
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

static int draw_font_core(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	const char *text = lua_tostring(L, parameter_count+0);

	gpu_sample *sample = NULL;
	RGBQUAD color = {255,255,255,255};
	static HFONT font = create_font();
	wchar_t * p = UTF82W(text);

	wchar_t w[1024];
	MultiByteToWideChar(CP_UTF8, 0, text, -1, w, 1024);



	if (FAILED(g_renderer->drawFont(&sample, font, w, color)) || sample == NULL)
	{
		lua_pushboolean(L, 0);
		lua_pushstring(L, "failed loading bitmap file");
		return 2;
	}

	resource_userdata *resource = new resource_userdata;
	resource->resource_type = resource_userdata::RESOURCE_TYPE_GPU_SAMPLE;
	resource->pointer = sample;
	resource->managed = false;
	lua_pushlightuserdata(L, resource);
	lua_pushinteger(L, sample->m_width);
	lua_pushinteger(L, sample->m_height);
	return 3;
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



//// IPLayer functions

extern dx_player *g_player;
extern double UIScale;
static int get_mouse_pos(lua_State *L)
{
	POINT p;
	GetCursorPos(&p);
	HWND wnd = g_player->get_window((int)g_lua_manager->get_variable("active_view")+1);
	ScreenToClient(wnd, &p);

	lua_pushinteger(L, p.x/UIScale);
	lua_pushinteger(L, p.y/UIScale);

	return 2;
}

static int pause(lua_State *L)
{
	g_player->pause();

	return 0;
}
static int play(lua_State *L)
{
	g_player->play();

	return 0;
}

static int stop(lua_State *L)
{
	g_player->stop();

	return 0;
}

static int toggle_fullscreen(lua_State *L)
{
	g_player->toggle_fullscreen();

	return 0;
}
static int toggle_3d(lua_State *L)
{
	g_player->toggle_force2d();

	return 0;
}


static int is_playing(lua_State *L)
{
	lua_pushboolean(L, g_player->is_playing());

	return 1;
}

static int is_fullscreen(lua_State *L)
{
	lua_pushboolean(L, g_player->is_fullsceen(1));

	return 1;
}

static int tell(lua_State *L)
{
	int t = 0;
	g_player->tell(&t);
	lua_pushinteger(L, t);

	return 1;
}

static int total(lua_State *L)
{
	int t = 0;
	g_player->total(&t);
	lua_pushinteger(L, t);

	return 1;
}

static int seek(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	if (parameter_count >= 0)
		return 0;

	int target = lua_tonumber(L, parameter_count+0);
	g_player->seek(target);

	lua_pushboolean(L, TRUE);
	return 1;
}


static int get_volume(lua_State *L)
{
	double v = 0;
	g_player->get_volume(&v);
	lua_pushnumber(L, v);

	return 1;
}
static int set_volume(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	if (parameter_count >= 0)
		return 0;

	double v = lua_tonumber(L, parameter_count+0);
	g_player->set_volume(v);

	lua_pushboolean(L, TRUE);
	return 1;
}

static int reset_and_loadfile(lua_State *L)
{
	const char *filename1 = lua_tostring(L, -1);
	const char *filename2 = lua_tostring(L, -2);
	const bool stop = lua_isboolean(L, -3) ? lua_toboolean(L, -3) : false;

	HRESULT hr = g_player->reset_and_loadfile(filename1 ? UTF82W(filename1) : NULL, filename2 ? UTF82W(filename2) : NULL, stop);

	lua_pushboolean(L, SUCCEEDED(hr));
	lua_pushinteger(L, hr);
	return 2;
}

static int load_subtitle(lua_State *L)
{
	const char *filename = lua_tostring(L, -1);
	const bool reset = lua_isboolean(L, -2) ? lua_toboolean(L, -2) : false;

	HRESULT hr = g_player->load_subtitle(filename ? UTF82W(filename) : NULL, reset);

	lua_pushboolean(L, SUCCEEDED(hr));
	lua_pushinteger(L, hr);
	return 2;
}

static int show_mouse(lua_State *L)
{
	int parameter_count = -lua_gettop(L);
	if (parameter_count >= 0)
		return 0;

	bool v = lua_toboolean(L, parameter_count+0);
	g_player->show_mouse_core(v);

	lua_pushboolean(L, TRUE);
	return 1;
}
static int popup_menu(lua_State *L)
{
	g_player->popup_menu(g_player->get_window(1));

	return 0;
}


// renderer things
static int set_movie_rect(lua_State *L)
{
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
	g_lua_manager->get_variable("paint_core") = &paint_core;
	g_lua_manager->get_variable("set_clip_rect_core") = &set_clip_rect_core;
	g_lua_manager->get_variable("get_resource") = &get_resource;
	g_lua_manager->get_variable("load_bitmap_core") = &load_bitmap_core;
	g_lua_manager->get_variable("draw_font_core") = &draw_font_core;
	g_lua_manager->get_variable("release_resource_core") = &release_resource_core;
	g_lua_manager->get_variable("commit_resource_core") = &commit_resource_core;
	g_lua_manager->get_variable("decommit_resource_core") = &decommit_resource_core;
	g_lua_manager->get_variable("get_mouse_pos") = &get_mouse_pos;
	g_lua_manager->get_variable("play") = &play;
	g_lua_manager->get_variable("is_playing") = &is_playing;
	g_lua_manager->get_variable("is_fullscreen") = &is_fullscreen;
	g_lua_manager->get_variable("pause") = &pause;
	g_lua_manager->get_variable("stop") = &stop;
	g_lua_manager->get_variable("toggle_fullscreen") = &toggle_fullscreen;
	g_lua_manager->get_variable("total") = &total;
	g_lua_manager->get_variable("tell") = &tell;
	g_lua_manager->get_variable("seek") = &seek;
	g_lua_manager->get_variable("get_volume") = &get_volume;
	g_lua_manager->get_variable("set_volume") = &set_volume;
	g_lua_manager->get_variable("popup_menu") = &popup_menu;
	g_lua_manager->get_variable("show_mouse") = &show_mouse;
	g_lua_manager->get_variable("toggle_3d") = &toggle_3d;
	g_lua_manager->get_variable("reset_and_loadfile") = &reset_and_loadfile;
	g_lua_manager->get_variable("load_subtitle") = &load_subtitle;
	g_lua_manager->get_variable("set_movie_rect") = &set_movie_rect;
	g_lua_manager->get_variable("get_splayer_subtitle") = &lua_get_splayer_subtitle;

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
	if (luaL_loadfile(lua_state, tmp) || lua_mypcall(lua_state, 0, 0, 0))
	{
		const char * result = lua_tostring(lua_state, -1);
		printf("failed loading renderer lua script : %s\n", result);
		lua_settop(lua_state, 0);
	}

// 	strcpy(tmp, apppath);
// 	strcat(tmp, LUA_UI);
// 	if (luaL_loadfile(lua_state, tmp) || lua_mypcall(lua_state, 0, 0, 0))
// 	{
// 		const char * result = lua_tostring(lua_state, -1);
// 		printf("failed loading renderer lua script : %s\n", result);
// 		lua_settop(lua_state, 0);
// 	}

	return 0;
}

