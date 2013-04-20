#pragma once
#include <Windows.h>
#include <queue>

class Irunnable
{
public:
	virtual void run() = 0;
	virtual ~Irunnable(){}
};

class thread_pool
{
public:
	thread_pool(int n);
	~thread_pool();
	int submmit(Irunnable *task);
	int remove(Irunnable *task);

protected:
	std::queue<Irunnable*> m_tasks;
	CRITICAL_SECTION m_cs;
	HANDLE m_task_event;

	static DWORD WINAPI worker_thread(LPVOID p);
};