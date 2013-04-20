#include "runnable.h"

thread_pool::thread_pool(int n)
{
	InitializeCriticalSection(&m_cs);
	m_task_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	for(int i=0; i<n; i++)
		CreateThread(NULL, NULL, worker_thread, this, NULL, NULL);
}

thread_pool::~thread_pool()
{
	DeleteCriticalSection(&m_cs);
}

DWORD thread_pool::worker_thread(LPVOID p)
{
	thread_pool *k = (thread_pool*)p;

	while(true)
	{
		EnterCriticalSection(&k->m_cs);
		if (k->m_tasks.size() == 0)
		{
			LeaveCriticalSection(&k->m_cs);
			DWORD o = WaitForSingleObject(k->m_task_event, INFINITE);
			if (o != WAIT_OBJECT_0)
				break;
			continue;
		}

		Irunnable *task = k->m_tasks.front();
		k->m_tasks.pop();
		LeaveCriticalSection(&k->m_cs);

		task->run();
		delete task;
	}

	return 0;
}

int thread_pool::submmit(Irunnable *task)
{
	EnterCriticalSection(&m_cs);
	m_tasks.push(task);
	LeaveCriticalSection(&m_cs);
	SetEvent(m_task_event);
	return 0;
}

