#include "full_cache.h"
#include <assert.h>
#include "time.h"

static const __int64 PRELOADING_SIZE = 1024*1024;
static const DWORD WORKER_TIMEOUT = 5000;
static const DWORD WORKER_COUNT = 3;

#ifndef LINUX
	#include "..\httppost\httppost.h"
#define LOGE(...)
#else
	#include <../3dvlog.h>
	#include "httppost.h"
	#include <unistd.h>
	#include <sys/stat.h> 
	#include <fcntl.h>
	#define min(a,b) ((a)>(b) ? (b) : (a))
	#define max(a,b) ((a)>(b) ? (a) : (b))
	#define Sleep(x) usleep(x*1000)
	#define MAX_PATH 260
	#include <sys/time.h>

	DWORD GetTickCount()
	{
			struct timeval tv;
			if(gettimeofday(&tv, NULL) != 0)
					return 0;

			return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
	}
#endif


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
inet_worker_manager::inet_worker_manager(const wchar_t *URL, inet_file *manager)
:m_manager(manager)
,m_URL(URL)
,m_worker_pool(WORKER_COUNT)
{

}

inet_worker_manager::~inet_worker_manager()
{

}

int compare_inet_worker(const void *a, const void *b)
{
	int *a1 = (int*)a;
	int *b1 = (int*)b;
	if (*a1>*b1)
		return 1;
	if (*b1>*a1)
		return -1;
	return 0;
}

int inet_worker_manager::hint(void *httppost)
{
	inet_worker *worker = new inet_worker(httppost, this);

	myCAutoLock lck(&m_worker_cs);
	m_active_workers.push_back(worker);
	m_worker_pool.submmit(worker);

	return 0;
}

int inet_worker_manager::hint(fragment pos, bool open_new_worker_if_necessary, bool debug/*=false*/)
{
	myCAutoLock lck(&m_worker_cs);
	for(std::list<inet_worker*>::iterator i = m_active_workers.begin(); i != m_active_workers.end(); ++i)
	{
		(*i)->hint(pos.start);
		(*i)->hint(pos.end);
// 		(*i)->hint((pos.start + pos.end)/2);
	}

	bool someone_responsible = false;
	for(std::list<inet_worker*>::iterator i = m_active_workers.begin(); i != m_active_workers.end(); ++i)
		someone_responsible |= (*i)->responsible(pos.start);

	if (!someone_responsible && !open_new_worker_if_necessary)
		return -1;

	if (!someone_responsible && open_new_worker_if_necessary)
	{
		// kill the most unused thread if thread pool is full
		if (m_active_workers.size() >= WORKER_COUNT)
		{
			typedef struct
			{
				int timeout_left;
				inet_worker *p;
			} idx;
			idx index[WORKER_COUNT];
			int j=0;
			for(std::list<inet_worker*>::iterator i = m_active_workers.begin();
				i != m_active_workers.end() && j<WORKER_COUNT; ++i, ++j)
			{
				index[j].timeout_left = (*i)->get_timeout_left();
				index[j].p = (*i);
			}

			qsort(index, WORKER_COUNT, sizeof(idx), compare_inet_worker);

			index[0].p->signal_quit();
		}

		printf("new thread start @ %d\n", (int)pos.start);
		inet_worker *worker = new inet_worker(m_URL.c_str(), pos.start, this);
		m_active_workers.push_back(worker);
		m_worker_pool.submmit(worker);
	}

	else if (debug)
	{
		printf("");
	}

	return 0;
}


// inet worker
inet_worker::inet_worker(const wchar_t *URL, __int64 start, inet_worker_manager *manager)
:m_inet_file(NULL)
,m_exit_signaled(false)
{
	m_pos = start;
	m_maxpos = start + PRELOADING_SIZE;

	m_URL = URL;
	m_manager = manager;
	m_inet_file = new httppost(URL);

	m_last_inet_time = GetTickCount();
}

inet_worker::inet_worker(void *post, inet_worker_manager *manager)
:m_inet_file(NULL)
,m_exit_signaled(false)
{
	m_inet_file = (httppost*)post;
	m_pos = 0;
	m_maxpos = PRELOADING_SIZE;
	m_manager = manager;
	m_last_inet_time = GetTickCount();
}

inet_worker::~inet_worker()
{
	myCAutoLock lck(&m_manager->m_worker_cs);
	m_manager->m_active_workers.remove(this);
	// cleanup
	if (m_inet_file)
		delete ((httppost*)m_inet_file);
}

void inet_worker::signal_quit()
{
	if (m_inet_file)
		((httppost*)m_inet_file)->close_connection();
	m_exit_signaled = true;
}

int inet_worker::get_timeout_left()
{
	return max(0, m_last_inet_time + WORKER_TIMEOUT - GetTickCount());
}
class UTF82W_core
{
public:
	UTF82W_core(const char *in);
	~UTF82W_core();
	operator wchar_t*();
	wchar_t *p;
};

void inet_worker::run()
{
	inet_file *disk = (inet_file*)m_manager->m_manager;
	httppost *post = (httppost*)m_inet_file;
	wchar_t range_str[200];
	char range_str_utf[400];
	sprintf(range_str_utf, "bytes=%lld-", m_pos);
	wcscpy(range_str, UTF82W_core(range_str_utf));
	if (m_pos > 0)
	post->addHeader(L"Range", range_str);
	int response_code = post->send_request();
	if (response_code<200 || response_code > 299)
		return;

	char block[4096];

	while(true)
	{
		// hint timeout
		while(m_pos >= m_maxpos && !m_exit_signaled)
			if (get_timeout_left() <= 0)
			{
				printf("worker %08x timeout, %d worker left, pos: %d - %d\n", this, m_manager->m_active_workers.size(), (int)m_pos, (int)m_maxpos);
				return;
			}
			else
				Sleep(1);
		
		int o = post->read_content(block, 4096);

		m_last_inet_time = GetTickCount();

		// network error
		if (o<=0 || m_exit_signaled)
		{
			printf("worker %08x shutdown or network error, %d worker left, pos: %d - %d\n", this, m_manager->m_active_workers.size(), (int)m_pos, (int)m_maxpos);
			return;
		}

		fragment frag = {m_pos, m_pos + o};
		if (disk->feed(block, frag) != 0)
		{
			printf("worker %08x hit a disk wall, %d worker left, pos: %d - %d\n", this, m_manager->m_active_workers.size(), (int)m_pos, (int)m_maxpos);
			return;		// hit a disk wall or disk error
		}

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
// 	if (!responsible(pos))
// 		return -1;

	m_maxpos = min(max(m_maxpos, pos + PRELOADING_SIZE), m_pos + PRELOADING_SIZE*2);

	return 0;
}


bool inet_worker::responsible(__int64 pos)		// return if this worker is CURRENTLY responsible for the pos
{
	return pos >= m_pos && pos < m_maxpos;
}



// disk manager
typedef struct
{
	__int64 startpos;
	wchar_t file[MAX_PATH];
} configfile_entry;

inet_file::inet_file(const wchar_t *configfile)
:m_worker_manager(NULL)
,m_config_file_file(NULL)
{
	if (configfile)
		m_config_file = configfile;
	m_config_file_file = _wfopen(configfile, L"r+b");
	if (!m_config_file_file)
		m_config_file_file = _wfopen(configfile, L"wb");

	assert(m_config_file_file);
	if (!m_config_file_file)
		return;

	m_filesize = 0;
	fread(&m_filesize, sizeof(m_filesize), 1, m_config_file_file);
	fread(&m_time, sizeof(m_time), 1, m_config_file_file);

	configfile_entry entry;
	while (fread(&entry, sizeof(entry), 1, m_config_file_file))
	{
		//LOGE("%d disk_fragments(%s, %lld) loaded", ++i, W2UTF8(entry.file), entry.startpos);
		disk_fragment *p = new disk_fragment(entry.file, entry.startpos);
		m_fragments.push_back(p);
	}
}

int inet_file::setURL(const wchar_t *URL)
{
	if (m_worker_manager != NULL)
		return -1;

	m_worker_manager = new inet_worker_manager(URL, this);

	m_URL = URL;

	if (m_filesize == 0)
	{
		httppost *http = new httppost(URL);
		int responsecode = http->send_request();
		LOGE("disk_manager::setURL() : code = %d", responsecode);
		if (responsecode < 200 || responsecode > 299)
			return -2;
		m_filesize = http->get_content_length();
		if (m_filesize <= 0)
			return -3;

		m_worker_manager->hint(http);

		fseek(m_config_file_file, 0, SEEK_SET);
		fwrite(&m_filesize, sizeof(m_filesize), 1, m_config_file_file);
		fwrite(&m_time, sizeof(m_time), 1, m_config_file_file);
	}
	return 0;
}

inet_file::~inet_file()
{
	delete m_worker_manager;
	for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
		delete *i;
}


int inet_file::get(void *buf, fragment &pos)
{
	{
		myCAutoLock lck2(&m_access_lock);
		debug_info disk_item = {pos, debug_info::read, GetTickCount()};

		if (m_access_info.size() > 500)
			m_access_info.pop_front();
		m_access_info.push_back(disk_item);
	}

	// check range
	bool debug = (pos.end - pos.start) == 100;
	int rtn = 0;
	if (pos.start < 0)
		return -1;
	if (pos.end > m_filesize)
	{
		pos.end = m_filesize;
		rtn = 1;
	}

	pos.start = min(pos.start, pos.end);
	if (pos.end <= pos.start)
		return 0;

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
			m_worker_manager->hint(frag, true, debug);
			fragment_list.push_front(frag);
			Sleep(1);
		}
	}

	// update access timestamp
	myCAutoLock lck3(&m_access_lock);
	if (_time64(NULL) != m_time)
	{
		m_time = _time64(NULL);
		int t = ftell(m_config_file_file);
		fseek(m_config_file_file, sizeof(m_filesize), SEEK_SET);
		fwrite(&m_time, 1, sizeof(m_time), m_config_file_file);
		fseek(m_config_file_file, t, SEEK_SET);
		fflush(m_config_file_file);
	}


	return rtn;
}

int inet_file::pre_read(fragment &pos)
{
	{
		myCAutoLock lck2(&m_access_lock);
		debug_info disk_item = {pos, debug_info::preread, GetTickCount()};

		if (m_access_info.size() > 500)
			m_access_info.pop_front();
		m_access_info.push_back(disk_item);
	}

	std::list<fragment> fragment_list;
	fragment_list.push_back(pos);

	myCAutoLock lck2(&m_fragments_cs);
	for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
	{
		std::list<fragment> tmp;
		for(std::list<fragment>::iterator j=fragment_list.begin(); j!= fragment_list.end(); j++)
		{
			fragment piece[2];
			int piece_count = (*i)->remaining(*j, piece);

			for(int i=0; i<piece_count; i++)
				tmp.push_back(piece[i]);
		}
		fragment_list.clear();
		std::copy(tmp.begin(), tmp.end(), std::back_inserter(fragment_list));
	}

	for(std::list<fragment>::iterator j=fragment_list.begin(); j!= fragment_list.end(); j++)
		m_worker_manager->hint(*j, true);

	return 0;
}

__int64 inet_file::getdisksize()
{
	myCAutoLock lck2(&m_fragments_cs);

	__int64 disksize = 0;
	for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
	{
		disksize += (*i)->m_pos - (*i)->m_start;
	}

	return disksize;
}


int inet_file::feed(void *buf, fragment pos)
{
	myCAutoLock lck2(&m_fragments_cs);

	bool conflit = false;
	fragment left = pos;
	for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
	{
		left = (*i)->remaining2(pos, &buf);
		if (memcmp(&left, &pos, sizeof(pos)) != 0)
		{
			conflit = true;
			break;
		}
	}

	// if nothing remains
	if (left.end <= left.start)
		return conflit ? 1 : 0;


	for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
	{
		if ((*i)->tell() == left.start)
		{
			(*i)->put(buf, (int)(left.end - left.start));
			return conflit ? 1 : 0;
		}
	}

	// new fragment
	wchar_t new_name[MAX_PATH];
	#ifndef LINUX
	swprintf(new_name, MAX_PATH, L"%s.%03d", m_config_file.c_str(), (int)m_fragments.size());
	#else
	swprintf(new_name, MAX_PATH, L"/sdcard/%08x_%d_%d.tmp", this, (int)left.start, rand()*rand());
	#endif
	disk_fragment *p = new disk_fragment(new_name, left.start);
	p->put(buf, (int)(left.end - left.start));
	m_fragments.push_back(p);

	configfile_entry entry = {left.start};
	wcscpy(entry.file, new_name);
	myCAutoLock lck3(&m_access_lock);
	fseek(m_config_file_file, 0, SEEK_END);
	fwrite(&entry, 1, sizeof(entry), m_config_file_file);
	fflush(m_config_file_file);

	return 0;
}

std::list<debug_info> inet_file::debug()
{
	std::list<debug_info> out;

	{
		myCAutoLock lck2(&m_access_lock);
		for(std::list<debug_info>::iterator i = m_access_info.begin(); i != m_access_info.end(); ++i)
		{
			if (GetTickCount() - (*i).tick < WORKER_TIMEOUT && (*i).type == debug_info::preread)
				out.push_back(*i);
		}
	}

	{
		myCAutoLock lck(&m_fragments_cs);
		for(std::list<disk_fragment*>::iterator i = m_fragments.begin(); i != m_fragments.end(); ++i)
		{
			debug_info disk_item = {{0,0}};
			disk_item.type = debug_info::disk;
			disk_item.frag.start = (*i)->m_start;
			disk_item.frag.end = (*i)->m_pos;

			out.push_back(disk_item);
		}
	}

	{
		myCAutoLock lck(&m_worker_manager->m_worker_cs);
		for(std::list<inet_worker*>::iterator i = m_worker_manager->m_active_workers.begin(); i != m_worker_manager->m_active_workers.end(); ++i)
		{
			debug_info net_item = {{0,0}};
			net_item.type = debug_info::net;
			net_item.frag.start = (*i)->m_pos;
			net_item.frag.end = (*i)->m_pos;

			out.push_back(net_item);
		}
	}

	{
		myCAutoLock lck2(&m_access_lock);
		for(std::list<debug_info>::iterator i = m_access_info.begin(); i != m_access_info.end(); ++i)
		{
			if (GetTickCount() - (*i).tick < WORKER_TIMEOUT && (*i).type == debug_info::read)
				out.push_back(*i);
		}
	}


	return out;
}


#ifdef LINUX
bool utf8fromwcs(const wchar_t* wcs, size_t length, char* outbuf);

disk_fragment::disk_fragment(const wchar_t*file, __int64 startpos)
{
	m_start = startpos;
	char file_utf[1000] = "holy";
	utf8fromwcs(file, wcslen(file), file_utf);
	sprintf(file_utf, "/sdcard/%d.tmp", rand()*rand());
	remove(file_utf);

	m_file = open(file_utf, O_RDWR | O_CREAT);

	assert(m_file > 0);

	LOGE("disk_fragment::disk_fragment(%s,%d) = %d", file_utf, (int)startpos, m_file);


	lseek64(m_file, 0, SEEK_END);

	m_pos = startpos + lseek64(m_file, 0, SEEK_CUR);
}

disk_fragment::~disk_fragment()
{
	close(m_file);
}

int disk_fragment::put(void *buf, int size)
{
	myCAutoLock lck(&m_cs);

	m_pos += size;
	DWORD written = 0;
	write(m_file, buf, size);

	return 0;
}

#else

disk_fragment::disk_fragment(const wchar_t*file, __int64 startpos)
{
	m_start = startpos;
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
#endif
// this function subtract pos by {m_start, m_pos}
int disk_fragment::remaining(fragment pos, fragment out[2])
{
	myCAutoLock lck(&m_cs);

	fragment me = {m_start, m_pos};
	return  subtract_fragment(pos, me, out);
}

// this function subtract pos by {m_start, m_pos}, assuming there is only one piece left.
// and shift *pointer if not NULL
fragment disk_fragment::remaining2(fragment pos, void**pointer /*= NULL*/)
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
	buf = ((BYTE*)buf) + cross.start - pos.start;

	// get data if necessary
	if (cross.end > cross.start)
	{
#ifdef LINUX
		// save position
		__int64 old_pos = lseek64(m_file, 0, SEEK_CUR);

		// seek and get
		lseek64(m_file, cross.start - m_start, SEEK_SET);

		int count = read(m_file, buf, cross.end - cross.start);
		assert(count == cross.end - cross.start);

		// rewind
		lseek64(m_file, old_pos, SEEK_SET);
#else
		// save position
		LARGE_INTEGER old_pos = {0};
		LARGE_INTEGER tmp = {0};
		SetFilePointerEx(m_file, tmp, &old_pos, SEEK_CUR);

		// seek and get
		tmp.QuadPart = cross.start - m_start;
		SetFilePointerEx(m_file, tmp, &tmp, SEEK_SET);

		DWORD read = 0;
		ReadFile(m_file, buf, (DWORD)(cross.end - cross.start), &read, NULL );
		assert(read == cross.end - cross.start);

		// rewind
		SetFilePointerEx(m_file, old_pos, &tmp, SEEK_SET);
#endif
	}
	else
	{
		return -1;
	}

	if (cross.end - cross.start != 99999)
		return left_count;

	return left_count;
}