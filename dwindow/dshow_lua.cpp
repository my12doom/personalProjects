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
static int reset(lua_State *L)
{
	g_player->reset();

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

static int widi_start_scan(lua_State *L)
{
	lua_pushboolean(L, SUCCEEDED(g_player->widi_start_scan()));
	return 1;}
static int widi_get_adapters(lua_State *L)
{

	int c = g_player->m_widi_num_adapters_found;
	for(int i = 0; i<c; i++)
	{
		wchar_t o[500] = L"";
		g_player->widi_get_adapter_by_id(i, o);
		lua_pushstring(L, W2UTF8(o));
	}

	return c;
}
static int widi_get_adapter_information(lua_State *L)
{
	int n = lua_gettop(L);
	int i = lua_tointeger(L, -n+0);
	const char *p = lua_tostring(L, -n+1);
	wchar_t o[500] = L"";
	g_player->widi_get_adapter_information(i, o, p ? UTF82W(p) : NULL);

	lua_pushstring(L, W2UTF8(o));
	return 1;
}
static int widi_connect(lua_State *L)
{
	int n = lua_gettop(L);
	int i = lua_tointeger(L, -n+0);
	DWORD mode = GET_CONST("WidiScreenMode");
	if (n>=2)
		mode = lua_tointeger(L, -n+1);
	
	lua_pushboolean(L, SUCCEEDED(g_player->widi_connect(i, mode)));
	return 1;
}
static int widi_set_screen_mode(lua_State *L)
{
	GET_CONST("WidiScreenMode");
	int n = lua_gettop(L);
	DWORD mode = GET_CONST("WidiScreenMode");
	if (n>=1)
		mode = lua_tointeger(L, -n+0);

	lua_pushboolean(L, SUCCEEDED(g_player->widi_set_screen_mode(mode)));
	return 1;
}
static int widi_disconnect(lua_State *L)
{
	int id = -1;
	int n = lua_gettop(L);
	if (n>=1)
		id = lua_tointeger(L, -n+0);
	lua_pushboolean(L, SUCCEEDED(g_player->widi_disconnect(id)));
	return 1;
}

static int enable_audio_track(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return 0;

	int id = lua_tointeger(L, -1);
	lua_pushboolean(L, SUCCEEDED(g_player->enable_audio_track(id)));
	return 1;
}
static int enable_subtitle_track(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return 0;

	int id = lua_tointeger(L, -1);
	lua_pushboolean(L, SUCCEEDED(g_player->enable_subtitle_track(id)));
	return 1;
}
static int list_audio_track(lua_State *L)
{
	wchar_t tmp[32][1024];
	wchar_t *tmp2[32];
	bool connected[32];
	for(int i=0; i<32; i++)
		tmp2[i] = tmp[i];
	int found = 0;
	g_player->list_audio_track(tmp2, connected, &found);

	for(int i=0; i<found; i++)
	{
		lua_pushstring(L, W2UTF8(tmp[i]));
		lua_pushboolean(L, connected[i]);
	}

	return found * 2;
}
static int list_subtitle_track(lua_State *L)
{
	wchar_t tmp[32][1024];
	wchar_t *tmp2[32];
	bool connected[32];
	for(int i=0; i<32; i++)
		tmp2[i] = tmp[i];
	int found = 0;
	g_player->list_subtitle_track(tmp2, connected, &found);

	for(int i=0; i<found; i++)
	{
		lua_pushstring(L, W2UTF8(tmp[i]));
		lua_pushboolean(L, connected[i]);
	}

	return found * 2;
}

static int lua_find_main_movie(lua_State *L)
{
	if (lua_gettop(L) != 1)
		return 0;
	const char *p = lua_tostring(L, -1);

	wchar_t o[MAX_PATH];
	if (FAILED(find_main_movie(UTF82W(p), o)))
		return 0;

	lua_pushstring(L, W2UTF8(o));
	return 1;
}
static int enum_bd(lua_State *L)
{
	int n = 0;
	for(int i=L'Z'; i>L'B'; i--)
	{
		wchar_t tmp[MAX_PATH] = L"C:\\";
		wchar_t tmp2[MAX_PATH];
		tmp[0] = i;
		tmp[4] = NULL;
		if (GetDriveTypeW(tmp) == DRIVE_CDROM)
		{
			n++;
			lua_pushstring(L, W2UTF8(tmp));
			if (!GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0))
			{
				// no disc
				lua_pushnil(L);
				lua_pushboolean(L, false);
			}
			else if (FAILED(find_main_movie(tmp, tmp2)))
			{
				// not bluray
				GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
				lua_pushstring(L, W2UTF8(tmp2));
				lua_pushboolean(L, false);
			}
			else
			{
				GetVolumeInformationW(tmp, tmp2, MAX_PATH, NULL, NULL, NULL, NULL, 0);
				lua_pushstring(L, W2UTF8(tmp2));
				lua_pushboolean(L, true);
			}

		}
	}

	return n*3;
}
static int enum_drive(lua_State *L)
{
	int n = 0;

#ifdef ZHUZHU
	for(int i=3; i<26; i++)
#else
	for(int i=0; i<26; i++)
#endif
	{
		wchar_t path[50];
		wchar_t volume_name[MAX_PATH];
		swprintf(path, L"%c:\\", L'A'+i);
		if (GetVolumeInformationW(path, volume_name, MAX_PATH, NULL, NULL, NULL, NULL, 0))
#ifdef ZHUZHU
			if (GetDriveTypeW(path) != DRIVE_CDROM)
#endif
		{
			lua_pushstring(L, W2UTF8(path));
			n++;
		}
	}

	return n;

}
static int enum_folder(lua_State *L)
{
	int n = 0;
	if (lua_gettop(L) != 1)
		return 0;

	const char * p = lua_tostring(L, -1);
	if (!p)
		return 0;

	wchar_t path[1024];
	wcscpy(path, UTF82W(p));
	if (path[wcslen(path)-1] != L'\\')
		wcscat(path, L"\\");
	wcscat(path, L"*.*");

	WIN32_FIND_DATAW find_data;
	HANDLE find_handle = FindFirstFileW(path, &find_data);

	if (find_handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)!=0
				&& wcscmp(L".",find_data.cFileName ) !=0
				&& wcscmp(L"..", find_data.cFileName) !=0
				)
			{
				wchar_t tmp[MAX_PATH];
				wcscpy(tmp, find_data.cFileName);
				wcscat(tmp, L"\\");
				lua_pushstring(L, W2UTF8(tmp));
			}
			else if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)==0)

			{
				lua_pushstring(L, W2UTF8(find_data.cFileName));
			}

			n++;
		}
		while( FindNextFile(find_handle, &find_data ) );
	}

	return n;
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
	g_player_lua_manager->get_variable("reset") = &reset;
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

	g_player_lua_manager->get_variable("enable_audio_track") = &enable_audio_track;
	g_player_lua_manager->get_variable("enable_subtitle_track") = &enable_subtitle_track;
	g_player_lua_manager->get_variable("list_audio_track") = &list_audio_track;
	g_player_lua_manager->get_variable("list_subtitle_track") = &list_subtitle_track;

	g_player_lua_manager->get_variable("find_main_movie") = &lua_find_main_movie;
	g_player_lua_manager->get_variable("enum_bd") = &enum_bd;
	g_player_lua_manager->get_variable("enum_drive") = &enum_drive;
	g_player_lua_manager->get_variable("enum_folder") = &enum_folder;

	// widi
	g_player_lua_manager->get_variable("widi_start_scan") = &widi_start_scan;
	g_player_lua_manager->get_variable("widi_get_adapters") = &widi_get_adapters;
	g_player_lua_manager->get_variable("widi_get_adapter_information") = &widi_get_adapter_information;
	g_player_lua_manager->get_variable("widi_connect") = &widi_connect;
	g_player_lua_manager->get_variable("widi_set_screen_mode") = &widi_set_screen_mode;
	g_player_lua_manager->get_variable("widi_disconnect") = &widi_disconnect;

	return 0;
}
