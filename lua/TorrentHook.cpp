#include <stdlib.h>
#include "TorrentHook.h"
#include "libtorrent/entry.hpp"
#include "libtorrent/bencode.hpp"
#include "libtorrent/session.hpp"
#include <libtorrent/alert_types.hpp>
#include <boost/unordered_set.hpp>
#include <conio.h>
#include "libtorrent_reader.h"
#include <atlbase.h>


int saving = 0;
using namespace libtorrent;
session *s;

typedef std::multimap<std::string, libtorrent::torrent_handle> handles_t;
bool handle_alert(libtorrent::session& ses, libtorrent::alert* a)
{
	if (save_resume_data_alert* p = alert_cast<save_resume_data_alert>(a))
	{
		--saving;
		torrent_handle h = p->handle;
		TORRENT_ASSERT(p->resume_data);
		if (p->resume_data)
		{
			std::vector<char> out;
			bencode(std::back_inserter(out), *p->resume_data);
			//save_file(combine_path(h.save_path(), combine_path(".resume", to_hex(h.info_hash().to_string()) + ".resume")), out);
			FILE * f = fopen("Z:\\resume.txt", "wb");
			fwrite(&out[0], 1, out.size(), f);
			fclose(f);

			ses.remove_torrent(h);
		}
	}
	else if (save_resume_data_failed_alert* p = alert_cast<save_resume_data_failed_alert>(a))
	{
		--saving;
		torrent_handle h = p->handle;
		if (h.is_valid())
			ses.remove_torrent(h);
	}
	return false;
}

int init_torrent_hook()
{
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

	char large[40960];
	FILE *f=fopen("Z:\\state.txt", "rb");
	if (f)
	{
		int size = fread(large, 1, sizeof(large), f);
		fclose(f);
		lazy_entry load_state;
		lazy_bdecode(large, large+size, load_state);
		s->load_state(load_state);
	}

	return 0;
}
// 
// int save_all_torrent_state()
// {
// 	saving++;
// 	handle.pause();
// 	handle.save_resume_data();
// 
// 	while (saving)
// 	{
// 		// loop through the alert queue to see if anything has happened.
// 		std::deque<alert*> alerts;
// 		s->pop_alerts(&alerts);
// 		std::string now = time_now_string();
// 		for (std::deque<alert*>::iterator i = alerts.begin()
// 			, end(alerts.end()); i != end; ++i)
// 		{
// 			bool need_resort = false;
// 			TORRENT_TRY
// 			{
// 				if (!::handle_alert(*s, *i))
// 				{
// 					// unhandled alerts
// 				}
// 			} TORRENT_CATCH(std::exception& e) {}
// 
// 			delete *i;
// 		}
// 		alerts.clear();
// 	}
// 
// 	entry session_state;
// 	s->save_state(session_state);
// 	std::vector<char> out;
// 	bencode(std::back_inserter(out), session_state);
// 
// 	FILE *f=fopen("Z:\\state.txt", "wb");
// 	fwrite(&out[0], 1, out.size(), f);
// 	fclose(f);
// 
// 	return 0;
// }

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

	if (load_file("Z:\\resume.txt", resume_data, ec) == 0)
		p.resume_data = &resume_data;

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
	torrent_info info = hh->handle.get_torrent_info();
	hh->r = new reader(hh->handle, *s);
	return new TorrentHook(hh);
}
TorrentHook::~TorrentHook()			// automatically close()
{
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
