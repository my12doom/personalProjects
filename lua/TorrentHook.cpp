#include <stdlib.h>
#include <Windows.h>
#include "TorrentHook.h"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include <libtorrent/alert_types.hpp>
#include <boost/unordered_set.hpp>
#include <conio.h>
#include "libtorrent_reader.h"
#include <atlbase.h>


using namespace libtorrent;
session *s;
lua_State *g_myL;
CRITICAL_SECTION cs;

int init_torrent_hook(lua_State *L)
{
	InitializeCriticalSection(&cs);
	EnterCriticalSection(&cs);
	g_myL = L;

	s = new session();

	error_code ec;
	s->listen_on(std::make_pair(7881, 7889), ec);
	if (ec)
	{
		fprintf(stderr, "failed to open listen socket: %s\n", ec.message().c_str());
		return 1;
	}





 	s->add_dht_router(std::make_pair(std::string("router.bittorrent.com"), 6881));
 	s->add_dht_router(std::make_pair(std::string("router.utorrent.com"), 6881));
 	s->add_dht_router(std::make_pair(std::string("router.bitcomet.com"), 6881));
 	s->start_dht();

	session_settings settings = s->settings();
 	settings.broadcast_lsd = true;
	settings.allow_multiple_connections_per_ip = true;
	settings.peer_timeout = 10;
 	settings.user_agent = "DWindow";
 	s->set_settings(settings);

// 	lua_getglobal(g_myL, "bittorrent");
// 	lua_getfield(g_myL, -1, "load_session");
// 	lua_pcall(g_myL, 0, 1, 0);
// 
// 	if (lua_type(g_myL, -1) == LUA_TSTRING)
// 	{
// 		int size = lua_rawlen(g_myL, -1);
// 		const char* buf = lua_tostring(g_myL, -1);
// 
// 		lazy_entry load_state;
// 		lazy_bdecode(buf, buf+size, load_state);
// 		s->load_state(load_state);
// 	}

	lua_settop(g_myL,0);
	LeaveCriticalSection(&cs);

	return 0;
}

int save_all_torrent_state()
{
	int saving = 0;
	std::vector<torrent_handle> torrents = s->get_torrents();
	for(int i=0; i<torrents.size(); i++)
	{
		saving++;
		torrent_handle handle = torrents[i];
		handle.save_resume_data();
	}

	while (saving)
	{
		// loop through the alert queue to see if anything has happened.
		std::deque<alert*> alerts;
		s->pop_alerts(&alerts);
		for (std::deque<alert*>::iterator i = alerts.begin(), end(alerts.end()); i != end; ++i)
		{
			TORRENT_TRY
			{
				if (save_resume_data_alert* p = alert_cast<save_resume_data_alert>(*i))
				{
					--saving;
					torrent_handle h = p->handle;
					if (p->resume_data)
					{
						std::vector<char> out;
						bencode(std::back_inserter(out), *p->resume_data);						

						char info_hash_str[41] = {0};
						std::string info_hash = h.get_torrent_info().info_hash().to_string();
						for(int i=0; i<20; i++)
							sprintf(info_hash_str+i*2, "%02X", (unsigned char)info_hash[i]);
						
						EnterCriticalSection(&cs);
						lua_getglobal(g_myL, "bittorrent");
						lua_getfield(g_myL, -1, "save_torrent");
						lua_pushstring(g_myL, info_hash_str);
						lua_pushlstring(g_myL, &out[0], out.size());						
						lua_pcall(g_myL, 2, 0, 0);
						lua_settop(g_myL,0);
						LeaveCriticalSection(&cs);
					}
				}
				else if (save_resume_data_failed_alert* p = alert_cast<save_resume_data_failed_alert>(*i))
				{
					--saving;
				}
			} TORRENT_CATCH(std::exception& e) {}

			delete *i;
		}
		alerts.clear();
	}

	// save session state
	entry session_state;
	s->save_state(session_state);
	std::vector<char> out;
	bencode(std::back_inserter(out), session_state);

	EnterCriticalSection(&cs);
	lua_getglobal(g_myL, "bittorrent");
	lua_getfield(g_myL, -1, "save_session");
	lua_pushlstring(g_myL, &out[0], out.size());
	lua_pcall(g_myL, 1, 0, 0);
	lua_settop(g_myL,0);
	LeaveCriticalSection(&cs);

	return 0;
}

// the TorrentHook Class
typedef struct
{
	libtorrent::torrent_handle handle;
	__int64 size;
	__int64 base_offset;
	reader *r;
} TorrentHookStruct;

TorrentHook::TorrentHook(void *handle)
:m_handle(handle)
,m_closed(false)
{

}

TorrentHook * TorrentHook::create(const wchar_t *URL, void *extraInfo/* = NULL*/)			// create a instance, return NULL if not supported
{
	error_code ec;
	TorrentHookStruct *hh = new TorrentHookStruct;
	hh->size = 0;
	add_torrent_params p;
	p.save_path = "Z:\\out\\";
	p.ti = new torrent_info(URL, ec);


	std::vector<char> resume_data;
	char info_hash_str[41] = {0};
	std::string info_hash = p.ti->info_hash().to_string();
	for(int i=0; i<20; i++)
		sprintf(info_hash_str+i*2, "%02X", (unsigned char)info_hash[i]);
	
	EnterCriticalSection(&cs);
	lua_getglobal(g_myL, "bittorrent");
	lua_getfield(g_myL, -1, "load_torrent");
	lua_pushstring(g_myL, info_hash_str);
	lua_pcall(g_myL, 1, 1, 0);

	if (lua_type(g_myL, -1) == LUA_TSTRING)
	{
		int size = lua_rawlen(g_myL, -1);
		const char* buf = lua_tostring(g_myL, -1);
		resume_data.insert(resume_data.end(), buf, buf+size);

		p.resume_data = &resume_data;
	}

	lua_settop(g_myL, 0);
	LeaveCriticalSection(&cs);



	int index = -1;
	const file_storage &fs = p.ti->files();
	for (file_storage::iterator i = fs.begin(); i != fs.end(); i++)
	{
		boost::filesystem::path p(convert_to_native(i->filename()));
		std::string ext = p.extension().string();
		if (ext == ".rmvb" ||
			ext == ".wmv" ||
			ext == ".avi" ||
			ext == ".mkv" ||
			ext == ".flv" ||
			ext == ".rm" ||
			ext == ".mp4" ||
			ext == ".3gp" ||
			ext == ".webm" ||
			ext == ".mpg")
		{


			// 当前视频默认置为第一个视频.
			if (index == -1)
			{
				hh->base_offset = i->offset;
				hh->size = i->size;
				index = 0;
			}

		}
	}

	if (hh->size == 0)
		return NULL;

	hh->handle = s->add_torrent(p, ec);
	if (ec || !hh->handle.is_valid())
		return NULL;
	hh->r = new reader(hh->handle, *s);
	return new TorrentHook(hh);
}
TorrentHook::~TorrentHook()			// automatically close()
{
	close();
	save_all_torrent_state();
	TorrentHookStruct *hh = (TorrentHookStruct*)m_handle;
	delete hh->r;
	delete hh;
}
__int64 TorrentHook::size()
{
	TorrentHookStruct *hh = (TorrentHookStruct*)m_handle;
	return hh->size;
}
__int64 TorrentHook::get(void *buf, __int64 offset, int size)
{
	TorrentHookStruct *hh = (TorrentHookStruct*)m_handle;
	size = std::min((__int64)size, hh->size-offset);

	return hh->r->read((char*)buf, offset+hh->base_offset, size, &m_closed);
}
void TorrentHook::close()				// cancel all pending get() operation and reject all further get()
{
	m_closed = true;
}

// not yet implemented
std::map<__int64, __int64> TorrentHook::buffer()
{
	std::map<__int64, __int64> o;
	return o;
}
