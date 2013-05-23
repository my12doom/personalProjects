#pragma once
#include <queue>
#include <list>

#ifndef LINUX
#include <Windows.h>
#else
#include <pthread.h>
#define CRITICAL_SECTION pthread_mutex_t
#define EnterCriticalSection pthread_mutex_lock
#define LeaveCriticalSection pthread_mutex_unlock
#define SetEvent(x) mrevent_set(& x)
#include <stdbool.h>
struct mrevent {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool triggered;
};
void mrevent_init(struct mrevent *ev);
void mrevent_set(struct mrevent *ev);
void mrevent_reset(struct mrevent *ev);
bool mrevent_wait(struct mrevent *ev, int timeout);
void mrevent_destroy(struct mrevent *ev);
#endif

class Irunnable
{
public:
	virtual void run() = 0;
	virtual void signal_quit(){}
	virtual void join(){}
	virtual ~Irunnable(){}
};

class thread_pool
{
public:
	thread_pool(int n);
	~thread_pool();
	int submmit(Irunnable *task);
	int remove(Irunnable *task);
	int destroy();		// abandon all unstarted worker, wait for all working worker to finish, and reject any further works.

protected:
	std::queue<Irunnable*> m_tasks;
	std::list<Irunnable*> m_running_tasks;
	bool m_destroyed;
	int m_thread_count;

#ifdef LINUX
	pthread_mutex_t m_cs;
	struct mrevent m_task_event;
	bool m_quit_flag;
	pthread_t *m_threads;
	static void* worker_thread(void *p);
#else
	CRITICAL_SECTION m_cs;
	HANDLE m_task_event;
	HANDLE m_quit_event;
	HANDLE *m_threads;
	static DWORD WINAPI worker_thread(LPVOID p);
#endif

};