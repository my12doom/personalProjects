#include "runnable.h"


#ifdef LINUX
#include <unistd.h>
void mrevent_init(struct mrevent *ev) {
    pthread_mutex_init(&ev->mutex, 0);
    pthread_cond_init(&ev->cond, 0);
    ev->triggered = false;
}
void mrevent_set(struct mrevent *ev) {
    pthread_mutex_lock(&ev->mutex);
    ev->triggered = true;
    pthread_cond_signal(&ev->cond);
    pthread_mutex_unlock(&ev->mutex);
}
void mrevent_reset(struct mrevent *ev) {
    pthread_mutex_lock(&ev->mutex);
    ev->triggered = false;
    pthread_mutex_unlock(&ev->mutex);
}
bool mrevent_wait(struct mrevent *ev, int timeout) {
	 struct timespec timeout_i = {0, timeout*1000000};
     pthread_mutex_lock(&ev->mutex);
	 pthread_cond_timedwait(&ev->cond, &ev->mutex, &timeout_i);
	 bool o = ev->triggered;
	 ev->triggered = false;
     pthread_mutex_unlock(&ev->mutex);
	 return o;
}
void mrevent_destroy(struct mrevent *ev) {
    pthread_mutex_destroy(&ev->mutex);
    pthread_cond_destroy(&ev->cond);
    ev->triggered = false;
}
#endif

thread_pool::thread_pool(int n)
:m_destroyed(false)
,m_thread_count(n)
{
#ifndef LINUX
	InitializeCriticalSection(&m_cs);
	m_task_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_quit_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	m_threads = new HANDLE[n];
	for(int i=0; i<n; i++)
		m_threads[i] = CreateThread(NULL, NULL, worker_thread, this, NULL, NULL);
#else
	pthread_mutex_init(&m_cs, NULL);
	mrevent_init(&m_task_event);
	m_quit_flag = false;
	m_threads = new pthread_t[n];
	for(int i=0; i<n; i++)
		pthread_create(&m_threads[i], NULL, worker_thread, this);
#endif
}

thread_pool::~thread_pool()
{
	destroy();
#ifndef LINUX
	DeleteCriticalSection(&m_cs);
#else
	pthread_mutex_destroy(&m_cs);
#endif
}

int thread_pool::destroy()
{
	EnterCriticalSection(&m_cs);

	m_destroyed = true;

	while (!m_tasks.empty())
	{
		Irunnable *p = m_tasks.front();
		m_tasks.pop();

		p->signal_quit();
		delete p;
	}

	for(std::list<Irunnable*>::iterator i = m_running_tasks.begin(); i != m_running_tasks.end(); ++i)
		(*i)->signal_quit();

	LeaveCriticalSection(&m_cs);

	#ifndef LINUX
	SetEvent(m_quit_event);
	WaitForMultipleObjects(m_thread_count, m_threads, TRUE, INFINITE);
	CloseHandle(m_task_event);
	CloseHandle(m_quit_event);
	#else
		m_quit_flag = true;
		for(int i=0; i<m_thread_count; i++)
			pthread_join(m_threads[i], NULL);
		mrevent_destroy(&m_task_event);
	#endif

	delete [] m_threads;

	return 0;
}

#ifndef LINUX
DWORD thread_pool::worker_thread(LPVOID p)
#else
void *thread_pool::worker_thread(void* p)
#endif
{
	thread_pool *k = (thread_pool*)p;

	while(true)
	{
		EnterCriticalSection(&k->m_cs);
		if (k->m_tasks.size() == 0)
		{
			LeaveCriticalSection(&k->m_cs);
			#ifndef LINUX
			HANDLE events[2] = {k->m_task_event, k->m_quit_event};
			DWORD o = WaitForMultipleObjects(2, events, FALSE, INFINITE);
			if (o != WAIT_OBJECT_0)
				break;
			#else
				if (!mrevent_wait(&k->m_task_event, 100) && k->m_quit_flag)
					break;
				usleep(1000);
			#endif
			continue;
		}

		Irunnable *task = k->m_tasks.front();
		k->m_tasks.pop();

		k->m_running_tasks.push_back(task);
		LeaveCriticalSection(&k->m_cs);

		task->run();

		EnterCriticalSection(&k->m_cs);
		k->m_running_tasks.remove(task);
		LeaveCriticalSection(&k->m_cs);

		delete task;
	}

	return 0;
}

int thread_pool::submmit(Irunnable *task)
{
	EnterCriticalSection(&m_cs);
	if (!m_destroyed)
		m_tasks.push(task);
	LeaveCriticalSection(&m_cs);
	SetEvent(m_task_event);
	return 0;
}

