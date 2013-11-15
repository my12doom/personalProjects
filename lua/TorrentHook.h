#pragma once

#include "DWindowReader.h"
#include "lua.hpp"

class TorrentHook : public IDWindowReader
{
public:
	static TorrentHook * create(const wchar_t *URL, void *extraInfo = NULL);			// create a instance, return NULL if not supported
	~TorrentHook();			// automatically close()
	__int64 size();
	__int64 get(void *buf, __int64 offset, int size);
	void close();				// cancel all pending get() operation and reject all further get()
	// not yet implemented
	std::map<__int64, __int64> buffer();
private:
	TorrentHook(void *handle);
	bool m_closed;
	void *m_handle;
};

int init_torrent_hook(lua_State *g_L);
int save_all_torrent_state();
