#pragma once
// copy from wxutil.h

// wrapper for whatever critical section we have

#ifndef LINUX
#include <Windows.h>
#else
#include <pthread.h>
#define InitializeCriticalSection(x) pthread_mutex_init(x, NULL)
#define DeleteCriticalSection(x) pthread_mutex_destroy(x)
#define EnterCriticalSection(x) pthread_mutex_lock(x)
#define LeaveCriticalSection(x) pthread_mutex_unlock(x)
#endif

class myCCritSec {

	// make copy constructor and assignment operator inaccessible

	myCCritSec(const myCCritSec &refCritSec);
	myCCritSec &operator=(const myCCritSec &refCritSec);

	CRITICAL_SECTION m_CritSec;

public:
	myCCritSec() {
		InitializeCriticalSection(&m_CritSec);
	};

	~myCCritSec() {
		DeleteCriticalSection(&m_CritSec);
	};

	void Lock() {
		EnterCriticalSection(&m_CritSec);
	};

	void Unlock() {
		LeaveCriticalSection(&m_CritSec);
	};
};

// locks a critical section, and unlocks it automatically
// when the lock goes out of scope
class myCAutoLock {

	// make copy constructor and assignment operator inaccessible

	myCAutoLock(const myCAutoLock &refAutoLock);
	myCAutoLock &operator=(const myCAutoLock &refAutoLock);

protected:
	myCCritSec * m_pLock;

public:
	myCAutoLock(myCCritSec * plock)
	{
		m_pLock = plock;
		m_pLock->Lock();
	};

	~myCAutoLock() {
		m_pLock->Unlock();
	};
};
