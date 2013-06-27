#include "dshow_lua.h"
#include "global_funcs.h"
#include "dx_player.h"
#include "open_double_file.h"
#include "open_url.h"

lua_manager *g_player_lua_manager = NULL;
extern dx_player *g_player;



extern dx_player *g_player;

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
static int get_force2d(lua_State *L)
{
	bool b;

	g_player->get_force_2d(&b);

	lua_pushboolean(L, b);

	return 1;
}
static int set_force2d(lua_State *L)
{
	bool b = lua_toboolean(L, -1);

	g_player->set_force_2d(b);

	return 0;
}

static int is_playing(lua_State *L)
{
	lua_pushboolean(L, g_player->is_playing());

	return 1;
}
static int get_swapeyes(lua_State *L)
{
	bool b = false;
	g_player->get_swap_eyes(&b);
	lua_pushboolean(L, b);

	return 1;
}
static int set_swapeyes(lua_State *L)
{
	g_player->set_swap_eyes(lua_toboolean(L, -1));

	lua_pushboolean(L, 1);
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
	int n = lua_gettop(L);
	const char *filename1 = lua_tostring(L, -n+0);
	const char *filename2 = lua_tostring(L, -n+1);
	const bool stop = lua_isboolean(L, -n+2) ? lua_toboolean(L, -n+2) : false;

	HRESULT hr = g_player->reset_and_loadfile_core(filename1 ? UTF82W(filename1) : NULL, filename2 ? UTF82W(filename2) : NULL, stop);

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

int player_lua_init()
{
	g_player_lua_manager = new lua_manager("player");
	g_player_lua_manager->get_variable("play") = &play;
	g_player_lua_manager->get_variable("pause") = &pause;
	g_player_lua_manager->get_variable("stop") = &stop;
	g_player_lua_manager->get_variable("tell") = &tell;
	g_player_lua_manager->get_variable("total") = &total;
	g_player_lua_manager->get_variable("seek") = &seek;
	g_player_lua_manager->get_variable("is_playing") = &is_playing;
	g_player_lua_manager->get_variable("reset_and_loadfile") = &reset_and_loadfile;
	g_player_lua_manager->get_variable("load_subtitle") = &load_subtitle;
	g_player_lua_manager->get_variable("is_fullscreen") = &is_fullscreen;
	g_player_lua_manager->get_variable("toggle_fullscreen") = &toggle_fullscreen;
	g_player_lua_manager->get_variable("toggle_3d") = &toggle_3d;
	g_player_lua_manager->get_variable("get_swapeyes") = &get_swapeyes;
	g_player_lua_manager->get_variable("set_swapeyes") = &set_swapeyes;
	g_player_lua_manager->get_variable("get_volume") = &get_volume;
	g_player_lua_manager->get_variable("set_volume") = &set_volume;
	g_player_lua_manager->get_variable("show_mouse") = &show_mouse;
	g_player_lua_manager->get_variable("popup_menu") = &popup_menu;
	g_player_lua_manager->get_variable("get_splayer_subtitle") = &lua_get_splayer_subtitle;

	return 0;
}
