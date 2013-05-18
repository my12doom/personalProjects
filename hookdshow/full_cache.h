#pragma once

#include "InternetFile.h"
#include "..\dwindow\runnable.h"
#include <list>
#include <string>
#include "CCritSec.h"

class inet_worker_manager;
class disk_manager;

typedef struct
{
	__int64 start;
	__int64 end;
} fragment;
fragment cross_fragment(fragment a, fragment b);
int subtract_fragment(fragment frag, fragment subtractor, fragment o[2]);

class inet_worker : public Irunnable
{
public:
	inet_worker(const wchar_t *URL, __int64 start, inet_worker_manager *manager);
	~inet_worker();

	void run();
	int hint(__int64 pos);				// 3 reasons for worker to die: 
										// hint timeout, network error, reached a point that was done by other threads
										// so the manager need to hint workers periodically.

										// return value:
										// -1: not responsible currently and after hint()
										// 0: OK
	void signal_quit();

	bool responsible(__int64 pos);		// return if this worker is CURRENTLY responsible for the pos

	__int64 m_pos;
	__int64 m_maxpos;					// modified by hint();
protected:
	bool m_exit_signaled;
	void *m_inet_file;
	inet_worker_manager *m_manager;
	std::wstring m_URL;
};

class inet_worker_manager
{
public:
	inet_worker_manager(const wchar_t *URL, disk_manager *manager);
	~inet_worker_manager();

	int hint(fragment pos, bool open_new_worker_if_necessary);			// hint the workers to continue their work, and launch new worker if necessary

protected:
	friend class inet_worker;

	myCCritSec m_worker_cs;
	std::list<inet_worker*> m_active_workers;
	thread_pool m_worker_pool;
	std::wstring m_URL;
	disk_manager *m_manager;
};


class disk_fragment
{
public:
	disk_fragment(const wchar_t*file, __int64 startpos);
	~disk_fragment();

	int put(void *buf, int size);

	// return value:
	// 0: all requests completed.
	// 1/2: some request completed, 1/2 fragments left, output to fragment_left
	// -1: no requests completed.
	int get(void *buf, fragment pos, fragment fragment_left[2]);

	// this function subtract pos by {m_start, m_pos}, assuming there is only one piece left.
	// and shift *pointer if not NULL
	fragment remaining(fragment pos, void**pointer = NULL);



	__int64 tell(){return m_pos;}

protected:
	myCCritSec m_cs;
	__int64 m_pos;
	__int64 m_start;
	HANDLE m_file;
};

class disk_manager
{
public:
	disk_manager(const wchar_t *configfile);
	disk_manager():m_worker_manager(NULL){disk_manager(NULL);}
	~disk_manager();

	int setURL(const wchar_t *URL);
	int get(void *buf, fragment &pos);
	__int64 getsize(){return m_filesize;}

protected:
	friend class disk_fragment;
	friend class inet_worker_manager;
	friend class inet_worker;

	// return:
	// -1 on error (unlikely and not handled currently)
	// 0 on total success
	// 1 on success but some fragment has already exists in some fragments
	int feed(void *buf, fragment pos);

	std::list<disk_fragment*> m_fragments;
	inet_worker_manager *m_worker_manager;
	std::wstring m_URL;
	std::wstring m_config_file;
	myCCritSec m_fragments_cs;
	__int64 m_filesize;
};
