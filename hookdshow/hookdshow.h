#pragma once

#include <wchar.h>
#include <list>


int enable_hookdshow();
int disable_hookdshow();
int stop_all_handles();
int clear_all_handles();
const wchar_t *URL2Token(const wchar_t *URL);
const wchar_t *Token2URL(const wchar_t *Token);

class IHookProvider
{
public:
	virtual ~IHookProvider() {}			// automatically close()
	virtual __int64 size() = 0;
	virtual __int64 get(void *buf, __int64 offset, int size) = 0;
	virtual void close() = 0;				// cancel all pending get() operation and reject all further get()
};

class HTTPHook : public IHookProvider
{
public:
	static HTTPHook * create(const wchar_t *URL, void *extraInfo = NULL);			// create a instance, return NULL if not supported
	~HTTPHook();			// automatically close()
	__int64 size();
	__int64 get(void *buf, __int64 offset, int size);
	void close();				// cancel all pending get() operation and reject all further get()
private:
	void *m_core;
	bool m_closed;
	HTTPHook(void *core);
};

class TorrentHook : public IHookProvider
{
public:
	static TorrentHook * create(const wchar_t *URL, void *extraInfo = NULL);			// create a instance, return NULL if not supported
	~TorrentHook();			// automatically close()
	__int64 size();
	__int64 get(void *buf, __int64 offset, int size);
	void close();				// cancel all pending get() operation and reject all further get()
								// not yet implemented
private:
	TorrentHook();
	bool m_closed;
};
