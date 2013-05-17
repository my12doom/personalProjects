#include "full_cache.h"
#include <assert.h>
#include "..\httppost\httppost.h"

static const __int64 PRELOADING_SIZE = 1024*1024;
static const DWORD WORKER_TIMEOUT = 30;


// two helper function
fragment cross_fragment(fragment a, fragment b)
{
	fragment o;
	o.start = max(a.start, b.start);
	o.end = min(a.end, b.end);

	o.end = max(o.start, o.end);

	return o;
}

int subtract_fragment(fragment frag, fragment subtractor, fragment o[2])
{
	o[0] = o[1] = frag;

	fragment cross = cross_fragment(frag, subtractor);

	o[0].end = min(cross.start, o[0].end);
	o[0].end = max(o[0].start, o[0].end);

	o[1].start = max(cross.end, o[1].start);
	o[1].start = min(o[1].start, o[1].end);

	if (o[0].start == o[0].end)
		o[0] = o[1];

	if (o[0].start == o[0].end)
		return 0;

	if (o[1].start == o[1].end)
		return 1;

	return memcmp(&o[0], &o[1], sizeof(fragment)) ? 2 : 1;
}

// inet manager
inet_worker_manager::inet_worker_manager(const wchar_t *URL, disk_manager *manager)
:m_manager(manager)
,m_URL(URL)
,m_worker_pool(20)
{

}

inet_worker_manager::~inet_worker_manager()
{

}

int inet_worker_manager::hint(fragment pos, bool open_new_worker_if_necessary)
{
	myCAutoLock lck(&m_worker_cs);
	for(std::list<inet_worker*>::iterator i = m_active_workers.begin(); i != m_active_workers.end(); ++i)
		(*i)->hint(pos.start);

	bool someone_responsible = false;
	for(std::list<inet_worker*>::iterator i = m_active_workers.begin(); i != m_active_workers.end(); ++i)
		someone_responsible |= (*i)->responsible(pos.start);

	if (!someone_responsible && !open_new_worker_if_necessary)
		return -1;

	if (!someone_responsible && open_new_worker_if_necessary)
	{
		inet_worker *worker = new inet_worker(m_URL.c_str(), pos.start, this);
		m_active_workers.push_back(worker);
		m_worker_pool.submmit(worker);
	}

	return 0;
}


// inet worker
inet_worker::inet_worker(const wchar_t *URL, __int64 start, inet_worker_manager *manager)
:m_inet_file(NULL)
{
	m_pos = start;
	m_maxpos = start + PRELOADING_SIZE;

	m_URL = URL;
	m_manager = manager;
	m_inet_file = new httppost(URL);
}
inet_worker::~inet_worker()
{
	myCAutoLock lck(&m_manager->m_worker_cs);
	m_manager->m_active_workers.remove(this);
	// cleanup
	if (m_inet_file)
		delete ((httppost*)m_inet_file);
}

void inet_worker::run()
{
	disk_manager *disk = (disk_manager*)m_manager->m_manager;
	httppost *post = (httppost*)m_inet_file;
	wchar_t range_str[200];
	swprintf_s(range_str, L"bytes=%I64d-", m_pos);
	post->addHeader(L"Range", range_str);
	int response_code = post->send_request();
	if (response_code<200 || response_code > 299)
		return;

	DWORD last_inet_time = GetTickCount();
	char block[4096];

	while(true)
	{
		// hint timeout
		while(m_pos >= m_maxpos)
			if (GetTickCount() > last_inet_time + WORKER_TIMEOUT)
				return;
			else
				Sleep(1);
		
		int o = post->read_content(block, 4096);

		// network error
		if (o<=0)
			return;

		fragment frag = {m_pos, m_pos + o};
		if (disk->feed(block, frag) != 0)
			return;		// hit a disk wall or disk error

		m_pos += o;
	}
}

// 3 reasons for worker to die: 
// hint timeout, network error, reached a point that was done by other threads
// so the manager need to hint workers periodically.

// return value:
// -1: not responsible currently and after hint()
// 0: OK
int inet_worker::hint(__int64 pos)
{
	if (!responsible(pos))
		return -1;

	m_maxpos = max(m_maxpos, pos + PRELOADING_SIZE);

	return 0;
}


bool inet_worker::responsible(__int64 pos)		// return if this worker is CURRENTLY responsible for the pos
{
	return pos >= m_pos && pos < m_maxpos;
}



// disk manager


disk_manager::disk_manager(const wchar_t *URL, const wchar_t *configfile)
{
	m_URL = URL;
	m_config_file = configfile;
	m_worker_manager = new inet_worker_manager(URL, this);
}

disk_manager::~disk_manager()
{

}


int disk_manager::get(void *buf, fragment pos)
{
// 	myCAutoLock lck(&m_cs);

	// TODO: check range here

	std::list<fragment> fragment_list;
	fragment_list.push_back(pos);

	while (fragment_list.size() > 0)
	{
		// get one fragment
		fragment frag = *fragment_list.begin();
		fragment_list.pop_front();
		
		// hint the workers
		m_worker_manager->hint(frag, false);

		// get every fragments from every disk_fragments
		BYTE *buf2 = ((BYTE*)buf) + frag.start - pos.start;
		bool this_round_got = false;

		{
			myCAutoLock lck2(&m_fragments_cs);

			for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
			{
				fragment left[2];
				int c = (*i)->get(buf2, frag, left);

				if (c >= 0)
				{
					this_round_got = true;
					for(int j=0; j<c; j++)
						fragment_list.push_front(left[j]);
					break;
				}
			}
		}

		if (!this_round_got)
		{
			// this fragment is not handled by any of disk fragments.
			// start inet thread here
			m_worker_manager->hint(frag, true);
			fragment_list.push_front(frag);
			Sleep(1);
		}
	}

	return 0;
}

int disk_manager::feed(void *buf, fragment pos)
{
// 	myCAutoLock lck(&m_cs);
	myCAutoLock lck2(&m_fragments_cs);

	bool conflit = false;
	for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
	{
		fragment left = (*i)->remaining(pos, NULL);
		if (memcmp(&left, &pos, sizeof(pos)) != 0)
		{
			conflit = true;
			break;
		}
	}


	for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
	{
		if ((*i)->tell() == pos.start)
		{
			(*i)->put(buf, (int)(pos.end - pos.start));
			return conflit ? 1 : 0;
		}
	}

	// new fragment
	wchar_t new_name[MAX_PATH];
	wsprintf(new_name, L"Z:\\%d.tmp", pos.start);
	disk_fragment *p = new disk_fragment(new_name, pos.start);
	p->put(buf, (int)(pos.end - pos.start));
	m_fragments.push_back(p);

	return 0;
}





disk_fragment::disk_fragment(const wchar_t*file, __int64 startpos)
{
	m_start = startpos;
	_wremove(file);
	m_file = CreateFileW (file, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, 0, NULL);

	assert(m_file != INVALID_HANDLE_VALUE);

	LARGE_INTEGER li = {0}, l2;

	SetFilePointerEx(m_file, li, &l2, SEEK_END);

	m_pos = startpos + l2.QuadPart;
}

disk_fragment::~disk_fragment()
{
	CloseHandle(m_file);
}

int disk_fragment::put(void *buf, int size)
{
	myCAutoLock lck(&m_cs);

	m_pos += size;
	DWORD written = 0;
	WriteFile(m_file, buf, size, &written, NULL);

	return 0;
}

// this function subtract pos by {m_start, m_pos}, assuming there is only one piece left.
// and shift *pointer if not NULL
fragment disk_fragment::remaining(fragment pos, void**pointer /*= NULL*/)
{
	myCAutoLock lck(&m_cs);

	fragment o[2];
	fragment me = {m_start, m_pos};
	int count = subtract_fragment(pos, me, o);

	if (pointer && count > 0 && o[0].start > o[0].end)
		*pointer = ((BYTE*)*pointer) + o[0].start - pos.start;

	return o[0];
}

// return value:
// 0: all requests completed.
// 1/2: some request completed, 1/2 fragments left, output to fragment_left
// -1: no requests completed.
int disk_fragment::get(void *buf, fragment pos, fragment fragment_left[2])
{
	myCAutoLock lck(&m_cs);

	// calculate position
	fragment me = {m_start, m_pos};
	int left_count = subtract_fragment(pos, me, fragment_left);
	fragment cross = cross_fragment(pos, me);

	// get data if necessary
	if (cross.end > cross.start)
	{
		// save position
		LARGE_INTEGER old_pos = {0};
		LARGE_INTEGER tmp = {0};
		SetFilePointerEx(m_file, tmp, &old_pos, SEEK_CUR);

		// seek and get
		buf = ((BYTE*)buf) + cross.start - pos.start;
		tmp.QuadPart = cross.start - m_start;
		SetFilePointerEx(m_file, tmp, &tmp, SEEK_SET);

		DWORD read = 0;
		ReadFile(m_file, buf, (DWORD)(cross.end - cross.start), &read, NULL );
		assert(read == cross.end - cross.start);

		// rewind
		SetFilePointerEx(m_file, old_pos, &tmp, SEEK_SET);
	}
	else
	{
		return -1;
	}

	if (cross.end - cross.start != 99999)
		return left_count;

	return left_count;
}